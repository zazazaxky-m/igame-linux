// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

// Segmented control. options: [{ value, label, icon? }]
Rectangle {
    id: seg
    property var options: []
    property var current
    property color accent: Theme.accent
    signal activated(var value)

    readonly property int currentIndex: {
        for (var i = 0; i < options.length; ++i)
            if (options[i].value === current) return i
        return -1
    }

    implicitHeight: 38
    implicitWidth: options.length * 110
    radius: Theme.radiusSmall
    color: Theme.surface2
    border.color: Theme.border

    Rectangle {
        visible: seg.currentIndex >= 0
        y: 3
        height: parent.height - 6
        width: (seg.width - 6) / Math.max(1, seg.options.length)
        x: 3 + seg.currentIndex * width
        radius: Theme.radiusSmall - 3
        color: Theme.alpha(seg.accent, 0.2)
        border.color: Theme.alpha(seg.accent, 0.5)
        Behavior on x { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
    }

    Row {
        anchors.fill: parent
        anchors.margins: 3
        Repeater {
            model: seg.options
            Item {
                width: (seg.width - 6) / Math.max(1, seg.options.length)
                height: parent.height
                Row {
                    anchors.centerIn: parent
                    spacing: 7
                    Icon {
                        visible: !!modelData.icon
                        name: modelData.icon || ""
                        size: 15
                        color: index === seg.currentIndex ? Theme.text : Theme.textDim
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: modelData.label
                        color: index === seg.currentIndex ? Theme.text : (area.containsMouse ? Theme.text : Theme.textDim)
                        font.pixelSize: 13
                        font.weight: index === seg.currentIndex ? Font.DemiBold : Font.Normal
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    enabled: seg.enabled
                    onClicked: seg.activated(modelData.value)
                }
            }
        }
    }
}
