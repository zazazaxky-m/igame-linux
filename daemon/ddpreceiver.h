// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QByteArray>
#include <QColor>
#include <QElapsedTimer>
#include <QObject>
#include <QUdpSocket>

// DDP receiver (UDP 4048) for LedFx & co. Multi-pixel frames get averaged, the card only has one LED.
class DdpReceiver : public QObject
{
    Q_OBJECT
public:
    static constexpr quint16 kPort = 4048;

    explicit DdpReceiver(QObject *parent = nullptr);

    // port 0 = any free port (tests)
    bool listen(bool network, quint16 port = kPort);
    quint16 port() const { return m_socket.localPort(); }
    void close();
    bool listening() const { return m_socket.state() == QAbstractSocket::BoundState; }
    QString lastError() const { return m_error; }

    int framesPerSecond();
    QString source() const { return m_source; }

signals:
    void frame(const QColor &color);

private:
    void onReadyRead();
    void handle(const QByteArray &packet, const QString &source);
    void emitFrame();

    QUdpSocket m_socket;
    QString m_error;
    QByteArray m_pixels; // current frame
    int m_frameBytes = 0;
    int m_bytesPerPixel = 3;
    QElapsedTimer m_clock;
    qint64 m_lastPush = -100000; // for senders that never set PUSH
    QString m_source;
    int m_frames = 0;
    qint64 m_countStart = 0;
    int m_fps = 0;
};
