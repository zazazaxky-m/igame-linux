// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QHash>
#include <QString>
#include <QVector>

#include <optional>

// Minimal NVML wrapper, dlopen'd so the driver isn't a build dependency.
class Nvml
{
public:
    enum ClockType { ClockGraphics = 0, ClockSM = 1, ClockMem = 2, ClockVideo = 3 };
    static constexpr int Success = 0;
    static constexpr int ErrNotSupported = 3;
    static constexpr int ErrNoPermission = 4;
    static constexpr int ErrFunctionNotFound = 13;

    ~Nvml();

    bool init();
    void shutdown();
    bool ready() const { return m_device != nullptr; }
    QString lastError() const { return m_lastError; }

    // pci like "0000:01:00.0", empty = first GPU
    bool selectDevice(const QString &pciAddress = {});

    QString name();
    QString vbiosVersion();
    QString driverVersion();
    QString pciBusId();
    std::optional<uint> pcieGeneration(bool max);
    std::optional<uint> pcieWidth(bool max);
    std::optional<uint> gpuCores();
    std::optional<uint> memoryBusWidth();
    std::optional<uint> architecture();
    bool memoryInfo(quint64 &totalBytes, quint64 &usedBytes);
    std::optional<uint> temperature();
    std::optional<uint> temperatureThreshold(int type);
    std::optional<double> powerUsageW();
    std::optional<double> powerLimitW();
    std::optional<double> powerDefaultW();
    bool powerRangeW(double &min, double &max);
    std::optional<uint> clock(ClockType type);
    std::optional<uint> maxClock(ClockType type);
    bool utilization(uint &gpu, uint &memory);
    std::optional<uint> encoderUtilization();
    std::optional<uint> decoderUtilization();
    std::optional<uint> performanceState();
    std::optional<quint64> clockEventReasons();
    bool clockOffset(ClockType type, int &current, int &min, int &max);
    QVector<uint> supportedMemoryClocks();
    QVector<uint> supportedGraphicsClocks(uint memoryClock);
    uint fanCount();
    std::optional<uint> fanSpeed(uint fan);
    std::optional<uint> fanRpm(uint fan);
    std::optional<uint> fanTarget(uint fan);
    std::optional<uint> fanPolicy(uint fan);
    bool fanRange(uint &min, uint &max);

    // setters need root
    int setPowerLimitW(double watts);
    int setClockOffset(ClockType type, int mhz);
    int setLockedClocks(uint minMhz, uint maxMhz);
    int resetLockedClocks();
    int setFanSpeed(uint fan, uint percent);
    int setFanAuto(uint fan);

private:
    void *sym(const char *name);
    template <typename... Args>
    int call(const char *name, Args... args);
    template <typename... Args>
    int dev(const char *name, Args... args);
    int check(int code, const char *what);
    std::optional<uint> devUint(const char *name);

    void *m_lib = nullptr;
    void *m_device = nullptr;
    bool m_initialized = false;
    QHash<QByteArray, void *> m_syms;
    QString m_lastError;
};
