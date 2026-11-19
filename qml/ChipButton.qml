import QtQuick
import Cloppy 1.0

Rectangle {
    id: root
    property string text: ""
    property bool checked: false
    property bool primary: false
    signal clicked()

    implicitHeight: 30
    implicitWidth: label.implicitWidth + 24
    radius: 15
    opacity: enabled ? 1 : 0.45
    color: primary ? Theme.accent : (checked ? Theme.accentSoft : (hover.hovered ? Theme.surface2 : "transparent"))
    border.width: 1
    border.color: primary ? Theme.accent : (checked ? Theme.accent : Theme.border)

    HoverHandler { id: hover }

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root.primary ? Theme.accentText : (root.checked ? Theme.text : Theme.muted)
        font.family: Theme.font
        font.pixelSize: 12
        font.bold: root.primary || root.checked
    }

    MouseArea {
        anchors.fill: parent
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
