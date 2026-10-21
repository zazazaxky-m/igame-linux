// SPDX-License-Identifier: GPL-3.0-or-later
#include "rgbcontroller.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QThread>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

constexpr quint16 kAddress = 0x61;
constexpr quint16 kCmdHandshake = 0x0281;
constexpr quint16 kCmdSetColor = 0x0312;
constexpr uint kNvidiaVendor = 0x10de;
constexpr uint kColorfulSubvendor = 0x7377;

struct Model {
    quint16 id;
    int type;
    int leds;
};
constexpr Model kModels[] = {
    {0x5501, 0x17, 1}, {0x5502, 0x18, 50}, {0x6601, 0x15, 38}, {0x6602, 0x19, 80}, {0x6610, 0x1A, 80},
};

uint readSysfsHex(const QString &dir, const char *name)
{
    QFile file(dir + QLatin1Char('/') + QLatin1String(name));
    if (!file.open(QIODevice::ReadOnly))
        return 0;
    return file.readAll().trimmed().toUInt(nullptr, 0);
}

bool validReply(const QByteArray &reply, quint16 cmd, int length)
{
    if (reply.size() < length)
        return false;
    quint16 sum = 0;
    for (int i = 0; i < length - 2; ++i)
        sum += quint8(reply[i]);
    return quint8(reply[0]) == 0xAA && quint8(reply[1]) == 0xEF && quint8(reply[2]) == (cmd & 0xFF)
        && quint8(reply[3]) == (cmd >> 8) && quint8(reply[length - 2]) == (sum & 0xFF)
        && quint8(reply[length - 1]) == (sum >> 8);
}

} // namespace

QByteArray RgbController::packet(quint16 cmd, const QByteArray &payload)
{
    QByteArray p;
    p.append(char(0xAA)).append(char(0xEF)).append(char(cmd & 0xFF)).append(char(cmd >> 8)).append(payload);
    quint16 sum = 0;
    for (char c : std::as_const(p))
        sum += quint8(c);
    p.append(char(sum & 0xFF)).append(char(sum >> 8));
    return p;
}

bool RgbController::write(const QByteArray &data)
{
    i2c_msg msg{kAddress, 0, quint16(data.size()), reinterpret_cast<__u8 *>(const_cast<char *>(data.constData()))};
    i2c_rdwr_ioctl_data xfer{&msg, 1};
    return ioctl(m_fd, I2C_RDWR, &xfer) >= 0;
}

QByteArray RgbController::read(int length)
{
    QByteArray buf(length, '\0');
    i2c_msg msg{kAddress, I2C_M_RD, quint16(length), reinterpret_cast<__u8 *>(buf.data())};
    i2c_rdwr_ioctl_data xfer{&msg, 1};
    if (ioctl(m_fd, I2C_RDWR, &xfer) < 0)
        return {};
    return buf;
}

std::optional<quint16> RgbController::handshake()
{
    const QByteArray request = packet(kCmdHandshake);
    for (int attempt = 0; attempt < 5; ++attempt) {
        if (write(request)) {
            QThread::msleep(50);
            const QByteArray reply = read(0x40);
            if (validReply(reply, kCmdHandshake, 8))
                return quint16(quint8(reply[4]) | quint8(reply[5]) << 8);
        }
        QThread::msleep(25);
    }
    return std::nullopt;
}

bool RgbController::open()
{
    close();
    const QDir devices(QStringLiteral("/sys/bus/i2c/devices"));
    QList<int> buses;
    for (const QString &entry : devices.entryList({QStringLiteral("i2c-*")}, QDir::Dirs | QDir::System))
        buses << entry.mid(4).toInt();
    std::sort(buses.begin(), buses.end());

    bool sawCandidate = false;
    m_error.clear();
    for (int number : std::as_const(buses)) {
        const QString path = devices.filePath(QStringLiteral("i2c-%1").arg(number));
        QFile nameFile(path + QStringLiteral("/name"));
        if (!nameFile.open(QIODevice::ReadOnly) || !nameFile.readAll().startsWith("NVIDIA i2c adapter"))
            continue;
        const QString pciDir = QFileInfo(QFileInfo(path).canonicalFilePath()).path();
        if (readSysfsHex(pciDir, "vendor") != kNvidiaVendor
            || readSysfsHex(pciDir, "subsystem_vendor") != kColorfulSubvendor)
            continue;
        sawCandidate = true;

        const QByteArray node = QStringLiteral("/dev/i2c-%1").arg(number).toLocal8Bit();
        m_fd = ::open(node.constData(), O_RDWR | O_CLOEXEC);
        if (m_fd < 0) {
            m_error = QStringLiteral("rgb_open|%1|%2").arg(QString::fromLocal8Bit(node), QString::fromLocal8Bit(strerror(errno)));
            continue;
        }
        if (auto model = handshake()) {
            m_bus = number;
            m_model = *model;
            m_type = 0;
            m_leds = 0;
            for (const Model &m : kModels) {
                if (m.id == m_model) {
                    m_type = m.type;
                    m_leds = m.leds;
                }
            }
            m_pci = QFileInfo(pciDir).fileName();
            m_ids = QStringLiteral("%1:%2 sub %3:%4")
                        .arg(readSysfsHex(pciDir, "vendor"), 4, 16, QLatin1Char('0'))
                        .arg(readSysfsHex(pciDir, "device"), 4, 16, QLatin1Char('0'))
                        .arg(readSysfsHex(pciDir, "subsystem_vendor"), 4, 16, QLatin1Char('0'))
                        .arg(readSysfsHex(pciDir, "subsystem_device"), 4, 16, QLatin1Char('0'));
            return true;
        }
        ::close(m_fd);
        m_fd = -1;
    }
    if (!sawCandidate)
        m_error = QStringLiteral("rgb_no_bus");
    else if (m_error.isEmpty())
        m_error = QStringLiteral("rgb_no_reply");
    return false;
}

void RgbController::close()
{
    if (m_fd >= 0)
        ::close(m_fd);
    m_fd = -1;
}

bool RgbController::setColor(quint8 r, quint8 g, quint8 b)
{
    if (m_fd < 0 || !supported())
        return false;
    const char payload[] = {0x01, char(0xFF), char(r), char(g), char(b)};
    return write(packet(kCmdSetColor, QByteArray(payload, sizeof payload)));
}
