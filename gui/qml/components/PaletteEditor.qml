// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

// Row of color chips. Click a chip to edit it, x to remove, + to add (up to maxColors).
Item {
    id: editor
    property var colors: []
    property int maxColors: 8
    property int minColors: 1
    signal changed(var colors)

    property int editing: -1
    // Chips shrink (down to 34 px) so a full 8-color gradient plus "+" stays on one row;
    // if even that does not fit, the row wraps and the editor grows with it.
    readonly property int slots: colors.length + (colors.length < maxColors ? 1 : 0)
    readonly property int chip: Math.max(34, Math.min(48, Math.floor((width - (slots - 1) * flow.spacing) / Math.max(1, slots))))
    implicitHeight: flow.implicitHeight
    implicitWidth: flow.implicitWidth

    function update(index, c) {
        var list = colors.slice()
        list[index] = c.toString().toUpperCase()
        changed(list)
    }

    Flow {
        id: flow
        width: parent.width
        spacing: 10

        Repeater {
            model: editor.colors
            Item {
                width: editor.chip
                height: editor.chip
                Rectangle {
                    id: chip
                    anchors.fill: parent
                    radius: editor.chip * 0.29
                    color: modelData
                    border.color: editor.editing === index ? "#FFFFFF" : Theme.alpha("#FFFFFF", 0.18)
                    border.width: editor.editing === index ? 3 : 1
                    scale: chipArea.pressed ? 0.94 : 1
                    Behavior on scale { NumberAnimation { duration: 100 } }
                    MouseArea {
                        id: chipArea
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            editor.editing = index
                            picker.color = Qt.color(modelData)
                            var p = chip.mapToItem(editor, 0, chip.height + 8)
                            picker.x = p.x
                            picker.y = p.y
                            picker.open()
                        }
                    }
                }
                Rectangle {
                    visible: editor.colors.length > editor.minColors
                    width: 20
                    height: 20
                    radius: 10
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.topMargin: -6
                    anchors.rightMargin: -6
                    color: Theme.surface3
                    border.color: Theme.borderStrong
                    Icon {
                        anchors.centerIn: parent
                        name: "close"
                        size: 11
                        stroke: 2.4
                        color: Theme.textDim
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            var list = editor.colors.slice()
                            list.splice(index, 1)
                            editor.changed(list)
                        }
                    }
                }
            }
        }

        Rectangle {
            visible: editor.colors.length < editor.maxColors
            width: editor.chip
            height: editor.chip
            radius: editor.chip * 0.29
            color: addArea.containsMouse ? Theme.surface3 : Theme.surface2
            border.color: Theme.borderStrong
            Icon {
                anchors.centerIn: parent
                name: "plus"
                size: 18
                color: Theme.textDim
            }
            MouseArea {
                id: addArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    var list = editor.colors.slice()
                    var last = list.length ? Qt.color(list[list.length - 1]) : Qt.color("#FFFFFF")
                    var hue = last.hsvHue < 0 ? 0 : last.hsvHue
                    list.push(Qt.hsva((hue + 0.18) % 1, 1, 1, 1).toString().toUpperCase())
                    editor.changed(list)
                }
            }
        }
    }

    ColorPicker {
        id: picker
        onEdited: (c) => {
            if (editor.editing >= 0)
                editor.update(editor.editing, c)
        }
        onClosed: editor.editing = -1
    }
}
