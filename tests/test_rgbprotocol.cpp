// SPDX-License-Identifier: GPL-3.0-or-later
// The I2C packets sent to the card's lighting controller.

#include "rgbcontroller.h"

#include <QTest>

class TestRgbProtocol : public QObject
{
    Q_OBJECT

private slots:
    void handshake()
    {
        // AA EF, command 0x0281 little endian, no payload, 16-bit sum 0x021C little endian.
        QCOMPARE(RgbController::packet(0x0281), QByteArray::fromHex("aaef81021c02"));
    }

    void colorFrame()
    {
        // Color frame 0x0312 with payload 01 FF R G B (pure red).
        QCOMPARE(RgbController::packet(0x0312, QByteArray::fromHex("01ffff0000")),
                 QByteArray::fromHex("aaef120301ffff0000ad03"));
    }

    void checksumIsSumOfAllPreviousBytes()
    {
        const QByteArray p = RgbController::packet(0x0312, QByteArray::fromHex("01ff123456"));
        quint16 sum = 0;
        for (int i = 0; i < p.size() - 2; ++i)
            sum += quint8(p[i]);
        QCOMPARE(quint8(p[p.size() - 2]), quint8(sum & 0xFF));
        QCOMPARE(quint8(p[p.size() - 1]), quint8(sum >> 8));
    }
};

QTEST_GUILESS_MAIN(TestRgbProtocol)
#include "test_rgbprotocol.moc"
