// SPDX-License-Identifier: GPL-3.0-or-later
#include "daemon.h"
#include "protocol.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QSocketNotifier>

#include <csignal>
#include <sys/socket.h>
#include <unistd.h>

namespace {
int signalPipe[2] = {-1, -1};

void onSignal(int)
{
    const char byte = 1;
    if (::write(signalPipe[0], &byte, 1) < 0) {
        // nothing useful to do inside a signal handler
    }
}
} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("igamed"));
    app.setApplicationVersion(QLatin1String(igame::kVersion));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Control service for Colorful iGame GeForce cards: RGB, power limit, overclock, fans."));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption socketOption(QStringLiteral("socket"), QStringLiteral("Socket path."), QStringLiteral("path"),
                                          QLatin1String(igame::kDefaultSocket));
    const QCommandLineOption configOption(QStringLiteral("config"), QStringLiteral("Configuration file."), QStringLiteral("path"),
                                          QStringLiteral("/etc/igame-linux/config.json"));
    const QCommandLineOption stateOption(QStringLiteral("state"), QStringLiteral("State directory."), QStringLiteral("dir"),
                                         QStringLiteral("/var/lib/igame-linux"));
    const QCommandLineOption groupOption(QStringLiteral("group"), QStringLiteral("Group allowed to use the socket (default wheel, or sudo/admin where wheel does not exist)."),
                                         QStringLiteral("name"), QStringLiteral("wheel"));
    parser.addOptions({socketOption, configOption, stateOption, groupOption});
    parser.process(app);

    DaemonOptions options;
    options.socketPath = parser.value(socketOption);
    options.configPath = parser.value(configOption);
    options.stateDir = parser.value(stateOption);
    options.socketGroup = parser.value(groupOption);

    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, signalPipe) != 0)
        return 1;
    QSocketNotifier notifier(signalPipe[1], QSocketNotifier::Read);
    QObject::connect(&notifier, &QSocketNotifier::activated, &app, [&app] {
        char byte;
        if (::read(signalPipe[1], &byte, 1) >= 0)
            app.quit();
    });
    struct sigaction action = {};
    action.sa_handler = onSignal;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &action, nullptr);
    sigaction(SIGINT, &action, nullptr);
    signal(SIGPIPE, SIG_IGN);

    Daemon daemon(options);
    if (!daemon.start())
        return 1;
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &daemon, [&daemon] { daemon.shutdown(); });
    return app.exec();
}
