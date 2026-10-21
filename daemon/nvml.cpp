// SPDX-License-Identifier: GPL-3.0-or-later
#include "nvml.h"

#include <QStringList>

#include <cmath>
#include <dlfcn.h>

namespace {

struct ClockOffsetV1 {
    unsigned version;
    int type;
    int pstate;
    int offsetMHz;
    int minOffsetMHz;
    int maxOffsetMHz;
};
constexpr unsigned kClockOffsetV1 = sizeof(ClockOffsetV1) | (1u << 24);

struct MemoryV2 {
    unsigned version;
    unsigned long long total;
    unsigned long long reserved;
    unsigned long long free;
    unsigned long long used;
};
constexpr unsigned kMemoryV2 = sizeof(MemoryV2) | (2u << 24);

struct FanRpmV1 {
    unsigned version;
    unsigned fan;
    unsigned speed;
};
constexpr unsigned kFanRpmV1 = sizeof(FanRpmV1) | (1u << 24);

struct Utilization {
    unsigned gpu;
    unsigned memory;
};

struct PciInfo {
    char busIdLegacy[16];
    unsigned domain;
    unsigned bus;
    unsigned device;
    unsigned pciDeviceId;
    unsigned pciSubSystemId;
    char busId[32];
};

constexpr int kPstate0 = 0;
constexpr int kTemperatureGpu = 0;

} // namespace

Nvml::~Nvml()
{
    shutdown();
    if (m_lib)
        dlclose(m_lib);
}

void *Nvml::sym(const char *name)
{
    if (!m_lib)
        return nullptr;
    const QByteArray key(name);
    auto it = m_syms.constFind(key);
    if (it != m_syms.constEnd())
        return it.value();
    void *fn = dlsym(m_lib, name);
    m_syms.insert(key, fn);
    return fn;
}

template <typename... Args>
int Nvml::call(const char *name, Args... args)
{
    void *fn = sym(name);
    if (!fn)
        return ErrFunctionNotFound;
    return reinterpret_cast<int (*)(Args...)>(fn)(args...);
}

template <typename... Args>
int Nvml::dev(const char *name, Args... args)
{
    if (!m_device)
        return ErrNotSupported;
    return call(name, m_device, args...);
}

int Nvml::check(int code, const char *what)
{
    if (code != Success) {
        auto errorString = reinterpret_cast<const char *(*)(int)>(sym("nvmlErrorString"));
        const QString text = errorString ? QString::fromUtf8(errorString(code)) : QString::number(code);
        m_lastError = code == ErrNoPermission
                          ? QStringLiteral("nvml_permission|%1").arg(QLatin1String(what))
                          : QStringLiteral("nvml|%1|%2").arg(QLatin1String(what), text);
    }
    return code;
}

std::optional<uint> Nvml::devUint(const char *name)
{
    unsigned value = 0;
    if (dev(name, &value) != Success)
        return std::nullopt;
    return value;
}

bool Nvml::init()
{
    if (m_initialized)
        return true;
    if (!m_lib) {
        m_lib = dlopen("libnvidia-ml.so.1", RTLD_NOW | RTLD_LOCAL);
        if (!m_lib) {
            m_lastError = QStringLiteral("nvml_missing");
            return false;
        }
    }
    if (check(call("nvmlInit_v2"), "init") != Success)
        return false;
    m_initialized = true;
    return true;
}

void Nvml::shutdown()
{
    if (m_initialized)
        call("nvmlShutdown");
    m_initialized = false;
    m_device = nullptr;
}

bool Nvml::selectDevice(const QString &pciAddress)
{
    unsigned count = 0;
    if (check(call("nvmlDeviceGetCount_v2", &count), "device_count") != Success || count == 0)
        return false;

    unsigned wantDomain = 0, wantBus = 0, wantDevice = 0;
    bool haveWanted = false;
    const QStringList parts = pciAddress.split(QLatin1Char(':'));
    if (parts.size() == 3) {
        wantDomain = parts[0].toUInt(nullptr, 16);
        wantBus = parts[1].toUInt(nullptr, 16);
        wantDevice = parts[2].section(QLatin1Char('.'), 0, 0).toUInt(nullptr, 16);
        haveWanted = true;
    }

    void *chosen = nullptr;
    for (unsigned i = 0; i < count; ++i) {
        void *handle = nullptr;
        if (call("nvmlDeviceGetHandleByIndex_v2", i, &handle) != Success)
            continue;
        if (!chosen)
            chosen = handle;
        PciInfo pci{};
        if (haveWanted && call("nvmlDeviceGetPciInfo_v3", handle, &pci) == Success
            && pci.domain == wantDomain && pci.bus == wantBus && pci.device == wantDevice) {
            chosen = handle;
            break;
        }
    }
    m_device = chosen;
    return m_device != nullptr;
}

QString Nvml::name()
{
    char buf[96] = {};
    dev("nvmlDeviceGetName", buf, unsigned(sizeof buf));
    return QString::fromUtf8(buf);
}

QString Nvml::vbiosVersion()
{
    char buf[64] = {};
    dev("nvmlDeviceGetVbiosVersion", buf, unsigned(sizeof buf));
    return QString::fromUtf8(buf);
}

QString Nvml::driverVersion()
{
    char buf[80] = {};
    call("nvmlSystemGetDriverVersion", buf, unsigned(sizeof buf));
    return QString::fromUtf8(buf);
}

QString Nvml::pciBusId()
{
    PciInfo pci{};
    if (dev("nvmlDeviceGetPciInfo_v3", &pci) != Success)
        return {};
    return QString::fromLatin1(pci.busId);
}

std::optional<uint> Nvml::pcieGeneration(bool max)
{
    return devUint(max ? "nvmlDeviceGetMaxPcieLinkGeneration" : "nvmlDeviceGetCurrPcieLinkGeneration");
}

std::optional<uint> Nvml::pcieWidth(bool max)
{
    return devUint(max ? "nvmlDeviceGetMaxPcieLinkWidth" : "nvmlDeviceGetCurrPcieLinkWidth");
}

std::optional<uint> Nvml::gpuCores() { return devUint("nvmlDeviceGetNumGpuCores"); }
std::optional<uint> Nvml::memoryBusWidth() { return devUint("nvmlDeviceGetMemoryBusWidth"); }
std::optional<uint> Nvml::architecture() { return devUint("nvmlDeviceGetArchitecture"); }

bool Nvml::memoryInfo(quint64 &totalBytes, quint64 &usedBytes)
{
    MemoryV2 mem{kMemoryV2, 0, 0, 0, 0};
    if (dev("nvmlDeviceGetMemoryInfo_v2", &mem) != Success)
        return false;
    totalBytes = mem.total;
    usedBytes = mem.used;
    return true;
}

std::optional<uint> Nvml::temperature()
{
    unsigned value = 0;
    if (dev("nvmlDeviceGetTemperature", kTemperatureGpu, &value) != Success)
        return std::nullopt;
    return value;
}

std::optional<uint> Nvml::temperatureThreshold(int type)
{
    unsigned value = 0;
    if (dev("nvmlDeviceGetTemperatureThreshold", type, &value) != Success)
        return std::nullopt;
    return value;
}

std::optional<double> Nvml::powerUsageW()
{
    auto mw = devUint("nvmlDeviceGetPowerUsage");
    return mw ? std::optional<double>(*mw / 1000.0) : std::nullopt;
}

std::optional<double> Nvml::powerLimitW()
{
    auto mw = devUint("nvmlDeviceGetPowerManagementLimit");
    return mw ? std::optional<double>(*mw / 1000.0) : std::nullopt;
}

std::optional<double> Nvml::powerDefaultW()
{
    auto mw = devUint("nvmlDeviceGetPowerManagementDefaultLimit");
    return mw ? std::optional<double>(*mw / 1000.0) : std::nullopt;
}

bool Nvml::powerRangeW(double &min, double &max)
{
    unsigned lo = 0, hi = 0;
    if (dev("nvmlDeviceGetPowerManagementLimitConstraints", &lo, &hi) != Success)
        return false;
    min = lo / 1000.0;
    max = hi / 1000.0;
    return true;
}

std::optional<uint> Nvml::clock(ClockType type)
{
    unsigned value = 0;
    if (dev("nvmlDeviceGetClockInfo", int(type), &value) != Success)
        return std::nullopt;
    return value;
}

std::optional<uint> Nvml::maxClock(ClockType type)
{
    unsigned value = 0;
    if (dev("nvmlDeviceGetMaxClockInfo", int(type), &value) != Success)
        return std::nullopt;
    return value;
}

bool Nvml::utilization(uint &gpu, uint &memory)
{
    Utilization util{};
    if (dev("nvmlDeviceGetUtilizationRates", &util) != Success)
        return false;
    gpu = util.gpu;
    memory = util.memory;
    return true;
}

std::optional<uint> Nvml::encoderUtilization()
{
    unsigned value = 0, period = 0;
    if (dev("nvmlDeviceGetEncoderUtilization", &value, &period) != Success)
        return std::nullopt;
    return value;
}

std::optional<uint> Nvml::decoderUtilization()
{
    unsigned value = 0, period = 0;
    if (dev("nvmlDeviceGetDecoderUtilization", &value, &period) != Success)
        return std::nullopt;
    return value;
}

std::optional<uint> Nvml::performanceState()
{
    int value = 0;
    if (dev("nvmlDeviceGetPerformanceState", &value) != Success)
        return std::nullopt;
    return uint(value);
}

std::optional<quint64> Nvml::clockEventReasons()
{
    unsigned long long value = 0;
    if (dev("nvmlDeviceGetCurrentClocksEventReasons", &value) != Success)
        return std::nullopt;
    return quint64(value);
}

bool Nvml::clockOffset(ClockType type, int &current, int &min, int &max)
{
    ClockOffsetV1 info{kClockOffsetV1, int(type), kPstate0, 0, 0, 0};
    if (check(dev("nvmlDeviceGetClockOffsets", &info), "clock_offsets") != Success)
        return false;
    current = info.offsetMHz;
    min = info.minOffsetMHz;
    max = info.maxOffsetMHz;
    return true;
}

QVector<uint> Nvml::supportedMemoryClocks()
{
    unsigned count = 64;
    QVector<uint> clocks(static_cast<qsizetype>(count));
    if (dev("nvmlDeviceGetSupportedMemoryClocks", &count, clocks.data()) != Success)
        return {};
    clocks.resize(int(count));
    return clocks;
}

QVector<uint> Nvml::supportedGraphicsClocks(uint memoryClock)
{
    unsigned count = 512;
    QVector<uint> clocks(static_cast<qsizetype>(count));
    if (dev("nvmlDeviceGetSupportedGraphicsClocks", unsigned(memoryClock), &count, clocks.data()) != Success)
        return {};
    clocks.resize(int(count));
    return clocks;
}

uint Nvml::fanCount()
{
    return devUint("nvmlDeviceGetNumFans").value_or(0);
}

std::optional<uint> Nvml::fanSpeed(uint fan)
{
    unsigned value = 0;
    if (dev("nvmlDeviceGetFanSpeed_v2", unsigned(fan), &value) != Success)
        return std::nullopt;
    return value;
}

std::optional<uint> Nvml::fanRpm(uint fan)
{
    FanRpmV1 info{kFanRpmV1, unsigned(fan), 0};
    if (dev("nvmlDeviceGetFanSpeedRPM", &info) != Success)
        return std::nullopt;
    return info.speed;
}

std::optional<uint> Nvml::fanTarget(uint fan)
{
    unsigned value = 0;
    if (dev("nvmlDeviceGetTargetFanSpeed", unsigned(fan), &value) != Success)
        return std::nullopt;
    return value;
}

std::optional<uint> Nvml::fanPolicy(uint fan)
{
    unsigned value = 0;
    if (dev("nvmlDeviceGetFanControlPolicy_v2", unsigned(fan), &value) != Success)
        return std::nullopt;
    return value;
}

bool Nvml::fanRange(uint &min, uint &max)
{
    unsigned lo = 0, hi = 0;
    if (dev("nvmlDeviceGetMinMaxFanSpeed", &lo, &hi) != Success)
        return false;
    min = lo;
    max = hi;
    return true;
}

int Nvml::setPowerLimitW(double watts)
{
    return check(dev("nvmlDeviceSetPowerManagementLimit", unsigned(std::lround(watts * 1000))),
                 "power_limit");
}

int Nvml::setClockOffset(ClockType type, int mhz)
{
    ClockOffsetV1 info{kClockOffsetV1, int(type), kPstate0, mhz, 0, 0};
    return check(dev("nvmlDeviceSetClockOffsets", &info),
                 type == ClockMem ? "mem_offset" : "core_offset");
}

int Nvml::setLockedClocks(uint minMhz, uint maxMhz)
{
    return check(dev("nvmlDeviceSetGpuLockedClocks", unsigned(minMhz), unsigned(maxMhz)), "lock");
}

int Nvml::resetLockedClocks()
{
    return check(dev("nvmlDeviceResetGpuLockedClocks"), "unlock");
}

int Nvml::setFanSpeed(uint fan, uint percent)
{
    return check(dev("nvmlDeviceSetFanSpeed_v2", unsigned(fan), unsigned(percent)), "fan_speed");
}

int Nvml::setFanAuto(uint fan)
{
    return check(dev("nvmlDeviceSetDefaultFanSpeed_v2", unsigned(fan)), "fan_auto");
}
