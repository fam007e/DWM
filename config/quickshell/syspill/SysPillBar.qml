import QtQuick
import QtQuick.Layouts
import qs.core

// Animated mini progress-bar pill for the DWM panel.
// Shows an animated fill bar (Kali/XFCE style) that represents
// the dominant metric (RAM or CPU depending on mode).
// Clicking it opens the SysPill detail popup.
//
// In "both" mode, two stacked thin bars are shown.

PanelPill {
    id: root

    required property var sysPillModel
    property bool hovered: sysPillMouse.containsMouse
    active: root.sysPillModel.visible
    signal popupRequested()

    Layout.preferredWidth: barContent.implicitWidth + Theme.compactWidgetHorizontalPadding * 2
    Layout.preferredHeight: Theme.compactWidgetSize

    RowLayout {
        id: barContent
        anchors.centerIn: parent
        spacing: Theme.compactSpacing

        // Icon: changes based on mode
        IconText {
            text: root.sysPillModel.mode === "cpu" ? "\udb83\udee0"
                : root.sysPillModel.mode === "both" ? "\uf0e4"
                : "\uefc5"
            color: Theme.textStrong
            font.pixelSize: Theme.scaledFontSize(14 * 1.73, 8)
        }

        // Animated progress bar area
        Item {
            Layout.preferredWidth: Theme.scaledSize(52)
            Layout.preferredHeight: root.sysPillModel.mode === "both" ? 16 : 10
            Layout.alignment: Qt.AlignVCenter

            // Single bar (mem or cpu mode)
            Rectangle {
                id: singleTrack
                visible: root.sysPillModel.mode !== "both"
                anchors.centerIn: parent
                width: parent.width
                height: 6
                radius: 3
                color: Theme.border

                Rectangle {
                    id: singleFill
                    height: parent.height
                    radius: parent.radius
                    width: parent.width * Math.min(1,
                        (root.sysPillModel.mode === "cpu"
                            ? root.sysPillModel.cpuPercent
                            : root.sysPillModel.memPercent) / 100)
                    color: fillColor(root.sysPillModel.mode === "cpu"
                        ? root.sysPillModel.cpuPercent
                        : root.sysPillModel.memPercent,
                        root.sysPillModel.mode === "cpu")

                    Behavior on width {
                        NumberAnimation { duration: Theme.animationNormal; easing.type: Easing.OutCubic }
                    }
                    Behavior on color {
                        ColorAnimation { duration: Theme.animationNormal }
                    }

                    // Shimmer / pulse animation for an alive feel
                    Rectangle {
                        id: shimmer
                        visible: singleFill.width > 8
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        width: 3
                        height: parent.height
                        radius: parent.radius
                        color: Qt.lighter(parent.color, 1.4)
                        opacity: pulseAnim.running ? 1.0 : 0

                        SequentialAnimation on opacity {
                            id: pulseAnim
                            running: shimmer.visible
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.3; duration: 800; easing.type: Easing.InOutSine }
                            NumberAnimation { to: 1.0; duration: 800; easing.type: Easing.InOutSine }
                        }
                    }
                }
            }

            // Dual bars (both mode)
            Column {
                visible: root.sysPillModel.mode === "both"
                anchors.centerIn: parent
                width: parent.width
                spacing: 2

                Rectangle {
                    width: parent.width
                    height: 5
                    radius: 2.5
                    color: Theme.border

                    Rectangle {
                        height: parent.height
                        radius: parent.radius
                        width: parent.width * Math.min(1, root.sysPillModel.memPercent / 100)
                        color: fillColor(root.sysPillModel.memPercent, false)

                        Behavior on width {
                            NumberAnimation { duration: Theme.animationNormal; easing.type: Easing.OutCubic }
                        }
                        Behavior on color {
                            ColorAnimation { duration: Theme.animationNormal }
                        }
                    }
                }

                Rectangle {
                    width: parent.width
                    height: 5
                    radius: 2.5
                    color: Theme.border

                    Rectangle {
                        height: parent.height
                        radius: parent.radius
                        width: parent.width * Math.min(1, root.sysPillModel.cpuPercent / 100)
                        color: fillColor(root.sysPillModel.cpuPercent, true)

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

        // Compact percentage text
        UiText {
            text: root.sysPillModel.mode === "cpu"
                ? Math.round(root.sysPillModel.cpuPercent) + "%"
                : root.sysPillModel.mode === "both"
                ? Math.round(root.sysPillModel.memPercent) + "|" + Math.round(root.sysPillModel.cpuPercent) + "%"
                : Math.round(root.sysPillModel.memPercent) + "%"
            color: Theme.textStrong
            font.pixelSize: Theme.panelFontSize
        }
    }

    MouseArea {
        id: sysPillMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                root.sysPillModel.launchMonitor();
            } else if (mouse.button === Qt.MiddleButton) {
                root.sysPillModel.setMode("mem");
            } else {
                root.popupRequested();
                root.sysPillModel.toggle();
            }
        }
        onWheel: function(wheel) {
            // Scroll to cycle modes
            if (wheel.angleDelta.y > 0 || wheel.angleDelta.y < 0) {
                root.sysPillModel.cycleMode();
            }
            wheel.accepted = true;
        }
    }

    function fillColor(percent, isCpu) {
        if (percent >= root.sysPillModel.warnPercent)
            return Theme.danger;
        if (percent >= root.sysPillModel.cautionPercent)
            return Theme.warning;
        return isCpu ? Theme.success : Theme.accent;
    }
}
