// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// HSV picker popup: saturation/value square, hue bar, hex field, swatches. Emits live edits.
Popup {
    id: pop
    property color color: "#FFFFFF"
    property real h: 0
    property real s: 0
    property real v: 1
    signal edited(color c)

    readonly property color current: Qt.hsva(h, s, v, 1)
    readonly property var swatches: ["#FFFFFF", "#FF0000", "#FF5A00", "#FFB000", "#FFE600", "#7CFF00",
                                     "#00FF55", "#00FFC8", "#00C8FF", "#0066FF", "#3D00FF", "#8C00FF",
                                     "#E100FF", "#FF00AA", "#FF0055", "#FF8A1F"]

    function load(c) {
        h = c.hsvHue < 0 ? h : c.hsvHue
        s = c.hsvSaturation
        v = c.hsvValue
    }
    function push() {
        pop.edited(pop.current)
        hexField.text = pop.current.toString().toUpperCase()
    }
    function clamp01(x) { return Math.max(0, Math.min(1, x)) }

    onAboutToShow: {
        load(color)
        hexField.text = color.toString().toUpperCase()
    }

    width: 300
    padding: 16
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 140 } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 100 } }

    background: Rectangle {
        radius: 16
        color: Theme.surface2
        border.color: Theme.borderStrong
    }

    contentItem: ColumnLayout {
        spacing: 14

        Item {
            id: sv
            Layout.fillWidth: true
            Layout.preferredHeight: 170
            Rectangle {
                anchors.fill: parent
                radius: 10
                color: Qt.hsva(pop.h, 1, 1, 1)
            }
            Rectangle {
                anchors.fill: parent
                radius: 10
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0; color: "#FFFFFFFF" }
                    GradientStop { position: 1; color: "#00FFFFFF" }
                }
            }
            Rectangle {
                anchors.fill: parent
                radius: 10
                gradient: Gradient {
                    GradientStop { position: 0; color: "#00000000" }
                    GradientStop { position: 1; color: "#FF000000" }
                }
            }
            Rectangle {
                x: pop.s * sv.width - width / 2
                y: (1 - pop.v) * sv.height - height / 2
                width: 18
                height: 18
                radius: 9
                color: pop.current
                border.color: "#FFFFFF"
                border.width: 3
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.CrossCursor
                function update(mouse) {
                    pop.s = pop.clamp01(mouse.x / width)
                    pop.v = pop.clamp01(1 - mouse.y / height)
                    pop.push()
                }
                onPressed: (mouse) => update(mouse)
                onPositionChanged: (mouse) => update(mouse)
            }
        }

        Item {
            id: hueBar
            Layout.fillWidth: true
            Layout.preferredHeight: 16
            Rectangle {
                anchors.fill: parent
                radius: 8
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#FF0000" }
                    GradientStop { position: 0.17; color: "#FFFF00" }
                    GradientStop { position: 0.33; color: "#00FF00" }
                    GradientStop { position: 0.5; color: "#00FFFF" }
                    GradientStop { position: 0.67; color: "#0000FF" }
                    GradientStop { position: 0.83; color: "#FF00FF" }
                    GradientStop { position: 1.0; color: "#FF0000" }
                }
            }
            Rectangle {
                x: pop.h * hueBar.width - width / 2
                anchors.verticalCenter: parent.verticalCenter
                width: 20
                height: 20
                radius: 10
                color: Qt.hsva(pop.h, 1, 1, 1)
                border.color: "#FFFFFF"
                border.width: 3
            }
            MouseArea {
                anchors.fill: parent
                anchors.margins: -6
                cursorShape: Qt.PointingHandCursor
                function update(mouse) {
                    pop.h = pop.clamp01((mouse.x - 6) / hueBar.width) * 0.9999
                    pop.push()
                }
                onPressed: (mouse) => update(mouse)
                onPositionChanged: (mouse) => update(mouse)
            }
        }

        RowLayout {
            spacing: 10
            Rectangle {
                width: 38
                height: 38
                radius: 10
                color: pop.current
                border.color: Theme.borderStrong
            }
            AppTextField {
                id: hexField
                Layout.fillWidth: true
                font.family: "monospace"
                maximumLength: 7
                validator: RegularExpressionValidator { regularExpression: /#?[0-9A-Fa-f]{0,6}/ }
                onEditingFinished: {
                    var t = text.startsWith("#") ? text : "#" + text
                    if (/^#[0-9A-Fa-f]{6}$/.test(t)) {
                        pop.load(Qt.color(t))
                        pop.push()
                    }
                }
            }
        }

        Grid {
            columns: 8
            spacing: 6
            Layout.alignment: Qt.AlignHCenter
            Repeater {
                model: pop.swatches
                Rectangle {
                    width: 28
                    height: 28
                    radius: 8
                    color: modelData
                    border.color: swatchArea.containsMouse ? "#FFFFFF" : Theme.alpha("#FFFFFF", 0.15)
                    border.width: swatchArea.containsMouse ? 2 : 1
                    MouseArea {
                        id: swatchArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            pop.load(Qt.color(modelData))
                            pop.push()
                        }
                    }
                }
            }
        }
    }
}
