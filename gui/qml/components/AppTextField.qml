// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls

TextField {
    id: tf
    implicitHeight: 38
    color: Theme.text
    placeholderTextColor: Theme.textFaint
    selectionColor: Theme.accent
    selectedTextColor: "#FFFFFF"
    font.pixelSize: 14
    leftPadding: 12
    rightPadding: 12
    background: Rectangle {
        radius: Theme.radiusSmall
        color: Theme.bgRaised
        border.color: tf.activeFocus ? Theme.accent : Theme.borderStrong
        border.width: tf.activeFocus ? 2 : 1
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }
}
