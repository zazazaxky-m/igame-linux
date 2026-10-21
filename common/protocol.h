// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Shared bits of the igamed <-> client protocol: newline-delimited JSON over a Unix socket.
// Requests carry "cmd" and an optional "id"; the daemon answers only requests that have an id.
// Subscribed clients also receive {"event": "telemetry" | "led" | "config" | "notice", ...}.

#include <QByteArray>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <cmath>

namespace igame {

#ifndef IGAME_VERSION
#define IGAME_VERSION "1.0.0" // set by CMake from the project version
#endif
constexpr const char *kVersion = IGAME_VERSION;
constexpr const char *kDefaultSocket = "/run/igame-linux/igamed.sock";

inline QString socketPath()
{
    const QByteArray env = qgetenv("IGAME_SOCKET");
    return env.isEmpty() ? QString::fromLatin1(kDefaultSocket) : QString::fromLocal8Bit(env);
}

inline QByteArray encode(const QJsonObject &obj)
{
    return QJsonDocument(obj).toJson(QJsonDocument::Compact) + '\n';
}

// QJsonValue::toInt() rejects non-whole doubles (e.g. 149.99 from a slider); round instead.
inline int toInt(const QJsonValue &value, int fallback = 0)
{
    return value.isDouble() ? int(std::lround(value.toDouble())) : fallback;
}

// Errors travel as "code|arg|arg" so clients can translate them; errorText() is the English text.
inline QString operationText(const QString &op)
{
    static const QHash<QString, QString> ops{
        {QStringLiteral("init"), QStringLiteral("Initialising NVML")},
        {QStringLiteral("device_count"), QStringLiteral("Listing GPUs")},
        {QStringLiteral("clock_offsets"), QStringLiteral("Reading clock offsets")},
        {QStringLiteral("power_limit"), QStringLiteral("Setting the power limit")},
        {QStringLiteral("core_offset"), QStringLiteral("Setting the core offset")},
        {QStringLiteral("mem_offset"), QStringLiteral("Setting the memory offset")},
        {QStringLiteral("lock"), QStringLiteral("Locking clocks")},
        {QStringLiteral("unlock"), QStringLiteral("Releasing the clock lock")},
        {QStringLiteral("fan_speed"), QStringLiteral("Setting the fan speed")},
        {QStringLiteral("fan_auto"), QStringLiteral("Returning fans to automatic control")},
    };
    return ops.value(op, op);
}

inline QString errorText(const QString &coded)
{
    static const QHash<QString, QString> templates{
        {QStringLiteral("invalid_json"), QStringLiteral("Invalid JSON")},
        {QStringLiteral("unknown_command"), QStringLiteral("Unknown command: %1")},
        {QStringLiteral("invalid_effect"), QStringLiteral("Unknown effect")},
        {QStringLiteral("invalid_colors_count"), QStringLiteral("Between 1 and 8 colors are required")},
        {QStringLiteral("invalid_color"), QStringLiteral("Colors must use the #RRGGBB format")},
        {QStringLiteral("invalid_speed"), QStringLiteral("Speed must be between 0.1 and 5")},
        {QStringLiteral("invalid_brightness"), QStringLiteral("Brightness must be between 0 and 100")},
        {QStringLiteral("invalid_sensor_range"), QStringLiteral("Invalid sensor range")},
        {QStringLiteral("invalid_sensitivity"), QStringLiteral("Sensitivity must be between 10 and 300%")},
        {QStringLiteral("fan_count"), QStringLiteral("Wrong number of fan settings")},
        {QStringLiteral("fan_mode"), QStringLiteral("Unknown fan mode")},
        {QStringLiteral("fan_speed_range"), QStringLiteral("Fan speed must be between %1 and %2%")},
        {QStringLiteral("curve_points"), QStringLiteral("A fan curve needs 2 to 12 points")},
        {QStringLiteral("curve_point"), QStringLiteral("Invalid fan curve point")},
        {QStringLiteral("curve_range"), QStringLiteral("Curve points must be within 0-110 °C and 0-100%")},
        {QStringLiteral("curve_order"), QStringLiteral("Curve points must be sorted by temperature")},
        {QStringLiteral("zero_rpm_temp"), QStringLiteral("The 0 RPM temperature must be between 20 and 80 °C")},
        {QStringLiteral("gpu_not_ready"), QStringLiteral("The GPU is not ready yet")},
        {QStringLiteral("power_range"), QStringLiteral("Power limit must be between %1 and %2 W")},
        {QStringLiteral("core_range"), QStringLiteral("Core offset must be between %1 and %2 MHz")},
        {QStringLiteral("mem_range"), QStringLiteral("Memory offset must be between %1 and %2 MHz")},
        {QStringLiteral("lock_range"), QStringLiteral("The clock lock must stay within %1-%2 MHz")},
        {QStringLiteral("nvml_permission"), QStringLiteral("%1: permission denied (igamed must run as root)")},
        {QStringLiteral("nvml"), QStringLiteral("%1: %2")},
        {QStringLiteral("nvml_missing"), QStringLiteral("libnvidia-ml.so.1 not found (is the NVIDIA driver installed?)")},
        {QStringLiteral("profile_name"), QStringLiteral("Profile names must be 1-40 characters long")},
        {QStringLiteral("profile_missing"), QStringLiteral("Profile not found")},
        {QStringLiteral("rgb_no_bus"), QStringLiteral("No NVIDIA I2C bus of a Colorful card was found (is the i2c-dev module loaded?)")},
        {QStringLiteral("rgb_no_reply"), QStringLiteral("The RGB controller did not answer the handshake at address 0x61")},
        {QStringLiteral("rgb_open"), QStringLiteral("Cannot open %1: %2")},
        {QStringLiteral("ddp_bind"), QStringLiteral("Cannot open UDP port %1 for LedFx: %2")},
    };
    if (coded.isEmpty())
        return {};
    const QStringList parts = coded.split(QLatin1Char('|'));
    QString text = templates.value(parts[0], parts[0]);
    for (int i = 1; i < parts.size(); ++i) {
        const bool isOperation = i == 1 && (parts[0] == QLatin1String("nvml") || parts[0] == QLatin1String("nvml_permission"));
        text = text.arg(isOperation ? operationText(parts[i]) : parts[i]);
    }
    return text;
}

inline QJsonObject errorObject(const QString &coded)
{
    const QStringList parts = coded.split(QLatin1Char('|'));
    return {
        {QStringLiteral("ok"), false},
        {QStringLiteral("error"), errorText(coded)},
        {QStringLiteral("code"), parts.value(0)},
        {QStringLiteral("args"), QJsonArray::fromStringList(parts.mid(1))},
    };
}

} // namespace igame
