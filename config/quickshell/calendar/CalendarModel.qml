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

    // Calendar grid: array of { day: int, inMonth: bool, isToday: bool, isSelected: bool, hasEvent: bool }
    readonly property var calendarGrid: {
        const _ = root.events;
        const sel = root.selectedDay;
        return root.buildGrid(root.viewYear, root.viewMonth, sel);
    }

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

    // Events filtered for the currently selected day, or all events if none on that day
    readonly property var eventsForSelectedDay: {
        const pad = (n) => n < 10 ? "0" + n : "" + n;
        const dateStr = pad(root.selectedDay) + "/" + pad(root.viewMonth + 1) + "/" + root.viewYear;
        const matched = [];
        for (let i = 0; i < root.events.length; i++) {
            if (root.events[i].startDate === dateStr) {
                matched.push(root.events[i]);
            }
        }
        return matched;
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
                hasEvent: root.dayHasEvent(d)
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

    function dayHasEvent(day) {
        const pad = (n) => n < 10 ? "0" + n : "" + n;
        const dateStr = pad(day) + "/" + pad(root.viewMonth + 1) + "/" + root.viewYear;
        for (let i = 0; i < root.events.length; i++) {
            if (root.events[i].startDate === dateStr) return true;
        }
        return false;
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
    }

    Process {
        id: interactiveProcess
        running: false
        command: ["true"]
    }
}
