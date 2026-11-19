import QtQuick
import Cloppy 1.0
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: win
    objectName: "cloppyMain"
    title: Lang.t("Cloppy — 클립보드 매니저", "Cloppy — Clipboard Manager")
    visible: app.mainVisible
    width: Math.min(1240, Screen.width - 40)
    height: Math.min(800, Screen.height - 80)
    x: Math.round((Screen.width - width) / 2)
    y: Math.round((Screen.height - height) / 2)
    minimumWidth: 940
    minimumHeight: 580
    color: Theme.bg

    Binding { target: Theme; property: "dark"; value: app.darkTheme }

    readonly property bool snippetMode: app.viewMode === "snippets"
    readonly property bool assetMode: app.viewMode === "assets"

    property var viewTabs: {
        var current = Lang.language
        return [
            { key: "clips", icon: "📋", label: Lang.t("클립보드", "Clips") },
            { key: "snippets", icon: "⌘", label: Lang.t("코드 블록", "Snippets") },
            { key: "assets", icon: "🗂", label: Lang.t("에셋", "Assets") }
        ]
    }

    property var smartFilters: {
        var current = Lang.language
        return [
            { key: "all", label: Lang.t("전체", "All") },
            { key: "favorites", label: Lang.t("즐겨찾기", "Favorites") },
            { key: "code", label: Lang.t("코드", "Code") },
            { key: "link", label: Lang.t("링크", "Links") },
            { key: "image", label: Lang.t("이미지", "Images") },
            { key: "files", label: Lang.t("파일", "Files") },
            { key: "command", label: Lang.t("명령어", "Commands") },
            { key: "color", label: Lang.t("색상", "Colors") },
            { key: "text", label: Lang.t("텍스트", "Text") }
        ]
    }

    property var currentItem: {
        var total = mainModel.count
        var index = list.currentIndex
        return index >= 0 && index < total ? mainModel.get(index) : ({})
    }

    property var currentAsset: {
        var total = app.assets.length
        var index = assetGrid.currentIndex
        return index >= 0 && index < total ? app.assets[index] : ({})
    }

    property var folderPalette: [
        "#7c6bff", "#4cc9f0", "#3ddc97", "#ffc857",
        "#ff6b81", "#f472b6", "#a78bfa", "#ff9f68"
    ]

    function smartCount(key) {
        if (key === "favorites")
            return app.counts.favorite || 0
        if (key === "all")
            return app.counts.all || 0
        return app.counts[key] || 0
    }

    function isSmartActive(key) {
        return !win.assetMode && !win.snippetMode && !app.mainSecret
                && app.mainFolder <= 0 && app.mainKind === key
    }

    function currentClipId() {
        return currentItem && currentItem.clipId !== undefined ? currentItem.clipId : 0
    }

    function indentFor(depth) {
        var text = ""
        for (var i = 0; i < depth; ++i)
            text += "   "
        return text
    }

    function folderName(id) {
        var source = win.assetMode ? app.assetFolders : app.folders
        for (var i = 0; i < source.length; ++i) {
            if (source[i].id === id)
                return source[i].name
        }
        return ""
    }

    function openFolderDialog(id, name, parentId, color, scope) {
        folderDialog.folderId = id
        folderDialog.parentId = parentId
        folderDialog.scope = scope || (win.assetMode ? "asset" : "clip")
        folderDialog.color = color || win.folderPalette[0]
        folderDialog.initialName = name || ""
        folderDialog.open()
    }

    function openAddFolder(parentId, scope) {
        openFolderDialog(0, "", parentId || -1, win.folderPalette[0], scope)
    }

    function openSnippet(id, title, language, content, folderId) {
        snippetDialog.editId = id || 0
        snippetDialog.folderId = folderId === undefined ? -1 : folderId
        snippetDialog.initialTitle = title || ""
        snippetDialog.initialLanguage = language || ""
        snippetDialog.initialContent = content || ""
        snippetDialog.open()
    }

    function handleListKey(event) {
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            if (currentClipId())
                app.copyItem(currentClipId())
            event.accepted = true
        } else if (event.key === Qt.Key_Delete) {
            if (currentClipId())
                app.deleteItem(currentClipId())
            event.accepted = true
        } else if (event.key === Qt.Key_F2) {
            if (currentClipId()) {
                var item = currentItem
                if (item.pinned)
                    openSnippet(item.clipId, item.title, item.language, item.content, item.folderId)
                else
                    app.openEdit(currentClipId())
            }
            event.accepted = true
        } else if (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_E) {
            if (currentClipId())
                app.openEdit(currentClipId())
            event.accepted = true
        } else if (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_L) {
            if (currentClipId())
                app.swapLanguage(currentClipId())
            event.accepted = true
        } else if (event.modifiers & Qt.ControlModifier && event.key === Qt.Key_D) {
            if (currentClipId())
                app.toggleFavorite(currentClipId())
            event.accepted = true
        }
    }

    onClosing: function(close) {
        close.accepted = false
        app.hideMain()
    }

    Shortcut { sequence: "Ctrl+F"; onActivated: search.forceActiveFocus() }
    Shortcut { sequence: "F1"; onActivated: app.openHelp() }
    Shortcut { sequence: "Ctrl+,"; onActivated: app.openSettings() }
    Shortcut { sequence: "Ctrl+P"; onActivated: app.setPopupPreview(!app.popupPreview) }
    Shortcut { sequence: "Ctrl+1"; onActivated: app.setViewMode("clips") }
    Shortcut { sequence: "Ctrl+2"; onActivated: app.setViewMode("snippets") }
    Shortcut { sequence: "Ctrl+3"; onActivated: app.setViewMode("assets") }
    Shortcut {
        sequence: "Ctrl+N"
        onActivated: win.snippetMode ? win.openSnippet(0, "", "", "", app.mainFolder)
                                     : win.openAddFolder(-1, win.assetMode ? "asset" : "clip")
    }
    Shortcut {
        sequence: "Ctrl+Shift+N"
        onActivated: {
            app.setViewMode("snippets")
            win.openSnippet(0, "", "", "", app.mainFolder)
        }
    }

    Timer {
        id: searchDebounce
        interval: 140
        onTriggered: app.setMainQuery(search.text)
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.bg

        RowLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillHeight: true
                Layout.preferredWidth: 248
                color: Theme.surface

                Rectangle {
                    anchors.right: parent.right
                    width: 1
                    height: parent.height
                    color: Theme.border
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14
                    anchors.rightMargin: 14
                    anchors.topMargin: 14
                    anchors.bottomMargin: 10
                    spacing: 6

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 9

                        Image {
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 32
                            sourceSize.width: 64
                            sourceSize.height: 64
                            source: "qrc:/assets/cloppy.png"
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }

                        Text {
                            text: "cloppy"
                            color: Theme.text
                            font.family: Theme.font
                            font.pixelSize: 21
                            font.bold: true
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Text {
                        text: app.hotkeyText + Lang.t("  ·  어디서든 열기", "  ·  Open anywhere")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 31
                        implicitHeight: 31
                        radius: 9
                        color: app.mainSecret ? Qt.rgba(Theme.red.r, Theme.red.g, Theme.red.b, 0.13)
                                             : (secretHover.hovered ? Theme.surface2 : "transparent")

                        HoverHandler { id: secretHover }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10

                            Text {
                                text: Lang.t("🔒  시크릿 클립", "🔒  Secret clips")
                                color: app.mainSecret ? Theme.red : Theme.muted
                                font.family: Theme.font
                                font.pixelSize: 12
                            }
                            Item { Layout.fillWidth: true }
                            Text {
                                text: (app.counts.secret || 0) > 0 ? app.counts.secret : ""
                                color: Theme.faint
                                font.family: Theme.font
                                font.pixelSize: 10
                            }
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (win.snippetMode || win.assetMode)
                                    app.setViewMode("clips")
                                app.setMainSecretView(!app.mainSecret)
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 31
                        implicitHeight: 31
                        radius: 9
                        color: settingsHover.hovered ? Theme.surface2 : "transparent"

                        HoverHandler { id: settingsHover }

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: Lang.t("⚙  설정", "⚙  Settings")
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 12
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: app.openSettings()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 31
                        implicitHeight: 31
                        radius: 9
                        color: helpHover.hovered ? Theme.surface2 : "transparent"

                        HoverHandler { id: helpHover }

                        Text {
                            anchors.left: parent.left
                            anchors.leftMargin: 10
                            anchors.verticalCenter: parent.verticalCenter
                            text: Lang.t("?  사용법", "?  Help")
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 12
                        }

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: app.openHelp()
                        }
                    }

                    Text {
                        Layout.topMargin: 6
                        visible: !win.assetMode
                        text: Lang.t("스마트 클립", "Smart clips")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                        font.letterSpacing: 0.8
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        visible: !win.assetMode
                        columns: 2
                        columnSpacing: 5
                        rowSpacing: 5

                        Repeater {
                            model: win.smartFilters

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 29
                                implicitHeight: 29
                                radius: 8
                                color: win.isSmartActive(modelData.key)
                                       ? Theme.accentSoft
                                       : (smartHover.hovered ? Theme.surface2 : "transparent")
                                border.width: win.isSmartActive(modelData.key) ? 1 : 0
                                border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.5)

                                HoverHandler { id: smartHover }

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    spacing: 5

                                    Rectangle {
                                        width: 5
                                        height: 5
                                        radius: 2.5
                                        color: Theme.kindColor(modelData.key === "all" ? "text"
                                                              : modelData.key === "favorites" ? "link"
                                                              : modelData.key)
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.label
                                        color: win.isSmartActive(modelData.key) ? Theme.text : Theme.muted
                                        font.family: Theme.font
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: win.smartCount(modelData.key) > 0 ? win.smartCount(modelData.key) : ""
                                        color: Theme.faint
                                        font.family: Theme.font
                                        font.pixelSize: 9
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        if (win.snippetMode)
                                            app.setViewMode("clips")
                                        app.setMainKind(modelData.key)
                                    }
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.topMargin: 12
                        spacing: 6

                        Text {
                            text: win.assetMode ? Lang.t("에셋 폴더", "Asset folders") : Lang.t("폴더", "Folders")
                            color: Theme.faint
                            font.family: Theme.font
                            font.pixelSize: 9
                            font.bold: true
                            font.letterSpacing: 0.8
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: win.snippetMode ? Lang.t("＋ 코드", "＋ Snippet") : "+"
                            color: Theme.muted
                            font.pixelSize: win.snippetMode ? 10 : 17
                            padding: 2
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (win.snippetMode)
                                        win.openSnippet(0, "", "", "", app.mainFolder)
                                    else
                                        win.openAddFolder(-1, win.assetMode ? "asset" : "clip")
                                }
                            }
                        }
                    }

                    ListView {
                        id: folderList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 48
                        clip: true
                        model: win.assetMode ? app.assetFolders : app.folders
                        spacing: 1

                        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                        delegate: Rectangle {
                            id: folderRow
                            width: ListView.view ? ListView.view.width : 210
                            height: 31
                            radius: 8
                            color: (win.assetMode ? app.assetFolder === modelData.id
                                                  : (!win.snippetMode && app.mainFolder === modelData.id))
                                   || (win.snippetMode && app.mainFolder === modelData.id)
                                   ? Theme.accentSoft
                                   : (folderHover.hovered ? Theme.surface2 : "transparent")

                            HoverHandler { id: folderHover }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 9 + Math.min(4, modelData.depth || 0) * 13
                                anchors.rightMargin: 9
                                spacing: 6

                                Text {
                                    visible: (modelData.depth || 0) > 0
                                    text: "↳"
                                    color: Theme.faint
                                    font.family: Theme.font
                                    font.pixelSize: 10
                                }

                                Rectangle {
                                    width: 7
                                    height: 7
                                    radius: 3.5
                                    color: app.colorFor(modelData.color)
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.name
                                    color: folderHover.hovered ? Theme.text : Theme.muted
                                    font.family: Theme.font
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: modelData.count > 0 ? modelData.count : ""
                                    color: Theme.faint
                                    font.family: Theme.font
                                    font.pixelSize: 10
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                acceptedButtons: Qt.LeftButton | Qt.RightButton
                                onClicked: function(mouse) {
                                    if (mouse.button === Qt.RightButton) {
                                        folderMenu.folder = modelData
                                        folderMenu.popup()
                                    } else if (win.assetMode) {
                                        app.setAssetFolder(modelData.id)
                                    } else {
                                        app.setMainFolder(modelData.id)
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: win.assetMode ? Lang.t("폴더 우클릭 · 이름 · 색상 · 정리", "Right-click a folder · name · color · organize")
                                            : Lang.t("폴더 우클릭 · 이름 · 색상 · 정리", "Right-click a folder · name · color · organize")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        elide: Text.ElideRight
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    anchors.topMargin: 18
                    anchors.bottomMargin: 12
                    spacing: 11

                    Item {
                        Layout.fillWidth: true
                        Layout.leftMargin: -20
                        Layout.rightMargin: -20
                        implicitHeight: 42

                        Rectangle {
                            anchors.fill: parent
                            color: Theme.bg
                        }

                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 1
                            color: Theme.border
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 2

                            Repeater {
                                model: win.viewTabs

                                Rectangle {
                                    id: topTab
                                    required property var modelData
                                    Layout.fillHeight: true
                                    implicitWidth: topTabRow.implicitWidth + 30
                                    color: topTabHover.hovered ? Theme.surface2 : "transparent"

                                    HoverHandler { id: topTabHover }

                                    Rectangle {
                                        visible: app.viewMode === topTab.modelData.key
                                        anchors.bottom: parent.bottom
                                        anchors.horizontalCenter: parent.horizontalCenter
                                        width: parent.width - 14
                                        height: 2
                                        radius: 1
                                        color: Theme.accent
                                    }

                                    RowLayout {
                                        id: topTabRow
                                        anchors.centerIn: parent
                                        spacing: 8

                                        Text {
                                            text: topTab.modelData.icon
                                            font.pixelSize: 13
                                        }
                                        Text {
                                            text: topTab.modelData.label
                                            color: app.viewMode === topTab.modelData.key ? Theme.text : Theme.muted
                                            font.family: Theme.font
                                            font.pixelSize: 13
                                            font.bold: app.viewMode === topTab.modelData.key
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: app.setViewMode(topTab.modelData.key)
                                    }
                                }
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        Text {
                            text: app.mainTitle
                            color: Theme.text
                            font.family: Theme.font
                            font.pixelSize: 21
                            font.bold: true
                        }
                        Rectangle {
                            implicitWidth: countLabel.implicitWidth + 16
                            implicitHeight: 22
                            radius: 11
                            color: Theme.surface2
                            border.width: 1
                            border.color: Theme.border

                            Text {
                                id: countLabel
                                anchors.centerIn: parent
                                text: win.assetMode ? app.assets.length + Lang.t("개", " items") : mainModel.count + Lang.t("개", " items")
                                color: Theme.muted
                                font.family: Theme.font
                                font.pixelSize: 10
                            }
                        }
                        Item { Layout.fillWidth: true }

                        ClipSearchField {
                            id: search
                            visible: !win.assetMode
                            Layout.preferredWidth: 320
                            placeholder: Lang.t("검색…  내용 · 언어 · 파일 이름", "Search…  content · language · file name")
                            onTextChanged: searchDebounce.restart()
                            onAccepted: {
                                if (text.length > 0)
                                    app.commitSearch(text)
                                app.setMainQuery(text)
                            }
                            onEscapePressed: {
                                search.text = ""
                                app.setMainQuery("")
                            }
                        }

                        ChipButton {
                            visible: win.assetMode
                            text: Lang.t("＋ 에셋 추가", "＋ Add assets")
                            primary: true
                            onClicked: app.addAssets()
                        }
                        ChipButton {
                            visible: win.snippetMode
                            text: Lang.t("＋ 새 코드 블록", "＋ New snippet")
                            primary: true
                            onClicked: win.openSnippet(0, "", "", "", app.mainFolder)
                        }

                        ChipButton {
                            visible: !win.assetMode
                            text: Lang.t("최근 검색", "Recent searches")
                            checked: mainHistoryMenu.visible
                            onClicked: mainHistoryMenu.visible ? mainHistoryMenu.close() : mainHistoryMenu.popup()
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 9

                        ChipButton {
                            visible: !win.assetMode
                            text: Lang.t("한/영 교환", "Convert case")
                            enabled: win.currentClipId() > 0
                            onClicked: app.swapLanguage(win.currentClipId())
                        }
                        ChipButton {
                            visible: !win.assetMode
                            text: Lang.t("선택 복사", "Copy selection")
                            enabled: win.currentClipId() > 0
                            onClicked: app.copyItem(win.currentClipId())
                        }
                        ChipButton {
                            visible: !win.assetMode
                            text: win.snippetMode ? Lang.t("코드 블록 편집", "Edit snippet") : Lang.t("편집", "Edit")
                            enabled: win.currentClipId() > 0
                            onClicked: {
                                var item = win.currentItem
                                if (win.snippetMode)
                                    win.openSnippet(item.clipId, item.title, item.language,
                                                    item.content, item.folderId)
                                else
                                    app.openEdit(win.currentClipId())
                            }
                        }
                        ChipButton {
                            visible: !win.assetMode
                            text: Lang.t("고정", "Pin")
                            checked: !!(win.currentItem && win.currentItem.pinned)
                            enabled: win.currentClipId() > 0
                            onClicked: app.setPinned(win.currentClipId(), !win.currentItem.pinned)
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            visible: !win.assetMode && win.width > 1150
                            text: win.snippetMode
                                  ? Lang.t("코드 블록은 정리·자동 삭제에서 보호됩니다", "Snippets are protected from cleanup and auto-delete")
                                  : Lang.t("클릭으로 선택 · 더블클릭 복사 · ", "Click to select · Double-click to copy · ") + (app.stats.bytesText || "0 B") + Lang.t(" 저장됨", " saved")
                            color: Theme.faint
                            font.family: Theme.font
                            font.pixelSize: 10
                            elide: Text.ElideRight
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 1
                        color: Theme.border
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: !win.assetMode
                        spacing: 12

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            ListView {
                                id: list
                                anchors.fill: parent
                                model: mainModel
                                clip: true
                                focus: true
                                spacing: 0
                                currentIndex: count > 0 ? 0 : -1
                                Keys.onPressed: function(event) { win.handleListKey(event) }

                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                delegate: ClipRow {
                                    onContextRequested: function(rowIndex) {
                                        itemMenu.target = mainModel.get(rowIndex)
                                        itemMenu.popup()
                                    }
                                    onActivated: function(clipId) { app.copyItem(clipId) }
                                }
                            }

                            Text {
                                anchors.centerIn: parent
                                visible: mainModel.count === 0
                                text: app.mainQuery.length > 0
                                      ? Lang.t("검색 결과가 없습니다", "No results")
                                      : (win.snippetMode
                                         ? Lang.t("코드 블록을 만들면 폴더에 보관됩니다", "Snippets you create are stored in folders")
                                         : Lang.t("복사한 내용이 여기에 쌓입니다", "Everything you copy will appear here"))
                                color: Theme.faint
                                font.family: Theme.font
                                font.pixelSize: 12
                            }
                        }

                        PreviewPane {
                            Layout.fillHeight: true
                            Layout.preferredWidth: 348
                            item: win.currentItem
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        visible: win.assetMode
                        spacing: 12

                        Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true

                            GridView {
                                id: assetGrid
                                anchors.fill: parent
                                cellWidth: Math.max(150, width / 3)
                                cellHeight: 148
                                clip: true
                                model: app.assets
                                currentIndex: count > 0 ? 0 : -1
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                delegate: Item {
                                    width: assetGrid.cellWidth
                                    height: assetGrid.cellHeight

                                    Rectangle {
                                        anchors.fill: parent
                                        anchors.margins: 5
                                        radius: 12
                                        color: assetGrid.currentIndex === index ? Theme.accentSoft
                                               : (assetHover.hovered ? Theme.surface2 : Theme.surface)
                                        border.width: 1
                                        border.color: assetGrid.currentIndex === index ? Theme.accent
                                                                                       : Theme.border

                                        HoverHandler { id: assetHover }

                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 10
                                            spacing: 6

                                            Rectangle {
                                                Layout.fillWidth: true
                                                Layout.preferredHeight: 68
                                                radius: 8
                                                color: Theme.thumb
                                                clip: true

                                                Image {
                                                    anchors.fill: parent
                                                    anchors.margins: 3
                                                    visible: modelData.kind === "image"
                                                    source: modelData.kind === "image" ? modelData.url : ""
                                                    fillMode: Image.PreserveAspectFit
                                                    asynchronous: true
                                                    cache: false
                                                }

                                                Text {
                                                    anchors.centerIn: parent
                                                    visible: modelData.kind !== "image"
                                                    text: modelData.ext.length > 0 ? modelData.ext : "FILE"
                                                    color: Theme.kindColor(modelData.kind)
                                                    font.family: "Consolas"
                                                    font.pixelSize: 15
                                                    font.bold: true
                                                }
                                            }

                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData.name
                                                color: Theme.text
                                                font.family: Theme.font
                                                font.pixelSize: 11
                                                elide: Text.ElideMiddle
                                            }

                                            Text {
                                                Layout.fillWidth: true
                                                text: modelData.sizeText
                                                color: Theme.faint
                                                font.family: Theme.font
                                                font.pixelSize: 9
                                            }
                                        }

                                        MouseArea {
                                            anchors.fill: parent
                                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: function(mouse) {
                                                assetGrid.currentIndex = index
                                                if (mouse.button === Qt.RightButton) {
                                                    assetMenu.asset = modelData
                                                    assetMenu.popup()
                                                }
                                            }
                                            onDoubleClicked: app.openAsset(modelData.assetId)
                                        }
                                    }
                                }
                            }

                            Text {
                                anchors.centerIn: parent
                                visible: app.assets.length === 0
                                text: Lang.t("에셋 추가로 이미지, 폰트, 코드 파일을 보관하세요", "Add assets to keep images, fonts and code files")
                                color: Theme.faint
                                font.family: Theme.font
                                font.pixelSize: 12
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 300
                            Layout.fillHeight: true
                            radius: Theme.radius
                            color: Theme.surface
                            border.width: 1
                            border.color: Theme.border

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 9

                                Text {
                                    Layout.fillWidth: true
                                    text: win.currentAsset.name !== undefined
                                          ? win.currentAsset.name : Lang.t("선택된 에셋 없음", "No asset selected")
                                    color: Theme.text
                                    font.family: Theme.font
                                    font.pixelSize: 13
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: win.currentAsset.sizeText !== undefined
                                          ? (win.currentAsset.ext + "  ·  " + win.currentAsset.sizeText)
                                          : Lang.t("왼쪽에서 에셋을 선택하세요", "Select an asset on the left")
                                    color: Theme.faint
                                    font.family: Theme.font
                                    font.pixelSize: 10
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: 1
                                    color: Theme.border
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    Layout.fillHeight: true
                                    radius: 10
                                    color: Theme.thumb
                                    clip: true

                                    Image {
                                        anchors.fill: parent
                                        anchors.margins: 6
                                        visible: win.currentAsset.kind === "image"
                                        source: win.currentAsset.kind === "image"
                                                ? win.currentAsset.url : ""
                                        fillMode: Image.PreserveAspectFit
                                        asynchronous: true
                                        cache: false
                                    }

                                    Text {
                                        anchors.centerIn: parent
                                        visible: win.currentAsset.kind !== "image"
                                        text: win.currentAsset.ext !== undefined
                                              ? win.currentAsset.ext : "FILE"
                                        color: Theme.muted
                                        font.family: "Consolas"
                                        font.pixelSize: 20
                                        font.bold: true
                                    }
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: win.currentAsset.path !== undefined ? win.currentAsset.path : ""
                                    color: Theme.faint
                                    font.family: Theme.font
                                    font.pixelSize: 9
                                    wrapMode: Text.WrapAnywhere
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }

                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    ChipButton {
                                        text: Lang.t("열기", "Open")
                                        primary: true
                                        enabled: win.currentAsset.assetId !== undefined
                                        onClicked: app.openAsset(win.currentAsset.assetId)
                                    }
                                    ChipButton {
                                        text: Lang.t("경로 복사", "Copy path")
                                        enabled: win.currentAsset.assetId !== undefined
                                        onClicked: app.copyAssetPath(win.currentAsset.assetId)
                                    }
                                    ChipButton {
                                        text: Lang.t("삭제", "Delete")
                                        enabled: win.currentAsset.assetId !== undefined
                                        onClicked: app.removeAsset(win.currentAsset.assetId)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    StyledMenu {
        id: mainHistoryMenu

        Repeater {
            model: app.searchHistory.length > 0 ? app.searchHistory : [Lang.t("검색 기록 없음", "No search history")]
            MenuItem {
                text: modelData
                enabled: app.searchHistory.length > 0
                onTriggered: {
                    search.text = modelData
                    app.setMainQuery(modelData)
                    app.commitSearch(modelData)
                }
            }
        }
    }

    StyledMenu {
        id: itemMenu
        property var target: ({})

        MenuItem {
            text: Lang.t("붙여넣기", "Paste")
            onTriggered: app.pasteItem(itemMenu.target.clipId, false)
        }
        MenuItem {
            text: Lang.t("서식 없이 붙여넣기", "Paste without formatting")
            onTriggered: app.pasteItem(itemMenu.target.clipId, true)
        }
        StyledMenu {
            title: Lang.t("변환하여 붙여넣기", "Paste as")
            MenuItem { text: Lang.t("JSON (압축)", "JSON (compact)"); onTriggered: app.pasteAs(itemMenu.target.clipId, "json") }
            MenuItem { text: Lang.t("JSON (정리)", "JSON (pretty)"); onTriggered: app.pasteAs(itemMenu.target.clipId, "json-pretty") }
            MenuItem { text: Lang.t("HTML (줄바꿈 <br>)", "HTML (<br> line breaks)"); onTriggered: app.pasteAs(itemMenu.target.clipId, "html") }
            MenuItem { text: Lang.t("Markdown 코드블록", "Markdown code block"); onTriggered: app.pasteAs(itemMenu.target.clipId, "markdown") }
            MenuItem { text: Lang.t("한 줄로", "Single line"); onTriggered: app.pasteAs(itemMenu.target.clipId, "oneline") }
            MenuItem { text: Lang.t("따옴표 문자열", "Quoted string"); onTriggered: app.pasteAs(itemMenu.target.clipId, "quote") }
            MenuItem { text: "Base64"; onTriggered: app.pasteAs(itemMenu.target.clipId, "base64") }
        }
        MenuItem {
            text: Lang.t("복사", "Copy")
            onTriggered: app.copyItem(itemMenu.target.clipId)
        }
        MenuSeparator {}
        MenuItem {
            text: Lang.t("한/영 교환", "Convert case")
            onTriggered: app.swapLanguage(itemMenu.target.clipId)
        }
        MenuItem {
            text: Lang.t("즐겨찾기 전환", "Toggle favorite")
            onTriggered: app.toggleFavorite(itemMenu.target.clipId)
        }
        MenuItem {
            text: itemMenu.target.pinned ? Lang.t("고정 해제", "Unpin") : Lang.t("코드 블록으로 고정", "Pin as snippet")
            onTriggered: app.setPinned(itemMenu.target.clipId, !itemMenu.target.pinned)
        }
        MenuItem {
            text: itemMenu.target.pinned ? Lang.t("코드 블록 편집…", "Edit snippet…") : Lang.t("편집…", "Edit…")
            onTriggered: {
                if (itemMenu.target.pinned)
                    win.openSnippet(itemMenu.target.clipId, itemMenu.target.title,
                                    itemMenu.target.language, itemMenu.target.content,
                                    itemMenu.target.folderId)
                else
                    app.openEdit(itemMenu.target.clipId)
            }
        }
        MenuSeparator {}
        StyledMenu {
            title: Lang.t("폴더로 이동", "Move to folder")
            Repeater {
                model: app.folders
                MenuItem {
                    text: win.indentFor(modelData.depth) + modelData.name
                    onTriggered: app.moveToFolder(itemMenu.target.clipId, modelData.id)
                }
            }
            MenuSeparator {}
            MenuItem {
                text: Lang.t("폴더에서 빼기", "Remove from folder")
                onTriggered: app.moveToFolder(itemMenu.target.clipId, -1)
            }
        }
        MenuItem {
            text: Lang.t("삭제", "Delete")
            onTriggered: app.deleteItem(itemMenu.target.clipId)
        }
    }

    StyledMenu {
        id: assetMenu
        property var asset: ({})

        MenuItem {
            text: Lang.t("열기", "Open")
            onTriggered: app.openAsset(assetMenu.asset.assetId)
        }
        MenuItem {
            text: Lang.t("경로 복사", "Copy path")
            onTriggered: app.copyAssetPath(assetMenu.asset.assetId)
        }
        StyledMenu {
            title: Lang.t("폴더로 이동", "Move to folder")
            Repeater {
                model: app.assetFolders
                MenuItem {
                    text: win.indentFor(modelData.depth) + modelData.name
                    onTriggered: app.moveAsset(assetMenu.asset.assetId, modelData.id)
                }
            }
            MenuSeparator {}
            MenuItem {
                text: Lang.t("폴더에서 빼기", "Remove from folder")
                onTriggered: app.moveAsset(assetMenu.asset.assetId, -1)
            }
        }
        MenuSeparator {}
        MenuItem {
            text: Lang.t("삭제", "Delete")
            onTriggered: app.removeAsset(assetMenu.asset.assetId)
        }
    }

    StyledMenu {
        id: folderMenu
        property var folder: ({})

        MenuItem {
            text: Lang.t("하위 폴더 만들기", "New subfolder")
            onTriggered: win.openAddFolder(folderMenu.folder.id,
                                           folderMenu.folder.scope === "asset" ? "asset" : "clip")
        }
        MenuItem {
            text: Lang.t("이름 · 색상 편집", "Edit name · color")
            onTriggered: win.openFolderDialog(folderMenu.folder.id, folderMenu.folder.name,
                                              folderMenu.folder.parentId, folderMenu.folder.color,
                                              folderMenu.folder.scope === "asset" ? "asset" : "clip")
        }
        StyledMenu {
            title: Lang.t("다른 폴더로 이동", "Move to another folder")
            MenuItem {
                text: Lang.t("최상위로", "Move to top level")
                onTriggered: app.moveFolder(folderMenu.folder.id, -1)
            }
            Repeater {
                model: folderMenu.folder.scope === "asset" ? app.assetFolders : app.folders
                MenuItem {
                    text: win.indentFor(modelData.depth) + modelData.name
                    enabled: modelData.id !== folderMenu.folder.id
                    onTriggered: app.moveFolder(folderMenu.folder.id, modelData.id)
                }
            }
        }
        MenuSeparator {}
        MenuItem {
            text: Lang.t("폴더 삭제", "Delete folder")
            onTriggered: {
                folderDeleteDialog.folderId = folderMenu.folder.id
                folderDeleteDialog.folderName = folderMenu.folder.name
                folderDeleteDialog.open()
            }
        }
    }

    StyledMenu {
        id: snippetFolderMenu

        Repeater {
            model: app.folders
            MenuItem {
                text: win.indentFor(modelData.depth) + modelData.name
                onTriggered: snippetDialog.folderId = modelData.id
            }
        }
        MenuSeparator {}
        MenuItem {
            text: Lang.t("폴더 없음", "No folders")
            onTriggered: snippetDialog.folderId = -1
        }
    }

    Dialog {
        id: folderDialog
        property int folderId: 0
        property int parentId: -1
        property string scope: "clip"
        property string initialName: ""
        property string color: "#7c6bff"
        modal: true
        focus: true
        width: 390
        parent: Overlay.overlay
        anchors.centerIn: parent
        padding: 20
        closePolicy: Popup.CloseOnEscape

        background: Rectangle {
            color: Theme.surface
            radius: 16
            border.width: 1
            border.color: Theme.border
        }

        contentItem: ColumnLayout {
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Rectangle {
                    width: 22
                    height: 22
                    radius: 11
                    color: folderDialog.color
                    border.width: 2
                    border.color: Qt.rgba(1, 1, 1, 0.24)
                }

                ColumnLayout {
                    spacing: 1
                    Text {
                        text: folderDialog.folderId > 0 ? Lang.t("폴더 편집", "Edit folder")
                              : (folderDialog.parentId > 0 ? Lang.t("하위 폴더 만들기", "New subfolder") : Lang.t("새 폴더", "New folder"))
                        color: Theme.text
                        font.family: Theme.font
                        font.pixelSize: 15
                        font.bold: true
                    }
                    Text {
                        visible: folderDialog.parentId > 0
                        text: Lang.t("상위 폴더 · ", "Parent folder · ") + win.folderName(folderDialog.parentId)
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                    }
                }
                Item { Layout.fillWidth: true }
            }

            Field {
                id: folderName
                Layout.fillWidth: true
                placeholderText: Lang.t("폴더 이름", "Folder name")
                onAccepted: folderDialog.commit()
            }

            Text {
                text: Lang.t("색상", "Colors")
                color: Theme.faint
                font.family: Theme.font
                font.pixelSize: 9
                font.bold: true
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: win.folderPalette
                    Rectangle {
                        width: 26
                        height: 26
                        radius: 13
                        color: modelData
                        border.width: folderDialog.color === modelData ? 3 : 1
                        border.color: folderDialog.color === modelData
                                      ? Theme.text
                                      : Qt.rgba(1, 1, 1, 0.12)

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: folderDialog.color = modelData
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                radius: 10
                color: folderDialog.color
                border.width: 1
                border.color: Theme.border

                Text {
                    anchors.centerIn: parent
                    text: folderDialog.color.toUpperCase()
                    color: "#ffffff"
                    font.family: "Consolas"
                    font.pixelSize: 12
                    font.bold: true
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Item { Layout.fillWidth: true }
                ChipButton {
                    text: Lang.t("취소", "Cancel")
                    onClicked: folderDialog.close()
                }
                ChipButton {
                    text: Lang.t("저장", "Save")
                    primary: true
                    onClicked: folderDialog.commit()
                }
            }
        }

        onOpened: {
            folderName.text = initialName
            folderName.forceActiveFocus()
        }

        function commit() {
            var name = folderName.text.trim()
            if (name.length === 0)
                return
            if (folderId > 0) {
                app.renameFolder(folderId, name)
                app.setFolderColor(folderId, color)
            } else {
                app.addFolder(name, parentId, color, scope)
            }
            close()
        }
    }

    Dialog {
        id: snippetDialog
        property int editId: 0
        property int folderId: -1
        property string initialTitle: ""
        property string initialLanguage: ""
        property string initialContent: ""
        modal: true
        focus: true
        width: 640
        height: 560
        parent: Overlay.overlay
        anchors.centerIn: parent
        padding: 18
        closePolicy: Popup.CloseOnEscape

        background: Rectangle {
            color: Theme.surface
            radius: 16
            border.width: 1
            border.color: Theme.border
        }

        contentItem: ColumnLayout {
            spacing: 10

            Text {
                text: snippetDialog.editId > 0 ? Lang.t("코드 블록 편집", "Edit snippet") : Lang.t("새 코드 블록", "New snippet")
                color: Theme.text
                font.family: Theme.font
                font.pixelSize: 16
                font.bold: true
            }

            Text {
                Layout.fillWidth: true
                text: Lang.t("제목과 언어를 지정해 보관합니다. 코드 블록은 자동 정리에서 삭제되지 않습니다.", "Save it with a title and language. Snippets are never removed by automatic cleanup.")
                color: Theme.faint
                font.family: Theme.font
                font.pixelSize: 10
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Field {
                    id: snippetTitle
                    Layout.fillWidth: true
                    placeholderText: Lang.t("제목 (예: 배포 스크립트)", "Title (e.g. deploy script)")
                }
                Field {
                    id: snippetLanguage
                    Layout.preferredWidth: 130
                    placeholderText: Lang.t("언어 (python)", "Language (python)")
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: Lang.t("폴더", "Folders")
                    color: Theme.faint
                    font.family: Theme.font
                    font.pixelSize: 10
                }
                ChipButton {
                    text: snippetDialog.folderId > 0 ? win.folderName(snippetDialog.folderId) : Lang.t("폴더 없음", "No folders")
                    onClicked: snippetFolderMenu.popup()
                }
                Item { Layout.fillWidth: true }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.thumb
                radius: 10
                border.width: 1
                border.color: Theme.border

                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 8
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AsNeeded

                    TextArea {
                        id: snippetBody
                        width: parent.width
                        wrapMode: TextEdit.NoWrap
                        selectByMouse: true
                        color: Theme.text
                        selectionColor: Theme.accent
                        selectedTextColor: Theme.accentText
                        font.family: "Cascadia Mono"
                        font.pixelSize: 12
                        background: null
                        padding: 6
                        placeholderText: Lang.t("코드를 붙여넣으세요", "Paste your code")
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                ChipButton {
                    text: Lang.t("한/영 교환", "Convert case")
                    onClicked: snippetBody.text = app.swapText(snippetBody.text)
                }

                Item { Layout.fillWidth: true }

                ChipButton {
                    text: Lang.t("취소", "Cancel")
                    onClicked: snippetDialog.close()
                }
                ChipButton {
                    text: Lang.t("저장", "Save")
                    primary: true
                    onClicked: {
                        if (snippetBody.text.trim().length === 0)
                            return
                        if (snippetDialog.editId > 0)
                            app.updateSnippet(snippetDialog.editId, snippetTitle.text,
                                              snippetBody.text, snippetLanguage.text,
                                              snippetDialog.folderId)
                        else
                            app.addSnippet(snippetTitle.text, snippetBody.text,
                                           snippetLanguage.text, snippetDialog.folderId)
                        snippetDialog.close()
                    }
                }
            }
        }

        onOpened: {
            snippetTitle.text = initialTitle
            snippetLanguage.text = initialLanguage
            snippetBody.text = initialContent
            snippetTitle.forceActiveFocus()
        }
    }

    Dialog {
        id: folderDeleteDialog
        property int folderId: 0
        property string folderName: ""
        modal: true
        focus: true
        width: 370
        parent: Overlay.overlay
        anchors.centerIn: parent
        padding: 20

        background: Rectangle {
            color: Theme.surface
            radius: 16
            border.width: 1
            border.color: Theme.border
        }

        contentItem: ColumnLayout {
            spacing: 10
            Text {
                text: Lang.t("폴더 삭제", "Delete folder")
                color: Theme.text
                font.family: Theme.font
                font.pixelSize: 15
                font.bold: true
            }
            Text {
                Layout.fillWidth: true
                text: "'" + folderDeleteDialog.folderName + Lang.t("' 폴더를 삭제할까요?\n하위 폴더도 함께 삭제됩니다. 항목은 지워지지 않고 폴더에서만 빠집니다.", "' Delete this folder?\nSubfolders are deleted too. Items are kept and only removed from the folder.")
                color: Theme.muted
                font.family: Theme.font
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
            RowLayout {
                spacing: 8
                Item { Layout.fillWidth: true }
                ChipButton {
                    text: Lang.t("취소", "Cancel")
                    onClicked: folderDeleteDialog.close()
                }
                ChipButton {
                    text: Lang.t("삭제", "Delete")
                    primary: true
                    onClicked: {
                        app.deleteFolder(folderDeleteDialog.folderId)
                        folderDeleteDialog.close()
                    }
                }
            }
        }
    }
}
