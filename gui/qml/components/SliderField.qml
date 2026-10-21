// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

// Label + live value + slider. Use setValue() to sync from outside without breaking the drag.
ColumnLayout {
    id: field
    property string label
    property string hint
    property alias from: slider.from
    property alias to: slider.to
    property alias stepSize: slider.stepSize
    property alias value: slider.value
    property alias pressed: slider.pressed
    property alias marks: slider.marks
    property alias accent: slider.accent
    property string unit
    property int decimals: 0
    property bool signed: false
    property string valueText: {
        var v = decimals > 0 ? slider.value.toFixed(decimals) : Math.round(slider.value)
        return (signed && slider.value > 0 ? "+" : "") + v + unit
    }
    signal moved()

    function setValue(v) {
        if (!slider.pressed)
            slider.value = v
    }

    spacing: 4
    Layout.fillWidth: true

    RowLayout {
        Layout.fillWidth: true
        Text {
            text: field.label
            color: Theme.text
            font.pixelSize: 14
            font.weight: Font.Medium
            Layout.fillWidth: true
        }
        Text {
            text: field.valueText
            color: field.enabled ? slider.accent : Theme.textFaint
            font.pixelSize: 14
            font.weight: Font.DemiBold
        }
    }
    AppSlider {
        id: slider
        Layout.fillWidth: true
        unit: field.unit
        decimals: field.decimals
        bubbleText: field.valueText
        onMoved: field.moved()
    }
    Text {
        visible: field.hint !== ""
        text: field.hint
        color: Theme.textFaint
        font.pixelSize: 12
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
