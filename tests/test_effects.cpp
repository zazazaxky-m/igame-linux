// SPDX-License-Identifier: GPL-3.0-or-later
// Lighting settings validation and the effect engine (brightness, LedFx takeover).

#include "effects.h"

#include <QJsonArray>
#include <QSignalSpy>
#include <QTest>

namespace {

QJsonObject settings(const QString &effect, const QJsonArray &colors = {QStringLiteral("#FF8000")})
{
    return {{QStringLiteral("effect"), effect}, {QStringLiteral("colors"), colors}};
}

QColor lastFrame(const QSignalSpy &spy)
{
    return spy.isEmpty() ? QColor() : spy.last().first().value<QColor>();
}

} // namespace

class TestEffects : public QObject
{
    Q_OBJECT

private slots:
    void everyEffectValidates()
    {
        for (const QString &effect : RgbSettings::effects())
            QVERIFY2(RgbSettings::validate(settings(effect)).isEmpty(), qPrintable(effect));
    }

    void rejectsBadSettings_data()
    {
        QTest::addColumn<QJsonObject>("input");
        QTest::addColumn<QString>("error");
        QTest::newRow("unknown effect") << settings(QStringLiteral("disco")) << "invalid_effect";
        QTest::newRow("no colors") << settings(QStringLiteral("static"), {}) << "invalid_colors_count";
        QJsonArray nine;
        for (int i = 0; i < 9; ++i)
            nine.append(QStringLiteral("#FFFFFF"));
        QTest::newRow("nine colors") << settings(QStringLiteral("cycle"), nine) << "invalid_colors_count";
        QTest::newRow("bad color") << settings(QStringLiteral("static"), {QStringLiteral("#GG0000")}) << "invalid_color";
        QTest::newRow("short color") << settings(QStringLiteral("static"), {QStringLiteral("#FFF")}) << "invalid_color";
        QJsonObject speed = settings(QStringLiteral("breathing"));
        speed[QStringLiteral("speed")] = 9;
        QTest::newRow("speed") << speed << "invalid_speed";
        QJsonObject brightness = settings(QStringLiteral("static"));
        brightness[QStringLiteral("brightness")] = 120;
        QTest::newRow("brightness") << brightness << "invalid_brightness";
        QJsonObject range = settings(QStringLiteral("temperature"));
        range[QStringLiteral("low")] = 80;
        range[QStringLiteral("high")] = 40;
        QTest::newRow("sensor range") << range << "invalid_sensor_range";
        QJsonObject sensitivity = settings(QStringLiteral("music"));
        sensitivity[QStringLiteral("sensitivity")] = 500;
        QTest::newRow("sensitivity") << sensitivity << "invalid_sensitivity";
    }

    void rejectsBadSettings()
    {
        QFETCH(QJsonObject, input);
        QFETCH(QString, error);
        QCOMPARE(RgbSettings::validate(input), error);
    }

    void jsonRoundTrip()
    {
        QJsonObject o = settings(QStringLiteral("aurora"), {QStringLiteral("#00FFA3"), QStringLiteral("#8A2BE2")});
        o[QStringLiteral("speed")] = 2.5;
        o[QStringLiteral("brightness")] = 70;
        o[QStringLiteral("sensitivity")] = 140;
        const RgbSettings s = RgbSettings::fromJson(o);
        QCOMPARE(s.effect, QStringLiteral("aurora"));
        QCOMPARE(s.colors.size(), 2);
        QCOMPARE(s.speed, 2.5);
        QCOMPARE(s.brightness, 70);
        QCOMPARE(s.sensitivity, 140);
        QCOMPARE(RgbSettings::fromJson(s.toJson()).toJson(), s.toJson());
    }

    void audioEffectsAreRecognised()
    {
        QVERIFY(RgbSettings::isAudio(QStringLiteral("music_smart")));
        QVERIFY(RgbSettings::isAudio(QStringLiteral("music_scroll")));
        QVERIFY(!RgbSettings::isAudio(QStringLiteral("static")));
    }

    void staticColorUsesBrightness()
    {
        EffectEngine engine;
        QSignalSpy frames(&engine, &EffectEngine::frame);
        QJsonObject o = settings(QStringLiteral("static"), {QStringLiteral("#FF8000")});
        o[QStringLiteral("brightness")] = 50;
        engine.setSettings(RgbSettings::fromJson(o));
        QTRY_VERIFY(!frames.isEmpty());
        const QColor c = lastFrame(frames);
        QVERIFY(std::abs(c.red() - 128) <= 1);
        QVERIFY(std::abs(c.green() - 64) <= 1);
        QCOMPARE(c.blue(), 0);
    }

    void takeoverShowsFramesAndHandsBack()
    {
        EffectEngine engine;
        engine.setSettings(RgbSettings::fromJson(settings(QStringLiteral("static"), {QStringLiteral("#FFFFFF")})));
        QSignalSpy frames(&engine, &EffectEngine::frame);
        QSignalSpy takeover(&engine, &EffectEngine::takeoverChanged);

        engine.takeoverFrame(QColor(0x12, 0x34, 0x56)); // shown as sent, no brightness scaling
        QTRY_COMPARE(lastFrame(frames), QColor(0x12, 0x34, 0x56));
        QVERIFY(engine.takenOver());
        QCOMPARE(takeover.count(), 1);

        // The configured effect returns once the frames stop.
        QTRY_VERIFY_WITH_TIMEOUT(!engine.takenOver(), int(EffectEngine::kTakeoverHold * 1000) + 1500);
        QTRY_COMPARE(lastFrame(frames), QColor(255, 255, 255));
    }

    void takeoverIsPacedTo30Fps()
    {
        EffectEngine engine;
        engine.setSettings(RgbSettings::fromJson(settings(QStringLiteral("static"))));
        QSignalSpy frames(&engine, &EffectEngine::frame);
        QElapsedTimer clock;
        clock.start();
        int sent = 0;
        while (clock.elapsed() < 1000) { // a sender at ~120 fps
            engine.takeoverFrame(QColor(sent % 256, 0, 255 - sent % 256));
            ++sent;
            QTest::qWait(8);
        }
        QVERIFY2(frames.count() <= 33, qPrintable(QStringLiteral("%1 frames in one second").arg(frames.count())));
        QVERIFY(frames.count() >= 20);
    }
};

QTEST_GUILESS_MAIN(TestEffects)
#include "test_effects.moc"
