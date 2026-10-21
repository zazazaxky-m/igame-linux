// SPDX-License-Identifier: GPL-3.0-or-later
// The DDP receiver LedFx streams to, on a free local port.

#include "ddpreceiver.h"

#include <QSignalSpy>
#include <QTest>
#include <QUdpSocket>

namespace {

// DDP packet: flags, sequence, data type, destination id, 32-bit offset, 16-bit length.
QByteArray ddp(const QByteArray &data, quint32 offset = 0, bool push = true, quint8 type = 0x01, bool timecode = false,
               quint8 id = 1, quint8 extraFlags = 0)
{
    QByteArray p;
    p.append(char(0x40 | (push ? 0x01 : 0) | (timecode ? 0x10 : 0) | extraFlags));
    p.append(char(1));
    p.append(char(type));
    p.append(char(id));
    for (int shift : {24, 16, 8, 0})
        p.append(char((offset >> shift) & 0xFF));
    p.append(char((data.size() >> 8) & 0xFF));
    p.append(char(data.size() & 0xFF));
    if (timecode)
        p.append(QByteArray(4, '\0'));
    return p + data;
}

} // namespace

class TestDdp : public QObject
{
    Q_OBJECT

private:
    DdpReceiver m_receiver;
    QUdpSocket m_sender;

    void send(const QByteArray &packet)
    {
        m_sender.writeDatagram(packet, QHostAddress::LocalHost, m_receiver.port());
    }

    QColor receive(const QByteArray &packet)
    {
        QSignalSpy spy(&m_receiver, &DdpReceiver::frame);
        send(packet);
        if (!spy.wait(2000))
            return {};
        return spy.last().first().value<QColor>();
    }

private slots:
    void initTestCase()
    {
        QVERIFY2(m_receiver.listen(false, 0), qPrintable(m_receiver.lastError()));
        QVERIFY(m_receiver.listening());
        QVERIFY(m_receiver.port() != 0);
    }

    void singlePixel()
    {
        QCOMPARE(receive(ddp(QByteArray::fromHex("ee1128"))), QColor(0xEE, 0x11, 0x28));
        QCOMPARE(m_receiver.source(), QStringLiteral("127.0.0.1"));
    }

    void framesAcrossPacketsAreAveraged()
    {
        // 600 pixels: 300 red then 300 blue, split like LedFx does (480 pixels per packet).
        const QByteArray pixels = QByteArray::fromHex("ff0000").repeated(300) + QByteArray::fromHex("0000ff").repeated(300);
        QSignalSpy spy(&m_receiver, &DdpReceiver::frame);
        send(ddp(pixels.left(1440), 0, false));
        send(ddp(pixels.mid(1440), 1440, true));
        QVERIFY(spy.wait(2000));
        QCOMPARE(spy.count(), 1); // only the PUSH packet shows the frame
        const QColor c = spy.last().first().value<QColor>();
        QCOMPARE(c.red(), 127);
        QCOMPARE(c.green(), 0);
        QCOMPARE(c.blue(), 127);
    }

    void timecodeHeader()
    {
        QCOMPARE(receive(ddp(QByteArray::fromHex("0ac81e"), 0, true, 0x0B, true)), QColor(0x0A, 0xC8, 0x1E));
    }

    void rgbwAddsWhite()
    {
        QCOMPARE(receive(ddp(QByteArray::fromHex("64000032"), 0, true, 0x1B)), QColor(0x96, 0x32, 0x32));
    }

    void ignoresWhatIsNotPixelData()
    {
        QSignalSpy spy(&m_receiver, &DdpReceiver::frame);
        send(QByteArray::fromHex("41"));                                    // too short
        QByteArray v2 = ddp(QByteArray::fromHex("ff0000"));
        v2[0] = char(0x81);                                                 // DDP version 2
        send(v2);
        send(ddp(QByteArray::fromHex("ff0000"), 0xFFFFFFF0u));              // offset far out of range
        send(ddp(QByteArray::fromHex("ff0000")).left(12));                  // shorter than its length
        send(ddp({}, 0, true, 0x01, false, 251, 0x02));                     // status query
        send(ddp(QByteArray::fromHex("ff0000"), 0, true, 0x01, false, 250)); // config destination
        QTest::qWait(300);
        QCOMPARE(spy.count(), 0);
    }

    void stillWorksAfterGarbage()
    {
        QCOMPARE(receive(ddp(QByteArray::fromHex("010203"))), QColor(1, 2, 3));
    }
};

QTEST_GUILESS_MAIN(TestDdp)
#include "test_ddp.moc"
