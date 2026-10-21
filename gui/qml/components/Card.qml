// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: card
    property string title
    property string subtitle
    property string icon
    property color iconColor: Theme.accent
    property int padding: 20
    property int spacing: 14
    default property alias content: body.data
    property alias actions: actionRow.data

    radius: Theme.radius
    color: Theme.surface
    border.color: Theme.border
    border.width: 1
    implicitWidth: column.implicitWidth + padding * 2
    implicitHeight: column.implicitHeight + padding * 2

    // subtle top highlight
    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: card.radius - 1
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.rgba(1, 1, 1, 0.025) }
            GradientStop { position: 0.35; color: "transparent" }
        }
    }

    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: card.padding
        spacing: card.spacing

        RowLayout {
            visible: card.title !== ""
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                visible: card.icon !== ""
                width: 34
                height: 34
                radius: 10
                color: Theme.alpha(card.iconColor, 0.14)
                Icon {
                    anchors.centerIn: parent
                    name: card.icon
                    size: 18
                    color: card.iconColor
                }
            }
            ColumnLayout {
                spacing: 1
                Layout.fillWidth: true
                Text {
                    text: card.title
                    color: Theme.text
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Text {
                    visible: card.subtitle !== ""
                    text: card.subtitle
                    color: Theme.textDim
                    font.pixelSize: 12
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
            }
            RowLayout {
                id: actionRow
                spacing: 8
            }
        }

        ColumnLayout {
            id: body
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: card.spacing
        }
    }
}
