// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ddpreceiver.h"
#include "effects.h"
#include "fancontroller.h"
#include "nvml.h"
#include "rgbcontroller.h"

#include <QHash>
#include <QJsonObject>
#include <QLocalServer>
#include <QObject>
#include <QTimer>

class QLocalSocket;

struct DaemonOptions {
    QString socketPath;
    QString configPath = QStringLiteral("/etc/igame-linux/config.json");
    QString stateDir = QStringLiteral("/var/lib/igame-linux");
    QString socketGroup = QStringLiteral("wheel");
};

class Daemon : public QObject
{
    Q_OBJECT
public:
    explicit Daemon(const DaemonOptions &options, QObject *parent = nullptr);

    bool start();
    void shutdown();

private:
    struct Client {
        QByteArray buffer;
        bool subscribed = false;
    };

    void loadConfig();
    void saveConfig();
    void tryInitHardware();
    void initRgb();
    bool initGpu();
    void readLimits();
    void applyBootOc();
    void normalizeFanConfig();

    void pollTelemetry();
    void onFrame(const QColor &color);

    void onNewConnection();
    void onReadyRead(QLocalSocket *socket);
    QJsonObject handle(QLocalSocket *socket, const QJsonObject &request);
    QJsonObject snapshot() const;
    void broadcast(const QJsonObject &event);
    void configChanged();

    QString applyOc(const QJsonObject &oc, QJsonObject *normalized = nullptr);
    QString setFans(const QJsonObject &fans);
    QString setRgb(const QJsonObject &rgb);
    static QJsonObject defaultOc();
    QJsonObject info() const;
    void applyLedfxOptions();
    QJsonObject ledfxStatus() const;
    void broadcastLedfx();

    DaemonOptions m_opts;
    Nvml m_nvml;
    RgbController m_rgb;
    EffectEngine m_effects;
    DdpReceiver m_ddp;
    int m_ddpFps = 0;
    int m_ddpRetry = 0;
    QString m_ddpLoggedError;
    FanController m_fans{m_nvml};
    QLocalServer m_server;
    QHash<QLocalSocket *, Client> m_clients;
    QTimer m_telemetryTimer;
    QTimer m_retryTimer;
    QTimer m_saveTimer;
    QJsonObject m_config;
    QJsonObject m_telemetry;
    QJsonObject m_gpuInfo;
    QJsonObject m_limits;

    bool m_gpuReady = false;
    bool m_lockActive = false;
    bool m_ocBootSkipped = false;
    bool m_bootOcDone = false;
    int m_rgbFailures = 0;
    int m_rgbAttempts = 0;
    qint64 m_nextRgbAttempt = 0;

    double m_powerMin = 0, m_powerMax = 0, m_powerDefault = 0;
    int m_coreMin = 0, m_coreMax = 0, m_memMin = 0, m_memMax = 0;
    int m_fanCount = 0;
    uint m_fanMin = 30, m_fanMax = 100;
    QVector<uint> m_gfxClocks;
};
