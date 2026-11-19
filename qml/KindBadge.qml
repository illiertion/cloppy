import QtQuick
import Cloppy 1.0

Rectangle {
    id: root
    property string kind: "text"
    property color accentColor: Theme.kindColor(kind)

    implicitWidth: 38
    implicitHeight: 38
    radius: 9
    color: Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.16)

    Text {
        anchors.centerIn: parent
        text: Theme.kindLabel(root.kind)
        color: root.accentColor
        font.family: Theme.font
        font.pixelSize: text.length > 2 ? 8 : (text.length > 1 ? 10 : 13)
        font.bold: true
    }
}
