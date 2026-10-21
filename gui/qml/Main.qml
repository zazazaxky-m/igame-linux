// SPDX-License-Identifier: GPL-3.0-or-later
import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Shapes
import IGame
import "components"
import "pages"

ApplicationWindow {
    id: win

    width: 1320
    height: 860
    minimumWidth: 1100
    minimumHeight: 700
    visible: true
    title: "iGame Control"
    color: Theme.bg
    font.family: Theme.fontFamily

    Settings {
        id: appSettings
        category: "ui"
        property bool closeToTray: true
        property string palettes: "{}"
        property int page: 0
    }

    onClosing: (close) => {
        if (appSettings.closeToTray && App.trayAvailable) {
            App.closeToTray()
        } else {
            Qt.quit()
        }
    }

    Connections {
        target: Daemon
        function onToast(message, error) { toast.show(message, error) }
    }

    // Ambient glow that follows the LED color ------------------------------------
    Shape {
        x: win.width - width * 0.62
        y: -height * 0.45
        width: 900
        height: 760
        opacity: 0.55
        ShapePath {
            strokeColor: "transparent"
            fillGradient: RadialGradient {
                centerX: 450
                centerY: 380
                centerRadius: 440
                focalX: 450
                focalY: 380
                GradientStop { position: 0; color: Theme.alpha(Theme.glow(Daemon.ledColor), 0.22) }
                GradientStop { position: 1; color: "transparent" }
            }
            startX: 0
            startY: 0
            PathLine { x: 900; y: 0 }
            PathLine { x: 900; y: 760 }
            PathLine { x: 0; y: 760 }
            PathLine { x: 0; y: 0 }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // Sidebar -------------------------------------------------------------------
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 248
            color: Theme.alpha(Theme.sidebar, 0.92)
            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Theme.border
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 6

                RowLayout {
                    spacing: 12
                    Layout.bottomMargin: 22
                    Layout.topMargin: 6
                    Rectangle {
                        width: 42
                        height: 42
                        radius: 13
                        gradient: Gradient {
                            orientation: Gradient.Horizontal
                            GradientStop { position: 0; color: Theme.glow(Daemon.ledColor) }
                            GradientStop { position: 1; color: Qt.darker(Theme.glow(Daemon.ledColor), 1.8) }
                        }
                        Text {
                            anchors.centerIn: parent
                            text: "iG"
                            color: "#FFFFFF"
                            font.pixelSize: 18
                            font.weight: Font.Black
                            font.italic: true
                        }
                    }
                    ColumnLayout {
                        spacing: 0
                        Text {
                            text: "iGame Control"
                            color: Theme.text
                            font.pixelSize: 16
                            font.weight: Font.Bold
                        }
                        Text {
                            text: qsTr("for Linux")
                            color: Theme.textDim
                            font.pixelSize: 12
                        }
                    }
                }

                Repeater {
                    model: [
                        { title: qsTr("Overview"), icon: "dashboard" },
                        { title: qsTr("Performance"), icon: "bolt" },
                        { title: qsTr("Fans"), icon: "fan" },
                        { title: qsTr("RGB lighting"), icon: "rgb" },
                        { title: qsTr("Profiles"), icon: "profiles" },
                        { title: qsTr("Info & settings"), icon: "info" }
                    ]
                    NavButton {
                        text: modelData.title
                        icon: modelData.icon
                        selected: stack.currentIndex === index
                        badge: index === 5 && Daemon.connected && !(Daemon.info.daemon && Daemon.info.daemon.root) ? "!" : ""
                        onClicked: {
                            stack.currentIndex = index
                            appSettings.page = index
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                // mini status
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: statusColumn.implicitHeight + 28
                    radius: 14
                    color: Theme.surface
                    border.color: Theme.border
                    ColumnLayout {
                        id: statusColumn
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 10
                        RowLayout {
                            spacing: 8
                            Rectangle {
                                width: 8
                                height: 8
                                radius: 4
                                color: Daemon.connected ? Theme.good : Theme.bad
                            }
                            Text {
                                text: Daemon.connected ? qsTr("igamed running") : qsTr("igamed offline")
                                color: Theme.textDim
                                font.pixelSize: 12
                                Layout.fillWidth: true
                            }
                        }
                        RowLayout {
                            spacing: 12
                            Rectangle {
                                width: 30
                                height: 30
                                radius: 15
                                color: Daemon.ledColor
                                border.color: Theme.alpha("#FFFFFF", 0.25)
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 42
                                    height: 42
                                    radius: 21
                                    color: "transparent"
                                    border.color: Theme.alpha(Theme.glow(Daemon.ledColor), 0.35)
                                    border.width: 2
                                }
                            }
                            ColumnLayout {
                                spacing: 0
                                Text {
                                    text: (Daemon.telemetry.temp || 0) + "°C · " + Math.round(Daemon.telemetry.power || 0) + " W"
                                    color: Theme.text
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    text: (Daemon.telemetry.clk_gfx || 0) + " MHz · " + (Daemon.telemetry.util || 0) + "%"
                                    color: Theme.textFaint
                                    font.pixelSize: 12
                                }
                            }
                        }
                    }
                }
            }
        }

        // Pages ---------------------------------------------------------------------
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                id: stack
                anchors.fill: parent
                currentIndex: Math.max(0, Math.min(5, appSettings.page))
                onCurrentIndexChanged: fadeIn.restart()

                DashboardPage {}
                PerformancePage {}
                FansPage {}
                RgbPage { settings: appSettings }
                ProfilesPage {}
                InfoPage { settings: appSettings }
            }
            NumberAnimation {
                id: fadeIn
                target: stack
                property: "opacity"
                from: 0.2
                to: 1
                duration: 220
                easing.type: Easing.OutCubic
            }

            // Shown while igamed is not reachable
            Rectangle {
                anchors.fill: parent
                visible: !Daemon.connected
                color: Theme.alpha(Theme.bg, 0.94)
                MouseArea { anchors.fill: parent; hoverEnabled: true }

                ColumnLayout {
                    anchors.centerIn: parent
                    width: Math.min(parent.width - 80, 560)
                    spacing: 16
                    Rectangle {
                        Layout.alignment: Qt.AlignHCenter
                        width: 76
                        height: 76
                        radius: 24
                        color: Theme.alpha(Theme.warn, 0.12)
                        border.color: Theme.alpha(Theme.warn, 0.35)
                        Icon {
                            anchors.centerIn: parent
                            name: "power"
                            size: 34
                            color: Theme.warn
                        }
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: Daemon.accessDenied ? qsTr("Your user may not use igamed yet") : qsTr("The igamed service is not running")
                        color: Theme.text
                        font.pixelSize: 22
                        font.weight: Font.Bold
                    }
                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: Daemon.accessDenied
                              ? qsTr("The service is running, but it only accepts members of the %1 group. Add your user to it, then log out and back in.").arg(Daemon.socketGroup || "wheel")
                              : qsTr("This app controls the graphics card through the igamed service, which runs as root. Enable it once and it will start with every boot.")
                        color: Theme.textDim
                        font.pixelSize: 14
                        lineHeight: 1.2
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 52
                        radius: Theme.radiusSmall
                        color: Theme.surface2
                        border.color: Theme.borderStrong
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 16
                            anchors.rightMargin: 8
                            Text {
                                id: fixCommand
                                text: Daemon.accessDenied ? "sudo usermod -aG %1 $USER".arg(Daemon.socketGroup || "wheel")
                                                          : "sudo systemctl enable --now igamed"
                                color: Theme.teal
                                font.family: "monospace"
                                font.pixelSize: 14
                                Layout.fillWidth: true
                            }
                            AppButton {
                                text: qsTr("Copy")
                                iconName: "copy"
                                kind: "ghost"
                                onClicked: {
                                    App.copyText(fixCommand.text)
                                    toast.show(qsTr("Command copied"), false)
                                }
                            }
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("Trying to connect to %1 ...").arg(Daemon.socketPath)
                        color: Theme.textFaint
                        font.pixelSize: 12
                    }
                    AppButton {
                        Layout.alignment: Qt.AlignHCenter
                        text: qsTr("Try again now")
                        iconName: "refresh"
                        kind: "primary"
                        onClicked: Daemon.reconnect()
                    }
                }
            }
        }
    }

    Toast {
        id: toast
        anchors.fill: parent
    }
}
