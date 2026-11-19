import QtQuick
import Cloppy 1.0
import QtQuick.Controls

TextField {
    id: root
    property string spec: ""
    readOnly: true
    selectByMouse: false
    horizontalAlignment: Text.AlignLeft
    placeholderText: "클릭한 뒤 단축키를 누르세요"
    color: Theme.text
    font.family: Theme.font
    font.pixelSize: 12
    background: Rectangle {
        color: Theme.surface2
        radius: 8
        border.width: 1
        border.color: root.activeFocus ? Theme.accent : Theme.border
    }
    onActiveFocusChanged: {
        app.setCapturing(activeFocus)
        if (!activeFocus)
            text = app.formatHotkey(spec)
    }

    function setSpec(value) {
        spec = value || ""
        text = app.formatHotkey(spec)
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Escape) {
            focus = false
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Control || event.key === Qt.Key_Shift
                || event.key === Qt.Key_Alt || event.key === Qt.Key_Meta)
            return
        if (event.key === Qt.Key_Backspace || event.key === Qt.Key_Delete) {
            spec = ""
            text = ""
            event.accepted = true
            return
        }

        var parts = []
        if (event.modifiers & Qt.MetaModifier)
            parts.push("win")
        if (event.modifiers & Qt.ControlModifier)
            parts.push("ctrl")
        if (event.modifiers & Qt.AltModifier)
            parts.push("alt")
        if (event.modifiers & Qt.ShiftModifier)
            parts.push("shift")

        var key = ""
        if (event.key >= Qt.Key_A && event.key <= Qt.Key_Z)
            key = String.fromCharCode(event.key).toLowerCase()
        else if (event.key >= Qt.Key_0 && event.key <= Qt.Key_9)
            key = String.fromCharCode(event.key)
        else if (event.key >= Qt.Key_F1 && event.key <= Qt.Key_F24)
            key = "f" + (event.key - Qt.Key_F1 + 1)
        else if (event.key === Qt.Key_Space)
            key = "space"
        else if (event.text && event.text.length === 1)
            key = event.text.toLowerCase()

        if (!key || parts.length === 0)
            return
        parts.push(key)
        spec = parts.join("+")
        text = app.formatHotkey(spec)
        focus = false
        event.accepted = true
    }
}
