import QtQuick
import Cloppy 1.0
import QtQuick.Controls

Menu {
    id: root
    background: Rectangle {
        color: Theme.surface2
        border.color: Theme.border
        border.width: 1
        radius: 8
        implicitWidth: 180
    }
    delegate: MenuItem {
        id: menuItem
        implicitHeight: 32
        contentItem: Text {
            text: menuItem.text
            color: menuItem.enabled ? Theme.text : Theme.faint
            font.family: Theme.font
            font.pixelSize: 12
            leftPadding: 10
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 6
            color: menuItem.highlighted ? Theme.accentSoft : "transparent"
        }
    }
}
