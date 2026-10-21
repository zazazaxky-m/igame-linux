// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QColor>
#include <QHash>
#include <QJsonObject>
#include <QLocalSocket>
#include <QObject>
#include <QTimer>
#include <QVariantMap>

// Connection to igamed. Exposes the daemon state to QML and turns calls into requests.
class DaemonClient : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)
    Q_PROPERTY(QString socketPath READ socketPath CONSTANT)
    // The service runs but refuses this user (not in the socket's group).
    Q_PROPERTY(bool accessDenied READ accessDenied NOTIFY accessDeniedChanged)
    Q_PROPERTY(QString socketGroup READ socketGroup NOTIFY accessDeniedChanged)
    Q_PROPERTY(QVariantMap info READ info NOTIFY infoChanged)
    Q_PROPERTY(QVariantMap config READ config NOTIFY configChanged)
    Q_PROPERTY(QVariantMap telemetry READ telemetry NOTIFY telemetryChanged)
    Q_PROPERTY(QVariantMap history READ history NOTIFY historyChanged)
    Q_PROPERTY(QColor ledColor READ ledColor NOTIFY ledColorChanged)
    Q_PROPERTY(QVariantMap ledfx READ ledfx NOTIFY ledfxChanged)
    Q_PROPERTY(int historySize READ historySize CONSTANT)

public:
    explicit DaemonClient(QObject *parent = nullptr);
    ~DaemonClient() override;

    bool isConnected() const { return m_connected; }
    QString socketPath() const { return m_path; }
    QVariantMap info() const { return m_info; }
    bool accessDenied() const { return m_accessDenied; }
    QString socketGroup() const;
    QVariantMap config() const { return m_config; }
    QVariantMap telemetry() const { return m_telemetry; }
    QVariantMap history() const { return m_historyMap; }
    QColor ledColor() const { return m_led; }
    QVariantMap ledfx() const { return m_ledfx; }
    bool ledfxActive() const { return m_ledfx.value(QStringLiteral("active")).toBool(); }
    int historySize() const { return kHistory; }

    Q_INVOKABLE void setRgb(const QVariantMap &rgb);
    Q_INVOKABLE void sendFrame(const QColor &color);
    Q_INVOKABLE void setOc(const QVariantMap &oc);
    Q_INVOKABLE void resetOc();
    Q_INVOKABLE void setFans(const QVariantMap &fans);
    Q_INVOKABLE void setOption(const QString &key, const QVariant &value);
    Q_INVOKABLE void saveProfile(const QString &name);
    Q_INVOKABLE void loadProfile(const QString &name);
    Q_INVOKABLE void deleteProfile(const QString &name);
    Q_INVOKABLE void rescanRgb();
    Q_INVOKABLE void reconnect();
    // Turns a daemon error ("code|arg|arg") into a message in the current UI language.
    Q_INVOKABLE QString translateError(const QString &coded) const;

signals:
    void connectedChanged();
    void accessDeniedChanged();
    void infoChanged();
    void configChanged();
    void telemetryChanged();
    void historyChanged();
    void ledColorChanged();
    void ledfxChanged();
    void toast(const QString &message, bool error);
    void requestFinished(const QString &cmd, bool ok, const QString &error);

private:
    static constexpr int kHistory = 120;

    void send(const QJsonObject &message);
    void request(const QString &cmd, QJsonObject args, const QString &okMessage = {});
    void flushRgb();
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void handleMessage(const QJsonObject &message);
    void applySnapshot(const QJsonObject &snapshot);
    void setTelemetry(const QJsonObject &telemetry);

    struct Pending {
        QString cmd;
        QString okMessage;
    };

    QString m_path;
    QLocalSocket m_socket;
    QTimer m_reconnectTimer;
    QTimer m_rgbTimer;
    QJsonObject m_pendingRgb;
    bool m_rgbQueued = false;
    QByteArray m_buffer;
    int m_nextId = 1;
    QHash<int, Pending> m_pending;
    bool m_connected = false;

    QVariantMap m_info;
    bool m_accessDenied = false;
    QVariantMap m_ledfx;
    QVariantMap m_config;
    QVariantMap m_telemetry;
    QVariantMap m_historyMap;
    QHash<QString, QVariantList> m_history;
    QColor m_led = QColor(255, 255, 255);
};
