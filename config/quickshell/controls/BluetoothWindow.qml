import QtQuick
import QtQuick.Layouts
import qs.core

pragma ComponentBehavior: Bound

ClickAwayPopup {
    id: root

    required property var bluetoothModel
    required property var panelWindow

    property int selectedIndex: 0

    readonly property int cardWidth: Theme.scaledSize(360)
    readonly property int cardHeight: Theme.scaledSize(420)

    visible: panelWindow !== null && panelWindow.screen !== null && bluetoothModel.visible
    targetWindow: panelWindow
    popupWidth: cardWidth
    popupHeight: cardHeight
    popupX: panelWindow
        ? Math.max(Theme.rowSpacing, panelWindow.width - cardWidth - Theme.rowSpacing)
        : Theme.rowSpacing
    popupY: Theme.panelHeight
    onDismissed: bluetoothModel.close()

    onVisibleChanged: {
        if (visible) {
            selectedIndex = 0;
            Qt.callLater(function() {
                surface.forceActiveFocus();
            });
        } else {
            root.bluetoothModel.close();
        }
    }

    ShellSurface {
        id: surface

        anchors.fill: parent
        focus: true

        Keys.onPressed: function(event) {
            const devCount = root.bluetoothModel.devices.length;

            if (event.key === Qt.Key_Escape) {
                root.bluetoothModel.close();
                event.accepted = true;
            } else if (event.key === Qt.Key_Down) {
                if (devCount > 0) {
                    root.selectedIndex = Math.min(devCount - 1, root.selectedIndex + 1);
                    deviceListView.positionViewAtIndex(root.selectedIndex, ListView.Contain);
                }
                event.accepted = true;
            } else if (event.key === Qt.Key_Up) {
                if (devCount > 0) {
                    root.selectedIndex = Math.max(0, root.selectedIndex - 1);
                    deviceListView.positionViewAtIndex(root.selectedIndex, ListView.Contain);
                }
                event.accepted = true;
            } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
                if (devCount > 0 && root.selectedIndex >= 0 && root.selectedIndex < devCount) {
                    const dev = root.bluetoothModel.devices[root.selectedIndex];
                    const actionName = dev.connected ? "bluetooth-disconnect"
                        : (dev.paired ? "bluetooth-connect" : "bluetooth-pair");
                    root.bluetoothModel.action(actionName, [dev.address]);
                } else if (root.bluetoothModel.available && root.bluetoothModel.actionsAvailable) {
                    root.bluetoothModel.action("bluetooth-power", [root.bluetoothModel.powered ? "off" : "on"]);
                }
                event.accepted = true;
            } else if (event.key === Qt.Key_S) {
                if (root.bluetoothModel.powered && root.bluetoothModel.actionsAvailable && !root.bluetoothModel.busy) {
                    root.bluetoothModel.refresh(true);
                }
                event.accepted = true;
            } else if (event.key === Qt.Key_P || event.key === Qt.Key_T) {
                if (root.bluetoothModel.available && root.bluetoothModel.actionsAvailable) {
                    root.bluetoothModel.action("bluetooth-power", [root.bluetoothModel.powered ? "off" : "on"]);
                }
                event.accepted = true;
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.popupSpacing

            PanelHero {
                Layout.fillWidth: true
                iconText: root.bluetoothModel.icon
                iconColor: !root.bluetoothModel.powered ? Theme.menuMutedText
                    : (root.bluetoothModel.connected ? Theme.accent : Theme.popupText)
                title: "Bluetooth"
                subtitle: root.bluetoothModel.statusText

                ShellButton {
                    label: "Scan"
                    enabled: root.bluetoothModel.powered && root.bluetoothModel.actionsAvailable
                        && !root.bluetoothModel.busy
                    onActivated: root.bluetoothModel.refresh(true)
                }

                PanelToggleSwitch {
                    checked: root.bluetoothModel.powered
                    busy: root.bluetoothModel.busy
                    enabled: root.bluetoothModel.available && root.bluetoothModel.actionsAvailable
                    accessibleName: "Bluetooth power"
                    accessibleDescription: "Turn the Bluetooth adapter on or off"
                    onToggled: root.bluetoothModel.action("bluetooth-power", [checked ? "off" : "on"])
                }
            }

            UiText {
                Layout.fillWidth: true
                visible: root.bluetoothModel.message.length > 0
                text: root.bluetoothModel.message
                color: Theme.menuMutedText
                elide: Text.ElideRight
            }

            PanelSeparator {}

            SectionLabel { label: "Devices" }

            ListView {
                id: deviceListView

                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: Theme.listSpacing
                model: root.bluetoothModel.devices

                delegate: Rectangle {
                    id: deviceRow

                    required property int index
                    required property var modelData

                    readonly property bool isSelected: index === root.selectedIndex

                    width: ListView.view.width
                    height: Theme.scaledSize(58)
                    radius: Theme.smallRadius
                    color: isSelected ? Theme.controlFocusFill
                        : (deviceMouse.containsMouse ? Theme.controlHoverFill : Theme.controlNormalFill)
                    border.color: isSelected ? Theme.controlFocusBorder
                        : (deviceMouse.containsMouse ? Theme.controlHoverBorder : Theme.controlNormalBorder)
                    border.width: isSelected ? Theme.controlFocusBorderWidth : Theme.controlBorderWidth

                    MouseArea {
                        id: deviceMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.selectedIndex = deviceRow.index
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        ColumnLayout {
                            Layout.fillWidth: true

                            UiText {
                                Layout.fillWidth: true
                                text: deviceRow.modelData.name.length > 0 ? deviceRow.modelData.name : deviceRow.modelData.address
                                color: Theme.readableText(Theme.textStrong, String(deviceRow.color))
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            UiText {
                                Layout.fillWidth: true
                                text: deviceRow.modelData.connected ? "Connected"
                                    : deviceRow.modelData.paired ? "Paired" : deviceRow.modelData.address
                                color: Theme.readableText(Theme.menuMutedText, String(deviceRow.color))
                                font.pixelSize: Theme.fontCaptionSize
                                elide: Text.ElideRight
                            }
                        }
                        ShellButton {
                            label: deviceRow.modelData.connected ? "Disconnect" : (deviceRow.modelData.paired ? "Connect" : "Pair")
                            enabled: root.bluetoothModel.actionsAvailable && !root.bluetoothModel.busy
                            onActivated: root.bluetoothModel.action(deviceRow.modelData.connected ? "bluetooth-disconnect" : (deviceRow.modelData.paired ? "bluetooth-connect" : "bluetooth-pair"), [deviceRow.modelData.address])
                        }
                    }
                }
            }

            UiText {
                Layout.fillWidth: true
                visible: root.bluetoothModel.devices.length === 0
                text: root.bluetoothModel.powered ? "No Bluetooth devices found" : "Turn Bluetooth on to scan"
                color: Theme.menuMutedText
                wrapMode: Text.WordWrap
            }
        }
    }
}
