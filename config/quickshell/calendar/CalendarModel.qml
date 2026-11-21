import QtQuick
import Quickshell
import Quickshell.Io
import qs.core

Scope {
    id: root

    property int viewYear: new Date().getFullYear()
    property int viewMonth: new Date().getMonth()  // 0-indexed
    property int selectedDay: new Date().getDate()
    property var events: []
    property bool eventsBusy: false

    // Days since epoch for a local calendar date (month is 0-indexed).
    // Matches the helper's startDay/endDay numbering.
    function epochDayOf(year, month, day) {
        return Math.floor(Date.UTC(year, month, day) / 86400000);
    }

    // Index over every date an event occupies: epochDay -> [events].
    // Covers multi-day events (startDay..endDay inclusive) so mid-month
    // lookups are O(1) instead of rescanning the event list per cell.
    readonly property var eventIndex: {
        const list = root.events;
        const index = ({});
        for (let i = 0; i < list.length; i++) {
            const ev = list[i];
            const start = ev.startDay;
            if (start === null || start === undefined) continue;
            // Guard against pathological feed ranges (cap at ~1 year).
            const end = (ev.endDay !== null && ev.endDay !== undefined)
                ? Math.min(ev.endDay, start + 366) : start;
            for (let d = start; d <= end; d++) {
                if (!index[d]) index[d] = [];
                index[d].push(ev);
            }
        }
        return index;
    }

    // Grid: array of { day: int, inMonth: bool, isToday: bool, isSelected: bool, hasEvent: bool }
    readonly property var calendarGrid: root.buildGrid(root.viewYear, root.viewMonth, root.selectedDay)

    readonly property string monthLabel: {
        const months = ["January", "February", "March", "April", "May", "June",
                        "July", "August", "September", "October", "November", "December"];
        return months[root.viewMonth] + " " + root.viewYear;
    }

    // Selected date formatted string for display
    readonly property string selectedDateLabel: {
        const pad = (n) => n < 10 ? "0" + n : "" + n;
        return pad(root.selectedDay) + "/" + pad(root.viewMonth + 1) + "/" + root.viewYear;
    }

    // All events occupying the selected day (including multi-day events that
    // started earlier), or empty if none.
    readonly property var eventsForSelectedDay: {
        return root.eventIndex[root.epochDayOf(root.viewYear, root.viewMonth, root.selectedDay)] || [];
    }

    // Next five not-yet-finished events, used when the selected day is empty.
    readonly property var upcomingEvents: {
        const list = root.events;
        const now = new Date();
        const today = root.epochDayOf(now.getFullYear(), now.getMonth(), now.getDate());
        const seen = new Set();
        const out = [];
        for (let i = 0; i < list.length && out.length < 5; i++) {
            const ev = list[i];
            const endDay = (ev.endDay !== null && ev.endDay !== undefined) ? ev.endDay : ev.startDay;
            if (endDay !== null && endDay !== undefined && endDay < today) continue;
            const key = ev.uid + "|" + ev.startDate + "|" + ev.startTime;
            if (seen.has(key)) continue;
            seen.add(key);
            out.push(ev);
        }
        return out;
    }

    function buildGrid(year, month, selectedD) {
        const firstDay = new Date(year, month, 1);
        let startWeekday = firstDay.getDay() - 1;
        if (startWeekday < 0) startWeekday = 6;
        const daysInMonth = new Date(year, month + 1, 0).getDate();
        const daysInPrevMonth = new Date(year, month, 0).getDate();
        const today = new Date();
        const isCurrentMonth = today.getFullYear() === year && today.getMonth() === month;
        const todayDate = today.getDate();

        const grid = [];

        // Previous month trailing days
        for (let i = startWeekday - 1; i >= 0; i--) {
            grid.push({
                day: daysInPrevMonth - i,
                inMonth: false,
                isToday: false,
                isSelected: false,
                hasEvent: false
            });
        }

        // Current month days
        for (let d = 1; d <= daysInMonth; d++) {
            grid.push({
                day: d,
                inMonth: true,
                isToday: isCurrentMonth && d === todayDate,
                isSelected: d === selectedD,
                hasEvent: root.eventIndex[root.epochDayOf(year, month, d)] !== undefined
            });
        }

        // Fill remaining cells to complete 6 rows (42 cells)
        const remaining = 42 - grid.length;
        for (let d = 1; d <= remaining; d++) {
            grid.push({
                day: d,
                inMonth: false,
                isToday: false,
                isSelected: false,
                hasEvent: false
            });
        }

        return grid;
    }

    function selectDay(day) {
        root.selectedDay = day;
    }

    function previousMonth() {
        if (root.viewMonth === 0) {
            root.viewMonth = 11;
            root.viewYear--;
        } else {
            root.viewMonth--;
        }
        root.selectedDay = 1;
        root.fetchEvents();
    }

    function nextMonth() {
        if (root.viewMonth === 11) {
            root.viewMonth = 0;
            root.viewYear++;
        } else {
            root.viewMonth++;
        }
        root.selectedDay = 1;
        root.fetchEvents();
    }

    function goToToday() {
        const now = new Date();
        root.viewYear = now.getFullYear();
        root.viewMonth = now.getMonth();
        root.selectedDay = now.getDate();
        root.fetchEvents();
    }

    function fetchEvents() {
        if (eventProcess.running) return;
        root.eventsBusy = true;
        eventProcess.command = Commands.calendarCommand("events", [
            String(root.viewMonth + 1),
            String(root.viewYear)
        ]);
        eventProcess.running = true;
    }

    function launchInteractive() {
        interactiveProcess.command = Commands.calendarCommand("interactive", []);
        interactiveProcess.running = true;
    }

    function parseEvents(raw) {
        try {
            const data = JSON.parse(raw.trim());
            root.events = Array.isArray(data) ? data : [];
        } catch (e) {
            root.events = [];
        }
        root.eventsBusy = false;
    }

    Component.onCompleted: root.fetchEvents()

    Process {
        id: eventProcess
        running: false
        command: ["true"]

        stdout: StdioCollector {
            onStreamFinished: root.parseEvents(this.text)
        }

        // Reset the busy state even when khal fails, so the popup can never
        // stay stuck on "Loading khal events...".
        onExited: (exitCode, exitStatus) => { // qmllint disable signal-handler-parameters
            root.eventsBusy = false;
        }
    }

    Process {
        id: interactiveProcess
        running: false
        command: ["true"]
    }
}
