import QtQuick
import qs.core

Rectangle {
    id: root

    required property string label
    property string detail: ""
    property bool active: false
    property bool selected: false
    property bool navigates: false

    signal activated

    implicitHeight: Theme.controlRowHeight
    radius: Theme.controlRadius
    color: root.selected ? Theme.controlFocusFill
        : root.active ? Theme.menuSelectedBackground
        : rowMouse.containsMouse ? Theme.menuHoverBackground : Theme.transparent
    border.color: root.selected ? Theme.controlFocusBorder : Theme.transparent
    border.width: root.selected ? Theme.controlFocusBorderWidth : 0

    UiText {
        anchors.left: parent.left
        anchors.leftMargin: Theme.controlPaddingX
        anchors.right: rowDetail.left
        anchors.rightMargin: Theme.spacingLg
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        color: root.selected ? Theme.controlFocusText
            : root.active ? Theme.menuSelectedText
            : rowMouse.containsMouse ? Theme.menuHoverText : Theme.menuText
        elide: Text.ElideRight
    }

    UiText {
        id: rowDetail

        anchors.right: parent.right
        anchors.rightMargin: Theme.controlPaddingX
        anchors.verticalCenter: parent.verticalCenter
        text: root.detail.length > 0 ? root.detail : root.navigates ? ">" : ""
        color: root.selected ? Theme.controlFocusText
            : root.active ? Theme.menuSelectedText : rowMouse.containsMouse ? Theme.menuHoverText : Theme.menuMutedText
    }

    MouseArea {
        id: rowMouse

        anchors.fill: parent
        enabled: root.enabled
        hoverEnabled: true
        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: root.activated()
    }
}
