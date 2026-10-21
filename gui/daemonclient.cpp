// SPDX-License-Identifier: GPL-3.0-or-later
#include "daemonclient.h"

#include "protocol.h"

#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>

namespace {
const QStringList kHistoryKeys{
    QStringLiteral("temp"), QStringLiteral("power"), QStringLiteral("util"), QStringLiteral("clk_gfx"),
    QStringLiteral("clk_mem"), QStringLiteral("fan"), QStringLiteral("vram"),
};
}

DaemonClient::DaemonClient(QObject *parent)
    : QObject(parent)
    , m_path(igame::socketPath())
{
    connect(&m_socket, &QLocalSocket::connected, this, &DaemonClient::onConnected);
    connect(&m_socket, &QLocalSocket::disconnected, this, &DaemonClient::onDisconnected);
    connect(&m_socket, &QLocalSocket::errorOccurred, this, [this](QLocalSocket::LocalSocketError error) {
        const bool denied = error == QLocalSocket::SocketAccessError;
        if (denied != m_accessDenied) {
            m_accessDenied = denied;
            emit accessDeniedChanged();
        }
        if (m_socket.state() != QLocalSocket::ConnectedState)
            onDisconnected();
    });
    connect(&m_socket, &QLocalSocket::readyRead, this, &DaemonClient::onReadyRead);

    m_reconnectTimer.setInterval(2000);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &DaemonClient::reconnect);
    m_rgbTimer.setSingleShot(true);
    m_rgbTimer.setInterval(40);
    connect(&m_rgbTimer, &QTimer::timeout, this, &DaemonClient::flushRgb);

    for (const QString &key : kHistoryKeys)
        m_history.insert(key, {});
    reconnect();
}

DaemonClient::~DaemonClient()
{
    // m_socket outlives the other members; its disconnect on destruction must not reach our slots.
    m_socket.disconnect(this);
    m_socket.abort();
}

void DaemonClient::reconnect()
{
    if (m_socket.state() == QLocalSocket::UnconnectedState)
        m_socket.connectToServer(m_path);
}

void DaemonClient::onConnected()
{
    m_reconnectTimer.stop();
    if (m_accessDenied) {
        m_accessDenied = false;
        emit accessDeniedChanged();
    }
    m_buffer.clear();
    request(QStringLiteral("subscribe"), {});
}

QString DaemonClient::socketGroup() const
{
    return QFileInfo(m_path).group();
}

void DaemonClient::onDisconnected()
{
    if (m_connected) {
        m_connected = false;
        emit connectedChanged();
    }
    m_buffer.clear();
    m_pending.clear();
    if (!m_reconnectTimer.isActive())
        m_reconnectTimer.start();
}

void DaemonClient::send(const QJsonObject &message)
{
    if (m_socket.state() == QLocalSocket::ConnectedState)
        m_socket.write(igame::encode(message));
}

void DaemonClient::request(const QString &cmd, QJsonObject args, const QString &okMessage)
{
    if (m_socket.state() != QLocalSocket::ConnectedState) {
        emit toast(tr("Not connected to the igamed service"), true);
        return;
    }
    const int id = m_nextId++;
    args[QStringLiteral("cmd")] = cmd;
    args[QStringLiteral("id")] = id;
    m_pending.insert(id, {cmd, okMessage});
    send(args);
}

void DaemonClient::onReadyRead()
{
    m_buffer += m_socket.readAll();
    qsizetype newline;
    while ((newline = m_buffer.indexOf('\n')) >= 0) {
        const QByteArray line = m_buffer.left(newline);
        m_buffer.remove(0, newline + 1);
        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (doc.isObject())
            handleMessage(doc.object());
    }
}

void DaemonClient::handleMessage(const QJsonObject &message)
{
    const QString event = message.value(QStringLiteral("event")).toString();
    if (event == QLatin1String("telemetry")) {
        setTelemetry(message.value(QStringLiteral("data")).toObject());
        return;
    }
    if (event == QLatin1String("led")) {
        const QColor color = QColor::fromString(message.value(QStringLiteral("color")).toString());
        if (color.isValid() && color != m_led) {
            m_led = color;
            emit ledColorChanged();
        }
        return;
    }
    if (event == QLatin1String("config")) {
        m_config = message.value(QStringLiteral("config")).toObject().toVariantMap();
        emit configChanged();
        return;
    }
    if (event == QLatin1String("info")) {
        m_info = message.value(QStringLiteral("info")).toObject().toVariantMap();
        emit infoChanged();
        return;
    }
    if (event == QLatin1String("ledfx")) {
        m_ledfx = message.value(QStringLiteral("ledfx")).toObject().toVariantMap();
        emit ledfxChanged();
        return;
    }

    const int id = message.value(QStringLiteral("id")).toInt(-1);
    if (!m_pending.contains(id))
        return;
    const Pending pending = m_pending.take(id);
    const bool ok = message.value(QStringLiteral("ok")).toBool();
    const QString error = message.value(QStringLiteral("error")).toString();
    if (pending.cmd == QLatin1String("subscribe") && ok) {
        applySnapshot(message);
        m_connected = true;
        emit connectedChanged();
    } else if (!ok) {
        QStringList coded{message.value(QStringLiteral("code")).toString()};
        for (const QJsonValue &arg : message.value(QStringLiteral("args")).toArray())
            coded << arg.toString();
        const QString text = coded.first().isEmpty() ? error : translateError(coded.join(QLatin1Char('|')));
        emit toast(text, true);
    } else if (!pending.okMessage.isEmpty()) {
        emit toast(pending.okMessage, false);
    }
    emit requestFinished(pending.cmd, ok, error);
}

void DaemonClient::applySnapshot(const QJsonObject &snapshot)
{
    m_info = snapshot.value(QStringLiteral("info")).toObject().toVariantMap();
    m_config = snapshot.value(QStringLiteral("config")).toObject().toVariantMap();
    const QColor led = QColor::fromString(snapshot.value(QStringLiteral("led")).toString());
    if (led.isValid())
        m_led = led;
    m_ledfx = snapshot.value(QStringLiteral("ledfx")).toObject().toVariantMap();
    emit infoChanged();
    emit configChanged();
    emit ledColorChanged();
    emit ledfxChanged();
    setTelemetry(snapshot.value(QStringLiteral("telemetry")).toObject());
}

void DaemonClient::setTelemetry(const QJsonObject &telemetry)
{
    if (telemetry.isEmpty())
        return;
    m_telemetry = telemetry.toVariantMap();

    auto push = [this](const QString &key, const QJsonValue &value) {
        if (!value.isDouble())
            return;
        QVariantList &list = m_history[key];
        list.append(value.toDouble());
        while (list.size() > kHistory)
            list.removeFirst();
    };
    for (const QString &key : {QStringLiteral("temp"), QStringLiteral("power"), QStringLiteral("util"),
                               QStringLiteral("clk_gfx"), QStringLiteral("clk_mem")})
        push(key, telemetry.value(key));
    push(QStringLiteral("vram"), telemetry.value(QStringLiteral("vram_used")));
    const QJsonArray fans = telemetry.value(QStringLiteral("fans")).toArray();
    if (!fans.isEmpty()) {
        double sum = 0;
        for (const QJsonValue &f : fans)
            sum += f.toObject().value(QStringLiteral("pct")).toDouble();
        push(QStringLiteral("fan"), sum / fans.size());
    }
    m_historyMap.clear();
    for (auto it = m_history.cbegin(); it != m_history.cend(); ++it)
        m_historyMap.insert(it.key(), it.value());

    emit telemetryChanged();
    emit historyChanged();
}

void DaemonClient::setRgb(const QVariantMap &rgb)
{
    m_pendingRgb = QJsonObject::fromVariantMap(rgb);
    m_rgbQueued = true;
    if (!m_rgbTimer.isActive())
        m_rgbTimer.start();
}

void DaemonClient::flushRgb()
{
    if (!m_rgbQueued)
        return;
    m_rgbQueued = false;
    request(QStringLiteral("set_rgb"), {{QStringLiteral("rgb"), m_pendingRgb}});
}

void DaemonClient::sendFrame(const QColor &color)
{
    send({{QStringLiteral("cmd"), QStringLiteral("rgb_frame")}, {QStringLiteral("color"), color.name()}});
}

void DaemonClient::setOc(const QVariantMap &oc)
{
    request(QStringLiteral("set_oc"), {{QStringLiteral("oc"), QJsonObject::fromVariantMap(oc)}},
            tr("Performance settings applied"));
}

void DaemonClient::resetOc()
{
    request(QStringLiteral("reset_oc"), {}, tr("Performance settings restored to defaults"));
}

void DaemonClient::setFans(const QVariantMap &fans)
{
    request(QStringLiteral("set_fans"), {{QStringLiteral("fans"), QJsonObject::fromVariantMap(fans)}},
            tr("Fan settings applied"));
}

void DaemonClient::setOption(const QString &key, const QVariant &value)
{
    request(QStringLiteral("set_options"),
            {{QStringLiteral("options"), QJsonObject{{key, QJsonValue::fromVariant(value)}}}});
}

void DaemonClient::saveProfile(const QString &name)
{
    request(QStringLiteral("profile_save"), {{QStringLiteral("name"), name}}, tr("Profile \"%1\" saved").arg(name));
}

void DaemonClient::loadProfile(const QString &name)
{
    request(QStringLiteral("profile_load"), {{QStringLiteral("name"), name}}, tr("Profile \"%1\" applied").arg(name));
}

void DaemonClient::deleteProfile(const QString &name)
{
    request(QStringLiteral("profile_delete"), {{QStringLiteral("name"), name}}, tr("Profile \"%1\" deleted").arg(name));
}

void DaemonClient::rescanRgb()
{
    request(QStringLiteral("rescan_rgb"), {}, tr("RGB controller found"));
}

QString DaemonClient::translateError(const QString &coded) const
{
    if (coded.isEmpty())
        return {};
    static const QHash<QString, const char *> templates{
        {QStringLiteral("invalid_json"), QT_TR_NOOP("Invalid request")},
        {QStringLiteral("unknown_command"), QT_TR_NOOP("Unknown command: %1")},
        {QStringLiteral("invalid_effect"), QT_TR_NOOP("Unknown effect")},
        {QStringLiteral("invalid_colors_count"), QT_TR_NOOP("Between 1 and 8 colors are required")},
        {QStringLiteral("invalid_color"), QT_TR_NOOP("Colors must use the #RRGGBB format")},
        {QStringLiteral("invalid_speed"), QT_TR_NOOP("Speed must be between 0.1 and 5")},
        {QStringLiteral("invalid_brightness"), QT_TR_NOOP("Brightness must be between 0 and 100")},
        {QStringLiteral("invalid_sensor_range"), QT_TR_NOOP("Invalid sensor range")},
        {QStringLiteral("invalid_sensitivity"), QT_TR_NOOP("Sensitivity must be between 10 and 300%")},
        {QStringLiteral("fan_count"), QT_TR_NOOP("Wrong number of fan settings")},
        {QStringLiteral("fan_mode"), QT_TR_NOOP("Unknown fan mode")},
        {QStringLiteral("fan_speed_range"), QT_TR_NOOP("Fan speed must be between %1 and %2%")},
        {QStringLiteral("curve_points"), QT_TR_NOOP("A fan curve needs 2 to 12 points")},
        {QStringLiteral("curve_point"), QT_TR_NOOP("Invalid fan curve point")},
        {QStringLiteral("curve_range"), QT_TR_NOOP("Curve points must be within 0-110 °C and 0-100%")},
        {QStringLiteral("curve_order"), QT_TR_NOOP("Curve points must be sorted by temperature")},
        {QStringLiteral("zero_rpm_temp"), QT_TR_NOOP("The 0 RPM temperature must be between 20 and 80 °C")},
        {QStringLiteral("gpu_not_ready"), QT_TR_NOOP("The GPU is not ready yet")},
        {QStringLiteral("power_range"), QT_TR_NOOP("Power limit must be between %1 and %2 W")},
        {QStringLiteral("core_range"), QT_TR_NOOP("Core offset must be between %1 and %2 MHz")},
        {QStringLiteral("mem_range"), QT_TR_NOOP("Memory offset must be between %1 and %2 MHz")},
        {QStringLiteral("lock_range"), QT_TR_NOOP("The clock lock must stay within %1-%2 MHz")},
        {QStringLiteral("nvml_permission"), QT_TR_NOOP("%1: permission denied, igamed must run as root")},
        {QStringLiteral("nvml"), QT_TR_NOOP("%1: %2")},
        {QStringLiteral("nvml_missing"), QT_TR_NOOP("libnvidia-ml.so.1 was not found. Is the NVIDIA driver installed?")},
        {QStringLiteral("profile_name"), QT_TR_NOOP("Profile names must be 1-40 characters long")},
        {QStringLiteral("profile_missing"), QT_TR_NOOP("Profile not found")},
        {QStringLiteral("rgb_no_bus"), QT_TR_NOOP("No NVIDIA I2C bus of a Colorful card was found. Is the i2c-dev module loaded?")},
        {QStringLiteral("rgb_no_reply"), QT_TR_NOOP("The RGB controller did not answer at address 0x61")},
        {QStringLiteral("rgb_open"), QT_TR_NOOP("Cannot open %1: %2")},
        {QStringLiteral("ddp_bind"), QT_TR_NOOP("Cannot open UDP port %1 for LedFx: %2")},
    };
    static const QHash<QString, const char *> operations{
        {QStringLiteral("init"), QT_TR_NOOP("Initialising NVML")},
        {QStringLiteral("device_count"), QT_TR_NOOP("Listing GPUs")},
        {QStringLiteral("clock_offsets"), QT_TR_NOOP("Reading clock offsets")},
        {QStringLiteral("power_limit"), QT_TR_NOOP("Setting the power limit")},
        {QStringLiteral("core_offset"), QT_TR_NOOP("Setting the core offset")},
        {QStringLiteral("mem_offset"), QT_TR_NOOP("Setting the memory offset")},
        {QStringLiteral("lock"), QT_TR_NOOP("Locking clocks")},
        {QStringLiteral("unlock"), QT_TR_NOOP("Releasing the clock lock")},
        {QStringLiteral("fan_speed"), QT_TR_NOOP("Setting the fan speed")},
        {QStringLiteral("fan_auto"), QT_TR_NOOP("Returning fans to automatic control")},
    };
    const QStringList parts = coded.split(QLatin1Char('|'));
    const char *source = templates.value(parts[0], nullptr);
    if (!source)
        return coded;
    QString text = tr(source);
    for (int i = 1; i < parts.size(); ++i) {
        const bool isOperation = i == 1 && parts[0].startsWith(QLatin1String("nvml"));
        const char *op = isOperation ? operations.value(parts[i], nullptr) : nullptr;
        text = text.arg(op ? tr(op) : parts[i]);
    }
    return text;
}
