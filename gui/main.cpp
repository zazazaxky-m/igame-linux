// SPDX-License-Identifier: GPL-3.0-or-later
#include "apphelper.h"
#include "audiocapture.h"
#include "daemonclient.h"
#include "protocol.h"

#include <QApplication>
#include <QElapsedTimer>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>

#include <memory>
#include <unistd.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("igame-control"));
    app.setOrganizationName(QStringLiteral("igame-linux"));
    app.setApplicationDisplayName(QStringLiteral("iGame Control"));
    app.setApplicationVersion(QLatin1String(igame::kVersion));
    app.setDesktopFileName(QStringLiteral("igame-control"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/app.svg")));
    app.setQuitOnLastWindowClosed(false);

    // single instance; --toggle is meant for a global shortcut
    const bool toggle = app.arguments().contains(QStringLiteral("--toggle"));
    const QString instanceKey = QStringLiteral("igame-control-%1").arg(getuid());
    {
        QLocalSocket probe;
        probe.connectToServer(instanceKey);
        if (probe.waitForConnected(300)) {
            probe.write(toggle ? "toggle\n" : "show\n");
            probe.waitForBytesWritten(300);
            return 0;
        }
    }
    QLocalServer instanceServer;
    QLocalServer::removeServer(instanceKey);
    instanceServer.listen(instanceKey);

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    DaemonClient daemon;
    AudioCapture audio;
    AppHelper helper;

    QObject::connect(&audio, &AudioCapture::frame, &daemon, &DaemonClient::sendFrame);
    const auto syncAudio = [&] {
        const QVariantMap rgb = daemon.config().value(QStringLiteral("rgb")).toMap();
        const QString effect = rgb.value(QStringLiteral("effect")).toString();
        audio.setColors(rgb.value(QStringLiteral("colors")).toList());
        audio.setSpeed(rgb.value(QStringLiteral("speed"), 1.0).toDouble());
        audio.setSensitivity(rgb.value(QStringLiteral("sensitivity"), 100).toInt());
        audio.setMode(effect);
        // no point capturing audio while LedFx has the light
        audio.setActive(daemon.isConnected() && effect.startsWith(QLatin1String("music")) && !daemon.ledfxActive());
    };
    QObject::connect(&daemon, &DaemonClient::configChanged, &audio, syncAudio);
    QObject::connect(&daemon, &DaemonClient::ledfxChanged, &audio, syncAudio);
    QObject::connect(&daemon, &DaemonClient::connectedChanged, &audio, syncAudio);

    QElapsedTimer tooltipClock;
    const auto updateTooltip = [&] {
        if (!daemon.isConnected()) {
            helper.setTrayTooltip(QStringLiteral("iGame Control\n")
                                  + QCoreApplication::translate("Tray", "Not connected to igamed"));
            return;
        }
        if (tooltipClock.isValid() && tooltipClock.elapsed() < 2000)
            return;
        tooltipClock.start();
        const QVariantMap t = daemon.telemetry();
        const QString name = daemon.info().value(QStringLiteral("gpu")).toMap().value(QStringLiteral("name")).toString();
        helper.setTrayTooltip(QStringLiteral("%1\n%2°C · %3 W · %4%")
                                  .arg(name.isEmpty() ? QStringLiteral("iGame Control") : name)
                                  .arg(t.value(QStringLiteral("temp")).toInt())
                                  .arg(qRound(t.value(QStringLiteral("power")).toDouble()))
                                  .arg(t.value(QStringLiteral("util")).toInt()));
    };
    QObject::connect(&daemon, &DaemonClient::telemetryChanged, &helper, updateTooltip);
    QObject::connect(&daemon, &DaemonClient::connectedChanged, &helper, updateTooltip);

    qmlRegisterSingletonInstance("IGame", 1, 0, "Daemon", &daemon);
    qmlRegisterSingletonInstance("IGame", 1, 0, "Audio", &audio);
    qmlRegisterSingletonInstance("IGame", 1, 0, "App", &helper);

    // QML only exists while the window is open
    std::unique_ptr<QQmlApplicationEngine> engine;
    const auto showUi = [&] {
        if (engine) {
            if (auto *window = qobject_cast<QQuickWindow *>(engine->rootObjects().value(0))) {
                window->show();
                window->raise();
                window->requestActivate();
            }
            return;
        }
        engine = std::make_unique<QQmlApplicationEngine>();
        QObject::connect(engine.get(), &QQmlApplicationEngine::objectCreationFailed, &app,
                         [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
        engine->load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    };
    // Qt/the GPU driver don't give the memory back after destroying the UI, so restart as a
    // fresh tray-only process instead
    const auto unloadUi = [&] {
        if (!engine)
            return;
        QTimer::singleShot(0, &app, [&] {
            engine.reset(); // also saves the QML settings
            instanceServer.close();
            if (QProcess::startDetached(QCoreApplication::applicationFilePath(), {QStringLiteral("--minimized")}))
                QCoreApplication::quit();
        });
    };

    QObject::connect(&helper, &AppHelper::languageChanged, &app, [&] {
        if (engine)
            engine->retranslate();
        tooltipClock.invalidate();
        updateTooltip();
    });
    QObject::connect(&helper, &AppHelper::showRequested, &app, showUi);
    QObject::connect(&helper, &AppHelper::unloadRequested, &app, unloadUi);
    QObject::connect(&helper, &AppHelper::quitRequested, &app, &QCoreApplication::quit);
    QObject::connect(&instanceServer, &QLocalServer::newConnection, &helper, [&] {
        while (QLocalSocket *s = instanceServer.nextPendingConnection()) {
            QObject::connect(s, &QLocalSocket::readyRead, &helper, [&, s] {
                if (!s->canReadLine())
                    return;
                const QByteArray command = s->readLine().trimmed();
                s->disconnectFromServer();
                s->deleteLater();
                if (command == "toggle" && engine && helper.trayAvailable())
                    unloadUi();
                else
                    helper.requestShow();
            });
        }
    });

    const bool minimized = app.arguments().contains(QStringLiteral("--minimized"));
    if (!minimized || !helper.trayAvailable())
        showUi();

    const int rc = app.exec();
    engine.reset();
    return rc;
}
