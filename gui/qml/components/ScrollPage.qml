// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Scrollable page with a title header; children go into a column below the header.
Item {
    id: page
    property string title
    property string subtitle
    property int bottomSpace: 32
    default property alias content: column.data
    property alias headerRight: headerRow.data
    property alias flickable: flick

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: column.implicitHeight + 28 + page.bottomSpace
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar {
            id: pageBar
            policy: ScrollBar.AsNeeded
            contentItem: Rectangle {
                implicitWidth: 6
                visible: pageBar.size < 1.0
                radius: 3
                color: Theme.surface3
            }
        }

        ColumnLayout {
            id: column
            x: 32
            y: 28
            width: flick.width - 64
            spacing: 18

            RowLayout {
                Layout.fillWidth: true
                spacing: 16
                ColumnLayout {
                    spacing: 4
                    Layout.fillWidth: true
                    Text {
                        text: page.title
                        color: Theme.text
                        font.pixelSize: 26
                        font.weight: Font.Bold
                    }
                    Text {
                        visible: page.subtitle !== ""
                        text: page.subtitle
                        color: Theme.textDim
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                }
                RowLayout {
                    id: headerRow
                    spacing: 10
                }
            }
        }
    }
}
