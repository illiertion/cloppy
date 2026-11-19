import QtQuick
import Cloppy 1.0
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: win
    objectName: "cloppyPopup"
    title: "Cloppy"
    visible: app.popupVisible
    x: app.popupX
    y: app.popupY
    width: 398
    height: 500
    minimumWidth: 360
    minimumHeight: 420
    color: "transparent"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    property bool justOpened: false

    property var kindFilters: {
        var current = Lang.language
        return [
            { key: "", label: Lang.t("전체", "All") },
            { key: "text", label: Lang.t("텍스트", "Text") },
            { key: "code", label: Lang.t("코드", "Code") },
            { key: "link", label: Lang.t("링크", "Links") },
            { key: "image", label: Lang.t("이미지", "Images") },
            { key: "files", label: Lang.t("파일", "Files") },
            { key: "command", label: Lang.t("명령", "Commands") }
        ]
    }

    Binding { target: Theme; property: "dark"; value: app.darkTheme }

    property var currentItem: {
        var total = popupModel.count
        var index = list.currentIndex
        return index >= 0 && index < total ? popupModel.get(index) : ({})
    }

    function currentId() {
        return currentItem && currentItem.clipId !== undefined ? currentItem.clipId : 0
    }

    function pasteCurrent(plain) {
        var id = currentId()
        if (id)
            app.pasteItem(id, plain)
    }

    function folderIndent(depth) {
        var text = ""
        for (var i = 0; i < depth; ++i)
            text += "   "
        return text
    }

    function quickPaste(slot) {
        var index = slot - 1
        if (index >= 0 && index < popupModel.count) {
            list.currentIndex = index
            app.pasteItem(popupModel.get(index).clipId, false)
        }
    }

    function handleListKey(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            if (event.modifiers & Qt.ControlModifier)
                app.copyItem(currentId())
            else
                pasteCurrent(event.modifiers & Qt.ShiftModifier)
            event.accepted = true
        } else if (event.key === Qt.Key_Delete) {
            if (currentId())
                app.deleteItem(currentId())
            event.accepted = true
        } else if (event.key === Qt.Key_Escape) {
            app.closePopup()
            event.accepted = true
        } else if (event.key === Qt.Key_Up && list.currentIndex === 0) {
            search.forceActiveFocus()
            event.accepted = true
        }
    }

    onVisibleChanged: {
        if (visible) {
            width = app.popupWidth
            height = app.popupHeight
            justOpened = true
            search.text = ""
            app.setPopupQuery("")
            openGuard.restart()
            requestActivate()
            search.forceActiveFocus()
            if (popupModel.count > 0)
                list.currentIndex = 0
        }
    }

    onActiveChanged: {
        if (!active && visible)
            hideTimer.restart()
    }

    onClosing: function(close) {
        close.accepted = false
        app.closePopup()
    }

    Timer {
        id: openGuard
        interval: 600
        onTriggered: win.justOpened = false
    }

    Timer {
        id: hideTimer
        interval: 220
        onTriggered: {
            if (!win.active && !app.settingsVisible && !app.helpVisible && !app.editVisible
                    && !historyMenu.visible && !contextMenu.visible && !win.justOpened)
                app.closePopup()
        }
    }

    Timer {
        id: searchDebounce
        interval: 120
        onTriggered: app.setPopupQuery(search.text)
    }

    Shortcut { sequences: ["Escape"]; onActivated: app.closePopup() }
    Shortcut { sequences: ["Ctrl+O"]; onActivated: app.openMain() }
    Shortcut { sequences: ["Ctrl+P"]; onActivated: app.setPopupPreview(!app.popupPreview) }
    Shortcut { sequences: ["Alt+S"]; onActivated: app.toggleSecretMode() }
    Shortcut {
        sequences: ["Ctrl+E"]
        onActivated: if (win.currentId()) app.openEdit(win.currentId())
    }
    Shortcut {
        sequences: ["Ctrl+L"]
        onActivated: if (win.currentId()) app.swapLanguage(win.currentId())
    }
    Shortcut {
        sequences: ["Ctrl+D"]
        onActivated: if (win.currentId()) app.toggleFavorite(win.currentId())
    }
    Shortcut { sequences: ["Ctrl+1"]; onActivated: win.quickPaste(1) }
    Shortcut { sequences: ["Ctrl+2"]; onActivated: win.quickPaste(2) }
    Shortcut { sequences: ["Ctrl+3"]; onActivated: win.quickPaste(3) }
    Shortcut { sequences: ["Ctrl+4"]; onActivated: win.quickPaste(4) }
    Shortcut { sequences: ["Ctrl+5"]; onActivated: win.quickPaste(5) }
    Shortcut { sequences: ["Ctrl+6"]; onActivated: win.quickPaste(6) }
    Shortcut { sequences: ["Ctrl+7"]; onActivated: win.quickPaste(7) }
    Shortcut { sequences: ["Ctrl+8"]; onActivated: win.quickPaste(8) }
    Shortcut { sequences: ["Ctrl+9"]; onActivated: win.quickPaste(9) }

    Rectangle {
        anchors.fill: parent
        radius: 14
        color: Theme.surface
        border.width: 1
        border.color: Theme.border

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 7

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Image {
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    sourceSize.width: 40
                    sourceSize.height: 40
                    source: "qrc:/assets/cloppy.png"
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }

                Text {
                    text: "cloppy"
                    color: Theme.text
                    font.family: Theme.font
                    font.pixelSize: 15
                    font.bold: true
                }

                Text {
                    text: (app.popupView === "assets" ? app.popupAssets.length : popupModel.count) + Lang.t("개", " items")
                    color: Theme.faint
                    font.family: Theme.font
                    font.pixelSize: 10
                }

                Rectangle {
                    visible: app.secretMode
                    implicitWidth: secretLabel.implicitWidth + 12
                    implicitHeight: 19
                    radius: 7
                    color: Qt.rgba(Theme.red.r, Theme.red.g, Theme.red.b, 0.14)
                    border.width: 1
                    border.color: Theme.red

                    Text {
                        id: secretLabel
                        anchors.centerIn: parent
                        text: Lang.t("시크릿", "Secret")
                        color: Theme.red
                        font.family: Theme.font
                        font.pixelSize: 9
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: "✕"
                    color: Theme.muted
                    font.pixelSize: 15
                    padding: 4
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: app.closePopup()
                    }
                }
            }

            ClipSearchField {
                id: search
                Layout.fillWidth: true
                placeholder: app.popupView === "assets" ? Lang.t("검색…  에셋 이름", "Search…  asset name")
                                                         : (app.popupView === "snippets" ? Lang.t("검색…  코드 블록", "Search…  snippets")
                                                                                          : Lang.t("검색…  내용, 언어, 파일", "Search…  content, language, files"))
                onTextChanged: searchDebounce.restart()
                onAccepted: {
                    if (text.length > 0)
                        app.commitSearch(text)
                    win.pasteCurrent(false)
                }
                onEscapePressed: app.closePopup()
                onDownPressed: {
                    list.forceActiveFocus()
                    if (list.currentIndex < 0 && list.count > 0)
                        list.currentIndex = 0
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                ChipButton {
                    text: Lang.t("클립", "Clips")
                    checked: app.popupView === "clips"
                    onClicked: app.setPopupView("clips")
                }
                ChipButton {
                    text: Lang.t("코드", "Code")
                    checked: app.popupView === "snippets"
                    onClicked: app.setPopupView("snippets")
                }
                ChipButton {
                    text: Lang.t("에셋", "Assets")
                    checked: app.popupView === "assets"
                    onClicked: app.setPopupView("assets")
                }
                Item { Layout.fillWidth: true }
            }

            ListView {
                id: kindRow
                Layout.fillWidth: true
                visible: app.popupView === "clips"
                height: 30
                orientation: ListView.Horizontal
                spacing: 4
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: win.kindFilters

                delegate: ChipButton {
                    text: modelData.label
                    checked: app.popupKind === modelData.key
                    onClicked: {
                        app.setPopupKind(modelData.key)
                        list.currentIndex = 0
                    }
                }
            }

            ListView {
                Layout.fillWidth: true
                height: 30
                visible: app.popupView !== "assets" && app.quickFolders.length > 0
                orientation: ListView.Horizontal
                spacing: 4
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                model: {
                    var items = [{ id: -1, name: Lang.t("폴더 전체", "All folders") }]
                    for (var i = 0; i < app.quickFolders.length; ++i)
                        items.push(app.quickFolders[i])
                    return items
                }

                delegate: ChipButton {
                    text: modelData.name
                    checked: app.popupFolder === modelData.id
                    onClicked: {
                        app.setPopupFolder(modelData.id)
                        list.currentIndex = 0
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                RowLayout {
                    anchors.fill: parent
                    spacing: 8
                    visible: app.popupView !== "assets"

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true

                        ListView {
                            id: list
                            anchors.fill: parent
                            model: popupModel
                            clip: true
                            focus: true
                            spacing: 0
                            currentIndex: count > 0 ? 0 : -1
                            Keys.onPressed: function(event) { win.handleListKey(event) }

                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            delegate: ClipRow {
                                activateOnClick: true
                                onContextRequested: function(rowIndex) {
                                    contextMenu.target = popupModel.get(rowIndex)
                                    contextMenu.popup()
                                }
                                onActivated: function(clipId) { app.pasteItem(clipId, false) }
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            width: parent.width - 24
                            visible: popupModel.count === 0
                            text: app.popupView === "snippets"
                                  ? Lang.t("고정한 코드 블록이 없습니다\n전체 창에서 클립을 고정해 보세요", "No pinned snippets\nPin a clip in the main window")
                                  : Lang.t("아직 기록이 없습니다\n무엇이든 복사하면 여기에 쌓입니다", "Nothing recorded yet\nAnything you copy will appear here")
                            color: Theme.faint
                            font.family: Theme.font
                            font.pixelSize: 11
                            horizontalAlignment: Text.AlignHCenter
                            lineHeight: 1.35
                        }
                    }

                    PreviewPane {
                        visible: app.popupView === "clips" && app.popupPreview && win.width >= 640
                        Layout.fillHeight: true
                        Layout.preferredWidth: 280
                        item: win.currentItem
                    }
                }

                ListView {
                    id: assetList
                    anchors.fill: parent
                    visible: app.popupView === "assets"
                    clip: true
                    spacing: 2
                    model: app.popupAssets

                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: Rectangle {
                        width: ListView.view ? ListView.view.width : 340
                        height: 42
                        radius: 9
                        color: assetHover.hovered ? Theme.surface2 : "transparent"

                        HoverHandler { id: assetHover }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            Text {
                                text: modelData.kind === "image" ? "🖼" : (modelData.kind === "font" ? "🔤" : "📄")
                                font.pixelSize: 14
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.name
                                color: Theme.text
                                font.family: Theme.font
                                font.pixelSize: 12
                                elide: Text.ElideMiddle
                            }
                            Text {
                                text: modelData.sizeText || ""
                                color: Theme.faint
                                font.family: Theme.font
                                font.pixelSize: 9
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: app.copyAssetPath(modelData.id)
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    width: parent.width - 24
                    visible: app.popupView === "assets" && app.popupAssets.length === 0
                    text: Lang.t("저장된 에셋이 없습니다\n전체 창의 에셋 탭에서 추가하세요", "No assets saved\nAdd them in the Assets tab")
                    color: Theme.faint
                    font.family: Theme.font
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignHCenter
                    lineHeight: 1.35
                }
            }

            Text {
                Layout.fillWidth: true
                text: app.popupView === "assets"
                      ? Lang.t("클릭 = 에셋 경로 복사", "Click = copy asset path")
                      : (app.popupView === "snippets"
                         ? Lang.t("클릭 = 코드 블록 붙여넣기 · Del 삭제", "Click = paste snippet · Del to delete")
                         : Lang.t("클릭 = 붙여넣기 · Enter 붙여넣기 · Del 삭제", "Click = paste · Enter to paste · Del to delete"))
                color: Theme.faint
                font.family: Theme.font
                font.pixelSize: 9
                elide: Text.ElideRight
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 5

                ChipButton {
                    text: Lang.t("최근", "Recent")
                    checked: historyMenu.visible
                    onClicked: historyMenu.visible ? historyMenu.close() : historyMenu.popup()
                }
                ChipButton {
                    text: Lang.t("한/영", "Case")
                    onClicked: if (win.currentId()) app.swapLanguage(win.currentId())
                }
                Item { Layout.fillWidth: true }
                ChipButton {
                    text: Lang.t("전체 창", "Full window")
                    onClicked: app.openMain()
                }
                ChipButton {
                    text: app.secretMode ? "🔒" : Lang.t("시크릿", "Secret")
                    checked: app.secretMode
                    onClicked: app.toggleSecretMode()
                }
            }
        }
    }

    StyledMenu {
        id: historyMenu

        Repeater {
            model: app.searchHistory.length > 0 ? app.searchHistory : [Lang.t("검색 기록 없음", "No search history")]
            MenuItem {
                text: modelData
                enabled: app.searchHistory.length > 0
                onTriggered: {
                    search.text = modelData
                    app.setPopupQuery(modelData)
                    app.commitSearch(modelData)
                }
            }
        }
    }

    StyledMenu {
        id: contextMenu
        property var target: ({})

        MenuItem {
            text: Lang.t("붙여넣기", "Paste")
            onTriggered: app.pasteItem(contextMenu.target.clipId, false)
        }
        MenuItem {
            text: Lang.t("서식 없이 붙여넣기", "Paste without formatting")
            onTriggered: app.pasteItem(contextMenu.target.clipId, true)
        }
        StyledMenu {
            title: Lang.t("변환하여 붙여넣기", "Paste as")
            MenuItem { text: Lang.t("JSON (압축)", "JSON (compact)"); onTriggered: app.pasteAs(contextMenu.target.clipId, "json") }
            MenuItem { text: Lang.t("JSON (정리)", "JSON (pretty)"); onTriggered: app.pasteAs(contextMenu.target.clipId, "json-pretty") }
            MenuItem { text: Lang.t("HTML (줄바꿈 <br>)", "HTML (<br> line breaks)"); onTriggered: app.pasteAs(contextMenu.target.clipId, "html") }
            MenuItem { text: Lang.t("Markdown 코드블록", "Markdown code block"); onTriggered: app.pasteAs(contextMenu.target.clipId, "markdown") }
            MenuItem { text: Lang.t("한 줄로", "Single line"); onTriggered: app.pasteAs(contextMenu.target.clipId, "oneline") }
            MenuItem { text: Lang.t("따옴표 문자열", "Quoted string"); onTriggered: app.pasteAs(contextMenu.target.clipId, "quote") }
            MenuItem { text: "Base64"; onTriggered: app.pasteAs(contextMenu.target.clipId, "base64") }
        }
        MenuItem {
            text: Lang.t("복사", "Copy")
            onTriggered: app.copyItem(contextMenu.target.clipId)
        }
        MenuSeparator {}
        MenuItem {
            text: Lang.t("즐겨찾기 전환", "Toggle favorite")
            onTriggered: app.toggleFavorite(contextMenu.target.clipId)
        }
        MenuItem {
            text: Lang.t("편집…", "Edit…")
            onTriggered: app.openEdit(contextMenu.target.clipId)
        }
        MenuItem {
            text: Lang.t("한/영 교환", "Convert case")
            onTriggered: app.swapLanguage(contextMenu.target.clipId)
        }
        MenuItem {
            text: Lang.t("코드 블록으로 고정", "Pin as snippet")
            onTriggered: app.setPinned(contextMenu.target.clipId, true)
        }
        MenuSeparator {}
        StyledMenu {
            title: Lang.t("폴더로 이동", "Move to folder")
            Repeater {
                model: app.folders
                MenuItem {
                    text: win.folderIndent(modelData.depth) + modelData.name
                    onTriggered: app.moveToFolder(contextMenu.target.clipId, modelData.id)
                }
            }
            MenuSeparator {}
            MenuItem {
                text: Lang.t("폴더에서 빼기", "Remove from folder")
                onTriggered: app.moveToFolder(contextMenu.target.clipId, -1)
            }
        }
        MenuItem {
            text: Lang.t("삭제", "Delete")
            onTriggered: app.deleteItem(contextMenu.target.clipId)
        }
    }

    // Frameless windows have no system resize border, so drag the edges.
    MouseArea {
        width: 6
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
        cursorShape: Qt.SizeHorCursor
        preventStealing: true
        onPositionChanged: {
            if (pressed)
                win.width = Math.max(360, app.cursorPos().x - win.x + 4)
        }
        onReleased: app.setPopupSize(Math.round(win.width), Math.round(win.height))
    }

    MouseArea {
        height: 6
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeVerCursor
        preventStealing: true
        onPositionChanged: {
            if (pressed)
                win.height = Math.max(420, app.cursorPos().y - win.y + 4)
        }
        onReleased: app.setPopupSize(Math.round(win.width), Math.round(win.height))
    }

    MouseArea {
        width: 16
        height: 16
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        cursorShape: Qt.SizeFDiagCursor
        preventStealing: true
        onPositionChanged: {
            if (!pressed)
                return
            var p = app.cursorPos()
            win.width = Math.max(360, p.x - win.x + 4)
            win.height = Math.max(420, p.y - win.y + 4)
        }
        onReleased: app.setPopupSize(Math.round(win.width), Math.round(win.height))
    }
}
