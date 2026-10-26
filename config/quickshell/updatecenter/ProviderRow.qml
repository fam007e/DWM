pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import qs.core

Rectangle {
    id: root

    required property var provider
    property bool expanded: false
    property bool globalBusy: false
    property bool providerRecoverable: false
    property bool providerActive: false
    property bool scanning: false
    property bool online: true
    property real nowSeconds: Math.floor(Date.now() / 1000)
    property int instantiatedItemDelegates: 0
    readonly property bool providerBusy: root.globalBusy && !root.providerRecoverable
    readonly property var providerItems: Array.isArray(root.provider.items) ? root.provider.items : []
    readonly property string providerDetail: root.text(root.provider.detail)
    readonly property string providerError: root.text(root.provider.errorCode)
    readonly property bool providerMissing: root.provider.status === "unavailable" && root.providerError === "missing-provider"
    readonly property string relativeCheckAge: root.formatCheckAge(root.provider.lastSuccess, root.nowSeconds)
    readonly property var providerIcons: ({
        "arch": "../assets/update-center/arch.svg",
        "dwm": "../assets/update-center/dwm.png",
        "flatpak": "../assets/update-center/flatpak.svg",
        "mise": "../assets/update-center/mise.svg",
        "other": "../assets/update-center/other.svg"
    })

    signal updateRequested(string providerId)
    signal recoverRequested(string providerId)
    signal openUrlRequested(string url)

    function iconFor(providerId) {
        return root.providerIcons[providerId] || root.providerIcons.other;
    }

    function text(value) {
        return (value === undefined || value === null) ? "" : String(value);
    }

    function shortRevision(value) {
        const v = root.text(value);
        return /^[0-9a-f]{40}$/.test(v) ? v.substring(0, 8) : v;
    }

    // One source of truth: the accessible name and the visible label must agree.
    function statusSummary() {
        const label = root.statusLabel();
        const pending = Number(root.provider.pending) || 0;
        return (label === "Available" && pending > 0) ? pending + " pending" : label;
    }

    function visibleDetail() {
        if (root.providerDetail.length > 0) return root.providerDetail;
        if (root.provider.restart === "session") return "Sign out and back in to complete this update.";
        if (root.provider.restart === "system") return "Restart the system to complete this update.";
        return "";
    }

    function formatCheckAge(lastSuccess, nowSeconds) {
        const timestamp = Number(lastSuccess || 0);
        if (timestamp <= 0) return "Not checked";
        const seconds = Math.max(0, Math.floor(Number(nowSeconds)) - timestamp);
        if (seconds < 60) return seconds + "s ago";
        if (seconds < 3600) return Math.floor(seconds / 60) + "m ago";
        if (seconds < 86400) return Math.floor(seconds / 3600) + "h ago";
        return Math.floor(seconds / 86400) + "d ago";
    }

    function checkAge() {
        return root.relativeCheckAge;
    }

    function itemDescription(item) {
        const scope = root.text(item.scope);
        const head = root.text(item.name) + (scope.length > 0 ? " (" + scope + ")" : "");
        const current = root.text(item.current);
        const available = root.text(item.available);
        if (item.action === "reinstall" || current === available) {
            // Same revision: this is a repair, not a version change
            return head + "\nManaged files differ from installed revision " + root.shortRevision(current);
        }
        return head + "\n" + root.shortRevision(current) + " -> " + root.shortRevision(available);
    }

    readonly property bool hasDetails: root.providerItems.length > 0 || root.visibleDetail().length > 0

    function statusLabel() {
        if (root.providerRecoverable) return "Recovery required";
        if (root.providerActive) return "Active";
        if (root.scanning) return "Checking";
        if (!root.online) return "Offline";
        if (root.provider.freshness === "error") return "Failed";
        if (root.provider.freshness === "stale") return "Stale";
        if (root.provider.restart === "session") return "Restart session";
        if (root.provider.restart === "system") return "Restart system";
        if (root.provider.updateAvailable || (Number(root.provider.pending) || 0) > 0) return "Available";
        if (root.provider.status === "restricted") return "Restricted";
        if (root.provider.status === "partial") return "Partial";
        return "Up to date";
    }

    function statusLabelColor() {
        if (root.scanning || !root.online) return Theme.menuMutedText;
        if (root.providerRecoverable || root.provider.freshness === "error") return Theme.danger;
        if (root.provider.updateAvailable || root.provider.pending > 0 || root.provider.freshness === "stale"
                || root.provider.restart === "session" || root.provider.restart === "system") return Theme.warning;
        if (root.provider.status === "available" && !root.provider.updateAvailable) return Theme.success;
        return Theme.menuMutedText;
    }

    function packageCount() {
        if (root.provider.restart === "none" && (root.provider.updateAvailable || (Number(root.provider.pending) || 0) > 0)) {
            return Number(root.provider.pending) || root.providerItems.length || 1;
        }
        return typeof root.provider.managed === "number" ? root.provider.managed : 0;
    }

    function packageSummary() {
        if (root.provider.restart !== "none") return "";
        const count = root.packageCount();
        return count + (count === 1 ? " Package" : " Packages");
    }

    function toggleExpanded() {
        if (!root.hasDetails) return false;
        root.expanded = !root.expanded;
        return true;
    }

    function requestAction() {
        if (root.providerBusy || root.providerMissing) return false;
        if (root.providerRecoverable) root.recoverRequested(root.provider.id);
        else root.updateRequested(root.provider.id);
        return true;
    }

    function requestOpen(item) {
        if (!item || typeof item.url !== "string" || !/^https:\/\/[A-Za-z0-9.-]+(:[0-9]+)?(\/[^\s]*)?$/.test(item.url)) return false;
        root.openUrlRequested(item.url);
        return true;
    }

    implicitHeight: rowColumn.implicitHeight + Theme.spacingXl * 2
    color: Theme.controlNormalFill
    border.color: root.activeFocus ? Theme.controlFocusBorder : Theme.controlNormalBorder
    border.width: root.activeFocus ? Theme.controlFocusBorderWidth : Theme.controlBorderWidth
    radius: Theme.controlRadius
    activeFocusOnTab: true
    Accessible.role: Accessible.ListItem
    Accessible.name: root.provider.name + ", " + root.statusSummary()
    Accessible.description: typeof root.provider.managed === "number" ? "Managed items " + root.provider.managed : root.provider.name + " updates"

    Timer {
        interval: 30000
        repeat: true
        running: root.visible
        triggeredOnStart: true
        onTriggered: root.nowSeconds = Math.floor(Date.now() / 1000)
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
            root.toggleExpanded();
            event.accepted = true;
        }
    }

    ColumnLayout {
        id: rowColumn

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.spacingXl
        spacing: Theme.spacingLg

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingXl

            Item {
                Layout.preferredWidth: Theme.scaledSize(32)
                Layout.preferredHeight: Theme.scaledSize(32)
                Accessible.role: Accessible.Graphic
                Accessible.name: root.provider.name + " logo"

                Image {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingSm
                    source: root.iconFor(root.provider.id)
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.compactSpacing

                UiText {
                    Layout.fillWidth: true
                    text: root.provider.name
                    color: Theme.menuText
                    font.bold: true
                    elide: Text.ElideRight
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 0

                    UiText {
                        text: root.statusLabel()
                        color: root.statusLabelColor()
                        font.pixelSize: Theme.fontBodySmallSize
                    }

                    UiText {
                        Layout.fillWidth: true
                        visible: root.packageSummary().length > 0
                        text: "  |  " + root.packageSummary()
                        color: Theme.menuMutedText
                        font.pixelSize: Theme.fontBodySmallSize
                        elide: Text.ElideRight
                    }
                }
            }

            ShellButton {
                label: root.providerBusy ? "Busy" : root.providerRecoverable ? "Recover" : "Update"
                accessibleDescription: root.provider.name + " provider action"
                enabled: !root.providerMissing && (root.providerRecoverable || (!root.globalBusy && root.provider.updateAvailable && root.provider.restart === "none"))
                primary: root.providerRecoverable
                onActivated: root.requestAction()
            }

            ShellButton {
                label: root.expanded ? "Collapse" : "Details"
                accessibleDescription: root.provider.name + " update details"
                enabled: root.hasDetails
                onActivated: root.toggleExpanded()
            }
        }

        UiText {
            Layout.fillWidth: true
            visible: (root.expanded || root.providerMissing) && root.visibleDetail().length > 0
            text: root.visibleDetail()
            color: root.providerError.length > 0 ? Theme.danger : Theme.menuMutedText
            font.pixelSize: Theme.fontBodySmallSize
            wrapMode: Text.Wrap
        }

        ListView {
            id: itemViewport

            objectName: "providerItemsViewport"
            Layout.fillWidth: true
            Layout.preferredHeight: root.expanded
                ? Math.min(itemViewport.contentHeight > 0 ? itemViewport.contentHeight
                                                          : root.providerItems.length * Theme.scaledSize(56),
                           Theme.scaledSize(180))
                : 0
            visible: root.expanded && root.providerItems.length > 0
            model: root.expanded ? root.providerItems : []
            spacing: Theme.spacingSm
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            reuseItems: true
            cacheBuffer: 0
            activeFocusOnTab: visible
            Accessible.role: Accessible.List
            Accessible.name: root.provider.name + " update items"

            Behavior on Layout.preferredHeight {
                NumberAnimation { duration: Theme.animationFast }
            }

            delegate: Rectangle {
                id: itemRow

                required property var modelData
                readonly property var item: modelData

                width: ListView.view.width
                height: Math.max(Theme.scaledSize(52), itemText.implicitHeight + Theme.spacingLg * 2)
                color: Theme.menuBackground
                border.color: Theme.controlNormalBorder
                border.width: Theme.controlBorderWidth
                radius: Theme.controlRadius
                Accessible.role: Accessible.ListItem
                Accessible.name: root.text(modelData.name) + ", " + root.text(modelData.current) + " to " + root.text(modelData.available)
                Component.onCompleted: root.instantiatedItemDelegates++
                Component.onDestruction: root.instantiatedItemDelegates--
                // reuseItems pools delegates instead of destroying them
                ListView.onPooled: root.instantiatedItemDelegates--
                ListView.onReused: root.instantiatedItemDelegates++

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingLg
                    spacing: Theme.spacingLg

                    UiText {
                        id: itemText

                        Layout.fillWidth: true
                        text: root.itemDescription(itemRow.item)
                        color: Theme.menuText
                        font.pixelSize: Theme.fontBodySmallSize
                        wrapMode: Text.Wrap
                    }

                    ShellButton {
                        visible: root.text(itemRow.item.url).length > 0
                        label: "Open"
                        accessibleDescription: "Open project link for " + root.text(itemRow.item.name)
                        onActivated: root.requestOpen(itemRow.item)
                    }
                }
            }
        }
    }
}
