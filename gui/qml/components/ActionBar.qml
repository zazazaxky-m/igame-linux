// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects

// Floating "unsaved changes" bar that slides up from the bottom of a page.
Item {
    id: bar
    property bool shown: false
    property string message: qsTr("You have unsaved changes")
    property string applyText: qsTr("Apply")
    property bool busy: false
    signal applyClicked()
    signal discardClicked()

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.bottom: parent.bottom
    height: 92
    visible: offset < 1

    property real offset: shown ? 0 : 1
    Behavior on offset { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }

    Rectangle {
        id: panel
        anchors.horizontalCenter: parent.horizontalCenter
        y: 18 + bar.offset * 90
        width: Math.min(parent.width - 64, 720)
        height: 58
        radius: 16
        color: Theme.surface3
        border.color: Theme.alpha(Theme.accent, 0.45)
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#000000"
            shadowOpacity: 0.6
            shadowBlur: 0.8
            shadowVerticalOffset: 6
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 18
            anchors.rightMargin: 10
            spacing: 10
            Rectangle {
                width: 9
                height: 9
                radius: 5
                color: Theme.warn
            }
            Text {
                text: bar.message
                color: Theme.text
                font.pixelSize: 14
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
            AppButton {
                text: qsTr("Discard")
                kind: "ghost"
                onClicked: bar.discardClicked()
            }
            AppButton {
                text: bar.applyText
                kind: "primary"
                iconName: "check"
                enabled: !bar.busy
                onClicked: bar.applyClicked()
            }
        }
    }
}
