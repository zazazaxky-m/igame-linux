// SPDX-License-Identifier: GPL-3.0-or-later
#include "audioanalyzer.h"

#include <QHash>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace {

constexpr int kFft = 1024;
constexpr int kHop = 512;
constexpr double kHopSec = double(kHop) / AudioAnalyzer::kRate;
constexpr double kBinHz = double(AudioAnalyzer::kRate) / kFft;
constexpr double kGate = 0.0004;     // ~ -68 dBFS
constexpr double kRangeDb = 20;      // dB below the running peak
constexpr double kFullScaleDb = 48;  // full-scale sine -> ~0 dB
constexpr size_t kOdfFrames = 562;   // ~6 s
constexpr size_t kKickHistory = 140; // ~1.5 s
constexpr size_t kFluxHistory = 47;  // ~0.5 s
constexpr double kLead = 0.025;      // fire beats a bit early to hide latency
constexpr int kPhaseSlots = AudioAnalyzer::kPhaseSlots;
constexpr double kPi = 3.14159265358979323846;

// melbank, same parameters as LedFx (4096 FFT, 60 frames/s, 20 Hz - 15 kHz)
constexpr int kMelFft = 4096;
constexpr int kMelHop = AudioAnalyzer::kRate / 60;
constexpr double kMelMinHz = 20, kMelMaxHz = 15000;
// LedFx peak_isolation 0.4 -> power of ~1.96
const double kPeakPower = std::tan(0.5 * kPi * (0.4 + 1) / 2);

double clamp01(double x)
{
    return std::clamp(x, 0.0, 1.0);
}

double ramp(double x, double a, double b)
{
    return clamp01((x - a) / (b - a));
}

// 0 -> 1 over a..b, 1 -> 0 over c..d
double trapezoid(double x, double a, double b, double c, double d)
{
    return std::min(ramp(x, a, b), 1.0 - ramp(x, c, d));
}

double smoothstep(double x)
{
    x = clamp01(x);
    return x * x * (3 - 2 * x);
}

// one-pole smoothing factor
double follow(double dt, double tau)
{
    return 1.0 - std::exp(-dt / std::max(tau, 1e-4));
}

QColor mix(const QColor &a, const QColor &b, double f)
{
    f = clamp01(f);
    return QColor(int(std::lround(a.red() + (b.red() - a.red()) * f)),
                  int(std::lround(a.green() + (b.green() - a.green()) * f)),
                  int(std::lround(a.blue() + (b.blue() - a.blue()) * f)));
}

QColor scaled(const QColor &c, double f)
{
    f = clamp01(f);
    return QColor(int(std::lround(c.red() * f)), int(std::lround(c.green() * f)), int(std::lround(c.blue() * f)));
}

// perceived brightness -> PWM, otherwise quiet parts look almost as bright as loud ones
QColor shade(const QColor &c, double perceived)
{
    const double p = clamp01(perceived);
    return scaled(c, p * p);
}

// LedFx ExpFilter (alphas per 60 fps frame)
void expFilter(double &value, double target, double rise, double decay)
{
    const double a = target > value ? rise : decay;
    value = a * target + (1 - a) * value;
}

// gradient with LedFx's eased blending between stops
QColor gradientAt(const QVector<QColor> &stops, double pos)
{
    if (stops.isEmpty())
        return QColor(255, 255, 255);
    if (stops.size() == 1)
        return stops.first();
    pos -= std::floor(pos);
    const double x = pos * (stops.size() - 1);
    const int i = std::min(int(x), int(stops.size()) - 2);
    const double f = x - i;
    const double p = std::pow(f, 1.5);
    return mix(stops[i], stops[i + 1], p / (p + std::pow(1 - f, 1.5)));
}

// LedFx output is linear, WLED gamma-corrects it, so do the same here
QColor ledfxColor(double r, double g, double b)
{
    const auto c = [](double v) { return int(std::lround(255 * std::pow(std::clamp(v, 0.0, 255.0) / 255, 2.2))); };
    return QColor(c(r), c(g), c(b));
}

// LedFx's default gradient
const QVector<QColor> &ledfxRainbow()
{
    static const QVector<QColor> stops{QColor(0xFF0000), QColor(0xFF7800), QColor(0xFFC800), QColor(0x00FF00),
                                       QColor(0x00C78C), QColor(0x0000FF), QColor(0x800080), QColor(0xFF00B2)};
    return stops;
}

template<typename T>
void meanStd(const std::deque<T> &values, double *mean, double *stddev)
{
    double m = 0, v = 0;
    for (T x : values)
        m += x;
    m /= double(values.size());
    for (T x : values)
        v += (x - m) * (x - m);
    *mean = m;
    *stddev = std::sqrt(v / double(values.size()));
}

// running peak: fast attack, falls 1.5 dB/s
void trackCeiling(double &ceil, double db)
{
    if (db > ceil)
        ceil += (db - ceil) * follow(kHopSec, 0.08);
    else
        ceil -= 1.5 * kHopSec;
    ceil = std::max(ceil, -50.0);
}

QString renderModeForEffect(const QString &effect)
{
    static const QHash<QString, QString> modes{
        {QStringLiteral("music"), QStringLiteral("bar")},
        {QStringLiteral("music_spectrum"), QStringLiteral("energy")},
        {QStringLiteral("music_scroll"), QStringLiteral("scroll")},
        {QStringLiteral("music_bass"), QStringLiteral("power")},
        {QStringLiteral("music_strobe"), QStringLiteral("strobe")},
        {QStringLiteral("music_realstrobe"), QStringLiteral("realstrobe")},
        {QStringLiteral("music_rainbow"), QStringLiteral("wavelength")},
        {QStringLiteral("music_fire"), QStringLiteral("magnitude")},
        {QStringLiteral("music_chill"), QStringLiteral("chill")},
    };
    return modes.value(effect, QStringLiteral("bar"));
}

// Smart Sound style -> effect
QString renderModeForStyle(const QString &style)
{
    static const QHash<QString, QString> modes{
        {QStringLiteral("edm"), QStringLiteral("power")},  {QStringLiteral("hiphop"), QStringLiteral("bar")},
        {QStringLiteral("rock"), QStringLiteral("magnitude")}, {QStringLiteral("pop"), QStringLiteral("energy")},
        {QStringLiteral("chill"), QStringLiteral("wavelength")}, {QStringLiteral("speech"), QStringLiteral("glow")},
        {QStringLiteral("silence"), QStringLiteral("idle")},
    };
    return modes.value(style, QStringLiteral("energy"));
}

QVector<QColor> stylePalette(const QString &style)
{
    static const QHash<QString, QVector<QColor>> palettes{
        {QStringLiteral("edm"), {QColor(0x00E5FF), QColor(0x7A00FF), QColor(0xFF00C8), QColor(0x00FF85)}},
        {QStringLiteral("hiphop"), {QColor(0x8C00FF), QColor(0xFF0066), QColor(0xFF8C00)}},
        {QStringLiteral("rock"), {QColor(0x2A0000), QColor(0xFF1A00), QColor(0xFF7A00), QColor(0xFFD000), QColor(0xFFF6E0)}},
        {QStringLiteral("pop"), {QColor(0xFF2E63), QColor(0xFFD000), QColor(0x08D9D6)}}, // bass, mids, treble
        {QStringLiteral("chill"), {QColor(0x3A86FF), QColor(0x8338EC), QColor(0x06D6A0)}},
        {QStringLiteral("speech"), {QColor(0xFFB36B)}},
        {QStringLiteral("silence"), {QColor(0x3A86FF)}},
    };
    return palettes.value(style, {QColor(0xFFFFFF)});
}

} // namespace

AudioAnalyzer::AudioAnalyzer()
{
    m_colors = {QColor(0xFF0055), QColor(0x00D5FF), QColor(0x8C00FF)};
    m_window.resize(kFft);
    for (int i = 0; i < kFft; ++i)
        m_window[i] = float(0.5 - 0.5 * std::cos(2 * kPi * i / (kFft - 1)));
    m_windowed.resize(kFft);
    m_mags.resize(kFft / 2 + 1);

    // triangular mel filters, unit area like aubio's
    m_melRing.assign(kMelFft, 0.0f);
    m_melWindow.resize(kMelFft);
    for (int i = 0; i < kMelFft; ++i)
        m_melWindow[i] = float(0.5 - 0.5 * std::cos(2 * kPi * i / (kMelFft - 1)));
    m_melWindowed.resize(kMelFft);
    m_melMag.resize(kMelFft / 2 + 1);
    const auto toMel = [](double hz) { return 2595 * std::log10(1 + hz / 700); };
    const auto toHz = [](double mel) { return 700 * (std::pow(10, mel / 2595) - 1); };
    const double binHz = double(kRate) / kMelFft;
    double edges[kMelBands + 2];
    for (int i = 0; i < kMelBands + 2; ++i)
        edges[i] = toHz(toMel(kMelMinHz) + (toMel(kMelMaxHz) - toMel(kMelMinHz)) * i / (kMelBands + 1));
    for (int b = 0; b < kMelBands; ++b) {
        MelFilter f;
        const double lo = edges[b], mid = edges[b + 1], hi = edges[b + 2];
        f.hz = mid;
        f.first = std::max(1, int(std::ceil(lo / binHz)));
        const int last = std::min(kMelFft / 2, int(std::floor(hi / binHz)));
        const double height = 2.0 / (hi - lo);
        for (int k = f.first; k <= last; ++k) {
            const double hz = k * binHz;
            const double w = hz < mid ? (hz - lo) / (mid - lo) : (hi - hz) / (hi - mid);
            f.weights.push_back(float(std::max(0.0, w) * height));
        }
        if (f.weights.empty()) { // narrower than a bin
            f.first = std::clamp(int(std::lround(mid / binHz)), 1, kMelFft / 2);
            f.weights.push_back(float(1.0 / binHz));
        }
        m_melFilters.push_back(f);
    }
    reset();
}

void AudioAnalyzer::reset()
{
    m_buffer.clear();
    m_logMag[0].assign(kFft / 2, 0.0f);
    m_logMag[1].assign(kFft / 2, 0.0f);
    m_hops = 0;
    m_ceilDb = -30;
    m_kickCeilDb = -40;
    std::fill(std::begin(m_bandCeilDb), std::end(m_bandCeilDb), -40.0);
    m_power = m_loud = m_loudSlow = 0;
    std::fill(std::begin(m_band), std::end(m_band), 0.0);
    m_silentSeconds = 0;
    m_kickDbHist[0] = m_kickDbHist[1] = -120;
    m_kickRiseHist.clear();
    m_fluxHist.clear();
    m_lastKick = m_lastOnset = -100;
    m_onsetTimes.clear();
    m_odf.clear();
    m_bpm = m_tempoConfidence = m_pendingBpm = 0;
    m_pendingVotes = 0;
    m_locked = false;
    m_phase = m_beatPhase = m_confirm = 0;
    m_histPeriod = 1;
    std::fill(std::begin(m_phaseHist), std::end(m_phaseHist), 0.0);
    m_lastBeatTime = -100;
    m_pendingBeat = 0;
    m_events.clear();
    m_swell = 0;

    std::fill(m_melRing.begin(), m_melRing.end(), 0.0f);
    m_melWrite = m_melFilled = 0;
    m_melSinceFrame = 0;
    m_melGain = 0.1;
    std::fill(std::begin(m_mel), std::end(m_mel), 0.0);
    std::fill(std::begin(m_melCommon), std::end(m_melCommon), 0.0);
    std::fill(std::begin(m_melFiltered), std::end(m_melFiltered), 0.0);
    m_lowsRaw = m_lows = m_mids = m_highs = 0;
    m_melOnset = m_melKick = false;
    m_melTime = 0;
    std::fill(std::begin(m_energy), std::end(m_energy), 0.0);
    std::fill(std::begin(m_scroll), std::end(m_scroll), 0.0);
    m_powerBass = m_powerOverlay = m_powerOverlayPos = m_sparks = 0;
    m_barColor = m_lastOsc = 0;
    m_barPhase = m_barBeat = 0;
    m_rsColor = m_rsShiftTime = m_rsBass = m_rsBassColor = m_rsStrobe = 0;
    m_rsLastBass = -100;

    m_listenTime = 0;
    m_candidate.clear();
    m_styleBlend = 1;
}

void AudioAnalyzer::setMode(const QString &effect)
{
    if (effect == m_effect)
        return;
    m_effect = effect;
    if (effect == QLatin1String("music_smart")) {
        m_listenTime = 0;
        m_styleBlend = 1;
        m_candidate.clear();
    }
}

void AudioAnalyzer::setSensitivity(int percent)
{
    m_sens = std::clamp(percent, 10, 300) / 100.0;
}

void AudioAnalyzer::setSpeed(double speed)
{
    m_speed = std::clamp(speed, 0.1, 5.0);
}

void AudioAnalyzer::setColors(const QVector<QColor> &colors)
{
    if (!colors.isEmpty())
        m_colors = colors;
}

std::vector<AudioAnalyzer::BeatEvent> AudioAnalyzer::takeBeatEvents()
{
    std::vector<BeatEvent> events;
    events.swap(m_events);
    return events;
}

double AudioAnalyzer::audioTime() const
{
    return (double(m_hops) * kHop + kFft) / kRate;
}

void AudioAnalyzer::feed(const float *samples, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        m_melRing[m_melWrite] = samples[i];
        m_melWrite = (m_melWrite + 1) % kMelFft;
        m_melFilled = std::min(m_melFilled + 1, size_t(kMelFft));
        if (++m_melSinceFrame >= kMelHop) {
            m_melSinceFrame = 0;
            if (m_melFilled == size_t(kMelFft))
                melFrame();
        }
    }
    m_buffer.insert(m_buffer.end(), samples, samples + count);
    size_t offset = 0;
    while (m_buffer.size() - offset >= size_t(kFft)) {
        analyze(m_buffer.data() + offset);
        offset += kHop;
    }
    m_buffer.erase(m_buffer.begin(), m_buffer.begin() + qsizetype(offset));
}

void AudioAnalyzer::analyze(const float *x)
{
    const double t = audioTime();
    ++m_hops;

    double sumSquares = 0;
    for (int i = 0; i < kFft; ++i) {
        sumSquares += double(x[i]) * x[i];
        m_windowed[i] = x[i] * m_window[i];
    }
    const double rms = std::sqrt(sumSquares / kFft);
    const bool silent = rms < kGate;
    m_silentSeconds = silent ? m_silentSeconds + kHopSec : 0;

    m_spectrum.magnitudes(m_windowed.data(), m_mags.data());
    // 46.9 Hz bins. kick 47-140 Hz, meters: <234 Hz, <2 kHz, above
    double kick = 0, bands[3] = {0, 0, 0}, e[5] = {0, 0, 0, 0, 0};
    double flux = 0, centroidNum = 0, centroidDen = 0;
    std::vector<float> &older = m_logMag[0];
    for (int k = 1; k < kFft / 2; ++k) {
        const double mag = m_mags[k];
        const double power = mag * mag;
        if (k <= 3)
            kick += power;
        bands[k <= 5 ? 0 : k <= 42 ? 1 : 2] += power;
        e[k <= 3 ? 0 : k <= 10 ? 1 : k <= 42 ? 2 : k <= 128 ? 3 : 4] += power;
        // compare with two hops back, the windows don't overlap there
        const float logMag = float(std::log1p(mag));
        const float diff = logMag - older[k];
        older[k] = logMag;
        if (diff > 0)
            flux += diff;
        centroidNum += k * kBinHz * mag;
        centroidDen += mag;
    }
    std::swap(m_logMag[0], m_logMag[1]);

    // sensitivity 200% = full brightness 6 dB below the peak
    const double sensShift = 6.0 * std::log2(m_sens);
    const double release = std::clamp(0.25 / m_speed, 0.05, 1.5);
    // smooth the power a bit first, raw windows wobble and flicker
    m_power += (sumSquares / kFft - m_power) * follow(kHopSec, 0.04);
    const double db = 10 * std::log10(m_power + 1e-18);
    double target = 0;
    if (!silent) {
        trackCeiling(m_ceilDb, db);
        target = clamp01((db - (m_ceilDb - sensShift - kRangeDb)) / kRangeDb);
    }
    m_loud += (target - m_loud) * follow(kHopSec, target > m_loud ? 0.04 : release);
    m_loudSlow += (m_loud - m_loudSlow) * follow(kHopSec, 0.8);
    for (int b = 0; b < 3; ++b) {
        const double bandDb = 10 * std::log10(bands[b] + 1e-12) - kFullScaleDb;
        double v = 0;
        if (!silent) {
            trackCeiling(m_bandCeilDb[b], bandDb);
            v = clamp01((bandDb - (m_bandCeilDb[b] - sensShift - kRangeDb)) / kRangeDb);
        }
        m_band[b] += (v - m_band[b]) * follow(kHopSec, v > m_band[b] ? 0.02 : release);
    }

    // kick = energy jump below 140 Hz over two hops
    const double kickDb = 10 * std::log10(kick + 1e-12) - kFullScaleDb;
    const double rise = kickDb - m_kickDbHist[0];
    m_kickDbHist[0] = m_kickDbHist[1];
    m_kickDbHist[1] = kickDb;
    if (!silent)
        trackCeiling(m_kickCeilDb, kickDb);

    // onsets: spectral flux vs adaptive threshold
    const double k = std::clamp(1.6 / std::sqrt(m_sens), 0.6, 3.0);
    bool onset = false;
    double fluxMean = 0, fluxStd = 0;
    if (m_fluxHist.size() >= 8) {
        meanStd(m_fluxHist, &fluxMean, &fluxStd);
        onset = !silent && flux > fluxMean + k * fluxStd && flux > 1.0 && t - m_lastOnset > 0.1;
    }
    m_fluxHist.push_back(float(flux));
    if (m_fluxHist.size() > kFluxHistory)
        m_fluxHist.pop_front();
    if (onset) {
        m_lastOnset = t;
        m_onsetTimes.push_back(t);
        // only strong ones, on one LED every spark flashes the whole light
        if (flux > fluxMean + 2 * k * fluxStd)
            m_melOnset = true;
    }
    while (!m_onsetTimes.empty() && t - m_onsetTimes.front() > 5.0)
        m_onsetTimes.pop_front();

    if (!silent)
        detectKick(t, rise, kickDb);
    m_kickRiseHist.push_back(float(std::max(0.0, rise)));
    if (m_kickRiseHist.size() > kKickHistory)
        m_kickRiseHist.pop_front();

    // weight by level so kicks beat off-beat bass notes
    const double attack = silent ? 0.0 : std::max(0.0, rise - 1.0) * clamp01((kickDb - (m_kickCeilDb - 24)) / 24);

    // weight kicks heavily so tempo follows quarter notes, not hats
    const double odf = silent ? 0.0 : 0.5 * flux / (fluxMean + 1e-6) + 2.0 * std::pow(attack / 6.0, 2);
    m_odf.push_back(float(odf));
    if (m_odf.size() > kOdfFrames)
        m_odf.pop_front();
    if (m_hops % 24 == 0)
        estimateTempo();

    updateBeatGrid(t, attack);

    // Smart Sound features, ~6 s averages
    if (!silent) {
        const double total = e[0] + e[1] + e[2] + e[3] + e[4] + 1e-12;
        const double a = follow(kHopSec, 6.0);
        m_lowRatio += (e[0] / total - m_lowRatio) * a;
        m_highRatio += ((e[3] + e[4]) / total - m_highRatio) * a;
        if (centroidDen > 0)
            m_centroid += (centroidNum / centroidDen - m_centroid) * a;
        m_dbMean += (db - m_dbMean) * a;
        m_dynamics += (std::abs(db - m_dbMean) - m_dynamics) * a;
    }
}

void AudioAnalyzer::detectKick(double t, double rise, double kickDb)
{
    if (m_kickRiseHist.size() < 30)
        return;
    double mean = 0, stddev = 0;
    meanStd(m_kickRiseHist, &mean, &stddev);
    const double k = std::clamp(1.6 / std::sqrt(m_sens), 0.6, 3.0);
    const double minRise = std::clamp(6.0 / std::sqrt(m_sens), 2.5, 12.0);
    const double threshold = std::max(mean + k * stddev, minRise);
    const double period = m_bpm > 0 ? 60.0 / m_bpm : 0.5;
    const double refractory = m_locked ? 0.45 * period : 0.22;
    if (rise <= threshold || kickDb < m_kickCeilDb - 18 || t - m_lastKick < refractory)
        return;
    m_lastKick = t;
    m_melKick = true;
    const double strength = std::clamp(0.6 + 0.4 * (rise / threshold - 1.0), 0.6, 1.0);
    const double when = t - kHopSec;

    if (m_locked) {
        // on-grid kicks confirm the grid, the grid itself fires the beats
        double d = std::abs(m_phase - m_beatPhase);
        d -= std::floor(d);
        if (std::min(d, 1.0 - d) < 0.15)
            m_confirm = 1.0;
        return;
    }

    if (m_bpm > 0) {
        double d = std::abs(m_phase - m_beatPhase);
        d -= std::floor(d);
        if (std::min(d, 1.0 - d) < 0.15)
            m_confirm = 1.0;
    }

    fireBeat(when, 0.7 * strength, true);
}

// Beat grid: histogram of where low-end attacks land inside the beat. The peak is the beat
// phase, so off-beat bass doesn't drag it. Once locked, beats come from the grid, so a missed
// kick doesn't break the rhythm. Unconfirmed for ~8 beats -> unlock.
void AudioAnalyzer::updateBeatGrid(double t, double attack)
{
    if (m_bpm <= 0 || m_silentSeconds > 1.0) {
        m_locked = false;
        if (m_silentSeconds > 1.0)
            std::fill(std::begin(m_phaseHist), std::end(m_phaseHist), 0.0);
        return;
    }
    const double period = 60.0 / m_bpm;
    if (std::abs(period / m_histPeriod - 1.0) > 0.03) {
        // tempo changed, start over
        std::fill(std::begin(m_phaseHist), std::end(m_phaseHist), 0.0);
        m_histPeriod = period;
        m_confirm = 0;
    }
    const double previous = m_phase;
    m_phase += kHopSec / period;
    if (m_phase > 1e6)
        m_phase -= std::floor(m_phase);

    const double decay = std::exp(-kHopSec / 4.0);
    double total = 0;
    for (double &h : m_phaseHist) {
        h *= decay;
        total += h;
    }
    const double w = attack;
    if (w > 0) {
        double at = m_phase - kHopSec / period;
        at -= std::floor(at);
        m_phaseHist[int(at * kPhaseSlots) % kPhaseSlots] += w * w;
        total += w * w;
    }

    int peak = 0;
    double peakSum = -1;
    for (int i = 0; i < kPhaseSlots; ++i) {
        const double sum = m_phaseHist[(i + kPhaseSlots - 1) % kPhaseSlots] + m_phaseHist[i]
                         + m_phaseHist[(i + 1) % kPhaseSlots];
        if (sum > peakSum) {
            peakSum = sum;
            peak = i;
        }
    }
    double num = 0;
    for (int j = -1; j <= 1; ++j)
        num += j * m_phaseHist[(peak + j + kPhaseSlots) % kPhaseSlots];
    double phase = (peak + 0.5 + num / std::max(peakSum, 1e-9)) / kPhaseSlots;
    phase -= std::floor(phase);
    m_beatPhase = phase;

    const bool grid = m_tempoConfidence > 0.15 && total > 30 && peakSum / total > 0.3;
    m_locked = grid && m_confirm > 0.1;
    if (!grid)
        return;

    const double target = m_beatPhase - kLead / period;
    const double before = previous - target - std::floor(previous - target);
    const double now = m_phase - target - std::floor(m_phase - target);
    if (now < before) {
        if (m_locked)
            fireBeat(t, 0.35 + 0.65 * m_confirm, false);
        m_confirm *= 0.75;
    }
}

void AudioAnalyzer::fireBeat(double t, double strength, bool real)
{
    m_pendingBeat = std::max(m_pendingBeat, strength);
    m_lastBeatTime = t;
    m_events.push_back({t, strength, real});
}

void AudioAnalyzer::estimateTempo()
{
    const size_t n = m_odf.size();
    if (n < 210) // need ~2 s
        return;
    std::vector<double> env(m_odf.begin(), m_odf.end());
    const double mean = std::accumulate(env.begin(), env.end(), 0.0) / double(n);
    double energy = 0;
    for (double &v : env) {
        v -= mean;
        energy += v * v;
    }
    if (energy <= 1e-9)
        return;

    const double rate = 1.0 / kHopSec;
    const int minLag = int(60.0 * rate / 200.0); // 200 BPM
    const int maxLag = int(60.0 * rate / 60.0) + 1; // 60 BPM
    std::vector<double> ac(size_t(2 * maxLag + 3), 0.0);
    const auto corr = [&](int lag) {
        double sum = 0;
        for (size_t i = 0; i + size_t(lag) < n; ++i)
            sum += env[i] * env[i + size_t(lag)];
        return sum / energy;
    };
    for (int lag = minLag - 1; lag <= maxLag + 1; ++lag)
        ac[size_t(lag)] = corr(lag);

    int best = -1;
    double bestScore = 0;
    for (int lag = minLag; lag <= maxLag; ++lag) {
        if (!(ac[size_t(lag)] > ac[size_t(lag - 1)] && ac[size_t(lag)] >= ac[size_t(lag + 1)]))
            continue;
        const double bpm = 60.0 * rate / lag;
        // prefer ~105 BPM (slower reading looks calmer), bonus if 2x lag also matches, and stick
        // with the current tempo unless something is clearly better
        const double prior = std::exp(-0.5 * std::pow(std::log2(bpm / 105.0), 2));
        const double harmonic = 2 * lag < int(n) / 2 ? 0.5 * std::max(0.0, corr(2 * lag)) : 0.0;
        double score = (ac[size_t(lag)] + harmonic) * prior;
        if (m_bpm > 0 && std::abs(bpm / m_bpm - 1.0) < 0.05)
            score *= 1.5;
        if (score > bestScore) {
            bestScore = score;
            best = lag;
        }
    }
    if (best < 0)
        return;
    const double y0 = ac[size_t(best - 1)], y1 = ac[size_t(best)], y2 = ac[size_t(best + 1)];
    const double denom = y0 - 2 * y1 + y2;
    const double shift = std::abs(denom) > 1e-9 ? std::clamp(0.5 * (y0 - y2) / denom, -0.5, 0.5) : 0.0;
    const double bpm = 60.0 * rate / (best + shift);
    const double confidence = clamp01(y1);

    m_tempoConfidence = m_tempoConfidence * 0.7 + confidence * 0.3;
    if (confidence < 0.1)
        return;
    if (m_bpm <= 0) {
        m_bpm = bpm;
        return;
    }
    const double ratio = bpm / m_bpm;
    if (std::abs(ratio - 1.0) < 0.06) {
        m_bpm = m_bpm * 0.8 + bpm * 0.2;
        m_pendingBpm = 0;
        m_pendingVotes = 0;
        return;
    }
    // new tempo needs ~2 s of agreeing estimates (~4 s for double/half time) before we switch
    if (m_pendingBpm > 0 && std::abs(bpm / m_pendingBpm - 1.0) < 0.04) {
        ++m_pendingVotes;
    } else {
        m_pendingBpm = bpm;
        m_pendingVotes = 1;
    }
    const bool octave = std::abs(ratio / 2 - 1.0) < 0.05 || std::abs(ratio * 2 - 1.0) < 0.05;
    if (m_pendingVotes >= (octave ? 16 : 8)) {
        m_bpm = m_pendingBpm;
        m_pendingBpm = 0;
        m_pendingVotes = 0;
    }
}

void AudioAnalyzer::classify(double dt)
{
    m_listenTime += dt;
    m_styleBlend = std::min(1.0, m_styleBlend + dt / 1.5);

    QString best;
    if (m_silentSeconds > 2.0) {
        best = QStringLiteral("silence");
    } else {
        if (m_listenTime < 4.0)
            return;
        const double conf = std::max(ramp(m_tempoConfidence, 0.05, 0.4), m_locked ? 0.8 : 0.0);
        const double bpm = m_bpm;
        const double onsetRate = double(m_onsetTimes.size()) / 5.0;
        const double low = m_lowRatio, high = m_highRatio, dyn = m_dynamics, centroid = m_centroid;
        // edm/hiphop need a steady beat too, lots of ballads are bass heavy
        const QList<QPair<QString, double>> scores{
            {QStringLiteral("edm"), conf * (1.4 * ramp(low, 0.3, 0.6) + 0.8 * trapezoid(bpm, 110, 118, 140, 155) * ramp(low, 0.2, 0.4) + 0.3)
                                        + 0.3 * ramp(-dyn, -6, -3)},
            {QStringLiteral("hiphop"), conf * (1.4 * ramp(low, 0.3, 0.6) + 0.8 * trapezoid(bpm, 65, 75, 100, 112) * ramp(low, 0.2, 0.4) + 0.2)},
            {QStringLiteral("rock"), 1.0 * ramp(high, 0.06, 0.18) + 0.8 * ramp(onsetRate, 3, 7)
                                         + 0.6 * ramp(centroid, 2000, 3500) + 0.3 * (1 - ramp(low, 0.2, 0.45))},
            {QStringLiteral("pop"), 0.9 + 0.5 * trapezoid(bpm, 85, 95, 130, 140) * conf - 0.4 * ramp(low, 0.45, 0.65)},
            {QStringLiteral("chill"), 0.9 * (1 - conf) + 0.4 * (1 - ramp(onsetRate, 2, 6)) + 0.3 * (1 - ramp(high, 0.03, 0.10))
                                          + 0.2 * ramp(dyn, 3, 7)},
            {QStringLiteral("speech"), (1 - ramp(low, 0.06, 0.18))
                                           * (0.6 + 0.5 * (1 - conf) + 0.4 * trapezoid(centroid, 500, 900, 2200, 3200)
                                              + 0.3 * ramp(dyn, 3, 6))},
        };
        double top = -1e9, current = -1e9;
        for (const auto &[style, score] : scores) {
            if (score > top) {
                top = score;
                best = style;
            }
            if (style == m_style)
                current = score;
        }
        // hysteresis
        if (best != m_style && top < current + 0.12)
            best = m_style;
    }

    if (best == m_style) {
        m_candidate.clear();
        return;
    }
    if (best != m_candidate) {
        m_candidate = best;
        m_candidateTime = 0;
        return;
    }
    m_candidateTime += dt;
    const double hold = (best == QLatin1String("silence") || m_style == QLatin1String("silence")) ? 0.8 : 5.0;
    if (m_candidateTime >= hold) {
        m_previousStyle = m_style;
        m_style = best;
        m_styleBlend = 0;
        m_candidate.clear();
    }
}

// 60/s: melbank + band powers + all LedFx effect states (so switching effects is seamless)
void AudioAnalyzer::melFrame()
{
    m_melTime += 1.0 / 60;
    const bool onset = m_melOnset, kick = m_melKick;
    m_melOnset = m_melKick = false;

    // like LedFx min_volume
    double sumSquares = 0;
    for (int i = 0; i < kMelHop; ++i) {
        const float v = m_melRing[(m_melWrite + kMelFft - 1 - i) % kMelFft];
        sumSquares += double(v) * v;
    }
    const bool silent = std::sqrt(sumSquares / kMelHop) < kGate;

    double raw[kMelBands];
    if (silent) {
        std::fill(std::begin(m_mel), std::end(m_mel), 0.0);
        std::fill(std::begin(m_melFiltered), std::end(m_melFiltered), 0.0);
    } else {
        for (int n = 0; n < kMelFft; ++n)
            m_melWindowed[n] = m_melRing[(m_melWrite + size_t(n)) % kMelFft] * m_melWindow[n];
        m_melSpectrum.magnitudes(m_melWindowed.data(), m_melMag.data());

        // same chain as LedFx: filterbank, peak isolation, gain, smoothing, minus a slow baseline
        for (int b = 0; b < kMelBands; ++b) {
            const MelFilter &f = m_melFilters[b];
            double sum = 0;
            for (size_t j = 0; j < f.weights.size(); ++j)
                sum += m_melMag[f.first + int(j)] * f.weights[j];
            raw[b] = std::pow(sum, kPeakPower);
        }
        static const double kernel[5] = {0.054, 0.242, 0.399, 0.242, 0.054}; // gaussian, sigma 1
        double peak = 0;
        for (int b = 0; b < kMelBands; ++b) {
            double v = 0;
            for (int j = -2; j <= 2; ++j)
                v += kernel[j + 2] * raw[std::clamp(b + j, 0, kMelBands - 1)];
            peak = std::max(peak, v);
        }
        expFilter(m_melGain, peak, 0.99, 0.01);
        for (int b = 0; b < kMelBands; ++b) {
            expFilter(m_mel[b], raw[b] / std::max(m_melGain, 1e-12), 0.99, 0.7);
            expFilter(m_melCommon[b], m_mel[b], 0.01, 0.99);
            expFilter(m_melFiltered[b], m_mel[b] - m_melCommon[b], 0.99, 0.15);
        }
    }

    const auto mel = [&](int b) { return std::clamp(m_mel[b] * m_sens, 0.0, 1.0); };
    const auto filtered = [&](int b) { return std::clamp(m_melFiltered[b] * m_sens, 0.0, 1.0); };

    // lows < 250 Hz, mids < 3 kHz, highs < 10 kHz
    double sums[3] = {0, 0, 0};
    int counts[3] = {0, 0, 0};
    for (int b = 0; b < kMelBands; ++b) {
        const double hz = m_melFilters[b].hz;
        const int r = hz < 250 ? 0 : hz < 3000 ? 1 : hz < 10000 ? 2 : -1;
        if (r >= 0) {
            sums[r] += mel(b);
            ++counts[r];
        }
    }
    m_lowsRaw = counts[0] ? std::min(1.0, sums[0] / counts[0]) : 0;
    expFilter(m_lows, m_lowsRaw, 0.97, 0.2);
    expFilter(m_mids, counts[1] ? std::min(1.0, sums[1] / counts[1]) : 0, 0.97, 0.2);
    expFilter(m_highs, counts[2] ? std::min(1.0, sums[2] / counts[2]) : 0, 0.97, 0.2);

    // LedFx's melbank_thirds split
    const int split1 = int(0.2 * kMelBands), split2 = int(0.5 * kMelBands);
    const int lo[3] = {0, split1, split2}, hi[3] = {split1, split2, kMelBands};
    const auto decayed = [&](double keep) { return std::pow(keep, m_speed); };

    // Energy (sensitivity 0.6 at speed 1)
    const double eSens = std::clamp(0.3 + 0.3 * m_speed, 0.3, 0.99);
    for (int x = 0; x < 3; ++x) {
        double mean = 0;
        for (int b = lo[x]; b < hi[x]; ++b)
            mean += mel(b);
        mean /= std::max(1, hi[x] - lo[x]);
        // LedFx uses 1.365, a bit more here since one LED is the average of the bar
        expFilter(m_energy[x], std::min(1.0, 2.0 * mean), eSens, (eSens - 0.1) * 0.7);
    }

    // Scroll, the 0.97 fade becomes an afterglow
    for (int x = 0; x < 3; ++x) {
        double peakBand = 0;
        for (int b = lo[x]; b < hi[x]; ++b)
            peakBand = std::max(peakBand, filtered(b));
        m_scroll[x] = std::max(peakBand * peakBand, m_scroll[x] * decayed(0.97));
    }

    // Power
    expFilter(m_powerBass, m_lowsRaw, 0.8, 0.1);
    m_powerOverlay *= decayed(0.95);
    if (m_powerBass >= m_powerOverlay) {
        m_powerOverlay = m_powerBass;
        m_powerOverlayPos = m_powerBass;
    }
    m_sparks *= decayed(0.85);
    if (onset)
        m_sparks = 1;

    // Bar: flips every beat, color steps 1/8 every two beats
    const double osc = beatOscillator();
    if (osc >= 0) {
        if (m_lastOsc > 0.2 && osc < 0.1) {
            m_barBeat = (m_barBeat + 1) % 4;
            m_barPhase = 1 - m_barPhase;
            if (m_barPhase == 0)
                m_barColor = std::fmod(m_barColor + 0.125, 1.0);
        }
        m_lastOsc = osc;
    }

    // Real Strobe
    if (m_melTime - m_rsShiftTime > 1.0) {
        m_rsColor = std::fmod(m_rsColor + 0.0625, 1.0);
        m_rsShiftTime = m_melTime;
    }
    m_rsBass *= decayed(0.5);
    m_rsStrobe *= decayed(0.5);
    if (kick && m_melTime - m_rsLastBass > 0.2) {
        m_rsBass = 1;
        m_rsBassColor = m_rsColor;
        m_rsLastBass = m_melTime;
    }
    if (onset)
        m_rsStrobe = 1;
}

double AudioAnalyzer::beatOscillator() const
{
    if (m_bpm <= 0 || m_silentSeconds > 1.0)
        return -1;
    const double period = 60.0 / m_bpm;
    double o = m_phase - (m_beatPhase - kLead / period);
    return o - std::floor(o);
}

QColor AudioAnalyzer::render(double dt)
{
    dt = std::clamp(dt, 0.0, 0.2);
    m_time += dt;
    if (m_pendingBeat > 0) {
        m_swell = std::max(m_swell, m_pendingBeat);
        ++m_beatCount;
    }
    m_pendingBeat = 0;
    m_swell *= std::exp(-dt * m_speed / 0.45);

    QColor out;
    if (m_effect == QLatin1String("music_smart")) {
        classify(dt);
        out = draw(renderModeForStyle(m_style), stylePalette(m_style));
        if (m_styleBlend < 1)
            out = mix(draw(renderModeForStyle(m_previousStyle), stylePalette(m_previousStyle)), out,
                      smoothstep(m_styleBlend));
    } else {
        out = draw(renderModeForEffect(m_effect), m_colors);
    }
    return out;
}

QColor AudioAnalyzer::draw(const QString &mode, const QVector<QColor> &paletteIn) const
{
    // palette = gradient; Energy/Scroll use the first 3 colors for bass/mids/treble
    const QVector<QColor> &gradient = paletteIn.size() >= 2 ? paletteIn : ledfxRainbow();
    const auto band = [&](int i) {
        static const QColor fallback[3] = {QColor(0xFF0000), QColor(0x00FF00), QColor(0x0000FF)};
        return i < paletteIn.size() ? paletteIn[i] : fallback[i];
    };
    const bool quiet = m_silentSeconds > 0.5;

    if (mode == QLatin1String("energy") || mode == QLatin1String("scroll")) {
        const double *level = mode == QLatin1String("energy") ? m_energy : m_scroll;
        double r = 0, g = 0, b = 0; // additive
        for (int i = 0; i < 3; ++i) {
            r += band(i).red() * level[i];
            g += band(i).green() * level[i];
            b += band(i).blue() * level[i];
        }
        return ledfxColor(r, g, b);
    }

    if (mode == QLatin1String("power")) {
        // background (melbank across the gradient) + bass overlay + sparks
        double r = 0, g = 0, b = 0;
        for (int i = 0; i < kMelBands; ++i) {
            const double v = std::clamp(m_melFiltered[i] * m_sens, 0.0, 1.0) / kMelBands;
            const QColor c = gradientAt(gradient, double(i) / (kMelBands - 1) * 0.999);
            r += c.red() * v;
            g += c.green() * v;
            b += c.blue() * v;
        }
        const QColor bass = gradientAt(gradient, std::min(m_powerOverlayPos, 0.999));
        const double spark = 0.3 * m_sparks;
        return ledfxColor(0.8 * r + bass.red() * m_powerOverlay + 255 * spark,
                          0.8 * g + bass.green() * m_powerOverlay + 255 * spark,
                          0.8 * b + bass.blue() * m_powerOverlay + 255 * spark);
    }

    if (mode == QLatin1String("magnitude")) {
        const QColor c = gradientAt(gradient, std::min(m_lows, 0.999));
        const double v = 0.2 + 0.8 * m_lows;
        return ledfxColor(c.red() * v, c.green() * v, c.blue() * v);
    }

    if (mode == QLatin1String("wavelength")) {
        // mix of band colors weighted by level
        double r = 0, g = 0, b = 0, total = 0, peak = 0;
        for (int i = 0; i < kMelBands; ++i) {
            const double v = std::clamp(m_melFiltered[i] * m_sens, 0.0, 1.0);
            const QColor c = gradientAt(gradient, double(i) / (kMelBands - 1) * 0.999);
            r += c.red() * v;
            g += c.green() * v;
            b += c.blue() * v;
            total += v;
            peak = std::max(peak, v);
        }
        if (total <= 1e-6)
            return QColor(0, 0, 0);
        const double level = std::min(1.0, 0.6 * peak + 2.0 * total / kMelBands);
        return ledfxColor(r / total * level, g / total * level, b / total * level);
    }

    if (mode == QLatin1String("bar")) {
        // "wipe" + ease-out, brightness = how much of the bar is lit
        const double osc = beatOscillator();
        const QColor c = gradientAt(gradient, m_barColor);
        double level;
        if (osc < 0) {
            level = 0.25 + 0.6 * m_lows; // no tempo yet
        } else {
            const double x = 1 - (osc - 1) * (osc - 1);
            level = m_barPhase == 0 ? x : 1 - x;
        }
        if (quiet)
            level = 0;
        return ledfxColor(c.red() * level, c.green() * level, c.blue() * level);
    }

    if (mode == QLatin1String("strobe")) {
        // BPM Strobe, 2 flashes per beat by default
        const double osc = beatOscillator();
        if (osc < 0 || quiet)
            return QColor(0, 0, 0);
        const double freq = m_speed < 0.75 ? 1 : m_speed <= 1.5 ? 2 : m_speed <= 3 ? 4 : 8;
        const double within = std::fmod(-osc + 1.0, 1.0 / freq) * freq;
        const double strobe = std::pow(within, 1.5) * std::pow(1 - osc, 2);
        const QColor c = gradientAt(gradient, (m_barBeat + osc) / 4 * 0.999);
        // LedFx squares it again, our gamma already does that
        return ledfxColor(c.red() * strobe, c.green() * strobe, c.blue() * strobe);
    }

    if (mode == QLatin1String("realstrobe")) {
        const QColor c = gradientAt(gradient, m_rsBassColor);
        const double white = 255 * 0.35 * m_rsStrobe;
        return ledfxColor(c.red() * m_rsBass + white, c.green() * m_rsBass + white, c.blue() * m_rsBass + white);
    }

    const QColor first = paletteIn.isEmpty() ? QColor(255, 255, 255) : paletteIn.first();

    if (mode == QLatin1String("chill")) {
        const double pos = 0.5 + 0.35 * std::sin(m_time * 0.25 * m_speed) + 0.15 * std::sin(m_time * 0.11 * m_speed + 1.0);
        const QColor c = paletteIn.size() > 1 ? gradientAt(paletteIn, pos * 0.999) : first;
        return shade(c, 0.35 + 0.45 * m_loudSlow + 0.2 * m_swell);
    }

    if (mode == QLatin1String("glow"))
        return shade(first, 0.5 + 0.3 * m_loudSlow);

    if (mode == QLatin1String("idle"))
        return shade(first, 0.25 + 0.1 * (1 - std::cos(2 * kPi * m_time / 6.0)) / 2);

    return shade(first, m_loud);
}
