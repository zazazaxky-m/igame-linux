// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import IGame
import "../components"

ScrollPage {
    id: page
    title: qsTr("Profiles")
    subtitle: qsTr("Save combinations of performance, fan and RGB settings, then apply them with one click.")

    readonly property var profiles: Daemon.config.profiles || ({})
    readonly property var names: Object.keys(profiles).sort(function (a, b) { return a.localeCompare(b) })
    readonly property var effectNames: ({
        static: qsTr("Static"),
        breathing: qsTr("Breathing"),
        rainbow_breathing: qsTr("Rainbow breathing"),
        spectrum: qsTr("Spectrum"),
        cycle: qsTr("Color cycle"),
        aurora: qsTr("Aurora"),
        plasma: qsTr("Plasma"),
        ocean: qsTr("Ocean"),
        heartbeat: qsTr("Heartbeat"),
        candle: qsTr("Candle"),
        twinkle: qsTr("Twinkle"),
        lightning: qsTr("Lightning"),
        glitch: qsTr("Glitch"),
        flash: qsTr("Flash"),
        double_flash: qsTr("Double flash"),
        strobe: qsTr("Strobe"),
        police: qsTr("Police"),
        daytime: qsTr("Time of day"),
        off: qsTr("Off"),
        music_smart: qsTr("Smart Sound"),
        music: qsTr("Bar"),
        music_spectrum: qsTr("Energy"),
        music_scroll: qsTr("Scroll"),
        music_bass: qsTr("Power"),
        music_strobe: qsTr("BPM Strobe"),
        music_realstrobe: qsTr("Real Strobe"),
        music_rainbow: qsTr("Wavelength"),
        music_fire: qsTr("Magnitude"),
        music_chill: qsTr("Ambient"),
        temperature: qsTr("Follow temperature"),
        load: qsTr("Follow load"),
        power: qsTr("Follow power")
    })
    property string pendingDelete

    function ocSummary(oc) {
        if (!oc) return "—"
        var p = oc.power_w > 0 ? oc.power_w + " W" : qsTr("Default power")
        var c = (oc.core_mhz > 0 ? "+" : "") + (oc.core_mhz || 0)
        var m = (oc.mem_mhz > 0 ? "+" : "") + (oc.mem_mhz || 0)
        return p + " · " + qsTr("core %1 · mem %2").arg(c).arg(m) + (oc.lock_enabled ? " · " + qsTr("lock %1–%2").arg(oc.lock_min).arg(oc.lock_max) : "")
    }
    function fanSummary(f) {
        if (!f || !f.fans || !f.fans.length) return "—"
        var names = { auto: qsTr("Automatic"), fixed: qsTr("Fixed"), curve: qsTr("Curve") }
        if (f.sync) {
            var s = f.fans[0]
            return names[s.mode] + (s.mode === "fixed" ? " " + s.fixed + "%" : "") + " · " + qsTr("all fans")
        }
        return f.fans.map(function (s, i) { return (i + 1) + ": " + names[s.mode] }).join(" · ")
    }
    function formatDate(iso) {
        if (!iso) return ""
        var d = new Date(iso)
        return isNaN(d.getTime()) ? iso : d.toLocaleString(Qt.locale(App.effectiveLanguage === "id" ? "id_ID" : "en_GB"), "d MMM yyyy, HH:mm")
    }

    Card {
        Layout.fillWidth: true
        title: qsTr("Save the current settings")
        subtitle: qsTr("The active performance, fan and RGB settings are saved to a profile. An existing name is overwritten.")
        icon: "save"
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            AppTextField {
                id: nameField
                Layout.fillWidth: true
                placeholderText: qsTr("Profile name, e.g. Gaming, Quiet, Work")
                maximumLength: 40
                onAccepted: saveButton.clicked()
            }
            AppButton {
                id: saveButton
                text: page.profiles[nameField.text.trim()] ? qsTr("Overwrite profile") : qsTr("Save profile")
                kind: "primary"
                iconName: "save"
                enabled: nameField.text.trim().length > 0
                onClicked: {
                    Daemon.saveProfile(nameField.text.trim())
                    nameField.text = ""
                }
            }
        }
    }

    Item {
        visible: page.names.length === 0
        Layout.fillWidth: true
        Layout.preferredHeight: 220
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 10
            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                width: 64
                height: 64
                radius: 20
                color: Theme.surface2
                Icon {
                    anchors.centerIn: parent
                    name: "profiles"
                    size: 28
                    color: Theme.textDim
                }
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("No profiles yet")
                color: Theme.text
                font.pixelSize: 16
                font.weight: Font.DemiBold
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Set up performance, fans and RGB the way you like, then save them above.")
                color: Theme.textDim
                font.pixelSize: 13
            }
        }
    }

    GridLayout {
        visible: page.names.length > 0
        Layout.fillWidth: true
        columns: width > 980 ? 3 : 2
        columnSpacing: 18
        rowSpacing: 18

        Repeater {
            model: page.names
            Card {
                id: profileCard
                readonly property var p: page.profiles[modelData] || ({})
                readonly property var colors: (p.rgb && p.rgb.colors) || []
                Layout.fillWidth: true
                Layout.fillHeight: true
                title: modelData
                subtitle: qsTr("Saved %1").arg(page.formatDate(p.saved))
                icon: "profiles"
                iconColor: colors.length ? Theme.glow(Qt.color(colors[0])) : Theme.accent

                ColumnLayout {
                    spacing: 10
                    Layout.fillWidth: true
                    RowLayout {
                        spacing: 10
                        Icon { name: "bolt"; size: 16; color: Theme.violet }
                        Text {
                            text: page.ocSummary(profileCard.p.oc)
                            color: Theme.textDim
                            font.pixelSize: 13
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                    }
                    RowLayout {
                        spacing: 10
                        Icon { name: "fan"; size: 16; color: Theme.teal }
                        Text {
                            text: page.fanSummary(profileCard.p.fans)
                            color: Theme.textDim
                            font.pixelSize: 13
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                        }
                    }
                    RowLayout {
                        spacing: 10
                        Icon { name: "rgb"; size: 16; color: Theme.warn }
                        Text {
                            text: page.effectNames[(profileCard.p.rgb || {}).effect] || "—"
                            color: Theme.textDim
                            font.pixelSize: 13
                        }
                        Row {
                            spacing: 4
                            Repeater {
                                model: profileCard.colors
                                Rectangle {
                                    width: 14
                                    height: 14
                                    radius: 7
                                    color: modelData
                                    border.color: Theme.alpha("#FFFFFF", 0.25)
                                }
                            }
                        }
                    }
                }
                Item { Layout.fillHeight: true }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    AppButton {
                        text: qsTr("Apply")
                        kind: "primary"
                        iconName: "play"
                        Layout.fillWidth: true
                        onClicked: Daemon.loadProfile(modelData)
                    }
                    AppButton {
                        kind: "danger"
                        iconName: "trash"
                        leftPadding: 12
                        rightPadding: 12
                        onClicked: {
                            page.pendingDelete = modelData
                            deleteDialog.open()
                        }
                    }
                }
            }
        }
    }

    ConfirmDialog {
        id: deleteDialog
        title: qsTr("Delete profile \"%1\"?").arg(page.pendingDelete)
        text: qsTr("This profile will be permanently removed from igamed's configuration.")
        confirmText: qsTr("Delete")
        danger: true
        onConfirmed: Daemon.deleteProfile(page.pendingDelete)
    }
}
