// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "audioanalyzer.h"

#include <QColor>
#include <QElapsedTimer>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>

// Reads the desktop audio monitor via parec and emits 30 colors/s. Lives in the GUI because
// the root daemon can't reach the user's pulse/pipewire session.
class AudioCapture : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(QString mode READ mode WRITE setMode NOTIFY modeChanged)
    Q_PROPERTY(int sensitivity READ sensitivity WRITE setSensitivity NOTIFY sensitivityChanged)
    Q_PROPERTY(QVariantList colors READ colors WRITE setColors NOTIFY colorsChanged)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(qreal level READ level NOTIFY analysisChanged)
    Q_PROPERTY(qreal bass READ bass NOTIFY analysisChanged)
    Q_PROPERTY(qreal mid READ mid NOTIFY analysisChanged)
    Q_PROPERTY(qreal treble READ treble NOTIFY analysisChanged)
    Q_PROPERTY(int beatCount READ beatCount NOTIFY beat)
    Q_PROPERTY(int bpm READ bpm NOTIFY tempoChanged)
    Q_PROPERTY(qreal tempoConfidence READ tempoConfidence NOTIFY tempoChanged)
    Q_PROPERTY(bool locked READ locked NOTIFY tempoChanged)
    Q_PROPERTY(QString style READ style NOTIFY styleChanged)

public:
    explicit AudioCapture(QObject *parent = nullptr);
    ~AudioCapture() override;

    bool active() const { return m_active; }
    void setActive(bool active);
    QString mode() const { return m_mode; }
    void setMode(const QString &mode);
    int sensitivity() const { return m_sensitivity; }
    void setSensitivity(int sensitivity);
    QVariantList colors() const;
    void setColors(const QVariantList &colors);
    qreal speed() const { return m_speed; }
    void setSpeed(qreal speed);
    QString error() const { return m_error; }

    qreal level() const { return m_analyzer.level(); }
    qreal bass() const { return m_analyzer.band(0); }
    qreal mid() const { return m_analyzer.band(1); }
    qreal treble() const { return m_analyzer.band(2); }
    int beatCount() const { return m_analyzer.beatCount(); }
    int bpm() const { return m_analyzer.tempoConfidence() > 0.15 ? int(std::lround(m_analyzer.bpm())) : 0; }
    qreal tempoConfidence() const { return m_analyzer.tempoConfidence(); }
    bool locked() const { return m_analyzer.locked(); }
    QString style() const { return m_analyzer.style(); }

signals:
    void activeChanged();
    void modeChanged();
    void sensitivityChanged();
    void colorsChanged();
    void speedChanged();
    void errorChanged();
    void analysisChanged();
    void beat();
    void tempoChanged();
    void styleChanged();
    void frame(const QColor &color);

private:
    void startProcess();
    void stopProcess();
    void onData();
    void tick();
    void setError(const QString &error);

    bool m_active = false;
    QString m_mode = QStringLiteral("music");
    int m_sensitivity = 100;
    qreal m_speed = 1.0;
    QVector<QColor> m_colors;
    QString m_error;

    AudioAnalyzer m_analyzer;
    QProcess m_process;
    QTimer m_timer;
    QTimer m_restartTimer;
    QElapsedTimer m_clock;
    qint64 m_lastTick = 0;
    QByteArray m_pendingByte;
    std::vector<float> m_samples;

    // to only emit on real changes
    int m_lastBeatCount = 0;
    int m_lastBpm = -1;
    bool m_lastLocked = false;
    QString m_lastStyle;
};
