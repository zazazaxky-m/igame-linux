// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

Switch {
    id: sw
    property color accent: Theme.accent
    property string description

    spacing: 12
    padding: 0

    indicator: Rectangle {
        implicitWidth: 44
        implicitHeight: 24
        x: sw.leftPadding
        y: parent.height / 2 - height / 2
        radius: 12
        color: sw.checked ? sw.accent : Theme.surface3
        border.color: sw.checked ? Qt.lighter(sw.accent, 1.15) : Theme.borderStrong
        opacity: sw.enabled ? 1 : 0.45
        Behavior on color { ColorAnimation { duration: 160 } }
        Rectangle {
            x: sw.checked ? parent.width - width - 3 : 3
            y: 3
            width: 18
            height: 18
            radius: 9
            color: "#FFFFFF"
            Behavior on x { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
        }
    }

    contentItem: Column {
        leftPadding: sw.indicator.width + sw.spacing
        spacing: 2
        topPadding: sw.description === "" ? 3 : 0
        Text {
            text: sw.text
            color: sw.enabled ? Theme.text : Theme.textFaint
            font.pixelSize: 14
            visible: text !== ""
        }
        Text {
            text: sw.description
            color: Theme.textDim
            font.pixelSize: 12
            visible: text !== ""
            width: sw.availableWidth - sw.indicator.width - sw.spacing
            wrapMode: Text.WordWrap
        }
    }
}
