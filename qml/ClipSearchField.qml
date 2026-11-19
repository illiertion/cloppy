import QtQuick
import Cloppy 1.0
import QtQuick.Controls

Item {
    id: root
    property alias text: input.text
    property string placeholder: "검색…"
    signal accepted()
    signal escapePressed()
    signal downPressed()

    implicitHeight: 42

    Rectangle {
        anchors.fill: parent
        radius: 10
        color: Theme.surface2
        border.width: 1
        border.color: input.activeFocus ? Theme.accent : Theme.border
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 13
        anchors.verticalCenter: parent.verticalCenter
        text: "⌕"
        color: Theme.faint
        font.pixelSize: 18
    }

    TextField {
        id: input
        anchors.fill: parent
        leftPadding: 38
        rightPadding: 12
        background: null
        color: Theme.text
        font.family: Theme.font
        font.pixelSize: 13
        placeholderText: root.placeholder
        placeholderTextColor: Theme.faint
        selectByMouse: true
        onAccepted: root.accepted()
        Keys.onEscapePressed: root.escapePressed()
        Keys.onDownPressed: root.downPressed()
        Keys.onUpPressed: root.downPressed()
    }
}
