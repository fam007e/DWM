import QtQuick
import QtQuick.Layouts
import qs.core

Rectangle {
    id: root

    property string title: ""
    property string detail: ""
    property string status: ""
    property bool selected: false
    readonly property color statusColor: root.status === "error" ? Theme.danger
        : root.status === "warn" ? Theme.warning : Theme.accent

    implicitHeight: Math.max(42, row.implicitHeight + 16)
    color: root.selected ? Theme.controlFocusFill : Theme.controlNormalFill
    border.color: root.selected ? Theme.controlFocusBorder : (root.status === "error" ? Theme.danger : Theme.controlNormalBorder)
    border.width: root.selected ? Theme.controlFocusBorderWidth : Theme.controlBorderWidth
    radius: Theme.radius

    RowLayout {
        id: row

        anchors.fill: parent
        anchors.margins: 10
        spacing: Theme.rowSpacing

        Rectangle {
            Layout.preferredWidth: 10
            Layout.preferredHeight: 10
            radius: 5
            color: root.statusColor
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.tightSpacing

            Text {
                Layout.fillWidth: true
                text: root.title
                color: Theme.textStrong
                font.family: Theme.fontFamily
                font.pixelSize: Theme.panelFontSize
                font.bold: true
                elide: Text.ElideRight
            }

            Text {
                Layout.fillWidth: true
                text: root.detail
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.smallFontSize
                elide: Text.ElideRight
            }
        }
    }
}
