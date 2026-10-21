// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

RowLayout {
    id: kv
    property string label
    property string value
    property color valueColor: Theme.text

    Layout.fillWidth: true
    spacing: 12

    Text {
        text: kv.label
        color: Theme.textDim
        font.pixelSize: 13
        Layout.preferredWidth: 150
        Layout.alignment: Qt.AlignTop
    }
    Text {
        text: kv.value
        color: kv.valueColor
        font.pixelSize: 13
        font.weight: Font.Medium
        Layout.fillWidth: true
        wrapMode: Text.Wrap
    }
}
