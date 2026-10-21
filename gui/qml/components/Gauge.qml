// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Shapes
import QtQuick.Effects

// 270° gauge
Item {
    id: g
    property real value: 0
    property real from: 0
    property real to: 100
    property string valueText: Math.round(value).toString()
    property string unit
    property string label
    property string sub
    property color color: Theme.accent
    property real thickness: 11

    implicitWidth: 168
    implicitHeight: 168

    readonly property real fraction: to > from ? Math.max(0, Math.min(1, (value - from) / (to - from))) : 0
    property real shown: fraction
    Behavior on shown { enabled: g.visible; NumberAnimation { duration: 700; easing.type: Easing.OutCubic } }
    property color shownColor: color
    Behavior on shownColor { enabled: g.visible; ColorAnimation { duration: 500 } }

    readonly property real r: Math.min(width, height) / 2 - thickness

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeColor: Theme.surface3
            strokeWidth: g.thickness
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: g.width / 2
                centerY: g.height / 2
                radiusX: g.r
                radiusY: g.r
                startAngle: 135
                sweepAngle: 270
            }
        }
    }

    Item {
        anchors.fill: parent
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: g.shownColor
            shadowBlur: 0.7
            shadowOpacity: 0.55
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 0
            blurMax: 24
        }
        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            visible: g.shown > 0.002
            ShapePath {
                strokeColor: g.shownColor
                strokeWidth: g.thickness
                fillColor: "transparent"
                capStyle: ShapePath.RoundCap
                PathAngleArc {
                    centerX: g.width / 2
                    centerY: g.height / 2
                    radiusX: g.r
                    radiusY: g.r
                    startAngle: 135
                    sweepAngle: 270 * g.shown
                }
            }
        }
    }

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 4
        spacing: 0
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 2
            Text {
                id: valueLabel
                text: g.valueText
                color: Theme.text
                font.pixelSize: g.width * 0.2
                font.weight: Font.DemiBold
            }
            Text {
                text: g.unit
                color: Theme.textDim
                font.pixelSize: g.width * 0.09
                anchors.baseline: valueLabel.baseline
            }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: g.label
            color: Theme.textDim
            font.pixelSize: 12
            font.weight: Font.Medium
        }
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 2
        text: g.sub
        color: Theme.textFaint
        font.pixelSize: 11
    }
}
