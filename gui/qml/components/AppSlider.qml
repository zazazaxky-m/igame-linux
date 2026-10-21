// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

Slider {
    id: s
    property color accent: Theme.accent
    property string unit
    property int decimals: 0
    property var marks: []
    property string bubbleText: (decimals > 0 ? value.toFixed(decimals) : Math.round(value)) + unit

    implicitHeight: 30
    leftPadding: 9
    rightPadding: 9

    background: Item {
        x: s.leftPadding
        y: s.topPadding + s.availableHeight / 2 - height / 2
        width: s.availableWidth
        height: 6
        Rectangle {
            anchors.fill: parent
            radius: 3
            color: Theme.surface3
        }
        Rectangle {
            width: Math.max(height, s.visualPosition * parent.width)
            height: parent.height
            radius: 3
            opacity: s.enabled ? 1 : 0.4
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: Qt.darker(s.accent, 1.5) }
                GradientStop { position: 1; color: s.accent }
            }
        }
        Repeater {
            model: s.marks
            Rectangle {
                x: (modelData - s.from) / (s.to - s.from) * parent.width - 1
                y: -4
                width: 2
                height: 14
                radius: 1
                color: Theme.textFaint
            }
        }
    }

    handle: Rectangle {
        x: s.leftPadding + s.visualPosition * s.availableWidth - width / 2
        y: s.topPadding + s.availableHeight / 2 - height / 2
        width: 18
        height: 18
        radius: 9
        color: "#FFFFFF"
        border.color: s.enabled ? s.accent : Theme.textFaint
        border.width: 4
        scale: s.pressed ? 1.18 : (s.hovered ? 1.08 : 1)
        Behavior on scale { NumberAnimation { duration: 120 } }

        Rectangle {
            visible: s.pressed
            anchors.bottom: parent.top
            anchors.bottomMargin: 10
            anchors.horizontalCenter: parent.horizontalCenter
            width: bubble.implicitWidth + 14
            height: 24
            radius: 7
            color: Theme.surface3
            border.color: Theme.borderStrong
            Text {
                id: bubble
                anchors.centerIn: parent
                text: s.bubbleText
                color: Theme.text
                font.pixelSize: 12
                font.weight: Font.DemiBold
            }
        }
    }
}
