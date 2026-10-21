// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import IGame
import "../components"

ScrollPage {
    id: page
    title: qsTr("Info & settings")
    subtitle: qsTr("Device details, service status and app preferences.")

    property var settings
    // Hidden pages stop following the LED (no redraws for invisible items); they catch up as soon as they are shown.
    readonly property color led: visible ? Daemon.ledColor : _frozenLed
    property color _frozenLed: "#000000"
    onVisibleChanged: if (!visible) _frozenLed = Daemon.ledColor
    readonly property var gpu: Daemon.info.gpu || ({})
    readonly property var lim: Daemon.info.limits || ({})
    readonly property var rgb: Daemon.info.rgb || ({})
    readonly property var daemonInfo: Daemon.info.daemon || ({})
    readonly property var t: Daemon.telemetry

    RowLayout {
        Layout.fillWidth: true
        spacing: 18

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.alignment: Qt.AlignTop
            spacing: 18

        Card {
            Layout.fillWidth: true
            title: qsTr("Graphics card")
            icon: "chip"
            KeyValue { label: qsTr("Name"); value: page.gpu.name || "—" }
            KeyValue { label: qsTr("Model"); value: "Colorful iGame Ultra W OC 12GB-V" }
            KeyValue { label: qsTr("Architecture"); value: page.gpu.architecture || "—" }
            KeyValue { label: qsTr("CUDA cores"); value: page.gpu.cores || "—" }
            KeyValue { label: "VRAM"; value: page.gpu.vram_total ? page.gpu.vram_total + " MiB · " + page.gpu.bus_width + "-bit" : "—" }
            KeyValue { label: qsTr("Max clocks"); value: qsTr("%1 MHz core · %2 MHz memory").arg(page.gpu.max_gfx || "—").arg(page.gpu.max_mem || "—") }
            KeyValue { label: qsTr("Temperature limits"); value: qsTr("throttle %1°C · slowdown %2°C · shutdown %3°C").arg(page.gpu.temp_max || "—").arg(page.gpu.temp_slowdown || "—").arg(page.gpu.temp_shutdown || "—") }
            KeyValue { label: qsTr("Power limit"); value: qsTr("%1–%2 W (default %3 W)").arg(page.lim.power_min || "—").arg(page.lim.power_max || "—").arg(page.lim.power_default || "—") }
            KeyValue { label: "PCIe"; value: qsTr("Gen %1 x%2 (max Gen %3 x%4)").arg(page.t.pcie_gen || "?").arg(page.t.pcie_width || "?").arg(page.gpu.pcie_gen_max || "?").arg(page.gpu.pcie_width_max || "?") }
            KeyValue { label: "VBIOS"; value: page.gpu.vbios || "—" }
            KeyValue { label: "Driver"; value: page.gpu.driver || "—" }
            KeyValue { label: qsTr("PCI address"); value: page.gpu.pci || "—" }
            KeyValue { label: qsTr("Fans"); value: qsTr("%1 fans · %2–%3%").arg(page.lim.fan_count || "—").arg(page.lim.fan_min || "?").arg(page.lim.fan_max || "?") }
        }

        Card {
            id: ledfxCard
            Layout.fillWidth: true
            readonly property var lf: Daemon.ledfx || ({})
            readonly property bool on: lf.enabled !== false
            title: "LedFx"
            subtitle: qsTr("Let LedFx drive the card's light, as a DDP device.")
            icon: "eq"
            iconColor: Theme.violet
            actions: [
                Chip {
                    text: !ledfxCard.on ? qsTr("Off")
                          : ledfxCard.lf.error ? qsTr("Port busy")
                          : ledfxCard.lf.active ? qsTr("Receiving · %1 fps").arg(ledfxCard.lf.fps || 0)
                          : qsTr("Waiting for LedFx")
                    tone: !ledfxCard.on ? Theme.textDim : ledfxCard.lf.error ? Theme.bad : ledfxCard.lf.active ? Theme.good : Theme.violet
                    strong: !!ledfxCard.lf.active
                }
            ]
            AppSwitch {
                Layout.fillWidth: true
                text: qsTr("Allow LedFx to control the light")
                description: qsTr("While LedFx is sending, it takes over. Your own effect comes back 2 seconds after it stops.")
                accent: Theme.violet
                checked: ledfxCard.on
                onToggled: Daemon.setOption("ledfx_enabled", checked)
            }
            AppSwitch {
                Layout.fillWidth: true
                text: qsTr("Accept LedFx from other devices")
                description: qsTr("Off: only LedFx running on this computer can connect.")
                accent: Theme.violet
                enabled: ledfxCard.on
                checked: !!ledfxCard.lf.network
                onToggled: Daemon.setOption("ledfx_network", checked)
            }
            Text {
                visible: !!ledfxCard.lf.error
                text: Daemon.translateError(ledfxCard.lf.error || "")
                color: Theme.warn
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Text {
                text: qsTr("In LedFx, add a device with these settings, then give it an effect:")
                color: Theme.textDim
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            KeyValue { label: qsTr("Device type"); value: "DDP" }
            KeyValue {
                label: qsTr("IP address")
                value: ledfxCard.lf.network && App.lanAddress() !== ""
                       ? qsTr("%1 (127.0.0.1 from this computer)").arg(App.lanAddress()) : "127.0.0.1"
            }
            KeyValue { label: qsTr("Port"); value: String(ledfxCard.lf.port || 4048) }
            KeyValue { label: qsTr("Pixel count"); value: qsTr("1 (more pixels are averaged)") }
            KeyValue { label: qsTr("Refresh rate"); value: qsTr("30 fps (the card shows up to 30)") }
        }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.preferredWidth: 1
            Layout.alignment: Qt.AlignTop
            spacing: 18

            Card {
                Layout.fillWidth: true
                title: qsTr("RGB controller")
                icon: "rgb"
                iconColor: Theme.glow(page.led)
                actions: [
                    Chip {
                        text: page.rgb.present ? (page.rgb.supported ? qsTr("Active") : qsTr("Not supported yet")) : qsTr("Not found")
                        tone: page.rgb.present ? (page.rgb.supported ? Theme.good : Theme.warn) : Theme.bad
                    }
                ]
                KeyValue { visible: !!page.rgb.present; label: qsTr("I2C bus"); value: (page.rgb.bus || "—") + " @ " + (page.rgb.address || "") }
                KeyValue { visible: !!page.rgb.present; label: qsTr("Model / type"); value: (page.rgb.model || "—") + " / " + (page.rgb.type || "—") + " · " + (page.rgb.leds || 0) + " LED" }
                KeyValue { visible: !!page.rgb.present; label: qsTr("PCI IDs"); value: page.rgb.ids || "—" }
                KeyValue { visible: !page.rgb.present; label: qsTr("Problem"); value: Daemon.translateError(page.rgb.error || "") || "—"; valueColor: Theme.warn }
                AppButton {
                    text: qsTr("Search for the controller again")
                    iconName: "refresh"
                    onClicked: Daemon.rescanRgb()
                }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("igamed service")
                icon: "power"
                iconColor: Daemon.connected ? Theme.good : Theme.bad
                actions: [
                    Chip {
                        text: Daemon.connected ? qsTr("Connected") : qsTr("Disconnected")
                        tone: Daemon.connected ? Theme.good : Theme.bad
                    }
                ]
                KeyValue { label: qsTr("Version"); value: page.daemonInfo.version || "—" }
                KeyValue {
                    label: qsTr("Privileges")
                    value: page.daemonInfo.root ? qsTr("root (all features available)") : qsTr("not root (RGB and monitoring only)")
                    valueColor: page.daemonInfo.root ? Theme.text : Theme.warn
                }
                KeyValue { label: "Socket"; value: Daemon.socketPath }
            }

            Card {
                Layout.fillWidth: true
                title: qsTr("Application")
                icon: "settings"
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        text: qsTr("Language")
                        color: Theme.text
                        font.pixelSize: 14
                    }
                    Segmented {
                        Layout.fillWidth: true
                        current: App.language
                        options: [
                            { value: "system", label: qsTr("Follow system") },
                            { value: "en", label: "English" },
                            { value: "id", label: "Indonesia" }
                        ]
                        onActivated: (v) => App.language = v
                    }
                }
                AppSwitch {
                    Layout.fillWidth: true
                    text: qsTr("Keep running in the tray when closed")
                    description: App.trayAvailable ? qsTr("The app keeps running in the tray. Audio effects need this.") : qsTr("No system tray is available on this desktop.")
                    enabled: App.trayAvailable
                    checked: page.settings ? page.settings.closeToTray : true
                    onToggled: if (page.settings) page.settings.closeToTray = checked
                }
                AppSwitch {
                    Layout.fillWidth: true
                    text: qsTr("Start at login")
                    description: qsTr("Opens the app hidden in the tray every time you log in.")
                    checked: App.autostart
                    onToggled: App.autostart = checked
                }
            }
        }
    }

    Card {
        Layout.fillWidth: true
        title: qsTr("About")
        icon: "info"
        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            color: Theme.textDim
            font.pixelSize: 13
            lineHeight: 1.25
            text: qsTr("iGame Control for Linux, version %1. The lighting protocol comes from Colorful's iGame Center 2.0.0.82 (handshake 0x0281, color frames 0x0312 on I2C 0x61). Power limit, overclocking and fans use NVML from the NVIDIA driver. This is an unofficial project, not affiliated with or endorsed by Colorful or NVIDIA. Colorful, iGame and iGame Center are trademarks of Colorful Technology; NVIDIA and GeForce are trademarks of NVIDIA Corporation. Overclocking is at your own risk. Free software under the GNU GPL v3 or later.").arg(Qt.application.version)
        }
    }
}
