// SPDX-License-Identifier: GPL-3.0-or-later
#include "apphelper.h"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QNetworkInterface>
#include <QSettings>
#include <QStandardPaths>

AppHelper::AppHelper(QObject *parent)
    : QObject(parent)
{
    m_language = QSettings().value(QStringLiteral("ui/language"), QStringLiteral("system")).toString();
    applyLanguage();

    m_tray.setIcon(QApplication::windowIcon());
    m_openAction = m_menu.addAction(QString(), this, &AppHelper::showRequested);
    m_menu.addSeparator();
    m_quitAction = m_menu.addAction(QString(), this, &AppHelper::quitRequested);
    m_tray.setContextMenu(&m_menu);
    retranslateTray();
    connect(&m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            emit showRequested();
    });
    if (trayAvailable())
        m_tray.show();
}

bool AppHelper::trayAvailable() const
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

QString AppHelper::effectiveLanguage() const
{
    if (m_language == QLatin1String("en") || m_language == QLatin1String("id"))
        return m_language;
    return QLocale::system().name().startsWith(QLatin1String("id")) ? QStringLiteral("id") : QStringLiteral("en");
}

void AppHelper::setLanguage(const QString &language)
{
    const QString value = (language == QLatin1String("en") || language == QLatin1String("id")) ? language
                                                                                              : QStringLiteral("system");
    if (value == m_language)
        return;
    m_language = value;
    QSettings().setValue(QStringLiteral("ui/language"), value);
    applyLanguage();
    retranslateTray();
    emit languageChanged();
}

void AppHelper::applyLanguage()
{
    if (m_translatorInstalled) {
        QCoreApplication::removeTranslator(&m_translator);
        m_translatorInstalled = false;
    }
    // English is the source language; other languages come from embedded .qm files.
    if (effectiveLanguage() == QLatin1String("id")
        && m_translator.load(QStringLiteral(":/i18n/igame_id.qm"))) {
        QCoreApplication::installTranslator(&m_translator);
        m_translatorInstalled = true;
    }
}

void AppHelper::retranslateTray()
{
    if (m_openAction)
        m_openAction->setText(tr("Open iGame Control"));
    if (m_quitAction)
        m_quitAction->setText(tr("Quit"));
}

QString AppHelper::autostartPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
         + QStringLiteral("/autostart/igame-control.desktop");
}

bool AppHelper::autostart() const
{
    return QFile::exists(autostartPath());
}

void AppHelper::setAutostart(bool enabled)
{
    if (enabled == autostart())
        return;
    const QString path = autostartPath();
    if (enabled) {
        QDir().mkpath(QFileInfo(path).path());
        QFile file(path);
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.write(QStringLiteral("[Desktop Entry]\nType=Application\nName=iGame Control\n"
                                      "Exec=\"%1\" --minimized\nIcon=igame-control\nX-GNOME-Autostart-enabled=true\n")
                           .arg(QCoreApplication::applicationFilePath())
                           .toUtf8());
        }
    } else {
        QFile::remove(path);
    }
    emit autostartChanged();
}

void AppHelper::copyText(const QString &text)
{
    QGuiApplication::clipboard()->setText(text);
}

void AppHelper::setTrayTooltip(const QString &text)
{
    m_tray.setToolTip(text);
}

QString AppHelper::lanAddress() const
{
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        const auto flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning) || (flags & QNetworkInterface::IsLoopBack))
            continue;
        for (const QNetworkAddressEntry &entry : iface.addressEntries()) {
            const QHostAddress ip = entry.ip();
            if (ip.protocol() == QAbstractSocket::IPv4Protocol && !ip.isLoopback())
                return ip.toString();
        }
    }
    return {};
}
