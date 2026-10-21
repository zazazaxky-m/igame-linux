// SPDX-License-Identifier: GPL-3.0-or-later
#include "ddpreceiver.h"

#include <QNetworkDatagram>

#include <algorithm>
#include <cstring>

namespace {

// DDP header: flags, sequence, data type, destination id, 32-bit offset, 16-bit length
// (big endian), plus a 32-bit timecode when the TIME flag is set.
constexpr int kHeader = 10;
constexpr quint8 kVersionMask = 0xC0;
constexpr quint8 kVersion1 = 0x40;
constexpr quint8 kPush = 0x01;
constexpr quint8 kQuery = 0x02;
constexpr quint8 kReply = 0x04;
constexpr quint8 kTime = 0x10;
constexpr quint8 kIdDefault = 1;
constexpr quint8 kIdAll = 255;
constexpr int kMaxBytes = 4096 * 3; // more pixels than any strip we would average

quint32 be32(const uchar *p)
{
    return quint32(p[0]) << 24 | quint32(p[1]) << 16 | quint32(p[2]) << 8 | p[3];
}

} // namespace

DdpReceiver::DdpReceiver(QObject *parent)
    : QObject(parent)
{
    m_clock.start();
    connect(&m_socket, &QUdpSocket::readyRead, this, &DdpReceiver::onReadyRead);
}

bool DdpReceiver::listen(bool network, quint16 port)
{
    close();
    const QHostAddress address = network ? QHostAddress(QHostAddress::AnyIPv4) : QHostAddress(QHostAddress::LocalHost);
    if (!m_socket.bind(address, port)) {
        m_error = QStringLiteral("ddp_bind|%1|%2").arg(port).arg(m_socket.errorString());
        return false;
    }
    m_error.clear();
    return true;
}

void DdpReceiver::close()
{
    m_socket.close();
    m_pixels.clear();
    m_frameBytes = 0;
    m_source.clear();
    m_fps = 0;
    m_frames = 0;
}

int DdpReceiver::framesPerSecond()
{
    const qint64 now = m_clock.elapsed();
    if (now - m_countStart >= 1000) {
        m_fps = int(m_frames * 1000 / (now - m_countStart));
        m_frames = 0;
        m_countStart = now;
    }
    return m_fps;
}

void DdpReceiver::onReadyRead()
{
    while (m_socket.hasPendingDatagrams()) {
        const QNetworkDatagram datagram = m_socket.receiveDatagram(1500 + kHeader + 4);
        if (datagram.isValid())
            handle(datagram.data(), datagram.senderAddress().toString());
    }
}

void DdpReceiver::handle(const QByteArray &packet, const QString &source)
{
    if (packet.size() < kHeader)
        return;
    const auto *p = reinterpret_cast<const uchar *>(packet.constData());
    const quint8 flags = p[0];
    if ((flags & kVersionMask) != kVersion1 || (flags & (kQuery | kReply)))
        return; // other versions, status queries and replies are not pixel data
    const quint8 type = p[2];
    const quint8 id = p[3];
    if (id != kIdDefault && id != kIdAll)
        return; // config/status/DMX destinations
    const int header = (flags & kTime) ? kHeader + 4 : kHeader;
    const quint32 offset = be32(p + 4);
    const int length = int(quint16(p[8]) << 8 | p[9]);
    if (packet.size() < header + length || offset > quint32(kMaxBytes))
        return;

    // Data type bits: C R TTT SSS. TTT 3 is RGBW; anything else is treated as 8-bit RGB
    // (LedFx and WLED send type 1 or 0x0B).
    m_bytesPerPixel = ((type >> 3) & 0x07) == 3 ? 4 : 3;
    const int end = std::min(int(offset) + length, kMaxBytes);
    if (m_pixels.size() < end)
        m_pixels.resize(end, '\0');
    if (end > int(offset))
        memcpy(m_pixels.data() + offset, p + header, size_t(end - int(offset)));
    m_frameBytes = std::max(m_frameBytes, end);
    m_source = source;

    const qint64 now = m_clock.elapsed();
    if (flags & kPush) {
        m_lastPush = now;
        emitFrame();
    } else if (now - m_lastPush > 1000) {
        emitFrame(); // a sender that never pushes: show what we have
    }
}

void DdpReceiver::emitFrame()
{
    const int pixels = m_frameBytes / m_bytesPerPixel;
    if (pixels <= 0)
        return;
    const auto *d = reinterpret_cast<const uchar *>(m_pixels.constData());
    quint64 r = 0, g = 0, b = 0;
    for (int i = 0; i < pixels; ++i) {
        const uchar *px = d + i * m_bytesPerPixel;
        const uint white = m_bytesPerPixel == 4 ? px[3] : 0;
        r += std::min(255u, px[0] + white);
        g += std::min(255u, px[1] + white);
        b += std::min(255u, px[2] + white);
    }
    m_frameBytes = 0;
    ++m_frames;
    emit frame(QColor(int(r / quint64(pixels)), int(g / quint64(pixels)), int(b / quint64(pixels))));
}
