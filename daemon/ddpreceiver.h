// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QByteArray>
#include <QColor>
#include <QElapsedTimer>
#include <QObject>
#include <QUdpSocket>

// Receives DDP (Distributed Display Protocol, UDP port 4048), which LedFx, xLights and WLED
// use to stream pixels. The card has one LED, so a frame with several pixels is averaged.
class DdpReceiver : public QObject
{
    Q_OBJECT
public:
    static constexpr quint16 kPort = 4048;

    explicit DdpReceiver(QObject *parent = nullptr);

    // Listens on 127.0.0.1, or on every interface when network is true. Returns false and sets
    // lastError() when the port cannot be opened. Port 0 picks a free one (used by the tests).
    bool listen(bool network, quint16 port = kPort);
    quint16 port() const { return m_socket.localPort(); }
    void close();
    bool listening() const { return m_socket.state() == QAbstractSocket::BoundState; }
    QString lastError() const { return m_error; }

    // Frames shown during the last second, and where they came from.
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
    QByteArray m_pixels;  // RGB bytes of the frame being assembled
    int m_frameBytes = 0; // highest byte written for this frame
    int m_bytesPerPixel = 3;
    QElapsedTimer m_clock;
    qint64 m_lastPush = -100000; // senders that never set PUSH get every packet shown
    QString m_source;
    int m_frames = 0;
    qint64 m_countStart = 0;
    int m_fps = 0;
};
