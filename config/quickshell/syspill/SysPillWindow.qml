import QtQuick
import QtQuick.Layouts
import qs.core

// Popup window for the SysPill widget.
// Drops down below the bar when clicking the status bar progress widget.
// Displays detailed metrics for Memory and CPU, visual animated bars,
// a mode switcher (RAM / CPU / Both), and a launch button for btop.

ClickAwayPopup {
    id: root

    required property var sysPillModel
    required property var panelWindow

    readonly property int cardWidth: Theme.scaledSize(320)
    readonly property int edgeMargin: Theme.rowSpacing

    visible: panelWindow !== null && panelWindow.screen !== null && sysPillModel.visible
    targetWindow: panelWindow
    popupWidth: cardWidth
    popupHeight: sysPillCard.implicitHeight
    popupX: panelWindow ? Math.max(edgeMargin, panelWindow.width - cardWidth - edgeMargin) : edgeMargin
    popupY: Theme.panelHeight
    onDismissed: sysPillModel.close()

    onVisibleChanged: {
        if (visible) {
            Qt.callLater(function() {
                sysPillCard.forceActiveFocus();
            });
        } else {
            root.sysPillModel.close();
        }
    }

    ShellSurface {
        id: sysPillCard

        anchors.fill: parent
        implicitHeight: contentColumn.implicitHeight + Theme.cardPadding * 2
        margin: Theme.cardPadding
        focus: true

        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Escape) {
                root.sysPillModel.close();
                event.accepted = true;
            } else if (event.key === Qt.Key_Tab || event.key === Qt.Key_M) {
                root.sysPillModel.cycleMode();
                event.accepted = true;
            } else if (event.key === Qt.Key_B) {
                root.sysPillModel.launchMonitor();
                root.sysPillModel.close();
                event.accepted = true;
            } else if (event.key === Qt.Key_1) {
                root.sysPillModel.setMode("mem");
                event.accepted = true;
            } else if (event.key === Qt.Key_2) {
                root.sysPillModel.setMode("cpu");
                event.accepted = true;
            } else if (event.key === Qt.Key_3) {
                root.sysPillModel.setMode("both");
                event.accepted = true;
            }
        }

        ColumnLayout {
            id: contentColumn

            anchors.fill: parent
            anchors.margins: Theme.cardPadding
            spacing: Theme.spacingLg

            // Header: Title and Mode Selection Chips
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                IconText {
                    text: "\uf0e4"
                    color: Theme.accent
                    font.pixelSize: Theme.scaledFontSize(16, 12)
                }

                UiText {
                    text: "System Resources"
                    color: Theme.textStrong
                    font.bold: true
                    font.pixelSize: Theme.fontSubtitleSize
                }

                Item { Layout.fillWidth: true }

                // Mode toggle buttons (mem, cpu, both)
                RowLayout {
                    spacing: Theme.spacingXs

                    Repeater {
                        model: ["mem", "cpu", "both"]

                        delegate: Rectangle {
                            required property string modelData
                            required property int index

                            readonly property bool isSelected: root.sysPillModel.mode === modelData
                            readonly property bool isHovered: modeMouse.containsMouse

                            width: chipText.implicitWidth + Theme.spacingMd * 2
                            height: Theme.compactWidgetSize
                            radius: Theme.pillRadius
                            color: isSelected ? Theme.controlSelectedFill
                                : isHovered ? Theme.controlHoverFill : Theme.controlNormalFill
                            border.color: isSelected ? Theme.controlSelectedBorder
                                : isHovered ? Theme.controlHoverBorder : Theme.controlNormalBorder
                            border.width: Theme.pillBorderWidth

                            Behavior on color { ColorAnimation { duration: Theme.animationFast } }

                            UiText {
                                id: chipText
                                anchors.centerIn: parent
                                text: parent.modelData.toUpperCase()
                                color: parent.isSelected ? Theme.accent : Theme.textMuted
                                font.pixelSize: Theme.tinyFontSize
                                font.bold: parent.isSelected
                            }

                            MouseArea {
                                id: modeMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.sysPillModel.setMode(parent.modelData)
                            }
                        }
                    }
                }
            }

            PanelSeparator { Layout.fillWidth: true }

            // ---- RAM Card Section ----
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: memContent.implicitHeight + Theme.spacingMd * 2
                color: Theme.controlNormalFill
                radius: Theme.radius
                border.color: Theme.controlNormalBorder
                border.width: Theme.controlBorderWidth

                ColumnLayout {
                    id: memContent
                    anchors.fill: parent
                    anchors.margins: Theme.spacingMd
                    spacing: Theme.spacingSm

                    RowLayout {
                        Layout.fillWidth: true

                        IconText {
                            text: "\uefc5"
                            color: root.levelColor(root.sysPillModel.memPercent, false)
                            font.pixelSize: Theme.scaledFontSize(14 * 1.85, 8)
                        }

                        UiText {
                            text: "Memory (RAM)"
                            color: Theme.textStrong
                            font.bold: true
                            font.pixelSize: Theme.panelFontSize
                        }

                        Item { Layout.fillWidth: true }

                        UiText {
                            text: Math.round(root.sysPillModel.memPercent) + "%"
                            color: root.levelColor(root.sysPillModel.memPercent, false)
                            font.bold: true
                            font.pixelSize: Theme.panelFontSize
                        }
                    }

                    // Usage text: used / total
                    RowLayout {
                        Layout.fillWidth: true

                        UiText {
                            text: root.sysPillModel.gib(root.sysPillModel.memUsedKb) + " used of " + root.sysPillModel.gib(root.sysPillModel.memTotalKb)
                            color: Theme.textMuted
                            font.pixelSize: Theme.smallFontSize
                        }

                        Item { Layout.fillWidth: true }

                        UiText {
                            text: root.sysPillModel.gib(root.sysPillModel.memTotalKb - root.sysPillModel.memUsedKb) + " free"
                            color: Theme.textMuted
                            font.pixelSize: Theme.smallFontSize
                        }
                    }

                    // Progress track & fill
                    Rectangle {
                        Layout.fillWidth: true
                        height: 8
                        radius: 4
                        color: Theme.border

                        Rectangle {
                            height: parent.height
                            radius: parent.radius
                            width: parent.width * Math.min(1, root.sysPillModel.memPercent / 100)
                            color: root.levelColor(root.sysPillModel.memPercent, false)

                            Behavior on width {
                                NumberAnimation { duration: Theme.animationNormal; easing.type: Easing.OutCubic }
                            }
                            Behavior on color {
                                ColorAnimation { duration: Theme.animationNormal }
                            }
                        }
                    }
                }
            }

            // ---- CPU Card Section ----
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: cpuContent.implicitHeight + Theme.spacingMd * 2
                color: Theme.controlNormalFill
                radius: Theme.radius
                border.color: Theme.controlNormalBorder
                border.width: Theme.controlBorderWidth

                ColumnLayout {
                    id: cpuContent
                    anchors.fill: parent
                    anchors.margins: Theme.spacingMd
                    spacing: Theme.spacingSm

                    RowLayout {
                        Layout.fillWidth: true

                        IconText {
                            text: "\udb83\udee0"
                            color: root.levelColor(root.sysPillModel.cpuPercent, true)
                            font.pixelSize: Theme.scaledFontSize(14 * 1.85, 8)
                        }

                        UiText {
                            text: "Processor (CPU)"
                            color: Theme.textStrong
                            font.bold: true
                            font.pixelSize: Theme.panelFontSize
                        }

                        Item { Layout.fillWidth: true }

                        UiText {
                            text: Math.round(root.sysPillModel.cpuPercent) + "%"
                            color: root.levelColor(root.sysPillModel.cpuPercent, true)
                            font.bold: true
                            font.pixelSize: Theme.panelFontSize
                        }
                    }

                    // Status descriptor
                    RowLayout {
                        Layout.fillWidth: true

                        UiText {
                            text: root.sysPillModel.cpuPercent >= 90 ? "Heavy load"
                                : root.sysPillModel.cpuPercent >= 50 ? "Moderate activity"
                                : "Normal activity"
                            color: Theme.textMuted
                            font.pixelSize: Theme.smallFontSize
                        }

                        Item { Layout.fillWidth: true }

                        UiText {
                            text: "Load: " + Math.round(root.sysPillModel.cpuPercent) + "%"
                            color: Theme.textMuted
                            font.pixelSize: Theme.smallFontSize
                        }
                    }

                    // Progress track & fill
                    Rectangle {
                        Layout.fillWidth: true
                        height: 8
                        radius: 4
                        color: Theme.border

                        Rectangle {
                            height: parent.height
                            radius: parent.radius
                            width: parent.width * Math.min(1, root.sysPillModel.cpuPercent / 100)
                            color: root.levelColor(root.sysPillModel.cpuPercent, true)

                            Behavior on width {
                                NumberAnimation { duration: Theme.animationNormal; easing.type: Easing.OutCubic }
                            }
                            Behavior on color {
                                ColorAnimation { duration: Theme.animationNormal }
                            }
                        }
                    }
                }
            }

            PanelSeparator { Layout.fillWidth: true }

            // ---- Footer Actions ----
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingSm

                UiText {
                    text: "Left-click: toggle · Wheel: cycle"
                    color: Theme.textMuted
                    font.pixelSize: Theme.tinyFontSize
                    Layout.fillWidth: true
                }

                ShellButton {
                    label: "Open btop"
                    compact: true
                    onActivated: {
                        root.sysPillModel.launchMonitor();
                        root.sysPillModel.close();
                    }
                }
            }
        }
    }

    function levelColor(percent, isCpu) {
        if (percent >= root.sysPillModel.warnPercent)
            return Theme.danger;
        if (percent >= root.sysPillModel.cautionPercent)
            return Theme.warning;
        return isCpu ? Theme.success : Theme.accent;
    }
}
