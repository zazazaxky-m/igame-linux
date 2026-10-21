// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

Rectangle {
    id: chip
    property string text
    property string icon
    property color tone: Theme.textDim
    property bool strong: false

    implicitHeight: 24
    implicitWidth: row.implicitWidth + 18
    radius: 12
    color: Theme.alpha(tone, strong ? 0.2 : 0.12)
    border.color: Theme.alpha(tone, 0.3)

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 5
        Icon {
            visible: chip.icon !== ""
            name: chip.icon
            size: 13
            color: chip.tone
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: chip.text
            color: chip.tone
            font.pixelSize: 12
            font.weight: Font.DemiBold
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
