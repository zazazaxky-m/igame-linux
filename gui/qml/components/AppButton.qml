// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

// kind: "primary" | "secondary" | "ghost" | "danger"
Button {
    id: b
    property string kind: "secondary"
    property string iconName
    property color accent: Theme.accent

    readonly property color fg: kind === "primary" ? "#FFFFFF"
                              : kind === "danger" ? Theme.bad
                              : Theme.text
    readonly property color bgColor: kind === "primary" ? accent
                                   : kind === "danger" ? Theme.alpha(Theme.bad, 0.12)
                                   : kind === "ghost" ? "transparent"
                                   : Theme.surface2

    implicitHeight: 38
    leftPadding: 16
    rightPadding: 16
    hoverEnabled: true
    font.pixelSize: 13
    font.weight: Font.DemiBold

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight
        Row {
            id: row
            anchors.centerIn: parent
            spacing: 8
            Icon {
                visible: b.iconName !== ""
                name: b.iconName
                size: 16
                color: b.fg
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: b.text
                color: b.fg
                font: b.font
                visible: text !== ""
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    background: Rectangle {
        radius: Theme.radiusSmall
        color: !b.enabled ? Theme.surface2
             : b.down ? Qt.darker(b.bgColor, 1.15)
             : b.hovered ? (b.kind === "ghost" ? Theme.surface2 : Qt.lighter(b.bgColor, 1.12))
             : b.bgColor
        border.color: b.kind === "secondary" ? Theme.borderStrong
                    : b.kind === "danger" ? Theme.alpha(Theme.bad, 0.35)
                    : "transparent"
        opacity: b.enabled ? 1 : 0.5
        Behavior on color { ColorAnimation { duration: 120 } }
    }
}
