// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

Item {
    id: host
    function show(text, error) {
        if (!text)
            return
        toastModel.append({ message: text, error: !!error })
        while (toastModel.count > 4)
            toastModel.remove(0)
    }

    ListModel { id: toastModel }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 26
        spacing: 8

        Repeater {
            model: toastModel
            Rectangle {
                id: toast
                anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                width: Math.min(host.width - 80, row.implicitWidth + 32)
                height: Math.max(44, row.implicitHeight + 20)
                radius: 14
                color: Theme.surface3
                border.color: error ? Theme.alpha(Theme.bad, 0.55) : Theme.alpha(Theme.good, 0.45)
                opacity: 0
                Component.onCompleted: appear.start()
                NumberAnimation on opacity { id: appear; running: false; from: 0; to: 1; duration: 180 }

                RowLayout {
                    id: row
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 10
                    Icon {
                        name: error ? "alert" : "check"
                        color: error ? Theme.bad : Theme.good
                        size: 18
                    }
                    Text {
                        text: message
                        color: Theme.text
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        Layout.maximumWidth: host.width - 160
                    }
                }
                Timer {
                    interval: error ? 6500 : 3200
                    running: true
                    onTriggered: toastModel.remove(index)
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: toastModel.remove(index)
                }
            }
        }
    }
}
