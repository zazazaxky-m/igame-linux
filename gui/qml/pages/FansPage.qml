// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import IGame
import "../components"

Item {
    id: root

    readonly property var lim: Daemon.info.limits || ({})
    readonly property var t: Daemon.telemetry
    readonly property var liveFans: t.fans || []
    readonly property int fanCount: lim.fan_count || liveFans.length || 1
    readonly property int fanMin: lim.fan_min || 30
    readonly property int fanMax: lim.fan_max || 100

    property var cfg: ({ sync: true, fans: [] })
    property bool loaded: false
    property int selected: 0
    readonly property int editIndex: cfg.sync ? 0 : Math.min(selected, Math.max(0, (cfg.fans || []).length - 1))
    readonly property var current: (cfg.fans && cfg.fans[editIndex]) || ({ mode: "auto", fixed: 50, curve: [[30, 30], [86, 100]], zero_rpm: true, zero_rpm_temp: 45 })
    readonly property bool dirty: JSON.stringify(cfg) !== JSON.stringify(Daemon.config.fans || ({}))

    readonly property var presets: [
        { name: qsTr("Quiet"), icon: "drop", curve: [[30, 30], [55, 35], [65, 45], [75, 60], [83, 80], [88, 100]] },
        { name: qsTr("Balanced"), icon: "gauge", curve: [[30, 30], [50, 40], [62, 55], [72, 70], [80, 85], [86, 100]] },
        { name: qsTr("Aggressive"), icon: "bolt", curve: [[30, 35], [45, 45], [55, 60], [65, 75], [72, 90], [78, 100]] },
        { name: qsTr("Maximum"), icon: "fan", curve: [[30, 100], [90, 100]] }
    ]
    readonly property var modeNames: ({ auto: qsTr("Automatic"), fixed: qsTr("Fixed"), curve: qsTr("Curve") })

    function clone(o) { return JSON.parse(JSON.stringify(o)) }
    function load() {
        var c = Daemon.config.fans
        cfg = c && c.fans ? clone(c) : { sync: true, fans: [] }
        loaded = !!(c && c.fans && c.fans.length)
        fixedField.setValue(current.fixed)
        zeroField.setValue(current.zero_rpm_temp)
    }
    function patch(changes) {
        var c = clone(cfg)
        if (!c.fans || !c.fans.length)
            return
        for (var k in changes)
            c.fans[editIndex][k] = changes[k]
        if (c.sync)
            for (var i = 1; i < c.fans.length; ++i)
                c.fans[i] = clone(c.fans[0])
        cfg = c
    }
    function setSync(on) {
        var c = clone(cfg)
        c.sync = on
        if (on)
            for (var i = 1; i < c.fans.length; ++i)
                c.fans[i] = clone(c.fans[0])
        cfg = c
    }
    function settingFor(i) { return cfg.fans && cfg.fans.length ? cfg.fans[cfg.sync ? 0 : Math.min(i, cfg.fans.length - 1)] : ({ mode: "auto" }) }

    onEditIndexChanged: {
        fixedField.setValue(current.fixed)
        zeroField.setValue(current.zero_rpm_temp)
    }
    Component.onCompleted: load()
    Connections {
        target: Daemon
        function onConfigChanged() { if (!root.loaded || !root.dirty) root.load() }
    }

    ScrollPage {
        anchors.fill: parent
        title: qsTr("Fans")
        subtitle: qsTr("Control each fan on its own or all of them together. igamed re-evaluates curves every second.")
        bottomSpace: 110

        Rectangle {
            visible: !!root.t.fan_error && Daemon.connected
            Layout.fillWidth: true
            implicitHeight: errRow.implicitHeight + 24
            radius: Theme.radiusSmall
            color: Theme.alpha(Theme.warn, 0.1)
            border.color: Theme.alpha(Theme.warn, 0.35)
            RowLayout {
                id: errRow
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10
                Icon { name: "alert"; color: Theme.warn; size: 18 }
                Text {
                    text: Daemon.translateError(root.t.fan_error || "")
                    color: Theme.text
                    font.pixelSize: 13
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
        }

        // Fan cards ------------------------------------------------------------
        GridLayout {
            Layout.fillWidth: true
            columns: root.fanCount
            columnSpacing: 18

            Repeater {
                model: root.fanCount
                Rectangle {
                    id: fanCard
                    readonly property var live: root.liveFans[index] || ({})
                    readonly property var setting: root.settingFor(index)
                    readonly property bool isSelected: root.cfg.sync || root.selected === index
                    Layout.fillWidth: true
                    implicitHeight: 128
                    radius: Theme.radius
                    color: Theme.surface
                    border.color: isSelected ? Theme.alpha(Theme.teal, 0.55) : (fanArea.containsMouse ? Theme.borderStrong : Theme.border)
                    border.width: isSelected ? 1.5 : 1
                    Behavior on border.color { ColorAnimation { duration: 150 } }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 18
                        spacing: 18
                        FanSpinner {
                            size: 82
                            rpm: fanCard.live.rpm || 0
                        }
                        ColumnLayout {
                            spacing: 3
                            Layout.fillWidth: true
                            RowLayout {
                                Text {
                                    text: qsTr("Fan %1").arg(index + 1)
                                    color: Theme.text
                                    font.pixelSize: 15
                                    font.weight: Font.DemiBold
                                    Layout.fillWidth: true
                                }
                                Chip {
                                    text: root.modeNames[fanCard.setting.mode] || "—"
                                    tone: fanCard.setting.mode === "auto" ? Theme.textDim : Theme.teal
                                }
                            }
                            Row {
                                spacing: 5
                                Text {
                                    id: rpmText
                                    text: fanCard.live.rpm || 0
                                    color: Theme.text
                                    font.pixelSize: 28
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    text: "RPM"
                                    color: Theme.textDim
                                    font.pixelSize: 13
                                    anchors.baseline: rpmText.baseline
                                }
                            }
                            Text {
                                text: qsTr("%1% · target %2%").arg(fanCard.live.pct || 0).arg(fanCard.live.target || 0) + " · " + (fanCard.live.manual ? qsTr("manual") : qsTr("driver"))
                                color: Theme.textFaint
                                font.pixelSize: 12
                            }
                        }
                    }
                    MouseArea {
                        id: fanArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.cfg.sync)
                                root.setSync(false)
                            root.selected = index
                        }
                    }
                }
            }
        }

        // Control ----------------------------------------------------------------
        Card {
            Layout.fillWidth: true
            title: root.cfg.sync ? qsTr("All fans") : qsTr("Fan %1").arg(root.editIndex + 1)
            subtitle: root.cfg.sync ? qsTr("One setting for %1 fans.").arg(root.fanCount) : qsTr("Settings for this fan only.")
            icon: "fan"
            iconColor: Theme.teal
            actions: [
                AppSwitch {
                    text: qsTr("Same for all fans")
                    checked: root.cfg.sync
                    accent: Theme.teal
                    onToggled: root.setSync(checked)
                }
            ]

            RowLayout {
                Layout.fillWidth: true
                spacing: 14
                Segmented {
                    visible: !root.cfg.sync
                    Layout.preferredWidth: root.fanCount * 100
                    accent: Theme.teal
                    current: root.selected
                    options: {
                        var list = []
                        for (var i = 0; i < root.fanCount; ++i)
                            list.push({ value: i, label: qsTr("Fan %1").arg(i + 1) })
                        return list
                    }
                    onActivated: (v) => root.selected = v
                }
                Segmented {
                    Layout.preferredWidth: 360
                    accent: Theme.teal
                    current: root.current.mode
                    options: [
                        { value: "auto", label: qsTr("Automatic"), icon: "refresh" },
                        { value: "fixed", label: qsTr("Fixed"), icon: "lock" },
                        { value: "curve", label: qsTr("Curve"), icon: "wave" }
                    ]
                    onActivated: (v) => root.patch({ mode: v })
                }
                Item { Layout.fillWidth: true }
            }

            // auto
            Text {
                visible: root.current.mode === "auto"
                text: qsTr("The NVIDIA driver controls the fans, including stopping them (0 RPM) while the GPU is cool.")
                color: Theme.textDim
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            // fixed
            SliderField {
                id: fixedField
                visible: root.current.mode === "fixed"
                label: qsTr("Speed")
                unit: "%"
                from: root.fanMin
                to: root.fanMax
                stepSize: 1
                accent: Theme.teal
                hint: qsTr("The driver minimum is %1%. The fans keep spinning at this speed.").arg(root.fanMin)
                onMoved: root.patch({ fixed: Math.round(value) })
            }

            // curve
            ColumnLayout {
                visible: root.current.mode === "curve"
                Layout.fillWidth: true
                spacing: 14

                RowLayout {
                    spacing: 8
                    Text {
                        text: qsTr("Presets")
                        color: Theme.textDim
                        font.pixelSize: 13
                        Layout.rightMargin: 4
                    }
                    Repeater {
                        model: root.presets
                        AppButton {
                            text: modelData.name
                            iconName: modelData.icon
                            implicitHeight: 34
                            onClicked: root.patch({ curve: modelData.curve })
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 320
                    radius: Theme.radiusSmall
                    color: Theme.bgRaised
                    border.color: Theme.border
                    FanCurveEditor {
                        anchors.fill: parent
                        anchors.margins: 8
                        points: root.current.curve || []
                        currentTemp: root.t.temp !== undefined ? root.t.temp : -1
                        pMin: root.fanMin
                        zeroRpmTemp: root.current.zero_rpm ? root.current.zero_rpm_temp : -1
                        accent: Theme.teal
                        onEdited: (pts) => root.patch({ curve: pts })
                    }
                }
                Text {
                    text: qsTr("Drag points to change them · double-click empty space to add a point · right-click a point to remove it")
                    color: Theme.textFaint
                    font.pixelSize: 12
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 24
                    AppSwitch {
                        text: qsTr("0 RPM when cool")
                        description: qsTr("Below this temperature the fans are handed to the driver, which stops them.")
                        accent: Theme.teal
                        checked: !!root.current.zero_rpm
                        onToggled: root.patch({ zero_rpm: checked })
                        Layout.preferredWidth: 320
                    }
                    SliderField {
                        id: zeroField
                        enabled: !!root.current.zero_rpm
                        label: qsTr("0 RPM temperature")
                        unit: "°C"
                        from: 20
                        to: 80
                        stepSize: 1
                        accent: Theme.teal
                        onMoved: root.patch({ zero_rpm_temp: Math.round(value) })
                    }
                }
            }
        }
    }

    ActionBar {
        shown: root.dirty
        message: qsTr("Fan settings not applied yet")
        onApplyClicked: Daemon.setFans(root.cfg)
        onDiscardClicked: root.load()
    }
}
