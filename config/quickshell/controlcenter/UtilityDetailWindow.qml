import QtQuick
import QtQuick.Layouts
import Quickshell
import qs.core

pragma ComponentBehavior: Bound

FloatingWindow {
    id: root

    required property var controlCenterModel

    property string query: ""
    property int selectedIndex: 0

    visible: controlCenterModel.utilityVisible
    screen: controlCenterModel.utilityScreen
    implicitWidth: 720
    implicitHeight: 560
    color: Theme.transparent
    // The prefix keeps this window compatible with preserved user rules that
    // already float the dwm control center by title substring.
    title: "dwm control center utility"

    function titleForPage() {
        if (controlCenterModel.utilityPage === "keybinds") return "Keybinds";
        return "System Info";
    }

    function rowsForPage() {
        if (controlCenterModel.utilityPage === "keybinds") return controlCenterModel.keybindRows;
        return controlCenterModel.infoRows;
    }

    readonly property var filteredRows: {
        const rows = root.rowsForPage() || [];
        const q = root.query.trim().toLowerCase();
        if (q.length === 0) return rows;
        return rows.filter(function(item) {
            const t = (root.controlCenterModel.utilityPage === "keybinds" ? (item.keys || "") : (item.label || "")).toLowerCase();
            const d = (root.controlCenterModel.utilityPage === "keybinds" ? (item.description || "") : (item.detail || item.value || "")).toLowerCase();
            return t.indexOf(q) >= 0 || d.indexOf(q) >= 0;
        });
    }

    onVisibleChanged: {
        if (visible) {
            root.query = "";
            root.selectedIndex = 0;
            Qt.callLater(function() {
                searchInput.forceActiveFocus();
            });
        }
    }

    ShellSurface {
        anchors.fill: parent
        focus: true

        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Escape) {
                root.controlCenterModel.closeUtility();
                event.accepted = true;
            }
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: Theme.popupSpacing

            RowLayout {
                Layout.fillWidth: true

                UiText {
                    Layout.fillWidth: true
                    text: root.titleForPage()
                    color: Theme.textStrong
                    font.pixelSize: Theme.titleFontSize
                    font.bold: true
                }

                UiText {
                    text: root.filteredRows.length + " entries"
                    color: Theme.textMuted
                    font.pixelSize: Theme.smallFontSize
                }

                ShellButton {
                    label: "Close"
                    onActivated: root.controlCenterModel.closeUtility()
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.scaledSize(42)
                color: Theme.controlNormalFill
                border.color: searchInput.activeFocus ? Theme.controlFocusBorder : Theme.controlNormalBorder
                border.width: searchInput.activeFocus ? Theme.controlFocusBorderWidth : Theme.controlBorderWidth
                radius: Theme.controlRadius

                TextInput {
                    id: searchInput

                    anchors.fill: parent
                    anchors.leftMargin: Theme.controlPaddingX + Theme.spacingSm
                    anchors.rightMargin: Theme.controlPaddingX + Theme.spacingSm
                    verticalAlignment: TextInput.AlignVCenter
                    color: Theme.textStrong
                    selectionColor: Theme.accent
                    selectedTextColor: Theme.accentText
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.inputFontSize
                    clip: true
                    text: root.query

                    onTextChanged: {
                        if (text !== root.query) {
                            root.query = text;
                            root.selectedIndex = 0;
                        }
                    }

                    Keys.onPressed: function(event) {
                        const count = root.filteredRows.length;
                        if (event.key === Qt.Key_Escape) {
                            root.controlCenterModel.closeUtility();
                            event.accepted = true;
                        } else if (event.key === Qt.Key_Down) {
                            if (count > 0) {
                                root.selectedIndex = Math.min(count - 1, root.selectedIndex + 1);
                                listView.positionViewAtIndex(root.selectedIndex, ListView.Contain);
                            }
                            event.accepted = true;
                        } else if (event.key === Qt.Key_Up) {
                            if (count > 0) {
                                root.selectedIndex = Math.max(0, root.selectedIndex - 1);
                                listView.positionViewAtIndex(root.selectedIndex, ListView.Contain);
                            }
                            event.accepted = true;
                        } else if (event.key === Qt.Key_PageDown) {
                            if (count > 0) {
                                root.selectedIndex = Math.min(count - 1, root.selectedIndex + 6);
                                listView.positionViewAtIndex(root.selectedIndex, ListView.Contain);
                            }
                            event.accepted = true;
                        } else if (event.key === Qt.Key_PageUp) {
                            if (count > 0) {
                                root.selectedIndex = Math.max(0, root.selectedIndex - 6);
                                listView.positionViewAtIndex(root.selectedIndex, ListView.Contain);
                            }
                            event.accepted = true;
                        } else if (event.key === Qt.Key_Home) {
                            root.selectedIndex = 0;
                            listView.positionViewAtIndex(0, ListView.Beginning);
                            event.accepted = true;
                        } else if (event.key === Qt.Key_End) {
                            if (count > 0) {
                                root.selectedIndex = count - 1;
                                listView.positionViewAtIndex(root.selectedIndex, ListView.End);
                            }
                            event.accepted = true;
                        }
                    }
                }

                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.controlPaddingX + Theme.spacingSm
                    anchors.verticalCenter: parent.verticalCenter
                    visible: searchInput.text.length === 0
                    text: root.controlCenterModel.utilityPage === "keybinds"
                        ? "Search keybindings (e.g. 'terminal', 'Super + b', 'layout')..."
                        : "Search system info..."
                    color: Theme.placeholder
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.inputFontSize
                }
            }

            UiText {
                Layout.fillWidth: true
                visible: root.controlCenterModel.message.length > 0 && root.query.length === 0
                text: root.controlCenterModel.message
                color: Theme.textMuted
            }

            ListView {
                id: listView

                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: Theme.listSpacing
                model: root.filteredRows

                delegate: Item {
                    id: rowItem

                    required property int index
                    required property var modelData

                    width: listView.width
                    height: rowCard.implicitHeight

                    ControlCenterRow {
                        id: rowCard

                        width: parent.width
                        title: root.controlCenterModel.utilityPage === "keybinds" ? (rowItem.modelData.keys || "") : (rowItem.modelData.label || "")
                        detail: root.controlCenterModel.utilityPage === "keybinds" ? (rowItem.modelData.description || "") : (rowItem.modelData.detail || rowItem.modelData.value || "")
                        status: rowItem.modelData.status || ""
                        selected: rowItem.index === root.selectedIndex
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            root.selectedIndex = rowItem.index;
                            searchInput.forceActiveFocus();
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true

                Text {
                    Layout.fillWidth: true
                    text: "↑/↓ / PgUp/PgDn Navigate  •  Type to filter  •  Esc Close"
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.smallFontSize
                }
            }
        }
    }
}
