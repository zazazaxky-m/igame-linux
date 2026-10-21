// SPDX-License-Identifier: GPL-3.0-or-later
#include "effects.h"

#include <QJsonArray>
#include <QTime>

#include "protocol.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;

double frac(double x)
{
    return x - std::floor(x);
}

double smoothstep(double x)
{
    x = std::clamp(x, 0.0, 1.0);
    return x * x * (3 - 2 * x);
}

QColor mix(const QColor &a, const QColor &b, double f)
{
    f = std::clamp(f, 0.0, 1.0);
    return QColor(int(std::lround(a.red() + (b.red() - a.red()) * f)),
                  int(std::lround(a.green() + (b.green() - a.green()) * f)),
                  int(std::lround(a.blue() + (b.blue() - a.blue()) * f)));
}

QColor scale(const QColor &c, double f)
{
    f = std::clamp(f, 0.0, 1.0);
    return QColor(int(std::lround(c.red() * f)), int(std::lround(c.green() * f)), int(std::lround(c.blue() * f)));
}

QColor hsv(double h, double s = 1.0, double v = 1.0)
{
    return QColor::fromHsvF(float(frac(h)), float(s), float(v)).toRgb();
}

QColor gradient(const QVector<QColor> &stops, double pos)
{
    if (stops.isEmpty())
        return QColor(255, 255, 255);
    if (stops.size() == 1)
        return stops.first();
    const double x = std::clamp(pos, 0.0, 1.0) * (stops.size() - 1);
    const int i = int(std::floor(x));
    if (i >= stops.size() - 1)
        return stops.last();
    return mix(stops[i], stops[i + 1], x - i);
}

// smooth pulse inside [start, start + width) of the cycle
double bump(double p, double start, double width)
{
    if (p < start || p > start + width)
        return 0;
    const double s = std::sin(kPi * (p - start) / width);
    return s * s;
}

} // namespace

const QStringList &RgbSettings::effects()
{
    static const QStringList list{
        QStringLiteral("static"),      QStringLiteral("breathing"), QStringLiteral("spectrum"),
        QStringLiteral("cycle"),       QStringLiteral("flash"),     QStringLiteral("double_flash"),
        QStringLiteral("heartbeat"),   QStringLiteral("strobe"),    QStringLiteral("candle"),
        QStringLiteral("police"),      QStringLiteral("twinkle"),   QStringLiteral("aurora"),
        QStringLiteral("rainbow_breathing"), QStringLiteral("plasma"), QStringLiteral("ocean"),
        QStringLiteral("glitch"),      QStringLiteral("lightning"), QStringLiteral("daytime"),
        QStringLiteral("temperature"), QStringLiteral("load"),      QStringLiteral("power"),
        QStringLiteral("music"),       QStringLiteral("music_smart"), QStringLiteral("music_spectrum"),
        QStringLiteral("music_bass"),  QStringLiteral("music_strobe"), QStringLiteral("music_rainbow"),
        QStringLiteral("music_fire"),  QStringLiteral("music_chill"), QStringLiteral("music_scroll"),
        QStringLiteral("music_realstrobe"),
        QStringLiteral("off"),
    };
    return list;
}

QString RgbSettings::validate(const QJsonObject &o)
{
    if (!effects().contains(o.value(QStringLiteral("effect")).toString()))
        return QStringLiteral("invalid_effect");
    const QJsonArray colors = o.value(QStringLiteral("colors")).toArray();
    if (colors.isEmpty() || colors.size() > 8)
        return QStringLiteral("invalid_colors_count");
    for (const QJsonValue &v : colors) {
        const QString s = v.toString();
        if (s.size() != 7 || !s.startsWith(QLatin1Char('#')) || !QColor::fromString(s).isValid())
            return QStringLiteral("invalid_color");
    }
    const double speed = o.value(QStringLiteral("speed")).toDouble(1);
    if (speed < 0.1 || speed > 5)
        return QStringLiteral("invalid_speed");
    const int brightness = igame::toInt(o.value(QStringLiteral("brightness")), 100);
    if (brightness < 0 || brightness > 100)
        return QStringLiteral("invalid_brightness");
    const double low = o.value(QStringLiteral("low")).toDouble(40);
    const double high = o.value(QStringLiteral("high")).toDouble(80);
    if (low < -50 || high > 500 || high <= low)
        return QStringLiteral("invalid_sensor_range");
    const int sensitivity = igame::toInt(o.value(QStringLiteral("sensitivity")), 100);
    if (sensitivity < 10 || sensitivity > 300)
        return QStringLiteral("invalid_sensitivity");
    return {};
}

RgbSettings RgbSettings::fromJson(const QJsonObject &o)
{
    RgbSettings s;
    const QString effect = o.value(QStringLiteral("effect")).toString();
    if (effects().contains(effect))
        s.effect = effect;
    QVector<QColor> colors;
    for (const QJsonValue &v : o.value(QStringLiteral("colors")).toArray()) {
        const QColor c = QColor::fromString(v.toString());
        if (c.isValid() && colors.size() < 8)
            colors << c;
    }
    if (!colors.isEmpty())
        s.colors = colors;
    s.speed = std::clamp(o.value(QStringLiteral("speed")).toDouble(1.0), 0.1, 5.0);
    s.brightness = std::clamp(igame::toInt(o.value(QStringLiteral("brightness")), 100), 0, 100);
    s.low = o.value(QStringLiteral("low")).toDouble(40);
    s.high = o.value(QStringLiteral("high")).toDouble(80);
    if (s.high <= s.low)
        s.high = s.low + 1;
    s.sensitivity = std::clamp(igame::toInt(o.value(QStringLiteral("sensitivity")), 100), 10, 300);
    return s;
}

QJsonObject RgbSettings::toJson() const
{
    QJsonArray list;
    for (const QColor &c : colors)
        list.append(c.name(QColor::HexRgb).toUpper());
    return {
        {QStringLiteral("effect"), effect},
        {QStringLiteral("colors"), list},
        {QStringLiteral("speed"), speed},
        {QStringLiteral("brightness"), brightness},
        {QStringLiteral("low"), low},
        {QStringLiteral("high"), high},
        {QStringLiteral("sensitivity"), sensitivity},
    };
}

EffectEngine::EffectEngine(QObject *parent)
    : QObject(parent)
{
    m_clock.start();
    connect(&m_timer, &QTimer::timeout, this, &EffectEngine::tick);
    m_timer.setTimerType(Qt::PreciseTimer);
    m_takeoverTimer.setSingleShot(true);
    m_takeoverTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_takeoverTimer, &QTimer::timeout, this, &EffectEngine::tick);
    setSettings(m_settings);
}

void EffectEngine::setSettings(const RgbSettings &settings)
{
    m_settings = settings;
    m_forceSend = true;
    m_sensorPos = -1;
    m_nextNoise = m_nextSparkle = 0;
    m_sparkle = 0;
    m_nextGlitch = m_glitchUntil = m_glitchStep = 0;
    m_nextStrike = 0;
    m_strikes.clear();
    m_timer.start(timerInterval());
    tick();
}

int EffectEngine::timerInterval() const
{
    // static/audio/takeover don't need 30 fps ticks, only the keepalive and a timeout check
    const QString &e = m_settings.effect;
    const bool idle = e == QLatin1String("static") || e == QLatin1String("off") || e == QLatin1String("daytime")
                   || RgbSettings::isAudio(e) || m_takenOver;
    return idle ? 250 : 33;
}

void EffectEngine::externalFrame(const QColor &color)
{
    m_external = color;
    m_externalTime = m_clock.elapsed() / 1000.0;
    // pass audio frames straight through, resampling them on our timer made flashes uneven
    if (RgbSettings::isAudio(m_settings.effect) && !m_takenOver)
        tick();
}

void EffectEngine::takeoverFrame(const QColor &color)
{
    m_takeover = color;
    m_takeoverTime = m_clock.elapsed() / 1000.0;
    if (m_takeoverTimer.isActive())
        return; // timer sends the latest one
    // max 30 fps, each I2C write costs ~2.4 ms of CPU in the nvidia driver
    const double wait = m_lastSend + 1.0 / 30 - m_takeoverTime;
    if (wait <= 0)
        tick();
    else
        m_takeoverTimer.start(int(std::ceil(wait * 1000)));
}

QColor EffectEngine::color(int index) const
{
    const auto &c = m_settings.colors;
    if (c.isEmpty())
        return QColor(255, 255, 255);
    const int n = int(c.size());
    return c[((index % n) + n) % n];
}

void EffectEngine::tick()
{
    const double t = m_clock.elapsed() / 1000.0;
    const double dt = std::clamp(t - m_lastTime, 0.0, 0.5);
    m_lastTime = t;
    const bool takenOver = t - m_takeoverTime < kTakeoverHold;
    if (takenOver != m_takenOver) {
        m_takenOver = takenOver;
        m_forceSend = true;
        m_timer.start(timerInterval());
        emit takeoverChanged(takenOver);
    }
    // LedFx handles brightness itself
    const QColor c = takenOver ? m_takeover : scale(compute(t, dt), m_settings.brightness / 100.0);
    // resend every 3s anyway so the card recovers after suspend (iGame Center does this too)
    if (c != m_last || m_forceSend || t - m_lastSend >= 3.0) {
        m_last = c;
        m_forceSend = false;
        m_lastSend = t;
        emit frame(c);
    }
}

QColor EffectEngine::compute(double t, double dt)
{
    const QString &e = m_settings.effect;
    const double s = m_settings.speed;

    if (e == QLatin1String("off"))
        return QColor(0, 0, 0);

    if (e == QLatin1String("breathing")) {
        const double phase = t * s / 4.0;
        const double level = (1 - std::cos(2 * kPi * frac(phase))) / 2;
        return scale(color(int(std::floor(phase))), std::pow(level, 1.6));
    }
    if (e == QLatin1String("spectrum"))
        return hsv(t * s / 8.0);

    if (e == QLatin1String("cycle")) {
        const double pos = t * s / 3.0;
        const double f = smoothstep((frac(pos) - 0.35) / 0.65);
        const int i = int(std::floor(pos));
        return mix(color(i), color(i + 1), f);
    }
    if (e == QLatin1String("flash")) {
        const double phase = t * s;
        return frac(phase) < 0.5 ? color(int(std::floor(phase))) : QColor(0, 0, 0);
    }
    if (e == QLatin1String("double_flash")) {
        const double phase = t * s / 1.4;
        const double p = frac(phase);
        const bool on = p < 0.07 || (p >= 0.15 && p < 0.22);
        return on ? color(int(std::floor(phase))) : QColor(0, 0, 0);
    }
    if (e == QLatin1String("heartbeat")) {
        const double p = frac(t * s / 1.1);
        return scale(color(0), std::max(bump(p, 0.0, 0.16), 0.65 * bump(p, 0.22, 0.18)));
    }
    if (e == QLatin1String("strobe")) {
        const double phase = t * s * 8.0;
        return frac(phase) < 0.3 ? color(int(std::floor(phase / 8.0))) : QColor(0, 0, 0);
    }
    if (e == QLatin1String("candle")) {
        if (t >= m_nextNoise) {
            m_noiseTarget = 0.45 + m_rng.generateDouble() * 0.55;
            m_nextNoise = t + (0.04 + m_rng.generateDouble() * 0.14) / s;
        }
        m_noise += (m_noiseTarget - m_noise) * std::min(1.0, dt * 14);
        return scale(color(0), m_noise);
    }
    if (e == QLatin1String("police")) {
        const double p = frac(t * s * 0.7);
        const double q = p < 0.5 ? p : p - 0.5;
        const bool on = q < 0.07 || (q >= 0.12 && q < 0.19);
        return on ? color(p < 0.5 ? 0 : 1) : QColor(0, 0, 0);
    }
    if (e == QLatin1String("twinkle")) {
        if (t >= m_nextSparkle) {
            m_sparkle = 1.0;
            m_sparkleColor = color(int(m_rng.bounded(quint32(std::max<qsizetype>(1, m_settings.colors.size())))));
            m_nextSparkle = t + (0.25 + m_rng.generateDouble() * 0.9) / s;
        }
        m_sparkle *= std::exp(-dt * 5 * s);
        if (!m_sparkleColor.isValid())
            m_sparkleColor = color(0);
        return scale(m_sparkleColor, std::max(0.06, m_sparkle));
    }
    if (e == QLatin1String("aurora")) {
        const double level = 0.72 + 0.28 * std::sin(t * s * 0.9);
        if (m_settings.colors.size() >= 2) {
            const double x = (std::sin(t * s * 0.23) + std::sin(t * s * 0.37 + 1.7)) * 0.25 + 0.5;
            return scale(gradient(m_settings.colors, x), level);
        }
        const double hue = color(0).hsvHueF() + 0.08 * std::sin(t * s * 0.3) + 0.05 * std::sin(t * s * 0.51 + 2);
        return hsv(hue, 0.85, level);
    }
    if (e == QLatin1String("temperature") || e == QLatin1String("load") || e == QLatin1String("power")) {
        const double value = e == QLatin1String("temperature") ? m_sensors.temperature
                           : e == QLatin1String("load")        ? m_sensors.load
                                                               : m_sensors.power;
        if (value < 0)
            return color(0);
        const double target = std::clamp((value - m_settings.low) / (m_settings.high - m_settings.low), 0.0, 1.0);
        m_sensorPos = m_sensorPos < 0 ? target : m_sensorPos + (target - m_sensorPos) * std::min(1.0, dt * 3);
        return gradient(m_settings.colors, m_sensorPos);
    }
    if (e == QLatin1String("rainbow_breathing")) {
        const double phase = t * s / 4.0;
        const double level = (1 - std::cos(2 * kPi * frac(phase))) / 2;
        return hsv(std::floor(phase) * 0.618034, 1.0, std::pow(level, 1.6));
    }
    if (e == QLatin1String("plasma")) {
        const double x = 0.5 + 0.25 * std::sin(t * s * 0.9) + 0.15 * std::sin(t * s * 1.7 + 1.0)
                       + 0.1 * std::sin(t * s * 2.9 + 2.0);
        const double level = 0.75 + 0.25 * std::sin(t * s * 1.3 + 0.5);
        return scale(gradient(m_settings.colors, x), level);
    }
    if (e == QLatin1String("ocean")) {
        const double wave = 0.5 + 0.5 * std::sin(t * s * 0.6);
        const double crest = 0.5 + 0.5 * std::sin(t * s * 1.1 + 2.0 * std::sin(t * s * 0.3));
        return scale(gradient(m_settings.colors, wave), 0.55 + 0.45 * crest * crest);
    }
    if (e == QLatin1String("glitch")) {
        const int n = std::max<int>(1, int(m_settings.colors.size()));
        if (t >= m_nextGlitch) {
            m_glitchUntil = t + 0.06 + m_rng.generateDouble() * 0.28;
            m_nextGlitch = m_glitchUntil + (0.3 + m_rng.generateDouble() * 1.6) / s;
            m_glitchStep = t;
            if (m_rng.generateDouble() < 0.35)
                m_glitchBase = int(m_rng.bounded(quint32(n)));
        }
        if (t < m_glitchUntil) {
            if (t >= m_glitchStep) {
                const double r = m_rng.generateDouble();
                m_glitchColor = r < 0.2  ? QColor(0, 0, 0)
                              : r < 0.35 ? QColor(255, 255, 255)
                              : r < 0.55 ? hsv(m_rng.generateDouble())
                                         : color(int(m_rng.bounded(quint32(n))));
                m_glitchStep = t + 0.025 + m_rng.generateDouble() * 0.05;
            }
            return m_glitchColor;
        }
        return color(m_glitchBase);
    }
    if (e == QLatin1String("lightning")) {
        if (t >= m_nextStrike) {
            const int flashes = 1 + int(m_rng.bounded(3u));
            double start = t;
            for (int i = 0; i < flashes; ++i) {
                m_strikes.append({start, 0.55 + m_rng.generateDouble() * 0.45});
                start += 0.07 + m_rng.generateDouble() * 0.16;
            }
            m_nextStrike = start + (1.5 + m_rng.generateDouble() * 5.0) / s;
        }
        double flash = 0;
        for (int i = int(m_strikes.size()) - 1; i >= 0; --i) {
            const double age = t - m_strikes[i].first;
            if (age > 0.6) {
                m_strikes.removeAt(i);
                continue;
            }
            if (age >= 0)
                flash = std::max(flash, m_strikes[i].second * std::exp(-age * 14));
        }
        const QColor storm = scale(color(0), 0.10 + 0.04 * std::sin(t * 0.7));
        const QColor bolt = m_settings.colors.size() > 1 ? color(1) : QColor(0xDD, 0xE6, 0xFF);
        return mix(storm, bolt, flash);
    }
    if (e == QLatin1String("daytime")) {
        // warm/dim at night, white during the day
        struct Key { double hour; QColor color; double level; };
        static const Key keys[] = {
            {0.0, QColor(0xFF, 0x7A, 0x1A), 0.22}, {5.5, QColor(0xFF, 0x7A, 0x1A), 0.22},
            {7.0, QColor(0xFF, 0xB3, 0x8A), 0.70}, {10.0, QColor(0xFF, 0xF4, 0xE5), 1.00},
            {16.0, QColor(0xFF, 0xF4, 0xE5), 1.00}, {18.5, QColor(0xFF, 0x6A, 0x3D), 0.85},
            {20.0, QColor(0xC2, 0x18, 0x5B), 0.60}, {22.0, QColor(0xFF, 0x7A, 0x1A), 0.30},
            {24.0, QColor(0xFF, 0x7A, 0x1A), 0.22},
        };
        const QTime now = QTime::currentTime();
        const double hour = now.hour() + now.minute() / 60.0 + now.second() / 3600.0;
        for (size_t i = 1; i < std::size(keys); ++i) {
            if (hour <= keys[i].hour) {
                const Key &a = keys[i - 1], &b = keys[i];
                const double f = (hour - a.hour) / (b.hour - a.hour);
                return scale(mix(a.color, b.color, f), a.level + (b.level - a.level) * f);
            }
        }
        return scale(keys[0].color, keys[0].level);
    }
    if (RgbSettings::isAudio(e)) {
        if (t - m_externalTime < 1.5)
            return m_external;
        return scale(color(0), 0.15);
    }
    return color(0);
}
