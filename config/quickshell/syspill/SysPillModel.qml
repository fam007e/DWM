import QtQuick
import Quickshell
import Quickshell.Io

// Backend model for the SysPill system monitor.
// Polls /proc/meminfo and /proc/stat, tracks mode, persists choice to disk.
//
//   Modes: "mem" → "cpu" → "both"  (cycled by left-click)
//   Right-click opens btop; middle-click resets to "mem".

Scope {
    id: root

    // ---- tunables ----
    property int    pollMs: 2000
    property real   cautionPercent: 75
    property real   warnPercent: 90
    property var    monitorCmd: ["alacritty", "-e", "btop"]

    // ---- state ----
    readonly property var modes: ["mem", "cpu", "both"]
    property string mode: "mem"
    property bool   visible: false

    // measurements
    property real memUsedKb: 0
    property real memTotalKb: 0
    property real memPercent: 0
    property real cpuPercent: 0
    property var  prevCpu: null   // [idle, total]
    property var  processes: []
    property string processError: ""

    readonly property string homeDir: Quickshell.env("HOME") || ""
    readonly property string modeFile: homeDir + "/.config/dwm/syspill-mode"

    // ---- helpers ----
    function gib(kb) { return (kb / 1048576).toFixed(1) + "G" }

    readonly property string memLabel: "RAM"
    readonly property string cpuLabel: "CPU"
    readonly property string memIcon: "\uefc5"
    readonly property string cpuIcon: "\udb83\udee0"

    readonly property string memText: memIcon + " " + gib(memUsedKb) + " / " + gib(memTotalKb)
    readonly property string memShort: memIcon + " " + gib(memUsedKb)
    readonly property string cpuText: cpuIcon + " " + Math.round(cpuPercent) + "%"
    readonly property string cpuShort: cpuIcon + " " + Math.round(cpuPercent) + "%"

    readonly property string tooltipText: {
        if (mode === "mem")  return "RAM: " + gib(memUsedKb) + " / " + gib(memTotalKb)
                                     + " (" + Math.round(memPercent) + "%)";
        if (mode === "cpu")  return "CPU: " + Math.round(cpuPercent) + "%";
        return "RAM: " + gib(memUsedKb) + " / " + gib(memTotalKb)
               + " (" + Math.round(memPercent) + "%)"
               + "  ·  CPU: " + Math.round(cpuPercent) + "%";
    }

    // ---- mode management ----
    function setMode(m) {
        mode = m;
        saveProc.command = ["sh", "-c",
            "mkdir -p \"$(dirname \"$1\")\" && printf %s \"$2\" > \"$1\"",
            "sh", modeFile, m];
        saveProc.running = true;
    }

    function cycleMode() {
        setMode(modes[(modes.indexOf(mode) + 1) % modes.length]);
    }

    function open()   { visible = true  }
    function close()  { visible = false }
    function toggle() { visible = !visible }

    function launchMonitor() {
        launchProc.command = monitorCmd;
        launchProc.running = true;
    }

    // ---- parsing ----
    function applyMem(raw) {
        var t = 0, a = 0;
        var lines = raw.split("\n");
        for (var i = 0; i < lines.length; i++) {
            var p = lines[i].split(/\s+/);
            if (p[0] === "MemTotal")     t = Number(p[1]);
            else if (p[0] === "MemAvailable") a = Number(p[1]);
        }
        if (t <= 0) return;
        memTotalKb = t;
        memUsedKb  = t - a;
        memPercent = (t - a) / t * 100;
    }

    function applyCpu(raw) {
        var f = raw.trim().split(/\s+/).slice(1).map(Number);
        if (f.length < 5 || f.some(isNaN)) return;
        var idle  = f[3] + f[4];
        var total = 0;
        for (var i = 0; i < Math.min(f.length, 8); i++) total += f[i];
        if (prevCpu) {
            var dTotal = total - prevCpu[1];
            var dIdle  = idle  - prevCpu[0];
            if (dTotal > 0)
                cpuPercent = Math.max(0, Math.min(100, (1 - dIdle / dTotal) * 100));
        }
        prevCpu = [idle, total];
    }

    function refreshProcesses() {
        if (processProc.running) return;
        const sortKey = root.mode === "mem" ? "--sort=-rss" : "--sort=-pcpu";
        processProc.command = ["env", "LC_ALL=C", "ps", "-eo", "pid=,comm=,pcpu=,rss=", sortKey];
        processProc.running = true;
    }

    function parseProcesses(raw) {
        const rows = [];
        const lines = raw.trim().split("\n");
        for (const line of lines) {
            const fields = line.trim().split(/\s+/);
            if (fields.length < 4) continue;
            const pid = Number(fields[0]);
            const cpuPercent = Number(fields[fields.length - 2]);
            const rssKb = Number(fields[fields.length - 1]);
            const name = fields.slice(1, -2).join(" ");
            if (!Number.isInteger(pid) || !isFinite(cpuPercent) || !isFinite(rssKb) || name.length === 0)
                continue;
            rows.push({ pid: pid, name: name, cpuPercent: cpuPercent, rssKb: rssKb });
            if (rows.length === 12) break;
        }
        root.processes = rows;
        root.processError = "";
    }

    // ---- data sources ----
    Process {
        id: memProc
        command: ["awk",
            "/^(MemTotal|MemAvailable):/ { gsub(\":\", \"\", $1); print $1, $2 }",
            "/proc/meminfo"]
        stdout: StdioCollector { waitForEnd: true; onStreamFinished: root.applyMem(text) }
    }
    Process {
        id: cpuProc
        command: ["head", "-n", "1", "/proc/stat"]
        stdout: StdioCollector { waitForEnd: true; onStreamFinished: root.applyCpu(text) }
    }
    Process {
        id: processProc
        running: false
        stdout: StdioCollector {
            waitForEnd: true
            onStreamFinished: root.parseProcesses(text)
        }
        stderr: StdioCollector {
            waitForEnd: true
            onStreamFinished: {
                const error = text.trim();
                if (error.length > 0) root.processError = error;
            }
        }
        onExited: (exitCode, exitStatus) => { // qmllint disable signal-handler-parameters
            if (exitCode !== 0 || exitStatus !== 0) {
                root.processes = [];
                if (root.processError.length === 0)
                    root.processError = "Could not read process usage";
            }
        }
    }
    Process { id: saveProc }
    Process {
        id: loadProc
        command: ["cat", root.modeFile]
        stdout: StdioCollector {
            waitForEnd: true
            onStreamFinished: {
                var m = text.trim();
                if (root.modes.indexOf(m) >= 0) root.mode = m;
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
            if ((root.mode === "mem" || root.mode === "both") && !memProc.running)
                memProc.running = true;
            if ((root.mode === "cpu" || root.mode === "both") && !cpuProc.running)
                cpuProc.running = true;
            root.refreshProcesses();
        }
    }

    Component.onCompleted: loadProc.running = true

    onModeChanged: {
        prevCpu = null;
        refreshProcesses();
    }
}
