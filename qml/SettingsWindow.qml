import QtQuick
import Cloppy 1.0
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: settingsWindow
    objectName: "cloppySettings"
    title: Lang.t("Cloppy 설정", "Cloppy Settings")
    visible: app.settingsVisible
    width: Math.min(760, Screen.width - 40)
    height: Math.min(800, Screen.height - 60)
    minimumWidth: 640
    minimumHeight: 480
    color: Theme.bg
    flags: Qt.Dialog
    modality: Qt.ApplicationModal
    x: Math.round((Screen.width - width) / 2)
    y: Math.round((Screen.height - height) / 2)

    Binding { target: Theme; property: "dark"; value: app.darkTheme }

    property bool dark: app.darkTheme
    property string language: "en"
    property string swapDirection: "auto"
    property var values: ({})
    property var expansionRows: []
    property var quickFolderIds: []

    function isQuickFolder(id) {
        return quickFolderIds.indexOf(id) >= 0
    }

    function toggleQuickFolder(id) {
        var ids = quickFolderIds.slice()
        var at = ids.indexOf(id)
        if (at >= 0)
            ids.splice(at, 1)
        else
            ids.push(id)
        quickFolderIds = ids
    }

    function setRow(rowIndex, key, value) {
        var rows = expansionRows.slice()
        var row = rows[rowIndex]
        rows[rowIndex] = { trigger: row.trigger, expansion: row.expansion, enabled: row.enabled }
        rows[rowIndex][key] = value
        expansionRows = rows
    }

    function removeRow(rowIndex) {
        var rows = expansionRows.slice()
        rows.splice(rowIndex, 1)
        expansionRows = rows
    }

    function addRow() {
        expansionRows = expansionRows.concat([{ trigger: "", expansion: "", enabled: true }])
    }

    function load() {
        values = app.settings
        dark = values.darkTheme !== false
        language = values.language === "ko" ? "ko" : "en"
        quickField.setSpec(values.hotkey || "win+v")
        langField.setSpec(values.langswapHotkey || "")
        swapField.setSpec(values.selectionHotkey === undefined ? "ctrl+alt+l"
                                                               : values.selectionHotkey)
        swapDirection = values.swapDirection || "auto"
        blockWinV.checked = values.blockWinV !== false
        disableHistory.checked = values.disableWinHistory !== false
        autostart.checked = values.autostart === true
        showPreview.checked = values.showPreviewInPopup !== false
        var chosen = String(values.popupFolders || "").split(",")
        var ids = []
        for (var f = 0; f < chosen.length; ++f) {
            var folderNumber = parseInt(chosen[f], 10)
            if (!isNaN(folderNumber) && folderNumber > 0)
                ids.push(folderNumber)
        }
        quickFolderIds = ids
        closeAfter.checked = values.closeAfterPaste !== false
        notifications.checked = values.notifications !== false
        programmerMode.checked = values.programmerMode === true
        terminalCleanCopy.checked = values.terminalCleanCopy !== false
        terminalPasteChord.checked = values.terminalPasteChord !== false
        terminalPlainPaste.checked = values.terminalPlainPaste !== false
        snippetsEnabled.checked = values.snippetsEnabled !== false
        maxItems.text = String(values.maxItems || 3000)
        secretTtl.text = String(values.secretTtlHours === undefined ? 24 : values.secretTtlHours)
        pasteDelay.text = String(values.pasteDelayMs === undefined ? 120 : values.pasteDelayMs)
        ignoreApps.text = values.ignoreApps || ""
        terminalApps.text = values.terminalApps || ""

        var rows = []
        var list = app.expansions
        for (var i = 0; i < list.length; ++i) {
            rows.push({
                trigger: list[i].trigger,
                expansion: list[i].expansion,
                enabled: list[i].enabled !== false
            })
        }
        expansionRows = rows
        errorText.text = ""
    }

    function save() {
        if (!quickField.spec) {
            errorText.text = Lang.t("빠른 창 단축키를 지정해 주세요.", "Please set a shortcut for the quick window.")
            return
        }
        if (langField.spec && app.formatHotkey(quickField.spec) === app.formatHotkey(langField.spec)) {
            errorText.text = Lang.t("두 단축키가 같습니다. 다른 조합을 사용해 주세요.", "Both shortcuts are the same. Choose a different combination.")
            return
        }
        if (swapField.spec && app.formatHotkey(quickField.spec) === app.formatHotkey(swapField.spec)) {
            errorText.text = Lang.t("선택 영역 단축키가 빠른 창 단축키와 같습니다.", "The selection shortcut is the same as the quick window shortcut.")
            return
        }
        if (langField.spec && swapField.spec
                && app.formatHotkey(langField.spec) === app.formatHotkey(swapField.spec)) {
            errorText.text = Lang.t("클립보드 변환과 선택 변환이 같은 조합입니다. 하나를 바꿔 주세요.", "Clipboard and selection conversion use the same combination. Change one of them.")
            return
        }

        var rows = []
        for (var i = 0; i < expansionRows.length; ++i) {
            var row = expansionRows[i]
            if (String(row.trigger).trim().length > 0)
                rows.push({ trigger: row.trigger, expansion: row.expansion, enabled: row.enabled })
        }
        app.saveExpansions(rows)

        app.saveSettings({
            hotkey: quickField.spec,
            langswapHotkey: langField.spec,
            selectionHotkey: swapField.spec,
            swapDirection: settingsWindow.swapDirection,
            darkTheme: dark,
            language: language,
            blockWinV: blockWinV.checked,
            disableWinHistory: disableHistory.checked,
            autostart: autostart.checked,
            showPreviewInPopup: showPreview.checked,
            popupFolders: quickFolderIds.join(","),
            closeAfterPaste: closeAfter.checked,
            notifications: notifications.checked,
            programmerMode: programmerMode.checked,
            terminalCleanCopy: terminalCleanCopy.checked,
            terminalPasteChord: terminalPasteChord.checked,
            terminalPlainPaste: terminalPlainPaste.checked,
            snippetsEnabled: snippetsEnabled.checked,
            maxItems: parseInt(maxItems.text) || 3000,
            secretTtlHours: parseInt(secretTtl.text) || 0,
            pasteDelayMs: parseInt(pasteDelay.text) || 120,
            ignoreApps: ignoreApps.text,
            terminalApps: terminalApps.text
        })
    }

    onVisibleChanged: {
        if (visible) {
            load()
            requestActivate()
        } else {
            app.setCapturing(false)
        }
    }

    onClosing: function(close) {
        close.accepted = false
        app.closeSettings()
    }

    Shortcut {
        sequence: "Escape"
        onActivated: app.closeSettings()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.bg

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 22
            spacing: 12

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: Lang.t("환경 설정", "Preferences")
                    color: Theme.text
                    font.family: Theme.font
                    font.pixelSize: 23
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "✕"
                    color: Theme.muted
                    font.pixelSize: 17
                    padding: 4
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: app.closeSettings()
                    }
                }
            }

            Text {
                text: Lang.t("단축키, 개발자 도구, 텍스트 확장, 저장 공간을 관리합니다.", "Manage shortcuts, developer tools, text expansion and storage.")
                color: Theme.muted
                font.family: Theme.font
                font.pixelSize: 11
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: Theme.border
            }

            ScrollView {
                id: settingsScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    width: settingsScroll.availableWidth - 8
                    spacing: 10

                    Text {
                        text: Lang.t("모양", "Appearance")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    RowLayout {
                        spacing: 6
                        ChipButton {
                            text: Lang.t("다크", "Dark")
                            checked: settingsWindow.dark
                            onClicked: settingsWindow.dark = true
                        }
                        ChipButton {
                            text: Lang.t("라이트", "Light")
                            checked: !settingsWindow.dark
                            onClicked: settingsWindow.dark = false
                        }
                        Item { Layout.fillWidth: true }
                        ToggleCheck {
                            id: showPreview
                            label: Lang.t("빠른 창을 넓혔을 때 미리보기 표시", "Show preview when the quick window is widened")
                        }
                    }

                    RowLayout {
                        spacing: 6
                        Text {
                            Layout.preferredWidth: 150
                            text: "언어"
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                        ChipButton {
                            text: "English"
                            checked: settingsWindow.language === "en"
                            onClicked: settingsWindow.language = "en"
                        }
                        ChipButton {
                            text: "한국어"
                            checked: settingsWindow.language === "ko"
                            onClicked: settingsWindow.language = "ko"
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    Text {
                        text: Lang.t("빠른 창 폴더", "Quick window folders")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 6

                        Repeater {
                            model: app.folders
                            delegate: ChipButton {
                                text: modelData.name
                                checked: settingsWindow.isQuickFolder(modelData.id)
                                onClicked: settingsWindow.toggleQuickFolder(modelData.id)
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: app.folders.length > 0
                              ? Lang.t("선택한 폴더가 빠른 창에 버튼으로 표시됩니다. 여러 개를 고를 수 있습니다.", "Chosen folders appear as buttons in the quick window. You can pick several.")
                              : Lang.t("폴더를 먼저 만들면 여기서 빠른 창에 표시할 폴더를 고를 수 있습니다.", "Create a folder first to choose which folders appear in the quick window.")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    Text {
                        text: Lang.t("전역 단축키", "Global shortcuts")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    RowLayout {
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 150
                            text: Lang.t("빠른 클립 창", "Quick clip window")
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                        HotkeyField {
                            id: quickField
                            Layout.fillWidth: true
                        }
                        ChipButton {
                            text: "Win+V"
                            onClicked: quickField.setSpec("win+v")
                        }
                    }

                    RowLayout {
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 150
                            text: Lang.t("클립보드 한/영 교환", "Convert clipboard case")
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                        HotkeyField {
                            id: langField
                            Layout.fillWidth: true
                        }
                        ChipButton {
                            text: Lang.t("해제", "Clear")
                            onClicked: langField.setSpec("")
                        }
                    }

                    RowLayout {
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 150
                            text: Lang.t("선택 영역 한/영 교환", "Convert selection case")
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                        HotkeyField {
                            id: swapField
                            Layout.fillWidth: true
                        }
                        ChipButton {
                            text: Lang.t("해제", "Clear")
                            onClicked: swapField.setSpec("")
                        }
                    }

                    RowLayout {
                        spacing: 6
                        Text {
                            Layout.preferredWidth: 150
                            text: Lang.t("선택 영역 변환 방향", "Selection conversion direction")
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                        ChipButton {
                            text: Lang.t("자동", "Auto")
                            checked: settingsWindow.swapDirection === "auto"
                            onClicked: settingsWindow.swapDirection = "auto"
                        }
                        ChipButton {
                            text: Lang.t("→ 한글", "→ Hangul")
                            checked: settingsWindow.swapDirection === "ko"
                            onClicked: settingsWindow.swapDirection = "ko"
                        }
                        ChipButton {
                            text: Lang.t("→ 영문", "→ English")
                            checked: settingsWindow.swapDirection === "en"
                            onClicked: settingsWindow.swapDirection = "en"
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: Lang.t("자동: 한글이면 영문 자판으로, 영문이면 한글로 알아서 바꿉니다. Ctrl+Alt+L은 드래그한 선택 영역을 복사 없이 바로 바꿔 붙여넣습니다.", "Auto converts Hangul to the English layout and English to Hangul. Ctrl+Alt+L converts the dragged selection in place and restores the previous clipboard afterwards.")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        Layout.fillWidth: true
                        text: Lang.t("단축키 상태 · ", "Hotkey status · ") + app.hotkeyStatus
                        color: app.hotkeyStatus.indexOf(Lang.t("실패", "Failed")) >= 0 ? Theme.red : Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    Text {
                        text: Lang.t("동작", "Behavior")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    ToggleCheck { id: autostart; label: Lang.t("Windows 시작 시 Cloppy 자동 실행", "Start Cloppy with Windows") }
                    ToggleCheck { id: closeAfter; label: Lang.t("붙여넣은 뒤 빠른 창 닫기", "Close the quick window after pasting") }
                    ToggleCheck { id: notifications; label: Lang.t("알림 표시 (복사·변환·저장 안내)", "Show notifications (copy, convert, save)") }

                    RowLayout {
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 150
                            text: Lang.t("붙여넣기 지연", "Paste delay")
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                        Field {
                            id: pasteDelay
                            Layout.preferredWidth: 90
                            validator: IntValidator { bottom: 0; top: 2000 }
                        }
                        Text {
                            text: "ms"
                            color: Theme.faint
                            font.family: Theme.font
                            font.pixelSize: 10
                        }
                        Item { Layout.fillWidth: true }
                    }

                    RowLayout {
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 150
                            text: Lang.t("최대 보관 개수", "Maximum items")
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                        Field {
                            id: maxItems
                            Layout.preferredWidth: 110
                            validator: IntValidator { bottom: 100; top: 100000 }
                        }
                        Item { Layout.fillWidth: true }
                    }

                    RowLayout {
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 150
                            text: Lang.t("시크릿 자동 삭제", "Auto-delete secrets")
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                        Field {
                            id: secretTtl
                            Layout.preferredWidth: 90
                            validator: IntValidator { bottom: 0; top: 720 }
                        }
                        Text {
                            text: Lang.t("시간 (0 = 삭제 안 함)", "hours (0 = never)")
                            color: Theme.faint
                            font.family: Theme.font
                            font.pixelSize: 10
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Text {
                        text: Lang.t("캡처 제외 앱", "Ignored apps")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    Field {
                        id: ignoreApps
                        Layout.fillWidth: true
                        placeholderText: "1password.exe, bitwarden.exe"
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    Text {
                        text: Lang.t("개발자", "Developer")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    ToggleCheck {
                        id: programmerMode
                        Layout.fillWidth: true
                        label: Lang.t("프로그래머 모드 (터미널 전용 동작 사용)", "Programmer mode (terminal-specific behavior)")
                    }

                    ToggleCheck {
                        id: terminalCleanCopy
                        Layout.fillWidth: true
                        enabled: programmerMode.checked
                        label: Lang.t("터미널 복사 정리 — ANSI 색상·끝 줄바꿈/공백 제거", "Clean terminal copies — strip ANSI colors and trailing newlines/spaces")
                    }
                    ToggleCheck {
                        id: terminalPasteChord
                        Layout.fillWidth: true
                        enabled: programmerMode.checked
                        label: Lang.t("터미널에서 Ctrl+Shift+V / Ctrl+Shift+C 사용", "Use Ctrl+Shift+V / Ctrl+Shift+C in terminals")
                    }
                    ToggleCheck {
                        id: terminalPlainPaste
                        Layout.fillWidth: true
                        enabled: programmerMode.checked
                        label: Lang.t("터미널에는 항상 서식 없이 붙여넣기", "Always paste into terminals without formatting")
                    }

                    Text {
                        Layout.fillWidth: true
                        text: Lang.t("터미널 앱은 아래 목록으로 판단합니다. 각 항목은 따로 켜고 끌 수 있습니다.", "Terminal apps are matched against the list below. Each behavior can be toggled separately.")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }

                    RowLayout {
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 150
                            text: Lang.t("터미널 앱", "Terminal apps")
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                        Field {
                            id: terminalApps
                            Layout.fillWidth: true
                            placeholderText: "WindowsTerminal.exe, wt.exe, cmd.exe"
                        }
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: Lang.t("텍스트 확장 단축어", "Text expansion triggers")
                            color: Theme.faint
                            font.family: Theme.font
                            font.pixelSize: 9
                            font.bold: true
                        }
                        Item { Layout.fillWidth: true }
                        ToggleCheck {
                            id: snippetsEnabled
                            label: Lang.t("사용", "Use")
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: Lang.t("어디서든 단축어를 입력하고 Space를 누르면 문구로 바뀝니다. 예: !mail → 내 메일 주소. 클릭해서 바로 수정할 수 있습니다.", "Type a trigger anywhere and press Space to replace it. Example: !mail -> my email address. Click to edit.")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5

                        Repeater {
                            model: settingsWindow.expansionRows

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 6

                                Field {
                                    Layout.preferredWidth: 118
                                    text: modelData.trigger
                                    placeholderText: "!mail"
                                    onEditingFinished: settingsWindow.setRow(index, "trigger", text)
                                }
                                Field {
                                    Layout.fillWidth: true
                                    text: modelData.expansion
                                    placeholderText: "me@example.com"
                                    onEditingFinished: settingsWindow.setRow(index, "expansion", text)
                                }
                                ToggleCheck {
                                    checked: modelData.enabled
                                    onToggled: settingsWindow.setRow(index, "enabled", checked)
                                }
                                Text {
                                    text: "✕"
                                    color: Theme.muted
                                    font.pixelSize: 14
                                    padding: 4
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: settingsWindow.removeRow(index)
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            ChipButton {
                                text: Lang.t("＋ 단축어 추가", "＋ Add trigger")
                                onClicked: settingsWindow.addRow()
                            }
                            Item { Layout.fillWidth: true }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    Text {
                        text: Lang.t("Windows 기본 클립보드", "Windows clipboard")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    ToggleCheck {
                        id: blockWinV
                        Layout.fillWidth: true
                        label: Lang.t("Win+V를 가로채 Cloppy를 열고 기본 클립보드 차단", "Intercept Win+V and open Cloppy instead of the Windows panel")
                    }
                    ToggleCheck {
                        id: disableHistory
                        Layout.fillWidth: true
                        label: Lang.t("Windows 클립보드 기록 저장 끄기 (레지스트리)", "Turn off Windows clipboard history (registry)")
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    Text {
                        text: Lang.t("데이터", "Data")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: app.dataPath
                        color: Theme.muted
                        font.family: Theme.font
                        font.pixelSize: 10
                        wrapMode: Text.WrapAnywhere
                    }

                    Text {
                        Layout.fillWidth: true
                        text: (app.stats.items || 0) + Lang.t("개 항목 · ", " items · ") + (app.stats.bytesText || "0 B")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        ChipButton {
                            text: Lang.t("데이터 폴더 열기", "Open data folder")
                            onClicked: app.openDataFolder()
                        }
                        ChipButton {
                            text: Lang.t("검색 기록 지우기", "Clear search history")
                            onClicked: app.clearSearches()
                        }
                        ChipButton {
                            text: Lang.t("즐겨찾기 외 삭제", "Delete all but favorites")
                            onClicked: confirmClear.open()
                        }
                    }

                    Item { Layout.preferredHeight: 8 }
                }
            }

            Text {
                id: errorText
                Layout.fillWidth: true
                color: Theme.red
                font.family: Theme.font
                font.pixelSize: 10
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: Theme.border
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    Layout.fillWidth: true
                    text: Lang.t("텍스트 확장은 Space·Tab·Enter로 실행됩니다.", "Text expansion runs on Space, Tab or Enter.")
                    color: Theme.faint
                    font.family: Theme.font
                    font.pixelSize: 9
                    elide: Text.ElideRight
                }
                ChipButton {
                    text: Lang.t("취소", "Cancel")
                    onClicked: app.closeSettings()
                }
                ChipButton {
                    text: Lang.t("저장", "Save")
                    primary: true
                    onClicked: settingsWindow.save()
                }
            }
        }
    }

    Dialog {
        id: confirmClear
        modal: true
        width: 380
        parent: Overlay.overlay
        anchors.centerIn: parent
        padding: 18

        background: Rectangle {
            color: Theme.surface
            radius: 12
            border.width: 1
            border.color: Theme.border
        }

        contentItem: ColumnLayout {
            spacing: 10
            Text {
                text: Lang.t("클립 삭제", "Delete clips")
                color: Theme.text
                font.family: Theme.font
                font.pixelSize: 14
                font.bold: true
            }
            Text {
                Layout.fillWidth: true
                text: Lang.t("즐겨찾기와 고정한 코드 블록을 제외한 모든 클립을 삭제할까요?\n이 작업은 되돌릴 수 없습니다.", "Delete every clip except favorites and pinned snippets?\nThis cannot be undone.")
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
                    onClicked: confirmClear.close()
                }
                ChipButton {
                    text: Lang.t("삭제", "Delete")
                    primary: true
                    onClicked: {
                        app.clearItems()
                        confirmClear.close()
                    }
                }
            }
        }
    }
}
