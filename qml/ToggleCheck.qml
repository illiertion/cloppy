import QtQuick
import Cloppy 1.0

Item {
    id: root
    property string label: ""
    property bool checked: false
    signal toggled(bool checked)
    implicitHeight: 28
    implicitWidth: 26 + labelText.implicitWidth + 6
    height: implicitHeight
    opacity: enabled ? 1 : 0.45

    Rectangle {
        id: box
        width: 18
        height: 18
        radius: 5
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        color: root.checked ? Theme.accent : Theme.surface2
        border.width: 1
        border.color: root.checked ? Theme.accent : Theme.border

        Text {
            anchors.centerIn: parent
            visible: root.checked
            text: "✓"
            color: Theme.accentText
            font.pixelSize: 11
            font.bold: true
        }
    }

    Text {
        id: labelText
        anchors.left: box.right
        anchors.leftMargin: 9
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        color: Theme.text
        font.family: Theme.font
        font.pixelSize: 12
        elide: Text.ElideRight
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.checked = !root.checked
            root.toggled(root.checked)
        }
    }
}
