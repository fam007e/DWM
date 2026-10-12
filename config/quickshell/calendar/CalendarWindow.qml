import QtQuick
import QtQuick.Layouts
import qs.core

ClickAwayPopup {
    id: root

    required property var clockModel
    required property var calendarModel
    required property var panelWindow

    readonly property int cardWidth: 360
    readonly property int edgeMargin: Theme.rowSpacing
    readonly property var dayHeaders: ["Mo", "Tu", "We", "Th", "Fr", "Sa", "Su"]

    function formatTime(timeStr, use24h) {
        if (!timeStr) return "";
        if (use24h) return timeStr;
        const parts = timeStr.split(":");
        if (parts.length < 2) return timeStr;
        let h = parseInt(parts[0], 10);
        const m = parts[1];
        const ampm = h >= 12 ? "PM" : "AM";
        h = h % 12 || 12;
        return h + ":" + m + " " + ampm;
    }

    readonly property var displayEvents: {
        const forDay = root.calendarModel.eventsForSelectedDay;
        if (forDay && forDay.length > 0) return forDay;
        const all = root.calendarModel.events;
        return all ? all.slice(0, 5) : [];
    }

    readonly property bool showingSelectedDayOnly: {
        const forDay = root.calendarModel.eventsForSelectedDay;
        return forDay && forDay.length > 0;
    }

    visible: panelWindow !== null && panelWindow.screen !== null && clockModel.calendarVisible
    targetWindow: panelWindow
    popupWidth: cardWidth
    popupHeight: calendarCard.implicitHeight
    popupX: panelWindow ? Math.max(edgeMargin, (panelWindow.width / 2) - (cardWidth / 2)) : edgeMargin
    popupY: Theme.panelHeight
    onDismissed: clockModel.closeCalendar()
    onVisibleChanged: {
        if (visible) {
            calendarModel.goToToday();
        }
    }

    ShellSurface {
        id: calendarCard

        anchors.fill: parent
        implicitHeight: contentColumn.implicitHeight + Theme.cardPadding * 2
        margin: 6

        ColumnLayout {
            id: contentColumn

            anchors.fill: parent
            anchors.margins: Theme.cardPadding
            spacing: Theme.spacingMd

            // ─── Header: Time, Date and 12h/24h toggle ───
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    UiText {
                        text: Qt.formatDateTime(new Date(root.clockModel.timestamp),
                            root.clockModel.use24Hour ? "HH:mm" : "hh:mm AP")
                        color: Theme.textStrong
                        font.bold: true
                        font.pixelSize: Theme.scaledFontSize(24, 10)
                    }

                    UiText {
                        text: Qt.formatDateTime(new Date(root.clockModel.timestamp), "dddd, dd MMMM yyyy")
                        color: Theme.textMuted
                        font.pixelSize: Theme.tinyFontSize
                        elide: Text.ElideRight
                    }
                }

                ShellButton {
                    label: root.clockModel.use24Hour ? "24H" : "12H"
                    leadingLabel: "󰥔"
                    accessibleDescription: "Toggle 12/24 hour time format"
                    onActivated: root.clockModel.toggleTimeFormat()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            // ─── Month navigation ───
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                ShellButton {
                    label: ""
                    leadingLabel: "󰅁"
                    accessibleDescription: "Previous month"
                    onActivated: root.calendarModel.previousMonth()
                }

                UiText {
                    Layout.fillWidth: true
                    text: root.calendarModel.monthLabel
                    color: Theme.textStrong
                    font.bold: true
                    font.pixelSize: Theme.panelFontSize
                    horizontalAlignment: Text.AlignHCenter
                }

                ShellButton {
                    label: "Today"
                    leadingLabel: "󰸗"
                    accessibleDescription: "Go to today"
                    onActivated: root.calendarModel.goToToday()
                }

                ShellButton {
                    label: ""
                    leadingLabel: "󰅂"
                    accessibleDescription: "Next month"
                    onActivated: root.calendarModel.nextMonth()
                }
            }

            // ─── Day-of-week headers ───
            GridLayout {
                Layout.fillWidth: true
                columns: 7
                rowSpacing: 0
                columnSpacing: 0

                Repeater {
                    model: root.dayHeaders

                    delegate: Item {
                        required property string modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 22

                        UiText {
                            anchors.centerIn: parent
                            text: modelData
                            color: Theme.textMuted
                            font.pixelSize: Theme.tinyFontSize
                            font.bold: true
                        }
                    }
                }
            }

            // ─── Calendar grid (6 rows × 7 cols) ───
            GridLayout {
                Layout.fillWidth: true
                columns: 7
                rowSpacing: 2
                columnSpacing: 2

                Repeater {
                    model: root.calendarModel.calendarGrid

                    delegate: Rectangle {
                        required property var modelData
                        required property int index

                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        color: modelData.isToday ? Theme.accent
                            : modelData.isSelected && modelData.inMonth ? Theme.surfaceActive
                            : dayMouse.containsMouse ? Theme.surfaceHover : Theme.transparent
                        border.width: modelData.isSelected && !modelData.isToday && modelData.inMonth ? 1 : 0
                        border.color: Theme.borderStrong
                        radius: Theme.controlRadius

                        UiText {
                            anchors.centerIn: parent
                            text: modelData.day
                            color: modelData.isToday ? Theme.accentText
                                : modelData.isSelected && modelData.inMonth ? Theme.accentSecondary
                                : modelData.inMonth ? Theme.textStrong : Theme.textMuted
                            font.bold: modelData.isToday || (modelData.isSelected && modelData.inMonth)
                            font.pixelSize: Theme.fontCaptionSize
                        }

                        // Event dot indicator
                        Rectangle {
                            width: 4
                            height: 4
                            radius: 2
                            color: modelData.isToday ? Theme.accentText : Theme.accent
                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 2
                            visible: modelData.hasEvent && modelData.inMonth
                        }

                        MouseArea {
                            id: dayMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (modelData.inMonth) {
                                    root.calendarModel.selectDay(modelData.day);
                                }
                            }
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.border
            }

            // ─── Khal Agenda Section Header ───
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                IconText {
                    text: "󰃰"
                    color: Theme.accent
                    font.pixelSize: Theme.scaledFontSize(14, 8)
                }

                UiText {
                    Layout.fillWidth: true
                    text: root.showingSelectedDayOnly
                        ? "Events (" + root.calendarModel.selectedDateLabel + ")"
                        : "Upcoming Events"
                    color: Theme.textStrong
                    font.bold: true
                    font.pixelSize: Theme.fontCaptionSize
                }

                UiText {
                    text: root.displayEvents.length > 0 ? String(root.displayEvents.length) : ""
                    color: Theme.accent
                    font.pixelSize: Theme.tinyFontSize
                    font.bold: true
                    visible: text !== ""
                }

                ShellButton {
                    label: ""
                    leadingLabel: "󰑓"
                    accessibleDescription: "Refresh khal events"
                    onActivated: root.calendarModel.fetchEvents()
                }
            }

            // Event list
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                visible: root.displayEvents.length > 0

                Repeater {
                    model: root.displayEvents

                    delegate: Rectangle {
                        required property var modelData
                        required property int index

                        Layout.fillWidth: true
                        Layout.preferredHeight: eventRow.implicitHeight + 10
                        color: Theme.surface
                        radius: Theme.controlRadius

                        RowLayout {
                            id: eventRow
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: Theme.spacingSm

                            Rectangle {
                                width: 3
                                Layout.fillHeight: true
                                color: Theme.accent
                                radius: 2
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 4

                                    UiText {
                                        Layout.fillWidth: true
                                        text: modelData.title || "Untitled"
                                        color: Theme.textStrong
                                        font.pixelSize: Theme.fontCaptionSize
                                        font.bold: true
                                        elide: Text.ElideRight
                                    }

                                    Rectangle {
                                        color: Theme.surfaceHover
                                        radius: 3
                                        Layout.preferredHeight: 16
                                        Layout.preferredWidth: calTag.implicitWidth + 8
                                        visible: modelData.calendar !== ""

                                        UiText {
                                            id: calTag
                                            anchors.centerIn: parent
                                            text: modelData.calendar
                                            color: Theme.accent
                                            font.pixelSize: Theme.tinyFontSize
                                        }
                                    }
                                }

                                RowLayout {
                                    spacing: Theme.spacingSm

                                    UiText {
                                        text: modelData.allDay ? "All day"
                                            : root.formatTime(modelData.startTime, root.clockModel.use24Hour)
                                                + (modelData.endTime ? " - " + root.formatTime(modelData.endTime, root.clockModel.use24Hour) : "")
                                        color: Theme.textMuted
                                        font.pixelSize: Theme.tinyFontSize
                                    }

                                    UiText {
                                        text: "•"
                                        color: Theme.textMuted
                                        font.pixelSize: Theme.tinyFontSize
                                        visible: modelData.startDate !== ""
                                    }

                                    UiText {
                                        text: modelData.startDate || ""
                                        color: Theme.textMuted
                                        font.pixelSize: Theme.tinyFontSize
                                        visible: text !== ""
                                    }
                                }

                                UiText {
                                    text: modelData.location || ""
                                    color: Theme.textMuted
                                    font.pixelSize: Theme.tinyFontSize
                                    visible: text !== ""
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }

            // Empty state
            UiText {
                Layout.fillWidth: true
                text: root.calendarModel.eventsBusy ? "Loading khal events..." : "No events scheduled"
                color: Theme.textMuted
                font.pixelSize: Theme.fontCaptionSize
                horizontalAlignment: Text.AlignHCenter
                visible: root.displayEvents.length === 0
            }

            // ─── Open khal in terminal ───
            ShellButton {
                Layout.fillWidth: true
                label: "Open in Terminal (khal)"
                leadingLabel: "󰃭"
                accessibleDescription: "Open khal interactive calendar in terminal"
                onActivated: {
                    root.clockModel.closeCalendar();
                    root.calendarModel.launchInteractive();
                }
            }
        }
    }
}
