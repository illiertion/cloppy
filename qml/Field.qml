import QtQuick
import Cloppy 1.0
import QtQuick.Controls

TextField {
    id: root
    color: Theme.text
    font.family: Theme.font
    font.pixelSize: 12
    selectByMouse: true
    background: Rectangle {
        color: Theme.surface2
        radius: 8
        border.width: 1
        border.color: root.activeFocus ? Theme.accent : Theme.border
    }
}
