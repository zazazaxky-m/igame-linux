// SPDX-License-Identifier: GPL-3.0-or-later
// Fan curve interpolation and fan settings validation.

#include "fancontroller.h"

#include <QJsonArray>
#include <QTest>

namespace {

QJsonObject fan(const QString &mode, const QJsonArray &curve = {QJsonArray{40, 30}, QJsonArray{80, 100}})
{
    return {
        {QStringLiteral("mode"), mode},
        {QStringLiteral("fixed"), 50},
        {QStringLiteral("curve"), curve},
        {QStringLiteral("zero_rpm"), true},
        {QStringLiteral("zero_rpm_temp"), 45},
    };
}

QJsonObject config(const QJsonObject &f)
{
    return {{QStringLiteral("sync"), true}, {QStringLiteral("fans"), QJsonArray{f}}};
}

} // namespace

class TestFans : public QObject
{
    Q_OBJECT

private slots:
    void interpolate_data()
    {
        QTest::addColumn<double>("temp");
        QTest::addColumn<int>("pct");
        QTest::newRow("below the curve") << 20.0 << 30;
        QTest::newRow("first point") << 40.0 << 30;
        QTest::newRow("between") << 50.0 << 40;
        QTest::newRow("second segment") << 70.0 << 75;
        QTest::newRow("last point") << 80.0 << 100;
        QTest::newRow("above the curve") << 95.0 << 100;
    }

    void interpolate()
    {
        QFETCH(double, temp);
        QFETCH(int, pct);
        const QVector<QPointF> curve{{40, 30}, {60, 50}, {80, 100}};
        QCOMPARE(FanController::interpolate(curve, temp), pct);
    }

    void defaultCurveIsValid()
    {
        QJsonArray curve;
        for (const QPointF &p : FanConfig::defaultCurve())
            curve.append(QJsonArray{p.x(), p.y()});
        QCOMPARE(FanConfig::validate(config(fan(QStringLiteral("curve"), curve)), 3, 30, 100), QString());
    }

    void rejectsBadConfigs_data()
    {
        QTest::addColumn<QJsonObject>("input");
        QTest::addColumn<QString>("error");
        QTest::newRow("unknown mode") << config(fan(QStringLiteral("turbo"))) << "fan_mode";
        QJsonObject slow = fan(QStringLiteral("fixed"));
        slow[QStringLiteral("fixed")] = 10;
        QTest::newRow("below minimum") << config(slow) << "fan_speed_range|30|100";
        QJsonArray onePoint; // not {QJsonArray{40, 30}}: braces would copy the point itself
        onePoint.append(QJsonArray{40, 30});
        QTest::newRow("one point") << config(fan(QStringLiteral("curve"), onePoint)) << "curve_points";
        QTest::newRow("unsorted") << config(fan(QStringLiteral("curve"), {QJsonArray{60, 50}, QJsonArray{40, 30}})) << "curve_order";
        QTest::newRow("over 100 %") << config(fan(QStringLiteral("curve"), {QJsonArray{40, 30}, QJsonArray{80, 120}})) << "curve_range";
        QJsonObject zero = fan(QStringLiteral("curve"));
        zero[QStringLiteral("zero_rpm_temp")] = 95;
        QTest::newRow("zero rpm temperature") << config(zero) << "zero_rpm_temp";
        QTest::newRow("too many fans") << QJsonObject{{QStringLiteral("fans"), QJsonArray{fan(QStringLiteral("auto")), fan(QStringLiteral("auto"))}}}
                                       << "fan_count";
    }

    void rejectsBadConfigs()
    {
        QFETCH(QJsonObject, input);
        QFETCH(QString, error);
        QCOMPARE(FanConfig::validate(input, 1, 30, 100), error);
    }
};

QTEST_GUILESS_MAIN(TestFans)
#include "test_fans.moc"
