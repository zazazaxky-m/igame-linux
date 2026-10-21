// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QColor>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QObject>
#include <QRandomGenerator>
#include <QStringList>
#include <QTimer>
#include <QVector>

// Lighting settings as stored in the config and sent by clients.
struct RgbSettings {
    QString effect = QStringLiteral("static");
    QVector<QColor> colors{QColor(255, 255, 255)};
    double speed = 1.0;      // 0.1 .. 5
    int brightness = 100;    // 0 .. 100
    double low = 40;         // sensor modes: value mapped to the first color
    double high = 80;        // sensor modes: value mapped to the last color
    int sensitivity = 100;   // audio modes (10 .. 300 %), used by the GUI's audio analyser

    static const QStringList &effects();
    // Audio modes are rendered by the GUI (user session audio) and streamed as frames.
    static bool isAudio(const QString &effect) { return effect.startsWith(QLatin1String("music")); }
    static QString validate(const QJsonObject &o);
    static RgbSettings fromJson(const QJsonObject &o);
    QJsonObject toJson() const;
};

struct Sensors {
    double temperature = -1;   // °C
    double load = -1;          // %
    double power = -1;         // % of the power limit
};

// Computes one color per frame (single-zone card) and emits it when it changes.
// Like iGame Center's EasyCalc, animation happens on the host and is streamed as frames.
class EffectEngine : public QObject
{
    Q_OBJECT
public:
    explicit EffectEngine(QObject *parent = nullptr);

    void setSettings(const RgbSettings &settings);
    const RgbSettings &settings() const { return m_settings; }
    void setSensors(const Sensors &sensors) { m_sensors = sensors; }
    void externalFrame(const QColor &color);
    // An outside controller (LedFx over DDP) takes over the light while its frames keep coming;
    // the configured effect returns kTakeoverHold seconds after the last one.
    void takeoverFrame(const QColor &color);
    bool takenOver() const { return m_takenOver; }
    QColor current() const { return m_last; }
    void resend() { m_forceSend = true; }

    static constexpr double kTakeoverHold = 2.0;

signals:
    void frame(const QColor &color);
    void takeoverChanged(bool active);

private:
    void tick();
    int timerInterval() const;
    QColor compute(double t, double dt);
    QColor color(int index) const;

    RgbSettings m_settings;
    Sensors m_sensors;
    QTimer m_timer;
    QElapsedTimer m_clock;
    double m_lastTime = 0;
    double m_lastSend = -100;
    QColor m_last;
    bool m_forceSend = true;

    QRandomGenerator m_rng{QRandomGenerator::securelySeeded()};
    double m_noise = 1.0, m_noiseTarget = 1.0, m_nextNoise = 0;
    double m_sparkle = 0, m_nextSparkle = 0;
    QColor m_sparkleColor;
    double m_sensorPos = -1;
    double m_nextGlitch = 0, m_glitchUntil = 0, m_glitchStep = 0;
    QColor m_glitchColor;
    int m_glitchBase = 0;
    double m_nextStrike = 0;
    QVector<QPair<double, double>> m_strikes; // (start time, intensity)
    QColor m_external;
    double m_externalTime = -100;
    QColor m_takeover;
    double m_takeoverTime = -100;
    bool m_takenOver = false;
    QTimer m_takeoverTimer; // paces takeover frames to the card's 30 fps
};
