// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import IGame
import "../components"

Item {
    id: root

    readonly property var lim: Daemon.info.limits || ({})
    readonly property var oc: Daemon.config.oc || ({})
    readonly property var t: Daemon.telemetry
    readonly property var gpu: Daemon.info.gpu || ({})
    readonly property bool isRoot: !!(Daemon.info.daemon && Daemon.info.daemon.root)
    readonly property real powerDefault: lim.power_default || 250
    readonly property var clocks: lim.gfx_clocks || []
    readonly property int clockMin: clocks.length ? clocks[clocks.length - 1] : 180
    readonly property int clockMax: clocks.length ? clocks[0] : 3090

    property real power: powerDefault
    property int core: 0
    property int mem: 0
    property bool lockEnabled: false
    property int lockMin: 0
    property int lockMax: 0
    property bool fullRange: false

    property bool loaded: false

    function storedPower() { return (oc.power_w || 0) > 0 ? oc.power_w : powerDefault }
    function configState() {
        var lock = !!oc.lock_enabled
        return JSON.stringify([Math.round(storedPower()), oc.core_mhz || 0, oc.mem_mhz || 0, lock,
                               lock ? oc.lock_min || 0 : 0, lock ? oc.lock_max || 0 : 0])
    }
    function localState() {
        return JSON.stringify([Math.round(power), core, mem, lockEnabled,
                               lockEnabled ? lockMin : 0, lockEnabled ? lockMax : 0])
    }
    readonly property bool dirty: localState() !== configState()

    function load() {
        loaded = !!Daemon.config.oc && !!Daemon.info.limits
        power = storedPower()
        core = oc.core_mhz || 0
        mem = oc.mem_mhz || 0
        lockEnabled = !!oc.lock_enabled
        lockMin = oc.lock_min || Math.max(clockMin, 210)
        lockMax = oc.lock_max || clockMax
        if (Math.abs(core) > 500 || mem < -1000 || mem > 3000)
            fullRange = true
        sync()
    }
    function sync() {
        powerField.setValue(power)
        coreField.setValue(core)
        memField.setValue(mem)
        lockMinField.setValue(lockMin)
        lockMaxField.setValue(lockMax)
    }
    function apply() {
        Daemon.setOc({
            power_w: Math.round(power) === Math.round(powerDefault) ? 0 : Math.round(power),
            core_mhz: core,
            mem_mhz: mem,
            lock_enabled: lockEnabled,
            lock_min: lockMin,
            lock_max: lockMax
        })
    }
    function preset(name) {
        if (name === "eco") {
            power = Math.max(lim.power_min || 0, Math.round(powerDefault * 0.8))
            core = 0; mem = 0; lockEnabled = false
        } else if (name === "default") {
            power = powerDefault; core = 0; mem = 0; lockEnabled = false
        } else if (name === "max") {
            power = lim.power_max || powerDefault
        }
        sync()
    }

    Component.onCompleted: load()
    Connections {
        target: Daemon
        function onConfigChanged() { if (!root.loaded || !root.dirty) root.load() }
        function onInfoChanged() { if (!root.loaded || !root.dirty) root.load() }
    }

    ScrollPage {
        id: page
        anchors.fill: parent
        title: qsTr("Performance")
        subtitle: qsTr("Set the power limit, overclock and clock lock. Changes are applied through the igamed service.")
        bottomSpace: 110

        Rectangle {
            visible: Daemon.connected && !root.isRoot
            Layout.fillWidth: true
            implicitHeight: warnRow.implicitHeight + 24
            radius: Theme.radiusSmall
            color: Theme.alpha(Theme.warn, 0.1)
            border.color: Theme.alpha(Theme.warn, 0.35)
            RowLayout {
                id: warnRow
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10
                Icon { name: "alert"; color: Theme.warn; size: 18 }
                Text {
                    text: qsTr("igamed is not running as root, so the driver will refuse performance and fan changes. Run it through systemd: sudo systemctl enable --now igamed")
                    color: Theme.text
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Text {
                text: qsTr("Quick presets")
                color: Theme.textDim
                font.pixelSize: 13
                Layout.rightMargin: 6
            }
            AppButton { text: qsTr("Power saving"); iconName: "drop"; onClicked: root.preset("eco") }
            AppButton { text: qsTr("Factory default"); iconName: "refresh"; onClicked: root.preset("default") }
            AppButton { text: qsTr("Maximum power"); iconName: "bolt"; onClicked: root.preset("max") }
            Item { Layout.fillWidth: true }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 18

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 3
                Layout.alignment: Qt.AlignTop
                spacing: 18

                Card {
                    Layout.fillWidth: true
                    title: qsTr("Power limit")
                    subtitle: qsTr("How many watts the GPU may draw.")
                    icon: "bolt"
                    iconColor: Theme.violet
                    SliderField {
                        id: powerField
                        label: qsTr("Power limit")
                        unit: " W"
                        from: root.lim.power_min || 175
                        to: root.lim.power_max || 270
                        stepSize: 1
                        accent: Theme.violet
                        marks: [root.powerDefault]
                        hint: qsTr("Default %1 W · range %2–%3 W (%4% of default)").arg(root.powerDefault).arg(root.lim.power_min || "?")
                              .arg(root.lim.power_max || "?").arg(Math.round(root.power / root.powerDefault * 100))
                        onMoved: root.power = Math.round(value)
                    }
                }

                Card {
                    Layout.fillWidth: true
                    title: qsTr("Overclock")
                    subtitle: qsTr("Offsets are added to the GPU's own clock curve.")
                    icon: "sparkle"
                    actions: [
                        AppSwitch {
                            text: qsTr("Full range")
                            checked: root.fullRange
                            onToggled: root.fullRange = checked
                        }
                    ]
                    SliderField {
                        id: coreField
                        label: qsTr("Core clock offset")
                        unit: " MHz"
                        signed: true
                        from: root.fullRange ? (root.lim.core_min || -1000) : Math.max(root.lim.core_min || -1000, -500)
                        to: root.fullRange ? (root.lim.core_max || 1000) : Math.min(root.lim.core_max || 1000, 500)
                        stepSize: 5
                        marks: [0]
                        hint: qsTr("Estimated max boost ≈ %1 MHz").arg((root.gpu.max_gfx || 0) + root.core)
                        onMoved: root.core = Math.round(value)
                    }
                    SliderField {
                        id: memField
                        label: qsTr("Memory clock offset")
                        unit: " MHz"
                        signed: true
                        from: root.fullRange ? (root.lim.mem_min || -2000) : Math.max(root.lim.mem_min || -2000, -1000)
                        to: root.fullRange ? (root.lim.mem_max || 6000) : Math.min(root.lim.mem_max || 6000, 3000)
                        stepSize: 50
                        marks: [0]
                        accent: Theme.teal
                        hint: qsTr("NVML units. Current memory clock %1 MHz.").arg(root.t.clk_mem || "—")
                        onMoved: root.mem = Math.round(value)
                    }
                }

                Card {
                    Layout.fillWidth: true
                    title: qsTr("Core clock lock")
                    subtitle: qsTr("Limit the clock range. Combined with a positive core offset, this is how you undervolt on Linux: the same clock at a lower voltage.")
                    icon: "lock"
                    iconColor: Theme.teal
                    AppSwitch {
                        text: qsTr("Enable clock lock")
                        checked: root.lockEnabled
                        onToggled: root.lockEnabled = checked
                    }
                    SliderField {
                        id: lockMinField
                        enabled: root.lockEnabled
                        label: qsTr("Minimum clock")
                        unit: " MHz"
                        from: root.clockMin
                        to: root.clockMax
                        stepSize: 15
                        accent: Theme.teal
                        onMoved: {
                            root.lockMin = Math.round(value)
                            if (root.lockMax < root.lockMin) {
                                root.lockMax = root.lockMin
                                lockMaxField.setValue(root.lockMax)
                            }
                        }
                    }
                    SliderField {
                        id: lockMaxField
                        enabled: root.lockEnabled
                        label: qsTr("Maximum clock")
                        unit: " MHz"
                        from: root.clockMin
                        to: root.clockMax
                        stepSize: 15
                        accent: Theme.teal
                        onMoved: {
                            root.lockMax = Math.round(value)
                            if (root.lockMin > root.lockMax) {
                                root.lockMin = root.lockMax
                                lockMinField.setValue(root.lockMin)
                            }
                        }
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 2
                Layout.alignment: Qt.AlignTop
                spacing: 18

                Card {
                    Layout.fillWidth: true
                    title: qsTr("Right now")
                    icon: "dashboard"
                    GridLayout {
                        columns: 2
                        columnSpacing: 24
                        rowSpacing: 16
                        StatBlock { label: qsTr("Core clock"); value: root.t.clk_gfx || 0; unit: "MHz" }
                        StatBlock { label: qsTr("Memory clock"); value: root.t.clk_mem || 0; unit: "MHz" }
                        StatBlock {
                            label: qsTr("Power")
                            value: Math.round(root.t.power || 0)
                            unit: "/ " + (root.t.power_limit || "—") + " W"
                        }
                        StatBlock {
                            label: qsTr("Temperature")
                            value: root.t.temp || 0
                            unit: "°C"
                            valueColor: Theme.tempColor(root.t.temp || 0)
                        }
                        StatBlock {
                            label: qsTr("Core offset")
                            value: ((root.t.core_offset || 0) > 0 ? "+" : "") + (root.t.core_offset || 0)
                            unit: "MHz"
                        }
                        StatBlock {
                            label: qsTr("Memory offset")
                            value: ((root.t.mem_offset || 0) > 0 ? "+" : "") + (root.t.mem_offset || 0)
                            unit: "MHz"
                        }
                    }
                }

                Card {
                    Layout.fillWidth: true
                    title: qsTr("When the service starts")
                    icon: "power"
                    iconColor: Theme.good
                    AppSwitch {
                        Layout.fillWidth: true
                        text: qsTr("Apply performance settings automatically")
                        description: qsTr("Used after boot. If the system crashes within 2 minutes of applying them, this option turns itself off on the next start.")
                        checked: !!(Daemon.config.options && Daemon.config.options.apply_oc_on_boot)
                        onToggled: Daemon.setOption("apply_oc_on_boot", checked)
                    }
                    Rectangle {
                        visible: !!(Daemon.info.daemon && Daemon.info.daemon.oc_boot_skipped)
                        Layout.fillWidth: true
                        implicitHeight: skipText.implicitHeight + 20
                        radius: Theme.radiusSmall
                        color: Theme.alpha(Theme.bad, 0.1)
                        border.color: Theme.alpha(Theme.bad, 0.35)
                        Text {
                            id: skipText
                            anchors.fill: parent
                            anchors.margins: 10
                            text: qsTr("Overclock was not applied at the last start because the previous start was unstable.")
                            color: Theme.text
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                        }
                    }
                }

                Card {
                    Layout.fillWidth: true
                    title: qsTr("Restore everything")
                    subtitle: qsTr("Default power limit, zero offsets, clock lock released.")
                    icon: "refresh"
                    iconColor: Theme.textDim
                    AppButton {
                        text: qsTr("Reset to defaults")
                        kind: "danger"
                        iconName: "refresh"
                        onClicked: resetDialog.open()
                    }
                }
            }
        }
    }

    ActionBar {
        shown: root.dirty
        onApplyClicked: {
            if (root.core !== 0 || root.mem !== 0 || root.lockEnabled)
                confirmDialog.open()
            else
                root.apply()
        }
        onDiscardClicked: root.load()
    }

    ConfirmDialog {
        id: confirmDialog
        title: qsTr("Apply overclock?")
        text: qsTr("Core offset %1 MHz, memory %2 MHz%3.\n\nAn overclock can crash games or cause artifacts. Raise it in small steps and test for stability.")
              .arg((root.core > 0 ? "+" : "") + root.core).arg((root.mem > 0 ? "+" : "") + root.mem)
              .arg(root.lockEnabled ? qsTr(", clock locked to %1–%2 MHz").arg(root.lockMin).arg(root.lockMax) : "")
        confirmText: qsTr("Apply")
        onConfirmed: root.apply()
    }
    ConfirmDialog {
        id: resetDialog
        title: qsTr("Restore defaults?")
        text: qsTr("Power limit, clock offsets and the clock lock will be returned to factory settings.")
        confirmText: qsTr("Reset")
        danger: true
        onConfirmed: Daemon.resetOc()
    }
}
