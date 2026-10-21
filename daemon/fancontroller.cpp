// SPDX-License-Identifier: GPL-3.0-or-later
#include "fancontroller.h"

#include "nvml.h"
#include "protocol.h"

#include <QDateTime>
#include <QJsonArray>

#include <algorithm>
#include <cmath>

namespace {

const QStringList kModes{QStringLiteral("auto"), QStringLiteral("fixed"), QStringLiteral("curve")};

QJsonObject settingToJson(const FanSetting &s)
{
    QJsonArray curve;
    for (const QPointF &p : s.curve)
        curve.append(QJsonArray{p.x(), p.y()});
    return {
        {QStringLiteral("mode"), s.mode},
        {QStringLiteral("fixed"), s.fixed},
        {QStringLiteral("curve"), curve},
        {QStringLiteral("zero_rpm"), s.zeroRpm},
        {QStringLiteral("zero_rpm_temp"), s.zeroRpmTemp},
    };
}

FanSetting settingFromJson(const QJsonObject &o)
{
    FanSetting s;
    const QString mode = o.value(QStringLiteral("mode")).toString();
    if (kModes.contains(mode))
        s.mode = mode;
    s.fixed = igame::toInt(o.value(QStringLiteral("fixed")), 50);
    for (const QJsonValue &v : o.value(QStringLiteral("curve")).toArray()) {
        const QJsonArray p = v.toArray();
        if (p.size() == 2)
            s.curve << QPointF(p[0].toDouble(), p[1].toDouble());
    }
    std::sort(s.curve.begin(), s.curve.end(), [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });
    if (s.curve.size() < 2)
        s.curve = FanConfig::defaultCurve();
    s.zeroRpm = o.value(QStringLiteral("zero_rpm")).toBool(true);
    s.zeroRpmTemp = igame::toInt(o.value(QStringLiteral("zero_rpm_temp")), 45);
    return s;
}

} // namespace

QVector<QPointF> FanConfig::defaultCurve()
{
    return {{30, 30}, {50, 40}, {62, 55}, {72, 70}, {80, 85}, {86, 100}};
}

QString FanConfig::validate(const QJsonObject &o, int fanCount, int minPct, int maxPct)
{
    const QJsonArray fans = o.value(QStringLiteral("fans")).toArray();
    if (fans.isEmpty() || fans.size() > std::max(1, fanCount))
        return QStringLiteral("fan_count");
    for (const QJsonValue &v : fans) {
        const QJsonObject f = v.toObject();
        if (!kModes.contains(f.value(QStringLiteral("mode")).toString()))
            return QStringLiteral("fan_mode");
        const int fixed = igame::toInt(f.value(QStringLiteral("fixed")), -1);
        if (fixed < minPct || fixed > maxPct)
            return QStringLiteral("fan_speed_range|%1|%2").arg(minPct).arg(maxPct);
        const QJsonArray curve = f.value(QStringLiteral("curve")).toArray();
        if (curve.size() < 2 || curve.size() > 12)
            return QStringLiteral("curve_points");
        double lastTemp = -1000;
        for (const QJsonValue &pv : curve) {
            const QJsonArray p = pv.toArray();
            if (p.size() != 2)
                return QStringLiteral("curve_point");
            const double temp = p[0].toDouble(-1), pct = p[1].toDouble(-1);
            if (temp < 0 || temp > 110 || pct < 0 || pct > 100)
                return QStringLiteral("curve_range");
            if (temp <= lastTemp)
                return QStringLiteral("curve_order");
            lastTemp = temp;
        }
        const int zeroTemp = igame::toInt(f.value(QStringLiteral("zero_rpm_temp")), -1);
        if (zeroTemp < 20 || zeroTemp > 80)
            return QStringLiteral("zero_rpm_temp");
    }
    return {};
}

FanConfig FanConfig::fromJson(const QJsonObject &o, int fanCount)
{
    FanConfig c;
    c.sync = o.value(QStringLiteral("sync")).toBool(true);
    for (const QJsonValue &v : o.value(QStringLiteral("fans")).toArray())
        c.fans << settingFromJson(v.toObject());
    const int wanted = std::max(1, fanCount);
    while (c.fans.size() < wanted) {
        FanSetting s = c.fans.isEmpty() ? FanSetting{} : c.fans.first();
        if (s.curve.isEmpty())
            s.curve = defaultCurve();
        c.fans << s;
    }
    c.fans.resize(wanted);
    return c;
}

QJsonObject FanConfig::toJson() const
{
    QJsonArray list;
    for (const FanSetting &s : fans)
        list.append(settingToJson(s));
    return {{QStringLiteral("sync"), sync}, {QStringLiteral("fans"), list}};
}

const FanSetting &FanConfig::settingFor(int fan) const
{
    static const FanSetting fallback;
    if (fans.isEmpty())
        return fallback;
    if (sync || fan < 0)
        return fans.first();
    return fans[std::min<int>(fan, int(fans.size()) - 1)];
}

void FanController::setConfig(const FanConfig &config, int fanCount, int minPct, int maxPct)
{
    m_config = config;
    m_min = minPct;
    m_max = maxPct;
    if (m_state.size() != fanCount)
        m_state.resize(fanCount);
    for (State &st : m_state) {
        st.verifyIn = 0;
        st.retryAt = 0;
        st.zeroIdle = false;
    }
}

QVector<int> FanController::requested() const
{
    QVector<int> out;
    for (const State &st : m_state)
        out << (st.applied >= 0 ? st.applied : -1);
    return out;
}

int FanController::interpolate(const QVector<QPointF> &curve, double temp)
{
    if (curve.isEmpty())
        return 100;
    if (temp <= curve.first().x())
        return int(std::lround(curve.first().y()));
    for (int i = 1; i < curve.size(); ++i) {
        if (temp <= curve[i].x()) {
            const QPointF a = curve[i - 1], b = curve[i];
            const double f = (temp - a.x()) / (b.x() - a.x());
            return int(std::lround(a.y() + (b.y() - a.y()) * f));
        }
    }
    return int(std::lround(curve.last().y()));
}

void FanController::applyAuto(int fan, State &st, qint64 now)
{
    if (st.applied == -1 || now < st.retryAt)
        return;
    if (m_nvml.setFanAuto(uint(fan)) == Nvml::Success) {
        st.applied = -1;
    } else {
        m_error = m_nvml.lastError();
        st.retryAt = now + 30000;
    }
}

void FanController::applyPercent(int fan, State &st, int pct, qint64 now)
{
    pct = std::clamp(pct, m_min, m_max);
    const bool verify = st.verifyIn <= 0;
    if (st.applied == pct && !verify)
        return;
    if (now < st.retryAt)
        return;
    if (verify && st.applied == pct) {
        // driver may take over again after suspend, re-apply now and then
        st.verifyIn = 15;
        const auto target = m_nvml.fanTarget(uint(fan));
        const auto policy = m_nvml.fanPolicy(uint(fan));
        if (target && *target == uint(pct) && policy && *policy == 1)
            return;
    }
    if (m_nvml.setFanSpeed(uint(fan), uint(pct)) == Nvml::Success) {
        st.applied = pct;
        st.verifyIn = 15;
        m_error.clear();
    } else {
        m_error = m_nvml.lastError();
        st.retryAt = now + 30000;
    }
}

void FanController::tick(std::optional<uint> temperature)
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (int fan = 0; fan < m_state.size(); ++fan) {
        State &st = m_state[fan];
        const FanSetting &s = m_config.settingFor(fan);
        --st.verifyIn;

        if (s.mode == QLatin1String("fixed")) {
            applyPercent(fan, st, s.fixed, now);
            continue;
        }
        if (s.mode != QLatin1String("curve") || !temperature) {
            applyAuto(fan, st, now); // also the fail-safe
            continue;
        }

        const double temp = *temperature;
        if (s.zeroRpm) {
            if (temp < s.zeroRpmTemp)
                st.zeroIdle = true;
            else if (temp >= s.zeroRpmTemp + 3)
                st.zeroIdle = false;
            if (st.zeroIdle) {
                applyAuto(fan, st, now);
                continue;
            }
        }
        int target = std::clamp(interpolate(s.curve, temp), m_min, m_max);
        if (st.applied >= 0 && target < st.applied)
            target = std::max(target, st.applied - 2);
        applyPercent(fan, st, target, now);
    }
}

void FanController::releaseAll()
{
    for (int fan = 0; fan < m_state.size(); ++fan) {
        if (m_state[fan].applied >= 0)
            m_nvml.setFanAuto(uint(fan));
        m_state[fan].applied = -1;
    }
}
