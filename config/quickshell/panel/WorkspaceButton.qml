import QtQuick
import QtQuick.Layouts
import qs.core

Rectangle {
    id: root

    required property string label
    required property bool selected
    required property bool occupied
    signal clicked()

    visible: root.selected || root.occupied
    Layout.preferredWidth: visible ? Theme.workspaceButtonSize : 0
    Layout.preferredHeight: visible ? Theme.workspaceButtonSize : 0
    radius: Theme.smallRadius
    color: selected ? Theme.controlSelectedFill
        : workspaceMouse.containsMouse ? Theme.controlHoverFill : Theme.transparent
    border.color: selected ? Theme.controlSelectedBorder
        : workspaceMouse.containsMouse ? Theme.controlHoverBorder : Theme.transparent
    border.width: selected || workspaceMouse.containsMouse ? Theme.pillBorderWidth : 0

    IconText {
        anchors.centerIn: parent
        text: root.label
        color: root.selected ? Theme.controlSelectedText
            : workspaceMouse.containsMouse ? Theme.controlHoverText : Theme.text
        font.bold: root.selected
        font.pixelSize: Theme.scaledFontSize(14, 8)
    }

    MouseArea {
        id: workspaceMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
