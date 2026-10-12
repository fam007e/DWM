import QtQuick
import QtQuick.Layouts
import Quickshell.Services.SystemTray
import qs.core

RowLayout {
    id: root

    readonly property var visibleItems: {
        const seen = {};
        return SystemTray.items.values.filter(function(item) {
            if (!item || item.id === "blueman" || item.id === "blueman-applet") return false;
            const key = (item.title || item.id || "").toLowerCase();
            if (key.indexOf("flameshot") !== -1) {
                if (seen["flameshot"]) return false;
                seen["flameshot"] = true;
            }
            return true;
        });
    }
    visible: visibleItems.length > 0
    spacing: Theme.compactSpacing

    Repeater {
        model: root.visibleItems

        delegate: TrayItem {
            required property var modelData

            trayItem: modelData
        }
    }
}
