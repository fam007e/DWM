import QtQuick
import QtQuick.Layouts
import qs.core

pragma ComponentBehavior: Bound

ClickAwayPopup {
    id: root

    required property var controlCenterModel
    required property var healthModel
    required property var launcherModel
    required property var panelWindow
    required property var powerMenuModel
    required property var powerModel
    required property var settingsModel

    readonly property int cardWidth: Theme.controlCenterWidth
    readonly property int compactRowHeight: Math.max(28, Theme.fontBodySize + 12)
    readonly property int maximumHeight: panelWindow && panelWindow.screen
        ? Math.max(240, panelWindow.screen.height - Theme.panelHeight - Theme.popupMargin)
        : 240
    readonly property var powerPresets: root.powerModel.timeoutPresets

    property int selectedIndex: 0

    function itemCountForPage() {
        const page = controlCenterModel.page;
        if (page === "overview") return 10;
        if (page === "widgets") return 5;
        if (page === "actions") return controlCenterModel.actions.length;
        if (page === "appearance") return controlCenterModel.themeRows.length;
        if (page === "power") return 2;
        return 0;
    }

    function activateSelectedItem() {
        const page = controlCenterModel.page;
        const idx = selectedIndex;
        if (page === "overview") {
            switch (idx) {
                case 0: openApplications(); break;
                case 1: openSessionPower(); break;
                case 2: controlCenterModel.openWidgets(); break;
                case 3: controlCenterModel.openActions(); break;
                case 4: controlCenterModel.openAppearance(); break;
                case 5: controlCenterModel.openPower(); break;
                case 6: openSettings(); break;
                case 7: openSystemHealth(); break;
                case 8: openKeybinds(); break;
                case 9: openSystemInfo(); break;
            }
        } else if (page === "widgets") {
            const widgets = ["Volume", "Bluetooth", "Network", "Power", "Workspaces"];
            if (idx >= 0 && idx < widgets.length) controlCenterModel.toggleWidget(widgets[idx]);
        } else if (page === "actions") {
            const actions = controlCenterModel.actions;
            if (idx >= 0 && idx < actions.length) controlCenterModel.runAction(actions[idx].id);
        } else if (page === "appearance") {
            const themes = controlCenterModel.themeRows;
            if (idx >= 0 && idx < themes.length) controlCenterModel.setTheme(themes[idx].name);
        } else if (page === "power") {
            if (idx === 0) powerModel.setDpms(!powerModel.dpmsEnabled, "controlcenter");
            else if (idx === 1) powerModel.setLock(!powerModel.lockEnabled, "controlcenter");
        }
    }

    function pageTitle() {
        if (controlCenterModel.page === "widgets") return "Bar Widgets";
        if (controlCenterModel.page === "actions") return "Quick Actions";
        if (controlCenterModel.page === "appearance") return "Appearance";
        if (controlCenterModel.page === "power") return "Power Settings";
        return "Control Center";
    }

    function pageMessage() {
        if (root.controlCenterModel.page === "power")
            return root.powerModel.messageFor("controlcenter");
        if (root.controlCenterModel.page === "widgets"
                && root.controlCenterModel.panelSettingsModel) {
            const panelModel = root.controlCenterModel.panelSettingsModel;
            if (panelModel.message.length > 0 && !panelModel.actionSucceeded)
                return panelModel.message;
            if (panelModel.providerState !== "available" && panelModel.providerState !== "defaults")
                return panelModel.providerDetail;
            if (panelModel.message.length > 0) return panelModel.message;
        }
        return root.controlCenterModel.message;
    }

    function formatDuration(seconds) {
        if (seconds >= 3600 && seconds % 3600 === 0) {
            return (seconds / 3600) + "h";
        }
        if (seconds >= 60 && seconds % 60 === 0) {
            return (seconds / 60) + "m";
        }
        return seconds + "s";
    }

    function openApplications() {
        const targetScreen = root.panelWindow ? root.panelWindow.screen : null;
        root.controlCenterModel.close();
        Qt.callLater(function() {
            root.launcherModel.openOnScreen(targetScreen);
        });
    }

    function openSessionPower() {
        root.controlCenterModel.close();
        Qt.callLater(function() {
            root.powerMenuModel.open("controlcenter");
        });
    }

    function openSystemHealth() {
        const targetScreen = root.panelWindow ? root.panelWindow.screen : null;
        root.controlCenterModel.close();
        Qt.callLater(function() {
            root.healthModel.openOnScreen(targetScreen);
        });
    }

    function openSettings() {
        const targetScreen = root.panelWindow ? root.panelWindow.screen : null;
        root.controlCenterModel.close();
        Qt.callLater(function() {
            root.settingsModel.openOnScreen(targetScreen);
        });
    }

    function openKeybinds() {
        const targetScreen = root.panelWindow ? root.panelWindow.screen : null;
        root.controlCenterModel.close();
        Qt.callLater(function() {
            root.controlCenterModel.openKeybindsOnScreen(targetScreen);
        });
    }

    function openSystemInfo() {
        const targetScreen = root.panelWindow ? root.panelWindow.screen : null;
        root.controlCenterModel.close();
        Qt.callLater(function() {
            root.controlCenterModel.openInfoOnScreen(targetScreen);
        });
    }

    visible: panelWindow !== null && panelWindow.screen !== null && controlCenterModel.visible
    targetWindow: panelWindow
    popupX: Theme.controlCenterX
    popupY: Theme.panelHeight
    popupWidth: cardWidth
    popupHeight: Math.min(controlCard.implicitHeight, maximumHeight)
    onDismissed: controlCenterModel.close()

    onVisibleChanged: {
        if (visible) {
            root.selectedIndex = 0;
            root.powerMenuModel.close("controlcenter");
            Qt.callLater(function() {
                controlCard.forceActiveFocus();
            });
        } else {
            root.controlCenterModel.close();
        }
    }

    Connections {
        target: root.controlCenterModel

        function onPageChanged() {
            root.selectedIndex = 0;
            menuFlick.contentY = 0;
            Qt.callLater(function() {
                controlCard.forceActiveFocus();
            });
        }
    }

    component PresetButton: Rectangle {
        id: presetButton

        property string label: ""
        property bool active: false
        signal activated()

        implicitHeight: Math.max(26, Theme.fontBodySize + 10)
        radius: Theme.smallRadius
        activeFocusOnTab: presetButton.enabled
        color: !presetButton.enabled ? Theme.controlDisabledFill
            : presetButton.activeFocus ? Theme.controlFocusFill
            : presetButton.active ? Theme.controlSelectedFill
            : presetMouse.containsMouse ? Theme.controlHoverFill : Theme.controlNormalFill
        border.color: !presetButton.enabled ? Theme.controlDisabledBorder
            : presetButton.activeFocus ? Theme.controlFocusBorder
            : presetButton.active ? Theme.controlSelectedBorder
            : presetMouse.containsMouse ? Theme.controlHoverBorder : Theme.controlNormalBorder
        border.width: presetButton.activeFocus ? Theme.controlFocusBorderWidth : Theme.controlBorderWidth

        Keys.onPressed: function(event) {
            if (!presetButton.enabled || event.isAutoRepeat) return;
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                presetButton.activated();
                event.accepted = true;
            }
        }

        UiText {
            anchors.centerIn: parent
            text: presetButton.label
            color: !presetButton.enabled ? Theme.controlDisabledText
                : presetButton.activeFocus ? Theme.controlFocusText
                : presetButton.active ? Theme.controlSelectedText
                : presetMouse.containsMouse ? Theme.controlHoverText : Theme.controlNormalText
        }

        MouseArea {
            id: presetMouse

            anchors.fill: parent
            enabled: presetButton.enabled
            hoverEnabled: true
            cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: {
                presetButton.forceActiveFocus();
                presetButton.activated();
            }
        }
    }

    ShellSurface {
        id: controlCard

        anchors.fill: parent
        implicitHeight: menuColumn.implicitHeight + margin * 2
        margin: Theme.spacingLg
        focus: true

        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Escape) {
                if (root.controlCenterModel.page !== "overview") {
                    root.controlCenterModel.openOverview();
                } else {
                    root.controlCenterModel.close();
                }
                event.accepted = true;
            } else if (event.key === Qt.Key_Down) {
                const count = root.itemCountForPage();
                if (count > 0) {
                    root.selectedIndex = Math.min(count - 1, root.selectedIndex + 1);
                }
                event.accepted = true;
            } else if (event.key === Qt.Key_Up) {
                const count = root.itemCountForPage();
                if (count > 0) {
                    root.selectedIndex = Math.max(0, root.selectedIndex - 1);
                }
                event.accepted = true;
            } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter
                    || event.key === Qt.Key_Space || event.key === Qt.Key_Right) {
                root.activateSelectedItem();
                event.accepted = true;
            } else if (event.key === Qt.Key_Left || event.key === Qt.Key_Backspace) {
                if (root.controlCenterModel.page !== "overview") {
                    root.controlCenterModel.openOverview();
                }
                event.accepted = true;
            }
        }

        Flickable {
            id: menuFlick

            anchors.fill: parent
            contentWidth: width
            contentHeight: menuColumn.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            ColumnLayout {
                id: menuColumn

                width: menuFlick.width
                spacing: Theme.compactSpacing

                MenuHeader {
                    Layout.fillWidth: true
                    title: root.pageTitle()
                    showBack: root.controlCenterModel.page !== "overview"
                    titleLetterSpacing: root.controlCenterModel.page === "overview" ? 2 : 1
                    onBackRequested: root.controlCenterModel.openOverview()
                    onCloseRequested: root.controlCenterModel.close()
                }

                UiText {
                    Layout.fillWidth: true
                    visible: root.pageMessage().length > 0
                    text: root.pageMessage()
                    color: Theme.textMuted
                    wrapMode: Text.Wrap
                }

                PanelSeparator {}

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root.controlCenterModel.page === "overview"
                    spacing: Theme.compactSpacing

                    MenuRow {
                        Layout.fillWidth: true
                        implicitHeight: root.compactRowHeight
                        label: "Applications"
                        selected: root.selectedIndex === 0
                        onActivated: root.openApplications()
                    }
                    MenuRow {
                        Layout.fillWidth: true
                        implicitHeight: root.compactRowHeight
                        label: "Power"
                        selected: root.selectedIndex === 1
                        navigates: true
                        onActivated: root.openSessionPower()
                    }

                    PanelSeparator {
                        Layout.topMargin: Theme.compactSpacing
                        Layout.bottomMargin: Theme.compactSpacing
                    }

                    MenuRow {
                        Layout.fillWidth: true
                        implicitHeight: root.compactRowHeight
                        label: "Bar Widgets"
                        selected: root.selectedIndex === 2
                        navigates: true
                        onActivated: root.controlCenterModel.openWidgets()
                    }
                    MenuRow {
                        Layout.fillWidth: true
                        implicitHeight: root.compactRowHeight
                        label: "Quick Actions"
                        selected: root.selectedIndex === 3
                        navigates: true
                        onActivated: root.controlCenterModel.openActions()
                    }
                    MenuRow {
                        Layout.fillWidth: true
                        implicitHeight: root.compactRowHeight
                        label: "Appearance"
                        selected: root.selectedIndex === 4
                        navigates: true
                        onActivated: root.controlCenterModel.openAppearance()
                    }
                    MenuRow {
                        Layout.fillWidth: true
                        implicitHeight: root.compactRowHeight
                        label: "Power Settings"
                        selected: root.selectedIndex === 5
                        navigates: true
                        onActivated: root.controlCenterModel.openPower()
                    }

                    PanelSeparator {
                        Layout.topMargin: Theme.compactSpacing
                        Layout.bottomMargin: Theme.compactSpacing
                    }

                    MenuRow {
                        Layout.fillWidth: true
                        implicitHeight: root.compactRowHeight
                        label: "Settings"
                        selected: root.selectedIndex === 6
                        onActivated: root.openSettings()
                    }
                    MenuRow {
                        Layout.fillWidth: true
                        implicitHeight: root.compactRowHeight
                        label: "System Health"
                        selected: root.selectedIndex === 7
                        onActivated: root.openSystemHealth()
                    }
                    MenuRow {
                        Layout.fillWidth: true
                        implicitHeight: root.compactRowHeight
                        label: "Keybinds"
                        selected: root.selectedIndex === 8
                        onActivated: root.openKeybinds()
                    }
                    MenuRow {
                        Layout.fillWidth: true
                        implicitHeight: root.compactRowHeight
                        label: "System Info"
                        selected: root.selectedIndex === 9
                        onActivated: root.openSystemInfo()
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root.controlCenterModel.page === "widgets"
                    spacing: 2

                    Repeater {
                        model: ["Volume", "Bluetooth", "Network", "Power", "Workspaces"]

                        delegate: MenuRow {
                            required property int index
                            required property string modelData

                            Layout.fillWidth: true
                            implicitHeight: root.compactRowHeight
                            label: modelData
                            selected: root.selectedIndex === index
                            detail: root.controlCenterModel.widgetEnabled(modelData) ? "On" : "Off"
                            active: root.controlCenterModel.widgetEnabled(modelData)
                            enabled: !root.controlCenterModel.panelSettingsModel
                                || (root.controlCenterModel.panelSettingsModel.mutationReady
                                    && !root.controlCenterModel.panelSettingsModel.busy)
                            onActivated: root.controlCenterModel.toggleWidget(modelData)
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root.controlCenterModel.page === "actions"
                    spacing: 2

                    Repeater {
                        model: root.controlCenterModel.actions

                        delegate: MenuRow {
                            required property int index
                            required property var modelData

                            Layout.fillWidth: true
                            implicitHeight: root.compactRowHeight
                            label: modelData.label
                            selected: root.selectedIndex === index
                            enabled: !root.controlCenterModel.busy
                            onActivated: root.controlCenterModel.runAction(modelData.id)
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root.controlCenterModel.page === "appearance"
                    spacing: 2

                    Repeater {
                        model: root.controlCenterModel.themeRows

                        delegate: MenuRow {
                            required property int index
                            required property var modelData

                            Layout.fillWidth: true
                            implicitHeight: root.compactRowHeight
                            label: modelData.name
                            selected: root.selectedIndex === index
                            detail: modelData.status === "active" ? "Active" : ""
                            active: modelData.status === "active"
                            enabled: !root.controlCenterModel.busy
                            onActivated: root.controlCenterModel.setTheme(modelData.name)
                        }
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    visible: root.controlCenterModel.page === "power"
                    spacing: Theme.spacingSm

                    MenuRow {
                        Layout.fillWidth: true
                        implicitHeight: root.compactRowHeight
                        label: "Screen Timeout"
                        selected: root.selectedIndex === 0
                        detail: root.powerModel.dpmsEnabled
                            ? root.formatDuration(root.powerModel.dpmsTimeout) : "Off"
                        active: root.powerModel.dpmsEnabled
                        enabled: root.powerModel.dpmsAvailable && !root.powerModel.busy
                        onActivated: root.powerModel.setDpms(!root.powerModel.dpmsEnabled, "controlcenter")
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 3
                        columnSpacing: Theme.spacingSm
                        rowSpacing: Theme.spacingSm

                        Repeater {
                            model: root.powerPresets

                            delegate: PresetButton {
                                required property var modelData

                                Layout.fillWidth: true
                                label: modelData.label
                                active: root.powerModel.dpmsEnabled
                                    && root.powerModel.dpmsTimeout === modelData.seconds
                                enabled: root.powerModel.dpmsAvailable && !root.powerModel.busy
                                onActivated: root.powerModel.setDpmsTimeout(modelData.seconds, "controlcenter")
                            }
                        }
                    }

                    PanelSeparator {
                        Layout.topMargin: Theme.compactSpacing
                        Layout.bottomMargin: Theme.compactSpacing
                    }

                    MenuRow {
                        Layout.fillWidth: true
                        implicitHeight: root.compactRowHeight
                        label: "Auto Lock"
                        selected: root.selectedIndex === 1
                        detail: !root.powerModel.lockAvailable ? "Unknown" : root.powerModel.lockEnabled
                            ? root.formatDuration(root.powerModel.lockTimeout) : "Off"
                        active: root.powerModel.lockEnabled
                        enabled: root.powerModel.lockAvailable && !root.powerModel.busy
                        onActivated: root.powerModel.setLock(!root.powerModel.lockEnabled, "controlcenter")
                    }
                    GridLayout {
                        Layout.fillWidth: true
                        columns: 3
                        columnSpacing: Theme.spacingSm
                        rowSpacing: Theme.spacingSm

                        Repeater {
                            model: root.powerPresets

                            delegate: PresetButton {
                                required property var modelData

                                Layout.fillWidth: true
                                label: modelData.label
                                active: root.powerModel.lockEnabled
                                    && root.powerModel.lockTimeout === modelData.seconds
                                enabled: root.powerModel.lockAvailable && !root.powerModel.busy
                                onActivated: root.powerModel.setLockTimeout(modelData.seconds, "controlcenter")
                            }
                        }
                    }
                }
            }
        }
    }
}
