import QtQuick
import Quickshell
import Quickshell.Io

// RAM / CPU syspill for a Quickshell bar (dwm). Self-contained: no Omarchy imports.
// Features:
//   - Animated mini progress bar on the status bar (Kali Linux / XFCE style)
//   - Mode toggle: mem -> cpu -> both
//   - Left-click: toggle detail popup / cycle mode
//   - Right-click: launch btop monitor
//   - Middle-click: reset to "mem"
//   - Mouse wheel: cycle modes
//   - Mode persisted in ~/.config/dwm/syspill-mode and restored on startup

Item {
    id: root

    // ---- settings you may want to change ----
    property int    pollMs: 2000
    property real   cautionPercent: 75
    property real   warnPercent: 90
    property color  normalColor: "#81a1c1"   // Nord accent
    property color  cpuNormalColor: "#a3be8c" // Nord green
    property color  cautionColor: "#ebcb8b"  // Nord yellow
    property color  warnColor: "#bf616a"     // Nord red
    property color  trackColor: "#3b4252"    // Nord border
    property color  textColor: "#eceff4"     // Nord light text
    property string fontFamily: "MesloLGS Nerd Font Mono"
    property int    fontPixelSize: 12
    property var    monitorCmd: ["alacritty", "-e", "btop"]
    // ------------------------------------------

    readonly property var modes: ["mem", "cpu", "both"]
    property string mode: "mem"
    property bool popupOpen: false

    // measurements
    property real memUsedKb: 0
    property real memTotalKb: 0
    property real memPercent: 0
    property real cpuPercent: 0
    property var  prevCpu: null   // [idle, total]

    readonly property string modeFile: Quickshell.env("HOME") + "/.config/dwm/syspill-mode"

    function levelColor(p, isCpu) {
        return p >= warnPercent ? warnColor
             : p >= cautionPercent ? cautionColor
             : isCpu ? cpuNormalColor : normalColor
    }

    function gib(kb) { return (kb / 1048576).toFixed(1) + "G" }

    readonly property string memText: gib(memUsedKb)
    readonly property string cpuText: Math.round(cpuPercent) + "%"

    function setMode(m) {
        mode = m
        saveProc.command = ["sh", "-c", "mkdir -p \"$(dirname \"$1\")\" && printf %s \"$2\" > \"$1\"", "sh", modeFile, m]
        saveProc.running = true
    }

    function cycleMode() {
        setMode(modes[(modes.indexOf(mode) + 1) % modes.length])
    }

    // ---- parsing ----
    function applyMem(raw) {
        var t = 0, a = 0
        var lines = raw.split("\n")
        for (var i = 0; i < lines.length; i++) {
            var p = lines[i].split(/\s+/)
            if (p[0] === "MemTotal") t = Number(p[1])
            else if (p[0] === "MemAvailable") a = Number(p[1])
        }
        if (t <= 0) return
        memTotalKb = t
        memUsedKb = t - a
        memPercent = (t - a) / t * 100
    }

    function applyCpu(raw) {
        var f = raw.trim().split(/\s+/).slice(1).map(Number)
        if (f.length < 5 || f.some(isNaN)) return
        var idle = f[3] + f[4]
        var total = 0
        for (var i = 0; i < Math.min(f.length, 8); i++) total += f[i]
        if (prevCpu) {
            var dTotal = total - prevCpu[1]
            var dIdle = idle - prevCpu[0]
            if (dTotal > 0) cpuPercent = Math.max(0, Math.min(100, (1 - dIdle / dTotal) * 100))
        }
        prevCpu = [idle, total]
    }

    // ---- data sources ----
    Process {
        id: memProc
        command: ["awk", "/^(MemTotal|MemAvailable):/ { gsub(\":\", \"\", $1); print $1, $2 }", "/proc/meminfo"]
        stdout: StdioCollector { waitForEnd: true; onStreamFinished: root.applyMem(text) }
    }
    Process {
        id: cpuProc
        command: ["head", "-n", "1", "/proc/stat"]
        stdout: StdioCollector { waitForEnd: true; onStreamFinished: root.applyCpu(text) }
    }
    Process { id: saveProc }
    Process {
        id: loadProc
        command: ["cat", root.modeFile]
        stdout: StdioCollector {
            waitForEnd: true
            onStreamFinished: {
                var m = text.trim()
                if (root.modes.indexOf(m) >= 0) root.mode = m
            }
        }
    }
    Process { id: launchProc }

    Timer {
        interval: root.pollMs
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            if ((root.mode === "mem" || root.mode === "both") && !memProc.running) memProc.running = true
            if ((root.mode === "cpu" || root.mode === "both") && !cpuProc.running) cpuProc.running = true
        }
    }

    Component.onCompleted: loadProc.running = true

    onModeChanged: prevCpu = null

    // ---- UI: Status Bar Pill ----
    implicitHeight: 26
    implicitWidth: pillRow.implicitWidth + 16

    Rectangle {
        id: pillBg
        anchors.fill: parent
        radius: 6
        color: pillMouse.containsMouse ? "#4c566a" : "#434c5e"
        border.color: root.popupOpen ? "#81a1c1" : (pillMouse.containsMouse ? "#81a1c1" : "#3b4252")
        border.width: 1

        Behavior on color { ColorAnimation { duration: 150 } }
        Behavior on border.color { ColorAnimation { duration: 150 } }

        Row {
            id: pillRow
            anchors.centerIn: parent
            spacing: 6

            // Icon
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.mode === "cpu" ? "\udb83\udee0" : (root.mode === "both" ? "\uf0e4" : "\uefc5")
                color: root.textColor
                font.family: root.fontFamily
                font.pixelSize: Math.round(root.fontPixelSize * 1.85)
            }

            // Animated Kali / XFCE style Progress Bar
            Item {
                anchors.verticalCenter: parent.verticalCenter
                width: 48
                height: root.mode === "both" ? 14 : 7

                // Single bar (mem or cpu mode)
                Rectangle {
                    visible: root.mode !== "both"
                    anchors.centerIn: parent
                    width: parent.width
                    height: 6
                    radius: 3
                    color: root.trackColor

                    Rectangle {
                        id: animFill
                        height: parent.height
                        radius: parent.radius
                        width: parent.width * Math.min(1, (root.mode === "cpu" ? root.cpuPercent : root.memPercent) / 100)
                        color: root.levelColor(root.mode === "cpu" ? root.cpuPercent : root.memPercent, root.mode === "cpu")

                        Behavior on width {
                            NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
                        }
                        Behavior on color {
                            ColorAnimation { duration: 200 }
                        }

                        // Pulse shimmer effect
                        Rectangle {
                            visible: animFill.width > 6
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: 3
                            height: parent.height
                            radius: parent.radius
                            color: Qt.lighter(parent.color, 1.4)
                            opacity: pulseAnim.running ? 0.9 : 0.3

                            SequentialAnimation on opacity {
                                id: pulseAnim
                                loops: Animation.Infinite
                                NumberAnimation { to: 0.2; duration: 900; easing.type: Easing.InOutSine }
                                NumberAnimation { to: 1.0; duration: 900; easing.type: Easing.InOutSine }
                            }
                        }
                    }
                }

                // Dual stacked bars (both mode)
                Column {
                    visible: root.mode === "both"
                    anchors.centerIn: parent
                    width: parent.width
                    spacing: 2

                    // RAM track & fill
                    Rectangle {
                        width: parent.width
                        height: 5
                        radius: 2.5
                        color: root.trackColor

                        Rectangle {
                            height: parent.height
                            radius: parent.radius
                            width: parent.width * Math.min(1, root.memPercent / 100)
                            color: root.levelColor(root.memPercent, false)
                            Behavior on width { NumberAnimation { duration: 250 } }
                        }
                    }

                    // CPU track & fill
                    Rectangle {
                        width: parent.width
                        height: 5
                        radius: 2.5
                        color: root.trackColor

                        Rectangle {
                            height: parent.height
                            radius: parent.radius
                            width: parent.width * Math.min(1, root.cpuPercent / 100)
                            color: root.levelColor(root.cpuPercent, true)
                            Behavior on width { NumberAnimation { duration: 250 } }
                        }
                    }
                }
            }

            // Metric Text
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: root.mode === "cpu" ? root.cpuText
                    : root.mode === "both" ? (Math.round(root.memPercent) + "|" + Math.round(root.cpuPercent) + "%")
                    : root.memText
                color: root.textColor
                font.family: root.fontFamily
                font.pixelSize: root.fontPixelSize
            }
        }

        MouseArea {
            id: pillMouse
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
            onClicked: function(e) {
                if (e.button === Qt.RightButton) {
                    launchProc.command = root.monitorCmd
                    launchProc.running = true
                } else if (e.button === Qt.MiddleButton) {
                    root.setMode("mem")
                } else {
                    root.popupOpen = !root.popupOpen
                }
            }
            onWheel: function(w) {
                root.cycleMode()
                w.accepted = true
            }
        }
    }
}
