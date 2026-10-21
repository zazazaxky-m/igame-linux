// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Shapes

// 24x24 stroke icon from Theme.icons
Item {
    id: root
    property string name
    property color color: Theme.text
    property real size: 20
    property real stroke: 1.8

    implicitWidth: size
    implicitHeight: size

    Shape {
        anchors.centerIn: parent
        width: 24
        height: 24
        scale: root.size / 24
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: root.color
            strokeWidth: root.stroke
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: Theme.icons[root.name] || "" }
        }
    }
}
