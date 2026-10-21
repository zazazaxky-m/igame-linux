// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QJsonObject>
#include <QPointF>
#include <QString>
#include <QVector>

#include <optional>

class Nvml;

struct FanSetting {
    QString mode = QStringLiteral("auto"); // auto | fixed | curve
    int fixed = 50;                        // percent, fixed mode
    QVector<QPointF> curve;                // (°C, %) sorted by temperature
    bool zeroRpm = true;                   // curve mode: hand back to the driver (0 RPM) when cool
    int zeroRpmTemp = 45;
};

struct FanConfig {
    bool sync = true;                      // fans[0] drives every fan
    QVector<FanSetting> fans;

    static QVector<QPointF> defaultCurve();
    static QString validate(const QJsonObject &o, int fanCount, int minPct, int maxPct);
    static FanConfig fromJson(const QJsonObject &o, int fanCount);
    QJsonObject toJson() const;
    const FanSetting &settingFor(int fan) const;
};

// Applies fan settings once per second. Curve mode ramps up immediately and down by at most
// 2 %/s; if the temperature cannot be read the fans go back to driver control.
class FanController
{
public:
    explicit FanController(Nvml &nvml) : m_nvml(nvml) {}

    void setConfig(const FanConfig &config, int fanCount, int minPct, int maxPct);
    void tick(std::optional<uint> temperature);
    void releaseAll();
    QString lastError() const { return m_error; }
    // Per fan: -1 = driver (auto), otherwise the percent this controller requested.
    QVector<int> requested() const;
    // Fan percent for a temperature on a (°C, %) curve sorted by temperature.
    static int interpolate(const QVector<QPointF> &curve, double temp);

private:
    struct State {
        int applied = -2; // -2 unknown, -1 driver auto, >= 0 percent
        bool zeroIdle = false;
        int verifyIn = 0;
        qint64 retryAt = 0;
    };
    void applyAuto(int fan, State &st, qint64 now);
    void applyPercent(int fan, State &st, int pct, qint64 now);

    Nvml &m_nvml;
    FanConfig m_config;
    QVector<State> m_state;
    int m_min = 30;
    int m_max = 100;
    QString m_error;
};
