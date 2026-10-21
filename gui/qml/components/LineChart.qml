// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

// area chart, newest sample on the right
Canvas {
    id: chart
    property var values: []
    property real minValue: 0
    property real maxValue: 100
    property bool autoScale: false
    property color color: Theme.accent
    property int capacity: 120

    renderStrategy: Canvas.Cooperative
    onValuesChanged: if (visible) requestPaint()
    onColorChanged: if (visible) requestPaint()
    onWidthChanged: if (visible) requestPaint()
    onHeightChanged: if (visible) requestPaint()
    onVisibleChanged: if (visible) requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        var w = width, h = height
        ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.045)
        ctx.lineWidth = 1
        for (var i = 1; i < 4; ++i) {
            var gy = Math.round(h * i / 4) + 0.5
            ctx.beginPath(); ctx.moveTo(0, gy); ctx.lineTo(w, gy); ctx.stroke()
        }
        var n = values ? values.length : 0
        if (n < 2)
            return
        var lo = minValue, hi = maxValue
        if (autoScale) {
            lo = Math.min.apply(null, values); hi = Math.max.apply(null, values)
            var pad = Math.max(1, (hi - lo) * 0.15)
            lo -= pad; hi += pad
        }
        if (hi <= lo) hi = lo + 1
        var step = w / (capacity - 1)
        var x0 = w - (n - 1) * step
        function yOf(v) { return h - 3 - (Math.max(lo, Math.min(hi, v)) - lo) / (hi - lo) * (h - 6) }

        ctx.beginPath()
        ctx.moveTo(x0, yOf(values[0]))
        for (var k = 1; k < n; ++k) {
            var px = x0 + (k - 1) * step, py = yOf(values[k - 1])
            var cx = x0 + k * step, cy = yOf(values[k])
            ctx.quadraticCurveTo(px, py, (px + cx) / 2, (py + cy) / 2)
        }
        ctx.lineTo(x0 + (n - 1) * step, yOf(values[n - 1]))
        ctx.strokeStyle = color
        ctx.lineWidth = 2
        ctx.lineJoin = "round"
        ctx.stroke()

        ctx.lineTo(w, h)
        ctx.lineTo(x0, h)
        ctx.closePath()
        var grad = ctx.createLinearGradient(0, 0, 0, h)
        grad.addColorStop(0, Qt.rgba(color.r, color.g, color.b, 0.32))
        grad.addColorStop(1, Qt.rgba(color.r, color.g, color.b, 0.0))
        ctx.fillStyle = grad
        ctx.fill()

        var lx = x0 + (n - 1) * step, ly = yOf(values[n - 1])
        ctx.beginPath()
        ctx.arc(lx - 3, ly, 3.5, 0, Math.PI * 2)
        ctx.fillStyle = color
        ctx.fill()
    }
}
