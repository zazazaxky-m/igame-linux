// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IGame
import "../components"

// left column stays put, only the effect grid scrolls
Item {
    id: page
    property var settings // remembers palettes per effect
    property string category: "all"

    // don't follow the LED/analyser while hidden, saves redraws
    readonly property color led: visible ? Daemon.ledColor : _frozenLed
    property color _frozenLed: "#000000"
    onVisibleChanged: if (!visible) _frozenLed = Daemon.ledColor
    readonly property bool listening: visible && Audio.active

    readonly property var effects: [
        { id: "static", cat: "anim", name: qsTr("Static"), desc: qsTr("One steady color"), icon: "drop", palette: ["#FFFFFF"], max: 1 },
        { id: "breathing", cat: "anim", name: qsTr("Breathing"), desc: qsTr("Fades slowly in and out"), icon: "wave", palette: ["#00A2FF"], max: 8, speed: true },
        { id: "rainbow_breathing", cat: "anim", name: qsTr("Rainbow breathing"), desc: qsTr("Every breath in a new color"), icon: "breath", palette: [], max: 0, speed: true, rainbow: true },
        { id: "spectrum", cat: "anim", name: qsTr("Spectrum"), desc: qsTr("Cycles through every hue"), icon: "spectrum", palette: [], max: 0, speed: true, rainbow: true },
        { id: "cycle", cat: "anim", name: qsTr("Color cycle"), desc: qsTr("Smooth fades between your colors"), icon: "cycle", palette: ["#FF0040", "#7A00FF", "#00C8FF"], max: 8, speed: true },
        { id: "aurora", cat: "anim", name: qsTr("Aurora"), desc: qsTr("Soft drifting gradients"), icon: "aurora", palette: ["#00FFA3", "#00B3FF", "#8A2BE2"], max: 8, speed: true },
        { id: "plasma", cat: "anim", name: qsTr("Plasma"), desc: qsTr("Restless swirling colors"), icon: "swirl", palette: ["#FF00AA", "#FF7A00", "#00E5FF"], max: 8, min: 2, speed: true },
        { id: "ocean", cat: "anim", name: qsTr("Ocean"), desc: qsTr("Rolling waves of blue"), icon: "ocean", palette: ["#0033AA", "#0099FF", "#00E5CC"], max: 8, min: 2, speed: true },
        { id: "heartbeat", cat: "anim", name: qsTr("Heartbeat"), desc: qsTr("A double pulse"), icon: "heart", palette: ["#FF1744"], max: 1, speed: true },
        { id: "candle", cat: "anim", name: qsTr("Candle"), desc: qsTr("A flickering flame"), icon: "flame", palette: ["#FF8A1F"], max: 1, speed: true },
        { id: "twinkle", cat: "anim", name: qsTr("Twinkle"), desc: qsTr("Random sparkles"), icon: "star", palette: ["#FFFFFF", "#00E5FF", "#FF00C8"], max: 8, speed: true },
        { id: "lightning", cat: "anim", name: qsTr("Lightning"), desc: qsTr("Storm glow with sudden strikes"), icon: "storm", palette: ["#1A2A6C", "#DDE6FF"], max: 2, min: 2, speed: true },
        { id: "glitch", cat: "anim", name: qsTr("Glitch"), desc: qsTr("A broken-signal flicker"), icon: "glitch", palette: ["#00FFC8", "#FF0055", "#7A00FF"], max: 8, speed: true },
        { id: "flash", cat: "anim", name: qsTr("Flash"), desc: qsTr("On and off"), icon: "flash", palette: ["#FFFFFF"], max: 8, speed: true },
        { id: "double_flash", cat: "anim", name: qsTr("Double flash"), desc: qsTr("Two blinks, then a pause"), icon: "flash", palette: ["#FF2A2A"], max: 8, speed: true },
        { id: "strobe", cat: "anim", name: qsTr("Strobe"), desc: qsTr("Very fast flashing"), icon: "bolt", palette: ["#FFFFFF"], max: 8, speed: true },
        { id: "police", cat: "anim", name: qsTr("Police"), desc: qsTr("A two-color siren"), icon: "siren", palette: ["#FF0000", "#0040FF"], max: 2, min: 2, speed: true },
        { id: "daytime", cat: "anim", name: qsTr("Time of day"), desc: qsTr("Warm at night, neutral white by day"), icon: "clock", palette: [], max: 0, swatch: ["#FF7A1A", "#FFF4E5"] },
        { id: "off", cat: "anim", name: qsTr("Off"), desc: qsTr("Lights off"), icon: "off", palette: [], max: 0 },
        { id: "music_smart", cat: "audio", name: qsTr("Smart Sound"), desc: qsTr("Recognises the music and picks a matching LedFx effect"), icon: "sparkle", palette: [], max: 0, speed: true, audio: true, smart: true, swatch: ["#00E5FF", "#FF00C8"] },
        { id: "music", cat: "audio", name: qsTr("Bar"), desc: qsTr("Rises and falls with the beat, stepping along the gradient"), icon: "wave", palette: ["#FF0000", "#FF7800", "#FFC800", "#00FF00", "#00C78C", "#0000FF", "#800080", "#FF00B2"], max: 8, min: 2, speed: true, audio: true, ledfx: true },
        { id: "music_spectrum", cat: "audio", name: qsTr("Energy"), desc: qsTr("Bass, mids and treble each light their own color"), icon: "eq", palette: ["#FF0000", "#00FF00", "#0000FF"], max: 3, min: 3, speed: true, audio: true, ledfx: true, bands: true },
        { id: "music_scroll", cat: "audio", name: qsTr("Scroll"), desc: qsTr("Color hits per band that glow out slowly"), icon: "aurora", palette: ["#FF0000", "#00FF00", "#0000FF"], max: 3, min: 3, speed: true, audio: true, ledfx: true, bands: true },
        { id: "music_bass", cat: "audio", name: qsTr("Power"), desc: qsTr("The bass drives the color, hits add white sparks"), icon: "speaker", palette: ["#FF0000", "#FF7800", "#FFC800", "#00FF00", "#00C78C", "#0000FF", "#800080", "#FF00B2"], max: 8, min: 2, speed: true, audio: true, ledfx: true },
        { id: "music_strobe", cat: "audio", name: qsTr("BPM Strobe"), desc: qsTr("Fading flashes in time with the beat"), icon: "bolt", palette: ["#FF0000", "#FF7800", "#FFC800", "#00FF00", "#00C78C", "#0000FF", "#800080", "#FF00B2"], max: 8, min: 2, speed: true, audio: true, ledfx: true },
        { id: "music_realstrobe", cat: "audio", name: qsTr("Real Strobe"), desc: qsTr("Bass strobes in shifting colors, white flashes on hits"), icon: "flash", palette: ["#FF0000", "#FF00B2", "#0000FF"], max: 8, min: 2, speed: true, audio: true, ledfx: true },
        { id: "music_rainbow", cat: "audio", name: qsTr("Wavelength"), desc: qsTr("The color shows where in the spectrum the sound is"), icon: "spectrum", palette: ["#FF0000", "#FF7800", "#FFC800", "#00FF00", "#00C78C", "#0000FF", "#800080", "#FF00B2"], max: 8, min: 2, speed: true, audio: true, ledfx: true },
        { id: "music_fire", cat: "audio", name: qsTr("Magnitude"), desc: qsTr("Brightness and color follow the bass, like a VU meter"), icon: "flame", palette: ["#2A0000", "#FF1A00", "#FF7A00", "#FFD000", "#FFF6E0"], max: 8, min: 2, speed: true, audio: true, ledfx: true },
        { id: "music_chill", cat: "audio", name: qsTr("Ambient"), desc: qsTr("Slow color drift that breathes with the music"), icon: "aurora", palette: ["#3A86FF", "#8338EC", "#06D6A0"], max: 8, speed: true, audio: true },
        { id: "temperature", cat: "sensor", name: qsTr("Follow temperature"), desc: qsTr("Color follows the GPU temperature"), icon: "thermo", palette: ["#00FF55", "#FFD000", "#FF2020"], max: 8, min: 2, sensor: "°C", low: 40, high: 80 },
        { id: "load", cat: "sensor", name: qsTr("Follow load"), desc: qsTr("Color follows the GPU load"), icon: "load", palette: ["#00B3FF", "#7A00FF", "#FF0040"], max: 8, min: 2, sensor: "%", low: 0, high: 100 },
        { id: "power", cat: "sensor", name: qsTr("Follow power"), desc: qsTr("Color follows the share of the power limit"), icon: "bolt", palette: ["#00FF88", "#FFB000", "#FF0000"], max: 8, min: 2, sensor: "%", low: 20, high: 100 }
    ]
    readonly property var shownEffects: category === "all" ? effects : effects.filter(function (e) { return e.cat === category })
    readonly property var styleInfo: ({
        silence: { name: qsTr("Silence"), desc: qsTr("Nothing is playing; the light breathes softly.") },
        chill: { name: qsTr("Calm"), desc: qsTr("Calm or acoustic music: Wavelength in cool colors.") },
        pop: { name: qsTr("Pop"), desc: qsTr("A steady rhythm: Energy, with bass, mids and treble in their own colors.") },
        edm: { name: qsTr("Electronic"), desc: qsTr("Strong bass and a steady tempo: Power in neon colors, with white sparks on hits.") },
        hiphop: { name: qsTr("Hip-hop"), desc: qsTr("Heavy bass at a slower tempo: Bar in purple and magenta, rising and falling with the beat.") },
        rock: { name: qsTr("Rock / metal"), desc: qsTr("Dense, bright, distorted sound: Magnitude with a fire gradient.") },
        speech: { name: qsTr("Voice"), desc: qsTr("Speech or podcasts: a calm, warm glow.") }
    })

    readonly property var rgb: Daemon.config.rgb || ({ effect: "static", colors: ["#FFFFFF"], speed: 1, brightness: 100, low: 40, high: 80, sensitivity: 100 })
    readonly property var meta: {
        for (var i = 0; i < effects.length; ++i)
            if (effects[i].id === rgb.effect) return effects[i]
        return effects[0]
    }

    function send(changes) {
        var s = {
            effect: rgb.effect, colors: rgb.colors, speed: rgb.speed, brightness: rgb.brightness,
            low: rgb.low, high: rgb.high, sensitivity: rgb.sensitivity !== undefined ? rgb.sensitivity : 100
        }
        for (var k in changes)
            s[k] = changes[k]
        Daemon.setRgb(s)
    }
    function memory() {
        try { return JSON.parse(settings ? settings.palettes : "{}") || {} } catch (e) { return {} }
    }
    function remember(effectId, colors) {
        if (!settings) return
        var m = memory()
        m[effectId] = colors
        settings.palettes = JSON.stringify(m)
    }
    function selectEffect(e) {
        var colors = memory()[e.id] || (e.palette.length ? e.palette : rgb.colors)
        if (e.max > 0 && colors.length > e.max) colors = colors.slice(0, e.max)
        if (e.min && colors.length < e.min) colors = e.palette
        var changes = { effect: e.id, colors: colors }
        if (e.sensor) {
            changes.low = e.low
            changes.high = e.high
        }
        send(changes)
    }
    function swatchFor(e) {
        if (e.swatch) return e.swatch
        if (e.rainbow) return ["#FF0000", "#7A00FF"]
        return e.palette.length ? e.palette : ["#5E6A80"]
    }
    function syncSliders() {
        speedField.setValue(rgb.speed)
        brightField.setValue(rgb.brightness)
        sensField.setValue(rgb.sensitivity !== undefined ? rgb.sensitivity : 100)
        lowField.setValue(rgb.low)
        highField.setValue(rgb.high)
    }
    Component.onCompleted: syncSliders()
    Connections {
        target: Daemon
        function onConfigChanged() { page.syncSliders() }
    }

    component MeterRow: RowLayout {
        id: meterRow
        property string label
        property real value: 0
        Layout.fillWidth: true
        spacing: 10
        Text {
            text: meterRow.label
            color: Theme.textDim
            font.pixelSize: 12
            Layout.preferredWidth: 46
        }
        Rectangle {
            Layout.fillWidth: true
            height: 6
            radius: 3
            color: Theme.surface3
            Rectangle {
                width: parent.width * Math.max(0, Math.min(1, meterRow.value))
                height: parent.height
                radius: 3
                color: Theme.glow(page.led)
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 32
        anchors.rightMargin: 32
        anchors.topMargin: 28
        anchors.bottomMargin: 24
        spacing: 18

        ColumnLayout {
            spacing: 4
            Layout.fillWidth: true
            Text {
                text: qsTr("RGB lighting")
                color: Theme.text
                font.pixelSize: 26
                font.weight: Font.Bold
            }
            Text {
                text: qsTr("Changes show on the card right away. Effects keep running when the app is closed, except audio effects, which need the app running in the tray.")
                color: Theme.textDim
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            // left: the card picture shrinks first, scrolling is the last resort
            Flickable {
                id: leftFlick
                Layout.preferredWidth: 4
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: width
                contentHeight: leftCol.implicitHeight
                clip: true
                interactive: contentHeight > height
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {
                    id: leftBar
                    policy: ScrollBar.AsNeeded
                    contentItem: Rectangle {
                        implicitWidth: 6
                        visible: leftBar.size < 1.0
                        radius: 3
                        color: Theme.surface3
                    }
                }

                // space left for the picture (100..210), minus a bit of slack for rounding
                readonly property real artHeight: Math.max(100, Math.min(210, Math.floor(
                    height - 4 - settingsCard.implicitHeight - leftCol.spacing
                    - (ledfxBanner.visible ? ledfxBanner.implicitHeight + leftCol.spacing : 0)
                    - previewCard.padding * 2 - nameRow.implicitHeight - previewCard.spacing
                    - (audioBox.visible ? audioBox.implicitHeight + previewCard.spacing : 0))))

                ColumnLayout {
                    id: leftCol
                    width: leftFlick.width - (leftFlick.contentHeight > leftFlick.height ? 12 : 0)
                    spacing: 18

                    Rectangle {
                        id: ledfxBanner
                        visible: !!Daemon.ledfx.active
                        Layout.fillWidth: true
                        implicitHeight: bannerRow.implicitHeight + 24
                        radius: Theme.radiusSmall
                        color: Theme.alpha(Theme.violet, 0.12)
                        border.color: Theme.alpha(Theme.violet, 0.45)
                        RowLayout {
                            id: bannerRow
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10
                            Icon {
                                name: "eq"
                                color: Theme.violet
                                size: 18
                                Layout.alignment: Qt.AlignTop
                            }
                            ColumnLayout {
                                spacing: 2
                                Layout.fillWidth: true
                                Text {
                                    text: qsTr("LedFx is controlling the light")
                                    color: Theme.text
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    text: qsTr("%1 fps from %2. The effect chosen here comes back 2 seconds after LedFx stops.")
                                          .arg(Daemon.ledfx.fps || 0).arg(Daemon.ledfx.source || "?")
                                    color: Theme.textDim
                                    font.pixelSize: 12
                                    wrapMode: Text.WordWrap
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }

                    Card {
                        id: previewCard
                        Layout.fillWidth: true
                        padding: 22
                        Item {
                            Layout.fillWidth: true
                            Layout.preferredHeight: leftFlick.artHeight
                            GpuArt {
                                anchors.centerIn: parent
                                width: Math.min(parent.width, 520, parent.height / 0.41)
                                height: width * 0.41
                                led: page.led
                                rpm: {
                                    var f = Daemon.telemetry.fans || []
                                    return page.visible && f.length ? (f[0].rpm || 0) : 0
                                }
                            }
                        }
                        RowLayout {
                            id: nameRow
                            Layout.fillWidth: true
                            spacing: 12
                            Rectangle {
                                width: 40
                                height: 40
                                radius: 12
                                color: Theme.alpha(Theme.glow(page.led), 0.16)
                                Icon {
                                    anchors.centerIn: parent
                                    name: page.meta.icon
                                    color: Theme.glow(page.led)
                                    size: 20
                                }
                            }
                            ColumnLayout {
                                spacing: 1
                                Layout.fillWidth: true
                                Text {
                                    text: page.meta.name
                                    color: Theme.text
                                    font.pixelSize: 17
                                    font.weight: Font.DemiBold
                                }
                                Text {
                                    text: page.meta.desc
                                    color: Theme.textDim
                                    font.pixelSize: 13
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                            Rectangle {
                                width: 18
                                height: 18
                                radius: 9
                                color: page.led
                                border.color: Theme.alpha("#FFFFFF", 0.3)
                            }
                            Text {
                                text: page.led.toString().toUpperCase()
                                color: Theme.textDim
                                font.family: "monospace"
                                font.pixelSize: 13
                            }
                        }

                        // audio analysis
                        ColumnLayout {
                            id: audioBox
                            visible: !!page.meta.audio && !Daemon.ledfx.active
                            Layout.fillWidth: true
                            spacing: 12

                            Rectangle {
                                Layout.fillWidth: true
                                height: 1
                                color: Theme.border
                            }
                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8
                                Rectangle {
                                    width: 10
                                    height: 10
                                    radius: 5
                                    color: Theme.glow(page.led)
                                    opacity: 0.2
                                    NumberAnimation on opacity { id: beatFlash; running: false; from: 1; to: 0.2; duration: 220 }
                                    Connections {
                                        target: Audio
                                        enabled: page.listening
                                        function onBeat() { beatFlash.restart() }
                                    }
                                }
                                Text {
                                    text: !Audio.active ? qsTr("not active")
                                          : Audio.bpm > 0 ? qsTr("%1 BPM").arg(Audio.bpm)
                                          : page.listening && Audio.level > 0.05 ? qsTr("no steady beat")
                                          : qsTr("listening…")
                                    color: Theme.textDim
                                    font.pixelSize: 13
                                }
                                Item { Layout.fillWidth: true }
                                Text {
                                    visible: !!page.meta.smart
                                    text: qsTr("Detected style")
                                    color: Theme.textDim
                                    font.pixelSize: 13
                                }
                                Chip {
                                    visible: !!page.meta.smart
                                    text: (page.styleInfo[Audio.style] || page.styleInfo.pop).name
                                    tone: Theme.glow(page.led)
                                    strong: true
                                }
                            }
                            Text {
                                visible: !!page.meta.smart
                                text: (page.styleInfo[Audio.style] || page.styleInfo.pop).desc
                                color: Theme.textDim
                                font.pixelSize: 13
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                columnSpacing: 24
                                rowSpacing: 8
                                MeterRow { label: qsTr("Level"); value: page.listening ? Audio.level : 0 }
                                MeterRow { label: qsTr("Bass"); value: page.listening ? Audio.bass : 0 }
                                MeterRow { label: qsTr("Mids"); value: page.listening ? Audio.mid : 0 }
                                MeterRow { label: qsTr("Treble"); value: page.listening ? Audio.treble : 0 }
                            }
                            Text {
                                visible: Audio.error !== "" || !Audio.active
                                text: Audio.error !== "" ? Audio.error
                                      : qsTr("Listens to whatever the computer is playing (the default output's monitor). Needs the app running, at least in the tray.")
                                color: Audio.error !== "" ? Theme.warn : Theme.textFaint
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }
                        }
                    }

                    Card {
                        id: settingsCard
                        Layout.fillWidth: true
                        title: qsTr("Effect settings")
                        icon: "settings"
                        iconColor: Theme.glow(page.led)

                        ColumnLayout {
                            visible: page.meta.max > 0
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
                                text: page.meta.sensor ? qsTr("Gradient (low → high)")
                                      : page.meta.bands ? qsTr("Colors: bass · mids · treble")
                                      : page.meta.ledfx ? qsTr("Gradient (%1/%2)").arg(page.rgb.colors.length).arg(page.meta.max)
                                      : page.meta.max === 1 ? qsTr("Color")
                                      : qsTr("Colors (%1/%2)").arg(page.rgb.colors.length).arg(page.meta.max)
                                color: Theme.text
                                font.pixelSize: 14
                                font.weight: Font.Medium
                            }
                            PaletteEditor {
                                Layout.fillWidth: true
                                colors: page.rgb.colors
                                maxColors: page.meta.max
                                minColors: page.meta.min || 1
                                onChanged: (list) => {
                                    page.send({ colors: list })
                                    page.remember(page.meta.id, list)
                                }
                            }
                        }

                        SliderField {
                            id: speedField
                            visible: !!page.meta.speed
                            label: page.meta.audio ? qsTr("Reaction speed") : qsTr("Speed")
                            unit: "×"
                            decimals: 1
                            from: 0.1
                            to: 5
                            stepSize: 0.1
                            marks: [1]
                            accent: Theme.glow(page.led)
                            onMoved: page.send({ speed: Math.round(value * 10) / 10 })
                        }
                        SliderField {
                            id: brightField
                            visible: page.meta.id !== "off"
                            label: qsTr("Brightness")
                            unit: "%"
                            from: 0
                            to: 100
                            stepSize: 1
                            accent: Theme.glow(page.led)
                            onMoved: page.send({ brightness: Math.round(value) })
                        }
                        SliderField {
                            id: sensField
                            visible: !!page.meta.audio
                            label: qsTr("Sound sensitivity")
                            unit: "%"
                            from: 10
                            to: 300
                            stepSize: 5
                            marks: [100]
                            accent: Theme.glow(page.led)
                            hint: qsTr("Raise it if the light barely reacts, lower it if it flashes too often.")
                            onMoved: page.send({ sensitivity: Math.round(value) })
                        }
                        ColumnLayout {
                            visible: !!page.meta.sensor
                            Layout.fillWidth: true
                            spacing: 10
                            SliderField {
                                id: lowField
                                accent: Theme.glow(page.led)
                                label: qsTr("Value for the first color")
                                unit: page.meta.sensor || ""
                                from: 0
                                to: 109
                                stepSize: 1
                                onMoved: page.send({ low: Math.min(Math.round(value), page.rgb.high - 1) })
                            }
                            SliderField {
                                id: highField
                                accent: Theme.glow(page.led)
                                label: qsTr("Value for the last color")
                                unit: page.meta.sensor || ""
                                from: 1
                                to: 110
                                stepSize: 1
                                onMoved: page.send({ high: Math.max(Math.round(value), page.rgb.low + 1) })
                            }
                        }
                        Text {
                            visible: !!page.meta.rainbow || page.meta.id === "daytime" || page.meta.id === "off"
                            text: page.meta.id === "off" ? qsTr("The light stays off until another effect is chosen.")
                                  : page.meta.id === "daytime" ? qsTr("Follows this computer's clock.")
                                  : qsTr("Uses every hue, so there are no colors to pick.")
                            color: Theme.textFaint
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // right: effect grid
            Card {
                Layout.preferredWidth: 5
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: qsTr("Effects")
                subtitle: qsTr("%1 modes · computed on the PC and sent to the card, just like iGame Center").arg(page.effects.length)
                icon: "sparkle"
                iconColor: Theme.glow(page.led)

                Segmented {
                    Layout.fillWidth: true
                    accent: Theme.glow(page.led)
                    current: page.category
                    options: [
                        { value: "all", label: qsTr("All") },
                        { value: "anim", label: qsTr("Animated") },
                        { value: "audio", label: qsTr("Audio"), icon: "music" },
                        { value: "sensor", label: qsTr("Sensors") }
                    ]
                    onActivated: (v) => {
                        page.category = v
                        gridFlick.contentY = 0
                    }
                }

                Flickable {
                    id: gridFlick
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentWidth: width
                    contentHeight: grid.implicitHeight
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar {
                        id: gridBar
                        policy: ScrollBar.AsNeeded
                        contentItem: Rectangle {
                            implicitWidth: 6
                            visible: gridBar.size < 1.0
                            radius: 3
                            color: Theme.surface3
                        }
                    }

                    GridLayout {
                        id: grid
                        width: gridFlick.width - 12
                        columns: width > 640 ? 3 : 2
                        columnSpacing: 12
                        rowSpacing: 12

                        Repeater {
                            model: page.shownEffects
                            Rectangle {
                                id: tile
                                readonly property bool selected: page.rgb.effect === modelData.id
                                readonly property var swatch: page.swatchFor(modelData)
                                readonly property color tint: modelData.id === "off" ? Theme.textDim : Qt.color(swatch[0])
                                Layout.fillWidth: true
                                implicitHeight: 88
                                radius: 14
                                color: selected ? Theme.alpha(Theme.glow(tint), 0.12) : (tileArea.containsMouse ? Theme.surface2 : Theme.bgRaised)
                                border.color: selected ? Theme.alpha(Theme.glow(tint), 0.7) : Theme.border
                                border.width: selected ? 1.5 : 1
                                Behavior on color { ColorAnimation { duration: 140 } }
                                scale: tileArea.pressed ? 0.97 : 1
                                Behavior on scale { NumberAnimation { duration: 100 } }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 14
                                    spacing: 12
                                    Rectangle {
                                        width: 42
                                        height: 42
                                        radius: 13
                                        gradient: Gradient {
                                            orientation: Gradient.Horizontal
                                            GradientStop { position: 0; color: Theme.alpha(Qt.color(tile.swatch[0]), modelData.id === "off" ? 0.1 : 0.35) }
                                            GradientStop { position: 1; color: Theme.alpha(Qt.color(tile.swatch[tile.swatch.length - 1]), modelData.id === "off" ? 0.1 : 0.35) }
                                        }
                                        Icon {
                                            anchors.centerIn: parent
                                            name: modelData.icon
                                            size: 20
                                            color: modelData.id === "off" ? Theme.textDim : "#FFFFFF"
                                        }
                                    }
                                    ColumnLayout {
                                        spacing: 2
                                        Layout.fillWidth: true
                                        RowLayout {
                                            spacing: 6
                                            Text {
                                                text: modelData.name
                                                color: Theme.text
                                                font.pixelSize: 14
                                                font.weight: tile.selected ? Font.DemiBold : Font.Medium
                                                elide: Text.ElideRight
                                                Layout.fillWidth: !modelData.smart
                                            }
                                            Chip {
                                                visible: !!modelData.smart
                                                text: "AUTO"
                                                tone: Theme.glow(tile.tint)
                                                implicitHeight: 18
                                            }
                                            Item {
                                                visible: !!modelData.smart
                                                Layout.fillWidth: true
                                            }
                                        }
                                        Text {
                                            text: modelData.desc
                                            color: Theme.textDim
                                            font.pixelSize: 12
                                            elide: Text.ElideRight
                                            Layout.fillWidth: true
                                        }
                                    }
                                }
                                Rectangle {
                                    visible: tile.selected
                                    anchors.top: parent.top
                                    anchors.right: parent.right
                                    anchors.margins: 8
                                    width: 18
                                    height: 18
                                    radius: 9
                                    color: Theme.glow(tile.tint)
                                    Icon {
                                        anchors.centerIn: parent
                                        name: "check"
                                        size: 12
                                        stroke: 2.6
                                        color: "#0B0F16"
                                    }
                                }
                                MouseArea {
                                    id: tileArea
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: page.selectEffect(modelData)
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
