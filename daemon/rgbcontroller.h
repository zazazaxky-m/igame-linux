// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QByteArray>
#include <QString>

#include <optional>

// Colorful iGame RGB controller, I2C 0x61 on the GPU bus.
// packet: AA EF <cmd lo> <cmd hi> <payload> <sum16 LE>
// 0x0281 handshake -> reply[4..5] = model id
// 0x0312 color frame: 01 FF R G B (single zone, type 0x17)
class RgbController
{
public:
    ~RgbController() { close(); }

    bool open();
    void close();
    bool present() const { return m_fd >= 0; }
    bool supported() const { return m_type == 0x17; }
    bool setColor(quint8 r, quint8 g, quint8 b);

    int busNumber() const { return m_bus; }
    int modelId() const { return m_model; }
    int deviceType() const { return m_type; }
    int ledCount() const { return m_leds; }
    QString pciAddress() const { return m_pci; }
    QString pciIds() const { return m_ids; }
    QString lastError() const { return m_error; }

    static QByteArray packet(quint16 cmd, const QByteArray &payload = {});

private:
    bool write(const QByteArray &data);
    QByteArray read(int length);
    std::optional<quint16> handshake();

    int m_fd = -1;
    int m_bus = -1;
    int m_model = 0;
    int m_type = 0;
    int m_leds = 0;
    QString m_pci;
    QString m_ids;
    QString m_error;
};
