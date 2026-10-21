// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

Item {
    id: root
    property string text
    property string icon
    property bool selected: false
    property string badge
    signal clicked()

    implicitHeight: 44
    Layout.fillWidth: true

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: root.selected ? Theme.alpha(Theme.accent, 0.14) : (mouse.containsMouse ? Theme.surface2 : "transparent")
        border.color: root.selected ? Theme.alpha(Theme.accent, 0.35) : "transparent"
        Behavior on color { ColorAnimation { duration: 140 } }
    }
    Rectangle {
        visible: root.selected
        width: 3
        height: 20
        radius: 2
        color: Theme.accent
        anchors.left: parent.left
        anchors.leftMargin: -1
        anchors.verticalCenter: parent.verticalCenter
    }
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 12
        spacing: 12
        Icon {
            name: root.icon
            size: 19
            color: root.selected ? Theme.accent : (mouse.containsMouse ? Theme.text : Theme.textDim)
        }
        Text {
            text: root.text
            color: root.selected ? Theme.text : Theme.textDim
            font.pixelSize: 14
            font.weight: root.selected ? Font.DemiBold : Font.Normal
            Layout.fillWidth: true
        }
        Rectangle {
            visible: root.badge !== ""
            radius: 8
            color: Theme.alpha(Theme.warn, 0.18)
            implicitWidth: badgeText.implicitWidth + 12
            implicitHeight: 18
            Text {
                id: badgeText
                anchors.centerIn: parent
                text: root.badge
                color: Theme.warn
                font.pixelSize: 10
                font.weight: Font.DemiBold
            }
        }
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
