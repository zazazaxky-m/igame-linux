// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

// Interactive fan curve: drag points, double-click empty space to add, right/double-click a point to remove.
Item {
    id: ed
    property var points: []
    property real currentTemp: -1
    property real tMin: 20
    property real tMax: 100
    property real pMin: 30
    property real zeroRpmTemp: -1
    property color accent: Theme.accent
    signal edited(var points)

    property var pts: []
    property int dragging: -1

    readonly property real padLeft: 46
    readonly property real padRight: 16
    readonly property real padTop: 16
    readonly property real padBottom: 30
    readonly property real plotW: Math.max(1, width - padLeft - padRight)
    readonly property real plotH: Math.max(1, height - padTop - padBottom)

    function xOf(t) { return padLeft + (t - tMin) / (tMax - tMin) * plotW }
    function yOf(p) { return padTop + (1 - p / 100) * plotH }
    function tOf(x) { return tMin + (x - padLeft) / plotW * (tMax - tMin) }
    function pOf(y) { return (1 - (y - padTop) / plotH) * 100 }
    function clamp(v, lo, hi) { return Math.max(lo, Math.min(hi, v)) }
    function copy(list) { return list.map(function (p) { return [p[0], p[1]] }) }
    function interp(t) {
        if (!pts.length) return 0
        if (t <= pts[0][0]) return pts[0][1]
        for (var i = 1; i < pts.length; ++i) {
            if (t <= pts[i][0]) {
                var a = pts[i - 1], b = pts[i]
                return a[1] + (b[1] - a[1]) * (t - a[0]) / (b[0] - a[0])
            }
        }
        return pts[pts.length - 1][1]
    }
    function movePoint(i, t, p) {
        var list = copy(pts)
        var lo = i > 0 ? list[i - 1][0] + 1 : tMin
        var hi = i < list.length - 1 ? list[i + 1][0] - 1 : tMax
        list[i] = [Math.round(clamp(t, lo, hi)), Math.round(clamp(p, pMin, 100))]
        pts = list
        edited(copy(list))
    }
    function addPoint(t, p) {
        if (pts.length >= 12) return
        t = Math.round(clamp(t, tMin, tMax))
        p = Math.round(clamp(p, pMin, 100))
        for (var i = 0; i < pts.length; ++i)
            if (pts[i][0] === t) return
        var list = copy(pts)
        list.push([t, p])
        list.sort(function (a, b) { return a[0] - b[0] })
        pts = list
        edited(copy(list))
    }
    function removePoint(i) {
        if (pts.length <= 2) return
        var list = copy(pts)
        list.splice(i, 1)
        pts = list
        edited(copy(list))
    }

    onPointsChanged: if (dragging < 0) pts = copy(points || [])
    onPtsChanged: canvas.requestPaint()
    onCurrentTempChanged: if (visible) canvas.requestPaint()
    onVisibleChanged: if (visible) canvas.requestPaint()
    onZeroRpmTempChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()
    Component.onCompleted: pts = copy(points || [])

    Canvas {
        id: canvas
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative
        onPaint: {
            var ctx = getContext("2d")
            ctx.reset()
            var L = ed.padLeft, T = ed.padTop, W = ed.plotW, H = ed.plotH
            ctx.font = "11px sans-serif"

            if (ed.zeroRpmTemp > ed.tMin) {
                var zx = ed.xOf(Math.min(ed.zeroRpmTemp, ed.tMax))
                ctx.fillStyle = Qt.rgba(0.2, 0.83, 0.6, 0.07)
                ctx.fillRect(L, T, zx - L, H)
                ctx.fillStyle = Qt.rgba(0.2, 0.83, 0.6, 0.65)
                ctx.fillText("0 RPM", L + 8, T + 16)
            }
            if (ed.pMin > 0) {
                ctx.fillStyle = Qt.rgba(1, 1, 1, 0.025)
                ctx.fillRect(L, ed.yOf(ed.pMin), W, T + H - ed.yOf(ed.pMin))
            }

            ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.06)
            ctx.lineWidth = 1
            ctx.fillStyle = Qt.rgba(0.6, 0.65, 0.73, 0.8)
            for (var t = ed.tMin; t <= ed.tMax; t += 10) {
                var gx = Math.round(ed.xOf(t)) + 0.5
                ctx.beginPath(); ctx.moveTo(gx, T); ctx.lineTo(gx, T + H); ctx.stroke()
                ctx.fillText(t + "°", gx - 8, T + H + 18)
            }
            for (var p = 0; p <= 100; p += 20) {
                var gy = Math.round(ed.yOf(p)) + 0.5
                ctx.beginPath(); ctx.moveTo(L, gy); ctx.lineTo(L + W, gy); ctx.stroke()
                ctx.fillText(p + "%", 6, gy + 4)
            }

            var pts = ed.pts
            if (pts.length < 2)
                return
            var c = ed.accent
            ctx.beginPath()
            ctx.moveTo(L, ed.yOf(pts[0][1]))
            for (var i = 0; i < pts.length; ++i)
                ctx.lineTo(ed.xOf(pts[i][0]), ed.yOf(pts[i][1]))
            ctx.lineTo(L + W, ed.yOf(pts[pts.length - 1][1]))
            ctx.strokeStyle = c
            ctx.lineWidth = 2.5
            ctx.lineJoin = "round"
            ctx.stroke()
            ctx.lineTo(L + W, T + H)
            ctx.lineTo(L, T + H)
            ctx.closePath()
            var grad = ctx.createLinearGradient(0, T, 0, T + H)
            grad.addColorStop(0, Qt.rgba(c.r, c.g, c.b, 0.28))
            grad.addColorStop(1, Qt.rgba(c.r, c.g, c.b, 0.02))
            ctx.fillStyle = grad
            ctx.fill()

            if (ed.currentTemp >= ed.tMin && ed.currentTemp <= ed.tMax) {
                var cx = ed.xOf(ed.currentTemp)
                var cy = ed.yOf(Math.max(ed.pMin, ed.interp(ed.currentTemp)))
                ctx.strokeStyle = Qt.rgba(0.98, 0.75, 0.14, 0.7)
                ctx.setLineDash([4, 4])
                ctx.beginPath(); ctx.moveTo(cx, T); ctx.lineTo(cx, T + H); ctx.stroke()
                ctx.setLineDash([])
                ctx.fillStyle = "#FBBF24"
                ctx.beginPath(); ctx.arc(cx, cy, 5, 0, Math.PI * 2); ctx.fill()
                ctx.fillText(Math.round(ed.currentTemp) + "°C", Math.min(cx + 6, L + W - 34), T + 12)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onDoubleClicked: (mouse) => {
            if (mouse.x >= ed.padLeft && mouse.y <= ed.padTop + ed.plotH)
                ed.addPoint(ed.tOf(mouse.x), ed.pOf(mouse.y))
        }
    }

    Repeater {
        model: ed.pts.length
        Rectangle {
            id: handle
            readonly property var pt: ed.pts[index] || [0, 0]
            readonly property bool active: ed.dragging === index || handleArea.containsMouse
            x: ed.xOf(pt[0]) - width / 2
            y: ed.yOf(pt[1]) - height / 2
            width: active ? 18 : 15
            height: width
            radius: width / 2
            color: active ? "#FFFFFF" : ed.accent
            border.color: "#FFFFFF"
            border.width: 2
            Behavior on width { NumberAnimation { duration: 100 } }

            Rectangle {
                visible: handle.active
                anchors.bottom: parent.top
                anchors.bottomMargin: 8
                anchors.horizontalCenter: parent.horizontalCenter
                width: tip.implicitWidth + 14
                height: 24
                radius: 7
                color: Theme.surface3
                border.color: Theme.borderStrong
                Text {
                    id: tip
                    anchors.centerIn: parent
                    text: handle.pt[0] + "°C · " + handle.pt[1] + "%"
                    color: Theme.text
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }
            }

            MouseArea {
                id: handleArea
                anchors.fill: parent
                anchors.margins: -7
                hoverEnabled: true
                preventStealing: true
                cursorShape: Qt.SizeAllCursor
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onPressed: (mouse) => {
                    if (mouse.button === Qt.RightButton) {
                        ed.removePoint(index)
                        return
                    }
                    ed.dragging = index
                }
                onReleased: ed.dragging = -1
                onCanceled: ed.dragging = -1
                onDoubleClicked: ed.removePoint(index)
                onPositionChanged: (mouse) => {
                    if (ed.dragging !== index)
                        return
                    var p = mapToItem(ed, mouse.x, mouse.y)
                    ed.movePoint(index, ed.tOf(p.x), ed.pOf(p.y))
                }
            }
        }
    }
}
