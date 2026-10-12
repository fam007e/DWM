import QtQuick
import QtQuick.Layouts
import qs.core

PanelPill {
    id: root

    signal activated

    Layout.preferredWidth: Theme.pillHeight
    Layout.preferredHeight: Theme.pillHeight
    hovered: logoMouse.containsMouse

    // Primary: Crisp dwm.png logo asset
    Image {
        id: logoImage

        anchors.centerIn: parent
        width: Theme.scaledSize(22)
        height: Theme.scaledSize(22)
        source: Qt.resolvedUrl("../assets/dwm.png")
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        smooth: true
        mipmap: true
    }

    // Fallback: Nerd Font nf-linux-dwm glyph (\uf356 / )
    IconText {
        id: logoIcon

        anchors.centerIn: parent
        visible: logoImage.status === Image.Error || logoImage.status === Image.Null
        text: "\u{f356}"
        color: Theme.accent
        font.pixelSize: Theme.scaledFontSize(14 * 1.3, 8)
    }

    MouseArea {
        id: logoMouse

        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.activated()
    }
}
