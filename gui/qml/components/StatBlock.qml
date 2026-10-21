// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: stat
    property string label
    property string value
    property string unit
    property string hint
    property color valueColor: Theme.text
    property int valueSize: 24

    spacing: 2

    Text {
        text: stat.label
        color: Theme.textDim
        font.pixelSize: 12
        font.weight: Font.Medium
    }
    Row {
        spacing: 4
        Text {
            id: big
            text: stat.value
            color: stat.valueColor
            font.pixelSize: stat.valueSize
            font.weight: Font.DemiBold
        }
        Text {
            text: stat.unit
            color: Theme.textDim
            font.pixelSize: Math.round(stat.valueSize * 0.55)
            anchors.baseline: big.baseline
        }
    }
    Text {
        visible: stat.hint !== ""
        text: stat.hint
        color: Theme.textFaint
        font.pixelSize: 12
    }
}
