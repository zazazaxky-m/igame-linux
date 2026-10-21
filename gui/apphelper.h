// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QAction>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
#include <QTranslator>

// Desktop integration for QML: tray icon, autostart entry, clipboard and UI language.
class AppHelper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool trayAvailable READ trayAvailable CONSTANT)
    Q_PROPERTY(bool autostart READ autostart WRITE setAutostart NOTIFY autostartChanged)
    // "system", "en" or "id"
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString effectiveLanguage READ effectiveLanguage NOTIFY languageChanged)

public:
    explicit AppHelper(QObject *parent = nullptr);

    bool trayAvailable() const;
    bool autostart() const;
    void setAutostart(bool enabled);
    QString language() const { return m_language; }
    void setLanguage(const QString &language);
    QString effectiveLanguage() const;

    Q_INVOKABLE void copyText(const QString &text);
    // This computer's first non-loopback IPv4 address, for "send to" instructions.
    Q_INVOKABLE QString lanAddress() const;
    Q_INVOKABLE void setTrayTooltip(const QString &text);
    Q_INVOKABLE void requestShow() { emit showRequested(); }
    // Window closed to the tray: the UI is unloaded to free its memory until shown again.
    Q_INVOKABLE void closeToTray() { emit unloadRequested(); }

signals:
    void showRequested();
    void quitRequested();
    void unloadRequested();
    void autostartChanged();
    void languageChanged();

private:
    QString autostartPath() const;
    void applyLanguage();
    void retranslateTray();

    QString m_language = QStringLiteral("system");
    QTranslator m_translator;
    bool m_translatorInstalled = false;
    QMenu m_menu; // declared before m_tray: the tray must be destroyed first
    QAction *m_openAction = nullptr;
    QAction *m_quitAction = nullptr;
    QSystemTrayIcon m_tray;
};
