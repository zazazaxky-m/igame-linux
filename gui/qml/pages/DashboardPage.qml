// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import IGame
import "../components"

ScrollPage {
    id: page
    title: qsTr("Overview")
    subtitle: qsTr("Live view of the graphics card.")
    // Hidden pages stop following the LED (no redraws for invisible items); they catch up as soon as they are shown.
    readonly property color led: visible ? Daemon.ledColor : _frozenLed
    property color _frozenLed: "#000000"
    onVisibleChanged: if (!visible) _frozenLed = Daemon.ledColor

    readonly property var t: Daemon.telemetry
    readonly property var gpu: Daemon.info.gpu || ({})
    readonly property var lim: Daemon.info.limits || ({})
    readonly property var hist: Daemon.history
    readonly property var fans: t.fans || []
    readonly property real fanAvg: {
        if (!fans.length) return 0
        var s = 0
        for (var i = 0; i < fans.length; ++i) s += fans[i].pct || 0
        return s / fans.length
    }
    readonly property real rpmAvg: {
        if (!fans.length) return 0
        var s = 0
        for (var i = 0; i < fans.length; ++i) s += fans[i].rpm || 0
        return s / fans.length
    }
    readonly property var reasonNames: ({
        idle: qsTr("Idle"), app_clocks: qsTr("App clocks"), power_cap: qsTr("Power limit"), hw_slowdown: qsTr("HW slowdown"),
        sync_boost: qsTr("Sync boost"), sw_thermal: qsTr("Thermal"), hw_thermal: qsTr("Thermal HW"),
        power_brake: qsTr("Power brake"), display_clock: qsTr("Display clock")
    })
    function reasonLabel(r) { return reasonNames[r] || qsTr("Code %1").arg(r) }
    function signedMhz(v) { return (v > 0 ? "+" : "") + (v || 0) + " MHz" }

    // Hero ---------------------------------------------------------------------
    Card {
        Layout.fillWidth: true
        padding: 26
        RowLayout {
            Layout.fillWidth: true
            spacing: 24
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 10
                Flow {
                    Layout.fillWidth: true
                    spacing: 8
                    Chip { text: page.gpu.architecture || "—"; tone: Theme.accent; icon: "chip" }
                    Chip { text: page.gpu.vram_total ? (Math.round(page.gpu.vram_total / 1024) + " GB GDDR7") : "VRAM"; tone: Theme.violet; icon: "memory" }
                    Chip {
                        text: "PCIe " + (page.t.pcie_gen || "?") + ".0 x" + (page.t.pcie_width || "?")
                        tone: Theme.teal
                    }
                    Chip {
                        text: Daemon.connected ? qsTr("igamed connected") : qsTr("igamed offline")
                        tone: Daemon.connected ? Theme.good : Theme.bad
                    }
                }
                Text {
                    text: (page.gpu.name || qsTr("Graphics card")).replace("NVIDIA ", "")
                    color: Theme.text
                    font.pixelSize: 34
                    font.weight: Font.Bold
                }
                Text {
                    text: qsTr("Colorful iGame Ultra W OC · %1 CUDA cores · %2-bit bus").arg(page.gpu.cores).arg(page.gpu.bus_width)
                    color: Theme.textDim
                    font.pixelSize: 14
                    visible: !!page.gpu.cores
                }
                RowLayout {
                    spacing: 28
                    Layout.topMargin: 8
                    StatBlock {
                        label: qsTr("Max boost")
                        value: page.gpu.max_gfx || "—"
                        unit: "MHz"
                        valueSize: 22
                    }
                    StatBlock {
                        label: qsTr("Driver")
                        value: page.gpu.driver || "—"
                        valueSize: 22
                    }
                    StatBlock {
                        label: qsTr("Light")
                        value: page.led.toString().toUpperCase()
                        valueColor: Theme.glow(page.led)
                        valueSize: 22
                    }
                }
            }
            GpuArt {
                Layout.preferredWidth: 430
                Layout.preferredHeight: 190
                led: page.led
                rpm: page.rpmAvg
            }
        }
    }

    // Gauges -------------------------------------------------------------------
    GridLayout {
        Layout.fillWidth: true
        columns: 4
        columnSpacing: 18
        rowSpacing: 18

        Card {
            Layout.fillWidth: true
            Gauge {
                Layout.alignment: Qt.AlignHCenter
                value: page.t.temp || 0
                to: page.gpu.temp_slowdown || 90
                unit: "°C"
                label: qsTr("GPU temperature")
                sub: qsTr("throttles at %1°C").arg(page.gpu.temp_max || 85)
                color: Theme.tempColor(page.t.temp || 0)
            }
        }
        Card {
            Layout.fillWidth: true
            Gauge {
                Layout.alignment: Qt.AlignHCenter
                value: page.t.util || 0
                unit: "%"
                label: qsTr("GPU load")
                sub: qsTr("memory %1%").arg(page.t.util_mem || 0)
                color: Theme.accent
            }
        }
        Card {
            Layout.fillWidth: true
            Gauge {
                Layout.alignment: Qt.AlignHCenter
                value: page.t.power || 0
                to: page.t.power_limit || 250
                unit: "W"
                label: qsTr("Power")
                sub: qsTr("limit %1 W").arg(page.t.power_limit || "—")
                color: Theme.mix(Theme.violet, Theme.bad, ((page.t.power || 0) / (page.t.power_limit || 250) - 0.7) / 0.3)
            }
        }
        Card {
            Layout.fillWidth: true
            Gauge {
                Layout.alignment: Qt.AlignHCenter
                value: page.fanAvg
                unit: "%"
                label: qsTr("Fans")
                sub: page.rpmAvg > 0 ? Math.round(page.rpmAvg) + " RPM" : qsTr("0 RPM · silent")
                color: Theme.teal
            }
        }
    }

    // Clocks / memory / limits -------------------------------------------------
    GridLayout {
        Layout.fillWidth: true
        columns: 3
        columnSpacing: 18
        rowSpacing: 18

        Card {
            title: qsTr("Clocks")
            icon: "bolt"
            iconColor: Theme.warn
            Layout.fillWidth: true
            Layout.fillHeight: true
            RowLayout {
                spacing: 26
                StatBlock {
                    label: qsTr("Core")
                    value: page.t.clk_gfx || 0
                    unit: "MHz"
                    hint: qsTr("offset %1").arg(page.signedMhz(page.t.core_offset))
                }
                StatBlock {
                    label: qsTr("Memory")
                    value: page.t.clk_mem || 0
                    unit: "MHz"
                    hint: qsTr("offset %1").arg(page.signedMhz(page.t.mem_offset))
                }
                StatBlock {
                    label: qsTr("Video")
                    value: page.t.clk_video || 0
                    unit: "MHz"
                    hint: "P" + (page.t.pstate !== undefined ? page.t.pstate : "?")
                }
            }
        }
        Card {
            id: vramCard
            title: qsTr("Video memory")
            icon: "memory"
            iconColor: Theme.violet
            Layout.fillWidth: true
            Layout.fillHeight: true
            readonly property real used: page.t.vram_used || 0
            readonly property real total: page.t.vram_total || page.gpu.vram_total || 1
            RowLayout {
                StatBlock {
                    label: qsTr("Used")
                    value: (vramCard.used / 1024).toFixed(1)
                    unit: "/ " + (vramCard.total / 1024).toFixed(1) + " GB"
                }
            }
            Rectangle {
                Layout.fillWidth: true
                height: 8
                radius: 4
                color: Theme.surface3
                Rectangle {
                    width: parent.width * Math.min(1, (page.t.vram_used || 0) / (page.t.vram_total || 1))
                    height: parent.height
                    radius: 4
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: Theme.accent }
                        GradientStop { position: 1; color: Theme.violet }
                    }
                    Behavior on width { enabled: page.visible; NumberAnimation { duration: 500; easing.type: Easing.OutCubic } }
                }
            }
            Text {
                text: qsTr("Encoder %1% · Decoder %2%").arg(page.t.enc || 0).arg(page.t.dec || 0)
                color: Theme.textFaint
                font.pixelSize: 12
            }
        }
        Card {
            title: qsTr("Performance limiters")
            icon: "gauge"
            iconColor: Theme.teal
            Layout.fillWidth: true
            Layout.fillHeight: true
            Flow {
                Layout.fillWidth: true
                spacing: 8
                Repeater {
                    model: (page.t.reasons || []).length ? page.t.reasons : ["none"]
                    Chip {
                        text: modelData === "none" ? qsTr("Not limited") : page.reasonLabel(modelData)
                        tone: modelData === "none" || modelData === "idle" ? Theme.good
                            : (modelData.indexOf("thermal") >= 0 || modelData === "power_cap") ? Theme.warn
                            : Theme.textDim
                    }
                }
            }
            Text {
                text: qsTr("Why the GPU is not running at its maximum clock right now.")
                color: Theme.textFaint
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }

    // History charts -----------------------------------------------------------
    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: 18
        rowSpacing: 18

        Repeater {
            model: [
                { key: "temp", label: qsTr("Temperature"), unit: "°C", color: Theme.warn, min: 25, max: 95, auto: false },
                { key: "power", label: qsTr("Power"), unit: "W", color: Theme.violet, min: 0, max: page.t.power_limit || 250, auto: false },
                { key: "clk_gfx", label: qsTr("Core clock"), unit: "MHz", color: Theme.accent, min: 0, max: page.gpu.max_gfx || 3200, auto: false },
                { key: "util", label: qsTr("GPU load"), unit: "%", color: Theme.teal, min: 0, max: 100, auto: false }
            ]
            Card {
                id: chartCard
                Layout.fillWidth: true
                padding: 18
                spacing: 8
                readonly property var series: page.hist[modelData.key] || []
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: modelData.label
                        color: Theme.textDim
                        font.pixelSize: 13
                        font.weight: Font.Medium
                        Layout.fillWidth: true
                    }
                    Text {
                        text: (chartCard.series.length ? Math.round(chartCard.series[chartCard.series.length - 1]) : "—") + " " + modelData.unit
                        color: modelData.color
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }
                }
                LineChart {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 110
                    values: chartCard.series
                    color: modelData.color
                    minValue: modelData.min
                    maxValue: modelData.max
                    capacity: Daemon.historySize
                }
                Text {
                    text: qsTr("Last 2 minutes")
                    color: Theme.textFaint
                    font.pixelSize: 11
                }
            }
        }
    }
}
