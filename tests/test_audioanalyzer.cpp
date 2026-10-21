// SPDX-License-Identifier: GPL-3.0-or-later
// The audio engine on synthetic music: tempo and beat lock, Smart Sound, and every effect.

#include "audioanalyzer.h"

#include <QSet>
#include <QTest>

#include <cmath>
#include <random>

namespace {

constexpr int kRate = AudioAnalyzer::kRate;
constexpr double kPi = 3.14159265358979323846;

// A simple groove: four-on-the-floor kick, snare on 2 and 4, off-beat hi-hats and bass notes,
// and a soft pad. Deterministic, so the expectations below are stable.
std::vector<float> groove(double bpm, double seconds)
{
    const size_t n = size_t(seconds * kRate);
    std::vector<double> out(n, 0.0);
    std::mt19937 rng(3);
    std::normal_distribution<double> noise(0, 1);
    const auto add = [&](double start, const std::vector<double> &s, double gain) {
        const size_t i = size_t(start * kRate);
        for (size_t k = 0; k < s.size() && i + k < n; ++k)
            out[i + k] += s[k] * gain;
    };
    std::vector<double> kick(size_t(0.35 * kRate));
    double phase = 0;
    for (size_t i = 0; i < kick.size(); ++i) {
        const double t = double(i) / kRate;
        phase += 2 * kPi * (45 + 90 * std::exp(-t * 30)) / kRate;
        kick[i] = std::sin(phase) * std::exp(-t * 9) * 0.9;
    }
    std::vector<double> hat(size_t(0.05 * kRate));
    double previous = 0;
    for (size_t i = 0; i < hat.size(); ++i) {
        const double v = noise(rng) * std::exp(-double(i) / kRate * 80) * 0.12;
        hat[i] = v - previous;
        previous = v;
    }
    std::vector<double> snare(size_t(0.2 * kRate));
    for (size_t i = 0; i < snare.size(); ++i) {
        const double t = double(i) / kRate;
        snare[i] = (noise(rng) * 0.3 + 0.4 * std::sin(2 * kPi * 190 * t)) * std::exp(-t * 18) * 0.6;
    }
    const double beat = 60.0 / bpm;
    const double notes[4] = {55, 55, 65.4, 49};
    int k = 0;
    for (double time = 0.5; time < seconds; time += beat, ++k) {
        add(time, kick, 1);
        add(time + beat / 2, hat, 1);
        if (k % 4 == 1 || k % 4 == 3)
            add(time, snare, 1);
        std::vector<double> bass(size_t(beat * 0.45 * kRate));
        for (size_t i = 0; i < bass.size(); ++i) {
            const double t = double(i) / kRate;
            bass[i] = 0.35 * (std::sin(2 * kPi * notes[(k / 4) % 4] * t) > 0 ? 1 : -1) * std::exp(-t * 4);
        }
        add(time + beat / 2, bass, 1);
    }
    double peak = 1e-9;
    for (size_t i = 0; i < n; ++i) {
        const double t = double(i) / kRate;
        out[i] += 0.06 * (std::sin(2 * kPi * 220 * t) + std::sin(2 * kPi * 277 * t) + std::sin(2 * kPi * 330 * t));
        peak = std::max(peak, std::abs(out[i]));
    }
    std::vector<float> samples(n);
    for (size_t i = 0; i < n; ++i)
        samples[i] = float(out[i] / peak * 0.7);
    return samples;
}

// Feeds audio the way the app does (20 ms chunks) and renders at 30 frames per second.
QVector<QColor> play(AudioAnalyzer &a, const std::vector<float> &samples)
{
    QVector<QColor> frames;
    const size_t chunk = kRate / 50;
    double untilFrame = 0;
    for (size_t off = 0; off + chunk <= samples.size(); off += chunk) {
        a.feed(samples.data() + off, chunk);
        untilFrame -= 0.02;
        if (untilFrame <= 0) {
            frames.append(a.render(1.0 / 30));
            untilFrame += 1.0 / 30;
        }
    }
    return frames;
}

} // namespace

class TestAudioAnalyzer : public QObject
{
    Q_OBJECT

private slots:
    void silenceStaysDark()
    {
        AudioAnalyzer a;
        a.setMode(QStringLiteral("music_spectrum"));
        const QVector<QColor> frames = play(a, std::vector<float>(size_t(3 * kRate), 0.0f));
        const QColor last = frames.last();
        QVERIFY(last.red() + last.green() + last.blue() <= 3);
        QCOMPARE(a.beatCount(), 0);
        QCOMPARE(a.level(), 0.0);
    }

    void findsTheTempo_data()
    {
        QTest::addColumn<double>("bpm");
        QTest::newRow("124 BPM") << 124.0;
        QTest::newRow("90 BPM") << 90.0;
    }

    void findsTheTempo()
    {
        QFETCH(double, bpm);
        AudioAnalyzer a;
        play(a, groove(bpm, 20));
        QVERIFY2(std::abs(a.bpm() - bpm) < 3, qPrintable(QStringLiteral("estimated %1 BPM").arg(a.bpm())));
        QVERIFY(a.tempoConfidence() > 0.2);
        QVERIFY(a.locked());
        QVERIFY(a.beatCount() > 20);
    }

    void smartSoundRecognisesElectronicMusic()
    {
        AudioAnalyzer a;
        a.setMode(QStringLiteral("music_smart"));
        play(a, groove(124, 30));
        QCOMPARE(a.style(), QStringLiteral("edm"));
    }

    void everyEffectFollowsTheMusic_data()
    {
        QTest::addColumn<QString>("effect");
        for (const char *e : {"music", "music_smart", "music_spectrum", "music_scroll", "music_bass", "music_strobe",
                              "music_realstrobe", "music_rainbow", "music_fire", "music_chill"})
            QTest::newRow(e) << QString::fromLatin1(e);
    }

    void everyEffectFollowsTheMusic()
    {
        QFETCH(QString, effect);
        AudioAnalyzer a;
        a.setMode(effect);
        const QVector<QColor> frames = play(a, groove(124, 12));
        QSet<QRgb> distinct;
        int lit = 0;
        for (const QColor &c : frames) {
            QVERIFY(c.isValid());
            distinct.insert(c.rgb());
            if (std::max({c.red(), c.green(), c.blue()}) > 20)
                ++lit;
        }
        QVERIFY2(distinct.size() > 20, qPrintable(QStringLiteral("only %1 different colors").arg(distinct.size())));
        QVERIFY2(lit > frames.size() / 20, qPrintable(QStringLiteral("lit in %1 of %2 frames").arg(lit).arg(frames.size())));
    }

    void settingsAreClamped()
    {
        AudioAnalyzer a;
        a.setSensitivity(1000);
        a.setSpeed(-3);
        a.setColors({});
        a.setMode(QStringLiteral("music_bass"));
        const QVector<QColor> frames = play(a, groove(124, 4));
        QVERIFY(!frames.isEmpty());
    }
};

QTEST_GUILESS_MAIN(TestAudioAnalyzer)
#include "test_audioanalyzer.moc"
