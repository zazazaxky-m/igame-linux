// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Shapes
import QtQuick.Effects

// Stylised white triple-fan iGame card. The light bar and ambient glow follow the real LED color,
// the fans spin with the real fan RPM.
Item {
    id: art
    property color led: "#FFFFFF"
    property real rpm: 0
    property bool showGlow: true

    readonly property color glowColor: Theme.glow(led)
    readonly property real ledLevel: Math.max(led.r, led.g, led.b)

    implicitWidth: 560
    implicitHeight: 230

    component FanDisc: Item {
        id: disc
        property real spin: 0
        Rectangle {
            anchors.fill: parent
            radius: width / 2
            gradient: Gradient {
                GradientStop { position: 0; color: "#2B313D" }
                GradientStop { position: 1; color: "#1A1F28" }
            }
            border.color: "#3A4250"
            border.width: 2
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.88
            height: width
            radius: width / 2
            color: "#E9EDF3"
        }
        Icon {
            id: blade
            anchors.centerIn: parent
            name: "fan"
            size: parent.width * 0.86
            stroke: 1.15
            color: "#8F98A8"
            rotation: disc.spin
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.26
            height: width
            radius: width / 2
            gradient: Gradient {
                GradientStop { position: 0; color: "#FFFFFF" }
                GradientStop { position: 1; color: "#CDD3DD" }
            }
            border.color: "#B5BDC9"
            Text {
                anchors.centerIn: parent
                text: "iG"
                color: "#7C8696"
                font.pixelSize: parent.width * 0.34
                font.weight: Font.Black
                font.italic: true
            }
        }
    }

    property real spin: 0
    FrameAnimation {
        running: art.rpm > 0 && art.visible
        onTriggered: art.spin = (art.spin + frameTime * 360 * Math.min(art.rpm, 3600) / 900) % 360
    }

    Shape {
        visible: art.showGlow
        anchors.centerIn: parent
        width: parent.width * 1.2
        height: parent.height * 1.6
        opacity: 0.25 + 0.75 * art.ledLevel
        ShapePath {
            strokeColor: "transparent"
            fillGradient: RadialGradient {
                centerX: art.width * 0.6
                centerY: art.height * 0.8
                centerRadius: art.width * 0.6
                focalX: centerX
                focalY: centerY
                GradientStop { position: 0; color: Theme.alpha(art.glowColor, 0.32) }
                GradientStop { position: 0.6; color: Theme.alpha(art.glowColor, 0.08) }
                GradientStop { position: 1; color: "transparent" }
            }
            startX: 0
            startY: 0
            PathLine { x: art.width * 1.2; y: 0 }
            PathLine { x: art.width * 1.2; y: art.height * 1.6 }
            PathLine { x: 0; y: art.height * 1.6 }
            PathLine { x: 0; y: 0 }
        }
    }

    Rectangle {
        id: body
        anchors.centerIn: parent
        width: parent.width * 0.94
        height: parent.height * 0.74
        radius: height * 0.16
        gradient: Gradient {
            GradientStop { position: 0; color: "#F7F8FB" }
            GradientStop { position: 0.55; color: "#E3E7EE" }
            GradientStop { position: 1; color: "#C3CAD6" }
        }
        border.color: "#FFFFFF"
        border.width: 1

        // brushed accent lines
        Repeater {
            model: 4
            Rectangle {
                x: body.width * (0.05 + index * 0.012)
                y: body.height * 0.18
                width: 2
                height: body.height * 0.64
                radius: 1
                color: "#B9C1CD"
                opacity: 0.7
            }
        }

        Row {
            id: fanRow
            anchors.verticalCenter: parent.verticalCenter
            x: body.width * 0.05
            spacing: body.width * 0.02
            Repeater {
                model: 3
                FanDisc {
                    width: body.height * 0.76
                    height: width
                    spin: art.spin + index * 40
                }
            }
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: fanRow.right
            anchors.right: parent.right
            spacing: body.height * 0.03
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "iGAME"
                color: "#7E8899"
                font.pixelSize: Math.max(8, body.height * 0.085)
                font.weight: Font.Black
                font.italic: true
            }
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: body.height * 0.05
                height: body.height * 0.34
                radius: width / 2
                color: art.ledLevel < 0.02 ? "#9AA3B2" : art.led
            }
        }
    }

    // RGB light bar along the top edge
    Item {
        anchors.horizontalCenter: body.horizontalCenter
        anchors.verticalCenter: body.top
        width: body.width * 0.78
        height: 7
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: art.glowColor
            shadowBlur: 1.0
            shadowOpacity: Math.min(1, 0.25 + art.ledLevel)
            shadowHorizontalOffset: 0
            shadowVerticalOffset: 0
            blurMax: 48
        }
        Rectangle {
            anchors.fill: parent
            radius: height / 2
            color: art.ledLevel < 0.02 ? "#3A4250" : art.led
        }
    }
}
