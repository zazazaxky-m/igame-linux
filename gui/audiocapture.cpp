// SPDX-License-Identifier: GPL-3.0-or-later
#include "audiocapture.h"

#include <QStandardPaths>

#include <algorithm>
#include <cmath>

AudioCapture::AudioCapture(QObject *parent)
    : QObject(parent)
{
    m_colors = {QColor(0xFF0055), QColor(0x00D5FF), QColor(0x8C00FF)};
    m_analyzer.setColors(m_colors);
    m_lastStyle = m_analyzer.style();

    m_timer.setInterval(33);
    m_timer.setTimerType(Qt::PreciseTimer);
    connect(&m_timer, &QTimer::timeout, this, &AudioCapture::tick);
    m_restartTimer.setSingleShot(true);
    m_restartTimer.setInterval(3000);
    connect(&m_restartTimer, &QTimer::timeout, this, &AudioCapture::startProcess);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, &AudioCapture::onData);
    connect(&m_process, &QProcess::finished, this, [this] {
        if (m_active) {
            setError(tr("Audio capture stopped, retrying..."));
            m_restartTimer.start();
        }
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart)
            setError(tr("parec was not found (install the libpulse package)"));
    });
    m_clock.start();
}

AudioCapture::~AudioCapture()
{
    m_active = false;
    m_process.disconnect(this);
    stopProcess();
}

void AudioCapture::setActive(bool active)
{
    if (m_active == active)
        return;
    m_active = active;
    if (active) {
        m_analyzer.reset();
        startProcess();
        m_lastTick = m_clock.elapsed();
        m_timer.start();
    } else {
        m_restartTimer.stop();
        m_timer.stop();
        stopProcess();
        m_analyzer.reset();
    }
    emit analysisChanged();
    emit tempoChanged();
    emit activeChanged();
}

void AudioCapture::setMode(const QString &mode)
{
    if (mode == m_mode)
        return;
    m_mode = mode;
    m_analyzer.setMode(mode);
    emit modeChanged();
}

void AudioCapture::setSensitivity(int sensitivity)
{
    sensitivity = std::clamp(sensitivity, 10, 300);
    if (sensitivity == m_sensitivity)
        return;
    m_sensitivity = sensitivity;
    m_analyzer.setSensitivity(sensitivity);
    emit sensitivityChanged();
}

QVariantList AudioCapture::colors() const
{
    QVariantList list;
    for (const QColor &c : m_colors)
        list << c;
    return list;
}

void AudioCapture::setColors(const QVariantList &colors)
{
    QVector<QColor> parsed;
    for (const QVariant &v : colors) {
        const QColor c = v.canConvert<QColor>() ? v.value<QColor>() : QColor::fromString(v.toString());
        if (c.isValid())
            parsed << c;
    }
    if (parsed.isEmpty() || parsed == m_colors)
        return;
    m_colors = parsed;
    m_analyzer.setColors(parsed);
    emit colorsChanged();
}

void AudioCapture::setSpeed(qreal speed)
{
    speed = std::clamp(speed, 0.1, 5.0);
    if (qFuzzyCompare(speed, m_speed))
        return;
    m_speed = speed;
    m_analyzer.setSpeed(speed);
    emit speedChanged();
}

void AudioCapture::setError(const QString &error)
{
    if (error == m_error)
        return;
    m_error = error;
    emit errorChanged();
}

void AudioCapture::startProcess()
{
    if (!m_active || m_process.state() != QProcess::NotRunning)
        return;
    const QString parec = QStandardPaths::findExecutable(QStringLiteral("parec"));
    if (parec.isEmpty()) {
        setError(tr("parec was not found (install the libpulse package)"));
        return;
    }
    setError({});
    m_pendingByte.clear();
    m_process.start(parec, {QStringLiteral("--device=@DEFAULT_MONITOR@"), QStringLiteral("--format=s16le"),
                            QStringLiteral("--rate=48000"), QStringLiteral("--channels=1"),
                            QStringLiteral("--latency-msec=20"), QStringLiteral("--raw"),
                            QStringLiteral("--client-name=iGame Control")});
}

void AudioCapture::stopProcess()
{
    if (m_process.state() == QProcess::NotRunning)
        return;
    m_process.terminate();
    if (!m_process.waitForFinished(500))
        m_process.kill();
}

void AudioCapture::onData()
{
    const QByteArray data = m_pendingByte + m_process.readAllStandardOutput();
    const qsizetype usable = data.size() & ~qsizetype(1);
    m_pendingByte = data.mid(usable);
    const auto *pcm = reinterpret_cast<const qint16 *>(data.constData());
    const qsizetype count = usable / 2;
    m_samples.resize(size_t(count));
    for (qsizetype i = 0; i < count; ++i)
        m_samples[size_t(i)] = pcm[i] / 32768.0f;
    m_analyzer.feed(m_samples.data(), m_samples.size());
}

void AudioCapture::tick()
{
    const qint64 now = m_clock.elapsed();
    const double dt = (now - m_lastTick) / 1000.0;
    m_lastTick = now;

    emit frame(m_analyzer.render(dt));
    m_analyzer.takeBeatEvents(); // only offline tools read these

    emit analysisChanged();
    if (m_analyzer.beatCount() != m_lastBeatCount) {
        m_lastBeatCount = m_analyzer.beatCount();
        emit beat();
    }
    if (bpm() != m_lastBpm || locked() != m_lastLocked) {
        m_lastBpm = bpm();
        m_lastLocked = locked();
        emit tempoChanged();
    }
    if (m_analyzer.style() != m_lastStyle) {
        m_lastStyle = m_analyzer.style();
        emit styleChanged();
    }
}
