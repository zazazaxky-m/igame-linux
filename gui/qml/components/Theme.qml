// SPDX-License-Identifier: GPL-3.0-or-later
pragma Singleton
import QtQuick

QtObject {
    readonly property color bg: "#090C12"
    readonly property color bgRaised: "#0E121A"
    readonly property color sidebar: "#0B0F16"
    readonly property color surface: "#121722"
    readonly property color surface2: "#19202D"
    readonly property color surface3: "#212A3A"
    readonly property color border: "#1F2736"
    readonly property color borderStrong: "#2C374B"
    readonly property color text: "#EAF0FA"
    readonly property color textDim: "#9BA6BA"
    readonly property color textFaint: "#5E6A80"
    readonly property color accent: "#5B8CFF"
    readonly property color accentSoft: "#2A3F73"
    readonly property color teal: "#2DD4BF"
    readonly property color violet: "#A78BFA"
    readonly property color good: "#34D399"
    readonly property color warn: "#FBBF24"
    readonly property color bad: "#F87171"

    readonly property int radius: 18
    readonly property int radiusSmall: 11
    readonly property string fontFamily: "Noto Sans"

    function mix(a, b, f) {
        f = Math.max(0, Math.min(1, f))
        return Qt.rgba(a.r + (b.r - a.r) * f, a.g + (b.g - a.g) * f, a.b + (b.b - a.b) * f, a.a + (b.a - a.a) * f)
    }
    function alpha(c, a) { return Qt.rgba(c.r, c.g, c.b, a) }
    // 0..1 -> green, amber, red
    function heat(f) {
        f = Math.max(0, Math.min(1, f))
        return f < 0.5 ? mix(good, warn, f * 2) : mix(warn, bad, (f - 0.5) * 2)
    }
    function tempColor(t) { return heat((t - 45) / 40) }
    // accent from the LED color that stays readable when it's dim or off
    function glow(c) {
        var m = Math.max(c.r, c.g, c.b)
        if (m < 0.08) return accent
        return Qt.rgba(c.r / m, c.g / m, c.b / m, 1)
    }

    readonly property var icons: ({
        dashboard: "M4 16a8 8 0 1 1 16 0 M12 16l4-5 M7 19.5h10",
        bolt: "M13 2.5L4.5 13.5H11l-1 8L19.5 10H13z",
        fan: "M12 10C10.2 7.6 10.4 4.2 13 3.6C15.6 3 16.4 7 13.6 10.2 M13.73 13C16.71 12.64 19.56 14.51 18.77 17.07C17.99 19.62 14.13 18.31 12.76 14.29 M10.27 13C9.09 15.76 6.05 17.29 4.23 15.33C2.41 13.38 5.47 10.69 9.64 11.51 M10 12a2 2 0 1 0 4 0a2 2 0 1 0 -4 0",
        rgb: "M7.5 8.5a4.5 4.5 0 1 0 9 0a4.5 4.5 0 1 0 -9 0 M4 14.5a4.5 4.5 0 1 0 9 0a4.5 4.5 0 1 0 -9 0 M11 14.5a4.5 4.5 0 1 0 9 0a4.5 4.5 0 1 0 -9 0",
        profiles: "M6 3.5h12a1 1 0 0 1 1 1V21l-7-4.5L5 21V4.5a1 1 0 0 1 1-1z",
        info: "M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0 M12 11v5.5 M12 7.6v.4",
        settings: "M4 6h10 M18 6h2 M16 4v4 M4 12h4 M12 12h8 M10 10v4 M4 18h12 M20 18h0 M18 16v4",
        chip: "M3 7h18v10H3z M7 17v2.5 M11 17v2.5 M15 17v2.5 M7.5 12a2 2 0 1 0 4 0a2 2 0 1 0 -4 0 M14.5 10h3.5 M14.5 13.5h3.5",
        thermo: "M10 13.5V5a2 2 0 1 1 4 0v8.5a4 4 0 1 1-4 0z M12 9v7",
        plus: "M12 5v14 M5 12h14",
        trash: "M4 7h16 M10 11v6 M14 11v6 M6 7l1 13h10l1-13 M9 7V4h6v3",
        check: "M5 12.5l4.5 4.5L19 7.5",
        close: "M6 6l12 12 M18 6L6 18",
        power: "M12 3v8 M7.1 6.6a7 7 0 1 0 9.8 0",
        refresh: "M20 12a8 8 0 1 1-2.3-5.7 M20 4v5h-5",
        save: "M12 4v11 M7.5 10.5L12 15l4.5-4.5 M5 20h14",
        play: "M8 5v14l11-7z",
        music: "M9 17V5l10-2v12 M4 17a2.5 2.5 0 1 0 5 0a2.5 2.5 0 1 0 -5 0 M14 15a2.5 2.5 0 1 0 5 0a2.5 2.5 0 1 0 -5 0",
        memory: "M4 8h16v8H4z M7.5 8V5.5 M12 8V5.5 M16.5 8V5.5 M7.5 16v2.5 M12 16v2.5 M16.5 16v2.5",
        lock: "M6 11h12v9H6z M8.5 11V8a3.5 3.5 0 0 1 7 0v3",
        sparkle: "M12 3l1.8 5.2L19 10l-5.2 1.8L12 17l-1.8-5.2L5 10l5.2-1.8z M19 16l.7 2 2 .7-2 .7-.7 2-.7-2-2-.7 2-.7z",
        alert: "M12 4L2.5 20h19z M12 10v4.5 M12 17.2v.3",
        copy: "M9 9h11v11H9z M5 15H4V4h11v1",
        drop: "M12 3.5c3.5 4.2 6 7.3 6 10.5a6 6 0 0 1-12 0c0-3.2 2.5-6.3 6-10.5z",
        wave: "M3 12c2-4 4-4 6 0s4 4 6 0 4-4 6 0",
        heart: "M12 19.5s-7.5-4.6-7.5-10A4.3 4.3 0 0 1 12 7a4.3 4.3 0 0 1 7.5 2.5c0 5.4-7.5 10-7.5 10z",
        flame: "M12 21a6 6 0 0 0 6-6c0-4-3-6-4-10-1.5 2-2 3.5-2 5-1-1-1.5-2-1.5-3C8 9 6 11.5 6 15a6 6 0 0 0 6 6z",
        siren: "M7 17v-5a5 5 0 0 1 10 0v5 M5 17h14v3H5z M12 3v2 M4.2 6.2l1.4 1.4 M19.8 6.2l-1.4 1.4",
        star: "M12 3.5l2.5 5.3 5.8.7-4.3 4 1.1 5.7L12 16.4l-5.1 2.8 1.1-5.7-4.3-4 5.8-.7z",
        flash: "M12 2v4 M12 18v4 M4.9 4.9l2.8 2.8 M16.3 16.3l2.8 2.8 M2 12h4 M18 12h4 M4.9 19.1l2.8-2.8 M16.3 7.7l2.8-2.8",
        off: "M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0 M5.6 5.6l12.8 12.8",
        gauge: "M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0 M12 12l3.5-3.5 M12 6.5v1",
        cycle: "M4 12a8 8 0 0 1 14-5.3 M20 12a8 8 0 0 1-14 5.3 M18 3v4h-4 M6 21v-4h4",
        spectrum: "M4 18a8 8 0 0 1 16 0 M7 18a5 5 0 0 1 10 0 M10 18a2 2 0 0 1 4 0",
        aurora: "M3 15c3-6 6 2 9-3s6 2 9-4 M3 19c3-4 6 1 9-2s6 1 9-3",
        load: "M4 20V10 M9 20V4 M14 20v-7 M19 20V8",
        breath: "M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0 M8 12a4 4 0 1 0 8 0a4 4 0 1 0 -8 0",
        clock: "M3 12a9 9 0 1 0 18 0a9 9 0 1 0 -18 0 M12 7v5l3 2",
        ocean: "M3 8c2-2 4-2 6 0s4 2 6 0 4-2 6 0 M3 13c2-2 4-2 6 0s4 2 6 0 4-2 6 0 M3 18c2-2 4-2 6 0s4 2 6 0 4-2 6 0",
        glitch: "M4 7h7 M14 7h6 M4 12h3 M10 12h10 M4 17h9 M16 17h4",
        swirl: "M20 12a8 8 0 1 1-8-8 M16 12a4 4 0 1 1-4-4 M12 12h.01",
        eq: "M5 20V11 M9 20V5 M13 20v-9 M17 20V8 M21 20v-5",
        speaker: "M4 9h4l5-4v14l-5-4H4z M16.5 8.5a5 5 0 0 1 0 7 M19 6a8.5 8.5 0 0 1 0 12",
        storm: "M7 16a4 4 0 1 1 1-7.9A5.5 5.5 0 0 1 18.5 10 3.5 3.5 0 0 1 18 17 M12 13l-2 4h3l-2 4",
        external: "M14 4h6v6 M20 4l-9 9 M18 14v6H4V6h6"
    })
}
