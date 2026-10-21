// SPDX-License-Identifier: GPL-3.0-or-later
#include "daemon.h"

#include "protocol.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLocalSocket>
#include <QSaveFile>

#include <algorithm>
#include <cmath>
#include <grp.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

const QString kRgb = QStringLiteral("rgb");
const QString kOc = QStringLiteral("oc");
const QString kFans = QStringLiteral("fans");
const QString kOptions = QStringLiteral("options");
const QString kProfiles = QStringLiteral("profiles");

QString architectureName(uint arch)
{
    switch (arch) {
    case 2: return QStringLiteral("Kepler");
    case 3: return QStringLiteral("Maxwell");
    case 4: return QStringLiteral("Pascal");
    case 5: return QStringLiteral("Volta");
    case 6: return QStringLiteral("Turing");
    case 7: return QStringLiteral("Ampere");
    case 8: return QStringLiteral("Ada Lovelace");
    case 9: return QStringLiteral("Hopper");
    case 10: return QStringLiteral("Blackwell");
    default: return QStringLiteral("Unknown (%1)").arg(arch);
    }
}

QJsonArray reasonList(quint64 bits)
{
    static const QList<QPair<quint64, const char *>> names{
        {0x1, "idle"},           {0x2, "app_clocks"},   {0x4, "power_cap"},
        {0x8, "hw_slowdown"},    {0x10, "sync_boost"},  {0x20, "sw_thermal"},
        {0x40, "hw_thermal"},    {0x80, "power_brake"}, {0x100, "display_clock"},
    };
    QJsonArray list;
    quint64 known = 0;
    for (const auto &[bit, name] : names) {
        known |= bit;
        if (bits & bit)
            list.append(QLatin1String(name));
    }
    if (bits & ~known)
        list.append(QStringLiteral("0x%1").arg(bits & ~known, 0, 16));
    return list;
}

double round1(double v)
{
    return std::round(v * 10) / 10;
}

} // namespace

Daemon::Daemon(const DaemonOptions &options, QObject *parent)
    : QObject(parent)
    , m_opts(options)
{
    connect(&m_effects, &EffectEngine::frame, this, &Daemon::onFrame);
    connect(&m_ddp, &DdpReceiver::frame, &m_effects, &EffectEngine::takeoverFrame);
    connect(&m_effects, &EffectEngine::takeoverChanged, this, [this](bool active) {
        if (active)
            qInfo().noquote() << QStringLiteral("LedFx (DDP from %1) took over the lighting").arg(m_ddp.source());
        else
            qInfo() << "LedFx stopped sending; back to the configured effect";
        m_ddpFps = 0;
        broadcastLedfx();
    });
    connect(&m_server, &QLocalServer::newConnection, this, &Daemon::onNewConnection);
    connect(&m_telemetryTimer, &QTimer::timeout, this, &Daemon::pollTelemetry);
    connect(&m_retryTimer, &QTimer::timeout, this, &Daemon::tryInitHardware);
    m_saveTimer.setSingleShot(true);
    connect(&m_saveTimer, &QTimer::timeout, this, &Daemon::saveConfig);
}

bool Daemon::start()
{
    loadConfig();
    m_effects.setSettings(RgbSettings::fromJson(m_config.value(kRgb).toObject()));
    applyLedfxOptions();

    QDir().mkpath(QFileInfo(m_opts.socketPath).path());
    QLocalServer::removeServer(m_opts.socketPath);
    if (!m_server.listen(m_opts.socketPath)) {
        qCritical().noquote() << "Cannot listen on" << m_opts.socketPath << ":" << m_server.errorString();
        return false;
    }
    const QByteArray path = QFile::encodeName(m_opts.socketPath);
    if (geteuid() == 0) {
        // debian/ubuntu have no wheel group
        QStringList candidates{m_opts.socketGroup};
        if (m_opts.socketGroup == QLatin1String("wheel"))
            candidates << QStringLiteral("sudo") << QStringLiteral("admin");
        gid_t gid = 0;
        QString used;
        for (const QString &name : std::as_const(candidates)) {
            if (const group *g = getgrnam(name.toLocal8Bit().constData())) {
                gid = g->gr_gid;
                used = name;
                break;
            }
        }
        if (used.isEmpty())
            qWarning().noquote() << "Group" << m_opts.socketGroup << "does not exist; only root can connect";
        else if (used != m_opts.socketGroup)
            qInfo().noquote() << "No" << m_opts.socketGroup << "group here; the socket is for group" << used;
        if (chown(path.constData(), 0, gid) != 0)
            qWarning() << "chown of the socket failed";
        chmod(path.constData(), 0660);
    } else {
        chmod(path.constData(), 0600);
        qWarning() << "Not running as root: the driver will refuse power, overclock and fan changes";
    }
    qInfo().noquote() << "igamed" << igame::kVersion << "listening on" << m_opts.socketPath;

    tryInitHardware();
    m_retryTimer.start(5000);
    m_telemetryTimer.start(1000);
    return true;
}

void Daemon::shutdown()
{
    if (m_saveTimer.isActive()) {
        m_saveTimer.stop();
        saveConfig();
    }
    if (m_gpuReady)
        m_fans.releaseAll();
    QFile::remove(m_opts.stateDir + QStringLiteral("/oc-pending")); // clean stop is not a crash
    for (QLocalSocket *s : m_clients.keys()) {
        s->disconnect(this);
        s->abort();
        s->deleteLater();
    }
    m_clients.clear();
    m_server.close();
    QFile::remove(m_opts.socketPath);
}

// --- configuration -------------------------------------------------------------

QJsonObject Daemon::defaultOc()
{
    return {
        {QStringLiteral("power_w"), 0},
        {QStringLiteral("core_mhz"), 0},
        {QStringLiteral("mem_mhz"), 0},
        {QStringLiteral("lock_enabled"), false},
        {QStringLiteral("lock_min"), 0},
        {QStringLiteral("lock_max"), 0},
    };
}

void Daemon::loadConfig()
{
    QJsonObject cfg;
    QFile file(m_opts.configPath);
    if (file.open(QIODevice::ReadOnly)) {
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject())
            cfg = doc.object();
        else
            qWarning() << "Configuration is unreadable, using defaults";
    }
    QJsonObject oc = defaultOc();
    const QJsonObject storedOc = cfg.value(kOc).toObject();
    for (auto it = storedOc.begin(); it != storedOc.end(); ++it)
        if (oc.contains(it.key()))
            oc.insert(it.key(), it.value());
    QJsonObject options{
        {QStringLiteral("apply_oc_on_boot"), false},
        {QStringLiteral("ledfx_enabled"), true},
        {QStringLiteral("ledfx_network"), false},
    };
    const QJsonObject storedOptions = cfg.value(kOptions).toObject();
    for (auto it = storedOptions.begin(); it != storedOptions.end(); ++it)
        options.insert(it.key(), it.value());

    m_config = QJsonObject{
        {QStringLiteral("version"), 1},
        {kRgb, RgbSettings::fromJson(cfg.value(kRgb).toObject()).toJson()},
        {kOc, oc},
        {kFans, cfg.value(kFans).toObject()},
        {kOptions, options},
        {kProfiles, cfg.value(kProfiles).toObject()},
    };
}

void Daemon::saveConfig()
{
    QDir().mkpath(QFileInfo(m_opts.configPath).path());
    QSaveFile file(m_opts.configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning().noquote() << "Cannot save" << m_opts.configPath << ":" << file.errorString();
        return;
    }
    file.write(QJsonDocument(m_config).toJson(QJsonDocument::Indented));
    if (!file.commit())
        qWarning().noquote() << "Saving the configuration failed:" << file.errorString();
}

void Daemon::configChanged()
{
    m_saveTimer.start(800);
    broadcast({{QStringLiteral("event"), QStringLiteral("config")}, {QStringLiteral("config"), m_config}});
}

// --- hardware ------------------------------------------------------------------

void Daemon::tryInitHardware()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    bool changed = false;
    if (!m_rgb.present() && now >= m_nextRgbAttempt) {
        initRgb();
        changed = m_rgb.present();
    }
    if (!m_gpuReady && initGpu()) {
        readLimits();
        normalizeFanConfig();
        applyBootOc();
        pollTelemetry();
        changed = true;
    }
    if (changed)
        broadcast({{QStringLiteral("event"), QStringLiteral("info")}, {QStringLiteral("info"), info()}});
    if (m_gpuReady && m_rgb.present())
        m_retryTimer.stop();
}

void Daemon::initRgb()
{
    ++m_rgbAttempts;
    if (m_rgb.open()) {
        qInfo().noquote() << QStringLiteral("RGB: /dev/i2c-%1 model 0x%2 type 0x%3 (%4 LED)%5")
                                 .arg(m_rgb.busNumber())
                                 .arg(m_rgb.modelId(), 4, 16, QLatin1Char('0'))
                                 .arg(m_rgb.deviceType(), 2, 16, QLatin1Char('0'))
                                 .arg(m_rgb.ledCount())
                                 .arg(m_rgb.supported() ? QString() : QStringLiteral(" - not supported yet"));
        m_rgbFailures = 0;
        m_effects.resend();
        return;
    }
    if (m_rgbAttempts == 1 || m_rgbAttempts % 10 == 0)
        qWarning().noquote() << "RGB:" << igame::errorText(m_rgb.lastError());
    m_nextRgbAttempt = QDateTime::currentMSecsSinceEpoch() + (m_rgbAttempts < 6 ? 5000 : 60000);
}

bool Daemon::initGpu()
{
    if (!m_nvml.init() || !m_nvml.selectDevice(m_rgb.pciAddress())) {
        static bool logged = false;
        if (!logged)
            qWarning().noquote() << "NVML:" << igame::errorText(m_nvml.lastError()) << "- retrying every 5 seconds";
        logged = true;
        return false;
    }
    m_gpuReady = true;
    qInfo().noquote() << "GPU:" << m_nvml.name() << "driver" << m_nvml.driverVersion();
    return true;
}

void Daemon::readLimits()
{
    m_nvml.powerRangeW(m_powerMin, m_powerMax);
    m_powerDefault = m_nvml.powerDefaultW().value_or(m_powerMax);
    int current = 0;
    m_nvml.clockOffset(Nvml::ClockGraphics, current, m_coreMin, m_coreMax);
    m_nvml.clockOffset(Nvml::ClockMem, current, m_memMin, m_memMax);
    m_fanCount = int(m_nvml.fanCount());
    m_nvml.fanRange(m_fanMin, m_fanMax);
    const QVector<uint> memClocks = m_nvml.supportedMemoryClocks();
    if (!memClocks.isEmpty())
        m_gfxClocks = m_nvml.supportedGraphicsClocks(*std::max_element(memClocks.begin(), memClocks.end()));
    std::sort(m_gfxClocks.begin(), m_gfxClocks.end(), std::greater<uint>());

    quint64 total = 0, used = 0;
    m_nvml.memoryInfo(total, used);
    m_gpuInfo = {
        {QStringLiteral("name"), m_nvml.name()},
        {QStringLiteral("vbios"), m_nvml.vbiosVersion()},
        {QStringLiteral("driver"), m_nvml.driverVersion()},
        {QStringLiteral("pci"), m_nvml.pciBusId()},
        {QStringLiteral("pcie_gen_max"), int(m_nvml.pcieGeneration(true).value_or(0))},
        {QStringLiteral("pcie_width_max"), int(m_nvml.pcieWidth(true).value_or(0))},
        {QStringLiteral("cores"), int(m_nvml.gpuCores().value_or(0))},
        {QStringLiteral("bus_width"), int(m_nvml.memoryBusWidth().value_or(0))},
        {QStringLiteral("architecture"), architectureName(m_nvml.architecture().value_or(0))},
        {QStringLiteral("vram_total"), int(total >> 20)},
        {QStringLiteral("temp_shutdown"), int(m_nvml.temperatureThreshold(0).value_or(0))},
        {QStringLiteral("temp_slowdown"), int(m_nvml.temperatureThreshold(1).value_or(0))},
        {QStringLiteral("temp_max"), int(m_nvml.temperatureThreshold(3).value_or(0))},
        {QStringLiteral("max_gfx"), int(m_nvml.maxClock(Nvml::ClockGraphics).value_or(0))},
        {QStringLiteral("max_mem"), int(m_nvml.maxClock(Nvml::ClockMem).value_or(0))},
    };
    QJsonArray clocks;
    for (uint c : std::as_const(m_gfxClocks))
        clocks.append(int(c));
    m_limits = {
        {QStringLiteral("power_min"), m_powerMin},
        {QStringLiteral("power_max"), m_powerMax},
        {QStringLiteral("power_default"), m_powerDefault},
        {QStringLiteral("core_min"), m_coreMin},
        {QStringLiteral("core_max"), m_coreMax},
        {QStringLiteral("mem_min"), m_memMin},
        {QStringLiteral("mem_max"), m_memMax},
        {QStringLiteral("fan_count"), m_fanCount},
        {QStringLiteral("fan_min"), int(m_fanMin)},
        {QStringLiteral("fan_max"), int(m_fanMax)},
        {QStringLiteral("gfx_clocks"), clocks},
    };
}

void Daemon::normalizeFanConfig()
{
    const FanConfig fans = FanConfig::fromJson(m_config.value(kFans).toObject(), m_fanCount);
    m_config[kFans] = fans.toJson();
    m_fans.setConfig(fans, m_fanCount, int(m_fanMin), int(m_fanMax));
}

void Daemon::applyBootOc()
{
    if (m_bootOcDone)
        return;
    m_bootOcDone = true;
    QJsonObject options = m_config.value(kOptions).toObject();
    if (!options.value(QStringLiteral("apply_oc_on_boot")).toBool())
        return;

    const QString marker = m_opts.stateDir + QStringLiteral("/oc-pending");
    if (QFile::exists(marker)) {
        // last boot applied OC and died within 2 min, so skip it this time
        m_ocBootSkipped = true;
        QFile::remove(marker);
        options[QStringLiteral("apply_oc_on_boot")] = false;
        m_config[kOptions] = options;
        saveConfig();
        qWarning() << "Overclock not applied: the previous start did not survive 2 minutes. Disabled 'apply on boot'.";
        return;
    }
    QDir().mkpath(m_opts.stateDir);
    QFile flag(marker);
    if (flag.open(QIODevice::WriteOnly))
        flag.close();
    const QString error = applyOc(m_config.value(kOc).toObject());
    if (error.isEmpty())
        qInfo() << "Applied the saved performance settings";
    else
        qWarning().noquote() << "Applying performance settings at start failed:" << igame::errorText(error);
    QTimer::singleShot(120000, this, [marker] { QFile::remove(marker); });
}

// --- runtime -------------------------------------------------------------------

void Daemon::pollTelemetry()
{
    if (m_effects.takenOver()) {
        m_ddpFps = m_ddp.framesPerSecond();
        broadcastLedfx();
    }
    // port was busy, retry every 5s
    if (!m_ddp.listening() && !m_ddp.lastError().isEmpty() && ++m_ddpRetry % 5 == 0
        && m_config.value(kOptions).toObject().value(QStringLiteral("ledfx_enabled")).toBool(true)) {
        applyLedfxOptions();
        if (m_ddp.listening())
            broadcastLedfx();
    }
    if (!m_gpuReady)
        return;
    QJsonObject t;
    const auto temp = m_nvml.temperature();
    const auto power = m_nvml.powerUsageW();
    const auto limit = m_nvml.powerLimitW();
    if (temp)
        t[QStringLiteral("temp")] = int(*temp);
    if (power)
        t[QStringLiteral("power")] = round1(*power);
    if (limit)
        t[QStringLiteral("power_limit")] = round1(*limit);
    uint util = 0, utilMem = 0;
    const bool haveUtil = m_nvml.utilization(util, utilMem);
    if (haveUtil) {
        t[QStringLiteral("util")] = int(util);
        t[QStringLiteral("util_mem")] = int(utilMem);
    }
    if (auto v = m_nvml.encoderUtilization())
        t[QStringLiteral("enc")] = int(*v);
    if (auto v = m_nvml.decoderUtilization())
        t[QStringLiteral("dec")] = int(*v);
    if (auto v = m_nvml.clock(Nvml::ClockGraphics))
        t[QStringLiteral("clk_gfx")] = int(*v);
    if (auto v = m_nvml.clock(Nvml::ClockMem))
        t[QStringLiteral("clk_mem")] = int(*v);
    if (auto v = m_nvml.clock(Nvml::ClockVideo))
        t[QStringLiteral("clk_video")] = int(*v);
    quint64 total = 0, used = 0;
    if (m_nvml.memoryInfo(total, used)) {
        t[QStringLiteral("vram_used")] = int(used >> 20);
        t[QStringLiteral("vram_total")] = int(total >> 20);
    }
    if (auto v = m_nvml.performanceState())
        t[QStringLiteral("pstate")] = int(*v);
    if (auto v = m_nvml.clockEventReasons())
        t[QStringLiteral("reasons")] = reasonList(*v);
    if (auto v = m_nvml.pcieGeneration(false))
        t[QStringLiteral("pcie_gen")] = int(*v);
    if (auto v = m_nvml.pcieWidth(false))
        t[QStringLiteral("pcie_width")] = int(*v);
    int current = 0, lo = 0, hi = 0;
    if (m_nvml.clockOffset(Nvml::ClockGraphics, current, lo, hi))
        t[QStringLiteral("core_offset")] = current;
    if (m_nvml.clockOffset(Nvml::ClockMem, current, lo, hi))
        t[QStringLiteral("mem_offset")] = current;

    m_fans.tick(temp);
    const QVector<int> requested = m_fans.requested();
    QJsonArray fans;
    for (int i = 0; i < m_fanCount; ++i) {
        QJsonObject f;
        if (auto v = m_nvml.fanSpeed(uint(i)))
            f[QStringLiteral("pct")] = int(*v);
        if (auto v = m_nvml.fanRpm(uint(i)))
            f[QStringLiteral("rpm")] = int(*v);
        if (auto v = m_nvml.fanTarget(uint(i)))
            f[QStringLiteral("target")] = int(*v);
        if (auto v = m_nvml.fanPolicy(uint(i)))
            f[QStringLiteral("manual")] = *v == 1;
        f[QStringLiteral("requested")] = requested.value(i, -1);
        fans.append(f);
    }
    t[QStringLiteral("fans")] = fans;
    t[QStringLiteral("fan_error")] = m_fans.lastError();
    t[QStringLiteral("fan_error_text")] = igame::errorText(m_fans.lastError());

    Sensors sensors;
    sensors.temperature = temp ? double(*temp) : -1;
    sensors.load = haveUtil ? double(util) : -1;
    sensors.power = (power && limit && *limit > 0) ? *power / *limit * 100 : -1;
    m_effects.setSensors(sensors);

    m_telemetry = t;
    broadcast({{QStringLiteral("event"), QStringLiteral("telemetry")}, {QStringLiteral("data"), t}});
}

void Daemon::onFrame(const QColor &color)
{
    if (m_rgb.present()) {
        if (m_rgb.setColor(quint8(color.red()), quint8(color.green()), quint8(color.blue()))) {
            m_rgbFailures = 0;
        } else if (++m_rgbFailures >= 30) {
            qWarning() << "RGB: repeated I2C failures, reconnecting";
            m_rgb.close();
            m_rgbAttempts = 0;
            m_nextRgbAttempt = 0;
            m_retryTimer.start(5000);
        }
    }
    broadcast({{QStringLiteral("event"), QStringLiteral("led")}, {QStringLiteral("color"), color.name().toUpper()}});
}

// --- protocol ------------------------------------------------------------------

void Daemon::onNewConnection()
{
    while (QLocalSocket *socket = m_server.nextPendingConnection()) {
        m_clients.insert(socket, Client{});
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] { onReadyRead(socket); });
        connect(socket, &QLocalSocket::disconnected, this, [this, socket] {
            m_clients.remove(socket);
            socket->deleteLater();
        });
    }
}

void Daemon::onReadyRead(QLocalSocket *socket)
{
    if (!m_clients.contains(socket))
        return;
    QByteArray &buffer = m_clients[socket].buffer;
    buffer += socket->readAll();
    if (buffer.size() > (1 << 20)) {
        socket->disconnectFromServer();
        return;
    }
    QList<QByteArray> lines;
    qsizetype newline;
    while ((newline = buffer.indexOf('\n')) >= 0) {
        lines << buffer.left(newline).trimmed();
        buffer.remove(0, newline + 1);
    }
    for (const QByteArray &line : std::as_const(lines)) {
        if (line.isEmpty())
            continue;
        const QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject()) {
            socket->write(igame::encode(igame::errorObject(QStringLiteral("invalid_json"))));
            continue;
        }
        const QJsonObject request = doc.object();
        QJsonObject response = handle(socket, request);
        if (request.contains(QStringLiteral("id"))) {
            response[QStringLiteral("id")] = request.value(QStringLiteral("id"));
            socket->write(igame::encode(response));
        }
    }
}

void Daemon::broadcast(const QJsonObject &event)
{
    QByteArray data;
    for (auto it = m_clients.cbegin(); it != m_clients.cend(); ++it) {
        if (!it->subscribed)
            continue;
        if (data.isEmpty())
            data = igame::encode(event);
        if (it.key()->bytesToWrite() < (1 << 20)) // skip clients that stopped reading
            it.key()->write(data);
    }
}

QJsonObject Daemon::info() const
{
    QJsonObject rgb{
        {QStringLiteral("present"), m_rgb.present()},
        {QStringLiteral("supported"), m_rgb.supported()},
        {QStringLiteral("error"), m_rgb.present() ? QString() : m_rgb.lastError()},
        {QStringLiteral("error_text"), m_rgb.present() ? QString() : igame::errorText(m_rgb.lastError())},
    };
    if (m_rgb.present()) {
        rgb[QStringLiteral("bus")] = QStringLiteral("/dev/i2c-%1").arg(m_rgb.busNumber());
        rgb[QStringLiteral("address")] = QStringLiteral("0x61");
        rgb[QStringLiteral("model")] = QStringLiteral("0x") + QString::number(m_rgb.modelId(), 16).toUpper().rightJustified(4, QLatin1Char('0'));
        rgb[QStringLiteral("type")] = QStringLiteral("0x") + QString::number(m_rgb.deviceType(), 16).toUpper().rightJustified(2, QLatin1Char('0'));
        rgb[QStringLiteral("leds")] = m_rgb.ledCount();
        rgb[QStringLiteral("pci")] = m_rgb.pciAddress();
        rgb[QStringLiteral("ids")] = m_rgb.pciIds();
    }
    return {
        {QStringLiteral("gpu"), m_gpuInfo},
        {QStringLiteral("limits"), m_limits},
        {QStringLiteral("rgb"), rgb},
        {QStringLiteral("daemon"),
         QJsonObject{
             {QStringLiteral("version"), QLatin1String(igame::kVersion)},
             {QStringLiteral("root"), geteuid() == 0},
             {QStringLiteral("gpu_ready"), m_gpuReady},
             {QStringLiteral("gpu_error"), m_gpuReady ? QString() : m_nvml.lastError()},
             {QStringLiteral("gpu_error_text"), m_gpuReady ? QString() : igame::errorText(m_nvml.lastError())},
             {QStringLiteral("oc_boot_skipped"), m_ocBootSkipped},
             {QStringLiteral("effects"), QJsonArray::fromStringList(RgbSettings::effects())},
         }},
    };
}

QJsonObject Daemon::snapshot() const
{
    return {
        {QStringLiteral("version"), QLatin1String(igame::kVersion)},
        {QStringLiteral("info"), info()},
        {QStringLiteral("config"), m_config},
        {QStringLiteral("telemetry"), m_telemetry},
        {QStringLiteral("led"), m_effects.current().name().toUpper()},
        {QStringLiteral("ledfx"), ledfxStatus()},
    };
}

// --- LedFx (DDP) -----------------------------------------------------------------

void Daemon::applyLedfxOptions()
{
    const QJsonObject options = m_config.value(kOptions).toObject();
    const bool enabled = options.value(QStringLiteral("ledfx_enabled")).toBool(true);
    const bool network = options.value(QStringLiteral("ledfx_network")).toBool(false);
    if (!enabled) {
        m_ddp.close();
        return;
    }
    if (m_ddp.listen(network))
        qInfo().noquote() << "LedFx: listening for DDP on" << (network ? "all interfaces" : "127.0.0.1") << "port"
                          << DdpReceiver::kPort;
    else if (m_ddp.lastError() != m_ddpLoggedError) // don't spam the log on retries
        qWarning().noquote() << "LedFx:" << igame::errorText(m_ddp.lastError());
    m_ddpLoggedError = m_ddp.lastError();
}

QJsonObject Daemon::ledfxStatus() const
{
    const QJsonObject options = m_config.value(kOptions).toObject();
    return {
        {QStringLiteral("enabled"), options.value(QStringLiteral("ledfx_enabled")).toBool(true)},
        {QStringLiteral("network"), options.value(QStringLiteral("ledfx_network")).toBool(false)},
        {QStringLiteral("port"), DdpReceiver::kPort},
        {QStringLiteral("listening"), m_ddp.listening()},
        {QStringLiteral("error"), m_ddp.lastError()},
        {QStringLiteral("error_text"), igame::errorText(m_ddp.lastError())},
        {QStringLiteral("active"), m_effects.takenOver()},
        {QStringLiteral("source"), m_effects.takenOver() ? m_ddp.source() : QString()},
        {QStringLiteral("fps"), m_effects.takenOver() ? m_ddpFps : 0},
    };
}

void Daemon::broadcastLedfx()
{
    broadcast({{QStringLiteral("event"), QStringLiteral("ledfx")}, {QStringLiteral("ledfx"), ledfxStatus()}});
}

QString Daemon::setRgb(const QJsonObject &rgb)
{
    const QString error = RgbSettings::validate(rgb);
    if (!error.isEmpty())
        return error;
    const RgbSettings settings = RgbSettings::fromJson(rgb);
    m_config[kRgb] = settings.toJson();
    m_effects.setSettings(settings);
    configChanged();
    return {};
}

QString Daemon::setFans(const QJsonObject &fans)
{
    if (!m_gpuReady)
        return QStringLiteral("gpu_not_ready");
    const QString error = FanConfig::validate(fans, m_fanCount, int(m_fanMin), int(m_fanMax));
    if (!error.isEmpty())
        return error;
    const FanConfig config = FanConfig::fromJson(fans, m_fanCount);
    m_config[kFans] = config.toJson();
    m_fans.setConfig(config, m_fanCount, int(m_fanMin), int(m_fanMax));
    configChanged();
    return {};
}

QString Daemon::applyOc(const QJsonObject &oc, QJsonObject *normalized)
{
    if (!m_gpuReady)
        return QStringLiteral("gpu_not_ready");
    const double power = oc.value(QStringLiteral("power_w")).toDouble(0);
    const int core = igame::toInt(oc.value(QStringLiteral("core_mhz")), 0);
    const int mem = igame::toInt(oc.value(QStringLiteral("mem_mhz")), 0);
    const bool lock = oc.value(QStringLiteral("lock_enabled")).toBool(false);
    const int lockMin = igame::toInt(oc.value(QStringLiteral("lock_min")), 0);
    const int lockMax = igame::toInt(oc.value(QStringLiteral("lock_max")), 0);

    if (power != 0 && (power < m_powerMin || power > m_powerMax))
        return QStringLiteral("power_range|%1|%2").arg(m_powerMin).arg(m_powerMax);
    if (core < m_coreMin || core > m_coreMax)
        return QStringLiteral("core_range|%1|%2").arg(m_coreMin).arg(m_coreMax);
    if (mem < m_memMin || mem > m_memMax)
        return QStringLiteral("mem_range|%1|%2").arg(m_memMin).arg(m_memMax);
    if (lock) {
        const int lo = m_gfxClocks.isEmpty() ? 0 : int(m_gfxClocks.last());
        const int hi = m_gfxClocks.isEmpty() ? 0 : int(m_gfxClocks.first());
        if (lockMin < lo || lockMax > hi || lockMin > lockMax)
            return QStringLiteral("lock_range|%1|%2").arg(lo).arg(hi);
    }

    if (m_nvml.setPowerLimitW(power > 0 ? power : m_powerDefault) != Nvml::Success)
        return m_nvml.lastError();
    if (m_nvml.setClockOffset(Nvml::ClockGraphics, core) != Nvml::Success)
        return m_nvml.lastError();
    if (m_nvml.setClockOffset(Nvml::ClockMem, mem) != Nvml::Success)
        return m_nvml.lastError();
    if (lock) {
        if (m_nvml.setLockedClocks(uint(lockMin), uint(lockMax)) != Nvml::Success)
            return m_nvml.lastError();
        m_lockActive = true;
    } else if (m_lockActive || oc.value(QStringLiteral("force_unlock")).toBool()) {
        if (m_nvml.resetLockedClocks() != Nvml::Success)
            return m_nvml.lastError();
        m_lockActive = false;
    }
    if (normalized) {
        *normalized = {
            {QStringLiteral("power_w"), power},
            {QStringLiteral("core_mhz"), core},
            {QStringLiteral("mem_mhz"), mem},
            {QStringLiteral("lock_enabled"), lock},
            {QStringLiteral("lock_min"), lockMin},
            {QStringLiteral("lock_max"), lockMax},
        };
    }
    return {};
}

QJsonObject Daemon::handle(QLocalSocket *socket, const QJsonObject &request)
{
    const QString cmd = request.value(QStringLiteral("cmd")).toString();
    const auto ok = [](QJsonObject extra = {}) {
        extra[QStringLiteral("ok")] = true;
        return extra;
    };
    const auto fail = [](const QString &coded) { return igame::errorObject(coded); };
    const auto result = [&](const QString &error) { return error.isEmpty() ? ok() : fail(error); };

    if (cmd == QLatin1String("hello"))
        return ok(snapshot());

    if (cmd == QLatin1String("subscribe")) {
        m_clients[socket].subscribed = true;
        return ok(snapshot());
    }

    if (cmd == QLatin1String("rgb_frame")) {
        const QColor color = QColor::fromString(request.value(QStringLiteral("color")).toString());
        if (!color.isValid())
            return fail(QStringLiteral("invalid_color"));
        m_effects.externalFrame(color);
        return ok();
    }

    if (cmd == QLatin1String("set_rgb"))
        return result(setRgb(request.value(kRgb).toObject()));

    if (cmd == QLatin1String("set_fans"))
        return result(setFans(request.value(kFans).toObject()));

    if (cmd == QLatin1String("set_oc") || cmd == QLatin1String("reset_oc")) {
        QJsonObject oc = cmd == QLatin1String("reset_oc") ? defaultOc() : request.value(kOc).toObject();
        if (cmd == QLatin1String("reset_oc"))
            oc[QStringLiteral("force_unlock")] = true;
        QJsonObject normalized;
        const QString error = applyOc(oc, &normalized);
        if (!error.isEmpty())
            return fail(error);
        m_config[kOc] = normalized;
        configChanged();
        return ok();
    }

    if (cmd == QLatin1String("set_options")) {
        QJsonObject options = m_config.value(kOptions).toObject();
        const QJsonObject incoming = request.value(kOptions).toObject();
        if (incoming.contains(QStringLiteral("apply_oc_on_boot"))) {
            options[QStringLiteral("apply_oc_on_boot")] = incoming.value(QStringLiteral("apply_oc_on_boot")).toBool();
            m_ocBootSkipped = false;
        }
        bool ledfx = false;
        for (const QString &key : {QStringLiteral("ledfx_enabled"), QStringLiteral("ledfx_network")}) {
            if (incoming.contains(key)) {
                options[key] = incoming.value(key).toBool();
                ledfx = true;
            }
        }
        m_config[kOptions] = options;
        configChanged();
        if (ledfx) {
            applyLedfxOptions();
            broadcastLedfx();
            if (!m_ddp.lastError().isEmpty())
                return fail(m_ddp.lastError());
        }
        return ok();
    }

    if (cmd.startsWith(QLatin1String("profile_"))) {
        const QString name = request.value(QStringLiteral("name")).toString().trimmed();
        if (name.isEmpty() || name.size() > 40)
            return fail(QStringLiteral("profile_name"));
        QJsonObject profiles = m_config.value(kProfiles).toObject();

        if (cmd == QLatin1String("profile_save")) {
            profiles[name] = QJsonObject{
                {kRgb, m_config.value(kRgb)},
                {kOc, m_config.value(kOc)},
                {kFans, m_config.value(kFans)},
                {QStringLiteral("saved"), QDateTime::currentDateTime().toString(Qt::ISODate)},
            };
            m_config[kProfiles] = profiles;
            configChanged();
            return ok();
        }
        if (!profiles.contains(name))
            return fail(QStringLiteral("profile_missing"));
        if (cmd == QLatin1String("profile_delete")) {
            profiles.remove(name);
            m_config[kProfiles] = profiles;
            configChanged();
            return ok();
        }
        if (cmd == QLatin1String("profile_load")) {
            const QJsonObject profile = profiles.value(name).toObject();
            QStringList errors;
            if (QString e = setRgb(profile.value(kRgb).toObject()); !e.isEmpty())
                errors << e;
            if (QString e = setFans(profile.value(kFans).toObject()); !e.isEmpty())
                errors << e;
            QJsonObject oc = profile.value(kOc).toObject();
            oc[QStringLiteral("force_unlock")] = true;
            QJsonObject normalized;
            if (QString e = applyOc(oc, &normalized); !e.isEmpty()) {
                errors << e;
            } else {
                m_config[kOc] = normalized;
                configChanged();
            }
            return errors.isEmpty() ? ok() : fail(errors.first());
        }
    }

    if (cmd == QLatin1String("rescan_rgb")) {
        m_rgb.close();
        m_rgbAttempts = 0;
        initRgb();
        broadcast({{QStringLiteral("event"), QStringLiteral("info")}, {QStringLiteral("info"), info()}});
        return m_rgb.present() ? ok() : fail(m_rgb.lastError());
    }

    return fail(QStringLiteral("unknown_command|%1").arg(cmd));
}
