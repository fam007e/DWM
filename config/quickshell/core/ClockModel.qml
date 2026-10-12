import QtQuick
import Quickshell
import Quickshell.Io

Scope {
    id: root

    // Only the system provider's published timezone state belongs here, never
    // a selected choice or an unverified mutation target.
    property var timezoneState: null
    property string observedTimezone: ""
    property bool initialized: false
    property string panelText: ""
    property string settingsText: ""
    property double timestamp: 0
    property bool use24Hour: true
    property bool calendarVisible: false

    readonly property string configuredConfigHome: Quickshell.env("XDG_CONFIG_HOME")
    readonly property string homeDir: Quickshell.env("HOME") || ""
    readonly property string configHome: root.configuredConfigHome && root.configuredConfigHome.startsWith("/")
        ? root.configuredConfigHome : root.homeDir + "/.config"
    readonly property string configPath: root.configHome + "/dwm/clock-format.conf"

    function refreshDisplay() {
        if (!root.initialized) return;
        const current = new Date(root.timestamp);
        if (root.use24Hour) {
            root.panelText = Qt.formatDateTime(current, "ddd dd MMM - HH:mm");
            root.settingsText = Qt.formatDateTime(current, "dddd, dd MMMM yyyy - HH:mm t");
        } else {
            root.panelText = Qt.formatDateTime(current, "ddd dd MMM - hh:mm AP");
            root.settingsText = Qt.formatDateTime(current, "dddd, dd MMMM yyyy - hh:mm AP");
        }
    }

    function refreshTimezone() {
        const state = root.timezoneState;
        if (!root.initialized || !state || state.status !== "available"
                || typeof state.value !== "string" || state.value.length === 0
                || state.value === "unknown" || state.value === root.observedTimezone) return;
        root.observedTimezone = state.value;
        Date.timeZoneUpdated();
        root.refreshDisplay();
    }

    function toggleTimeFormat() {
        root.use24Hour = !root.use24Hour;
        root.refreshDisplay();
        saveFormatProcess.running = true;
    }

    function toggleCalendar() {
        root.calendarVisible = !root.calendarVisible;
    }

    function closeCalendar() {
        root.calendarVisible = false;
    }

    function openCalendar() {
        root.calendarVisible = true;
    }

    onTimezoneStateChanged: root.refreshTimezone()
    Component.onCompleted: {
        root.timestamp = source.date.getTime();
        root.initialized = true;
        loadFormatProcess.running = true;
        root.refreshTimezone();
        root.refreshDisplay();
    }

    SystemClock {
        id: source
        precision: SystemClock.Minutes
        onDateChanged: {
            if (root.initialized) {
                root.timestamp = source.date.getTime();
                root.refreshDisplay();
            }
        }
    }

    // Load persisted clock format preference
    Process {
        id: loadFormatProcess
        command: ["sh", "-c", "cat \"" + root.configPath + "\" 2>/dev/null || echo '24'"]
        running: false

        stdout: StdioCollector {
            onStreamFinished: {
                const val = this.text.trim();
                root.use24Hour = val !== "12";
                root.refreshDisplay();
            }
        }
    }

    // Save clock format preference
    Process {
        id: saveFormatProcess
        command: ["sh", "-c",
            "mkdir -p \"$(dirname '" + root.configPath + "')\" && printf '%s\\n' '"
            + (root.use24Hour ? "24" : "12") + "' > '" + root.configPath + "'"]
        running: false
    }
}
