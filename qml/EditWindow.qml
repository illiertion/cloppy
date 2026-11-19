import QtQuick
import Cloppy 1.0
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: editWindow
    title: Lang.t("클립 내용 편집", "Edit clip")
    visible: app.editVisible
    width: Math.min(700, Screen.width - 40)
    height: Math.min(540, Screen.height - 80)
    minimumWidth: 480
    minimumHeight: 360
    color: Theme.bg
    flags: Qt.Dialog
    modality: Qt.ApplicationModal
    x: Math.round((Screen.width - width) / 2)
    y: Math.round((Screen.height - height) / 2)

    Binding { target: Theme; property: "dark"; value: app.darkTheme }

    function save() {
        app.saveEdit(editor.text)
    }

    onVisibleChanged: {
        if (visible) {
            editor.text = app.editText
            requestActivate()
            editor.forceActiveFocus()
        }
    }

    onClosing: function(close) {
        close.accepted = false
        app.closeEdit()
    }

    Shortcut {
        sequence: "Escape"
        onActivated: app.closeEdit()
    }
    Shortcut {
        sequence: "Ctrl+Return"
        onActivated: editWindow.save()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.bg

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 10

            RowLayout {
                Layout.fillWidth: true

                ColumnLayout {
                    spacing: 1
                    Text {
                        text: Lang.t("클립 내용 편집", "Edit clip")
                        color: Theme.text
                        font.family: Theme.font
                        font.pixelSize: 16
                        font.bold: true
                    }
                    Text {
                        text: app.editTitle + Lang.t("  ·  저장하면 자동으로 다시 분류됩니다", "  ·  Saved changes are re-classified automatically")
                        color: Theme.muted
                        font.family: Theme.font
                        font.pixelSize: 10
                    }
                }
                Item { Layout.fillWidth: true }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.surface
                radius: 10
                border.width: 1
                border.color: Theme.border

                ScrollView {
                    id: editorScroll
                    anchors.fill: parent
                    anchors.margins: 8
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AsNeeded

                    TextArea {
                        id: editor
                        width: editorScroll.availableWidth
                        wrapMode: TextEdit.Wrap
                        selectByMouse: true
                        color: Theme.text
                        selectionColor: Theme.accent
                        selectedTextColor: Theme.accentText
                        font.family: "Cascadia Mono"
                        font.pixelSize: 12
                        background: null
                        padding: 6
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ChipButton {
                    text: Lang.t("한/영 교환", "Convert case")
                    onClicked: editor.text = app.swapText(editor.text)
                }

                Item { Layout.fillWidth: true }

                ChipButton {
                    text: Lang.t("취소", "Cancel")
                    onClicked: app.closeEdit()
                }
                ChipButton {
                    text: Lang.t("저장", "Save")
                    primary: true
                    onClicked: editWindow.save()
                }
            }
        }
    }
}
