// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: dlg
    property string title
    property string text
    property string confirmText: qsTr("Continue")
    property bool danger: false
    signal confirmed()

    anchors.centerIn: Overlay.overlay
    width: 440
    modal: true
    focus: true
    padding: 24
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.55) }
    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 160 }
            NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: 160; easing.type: Easing.OutCubic }
        }
    }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 110 } }

    background: Rectangle {
        radius: 18
        color: Theme.surface2
        border.color: Theme.borderStrong
    }

    contentItem: ColumnLayout {
        spacing: 16
        RowLayout {
            spacing: 12
            Rectangle {
                width: 38
                height: 38
                radius: 11
                color: Theme.alpha(dlg.danger ? Theme.bad : Theme.warn, 0.15)
                Icon {
                    anchors.centerIn: parent
                    name: "alert"
                    size: 19
                    color: dlg.danger ? Theme.bad : Theme.warn
                }
            }
            Text {
                text: dlg.title
                color: Theme.text
                font.pixelSize: 17
                font.weight: Font.DemiBold
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }
        }
        Text {
            text: dlg.text
            color: Theme.textDim
            font.pixelSize: 14
            wrapMode: Text.WordWrap
            lineHeight: 1.2
            Layout.fillWidth: true
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 10
            AppButton {
                text: qsTr("Cancel")
                kind: "ghost"
                onClicked: dlg.close()
            }
            AppButton {
                text: dlg.confirmText
                kind: dlg.danger ? "danger" : "primary"
                onClicked: {
                    dlg.close()
                    dlg.confirmed()
                }
            }
        }
    }
}
