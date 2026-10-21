// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

// Fan glyph that spins proportionally to the real RPM (stops at 0 RPM).
Item {
    id: f
    property real rpm: 0
    property real size: 64
    property color color: Theme.teal

    implicitWidth: size
    implicitHeight: size

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.alpha(f.color, 0.08)
        border.color: Theme.alpha(f.color, f.rpm > 0 ? 0.45 : 0.2)
        border.width: 1.5
    }
    Icon {
        id: blades
        anchors.centerIn: parent
        name: "fan"
        size: f.size * 0.74
        stroke: 1.5
        color: f.rpm > 0 ? f.color : Theme.textDim
    }
    FrameAnimation {
        running: f.rpm > 0 && f.visible
        onTriggered: blades.rotation = (blades.rotation + frameTime * 360 * Math.min(f.rpm, 3600) / 900) % 360
    }
}
