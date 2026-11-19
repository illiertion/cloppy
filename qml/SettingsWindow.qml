import QtQuick
import Cloppy 1.0
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: settingsWindow
    objectName: "cloppySettings"
    title: "Cloppy 설정"
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
            errorText.text = "빠른 창 단축키를 지정해 주세요."
            return
        }
        if (langField.spec && app.formatHotkey(quickField.spec) === app.formatHotkey(langField.spec)) {
            errorText.text = "두 단축키가 같습니다. 다른 조합을 사용해 주세요."
            return
        }
        if (swapField.spec && app.formatHotkey(quickField.spec) === app.formatHotkey(swapField.spec)) {
            errorText.text = "선택 영역 단축키가 빠른 창 단축키와 같습니다."
            return
        }
        if (langField.spec && swapField.spec
                && app.formatHotkey(langField.spec) === app.formatHotkey(swapField.spec)) {
            errorText.text = "클립보드 변환과 선택 변환이 같은 조합입니다. 하나를 바꿔 주세요."
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
                    text: "환경 설정"
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
                text: "단축키, 개발자 도구, 텍스트 확장, 저장 공간을 관리합니다."
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
                        text: "모양"
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    RowLayout {
                        spacing: 6
                        ChipButton {
                            text: "다크"
                            checked: settingsWindow.dark
                            onClicked: settingsWindow.dark = true
                        }
                        ChipButton {
                            text: "라이트"
                            checked: !settingsWindow.dark
                            onClicked: settingsWindow.dark = false
                        }
                        Item { Layout.fillWidth: true }
                        ToggleCheck {
                            id: showPreview
                            label: "빠른 창을 넓혔을 때 미리보기 표시"
                        }
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    Text {
                        text: "빠른 창 폴더"
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
                              ? "선택한 폴더가 빠른 창에 버튼으로 표시됩니다. 여러 개를 고를 수 있습니다."
                              : "폴더를 먼저 만들면 여기서 빠른 창에 표시할 폴더를 고를 수 있습니다."
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    Text {
                        text: "전역 단축키"
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    RowLayout {
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 150
                            text: "빠른 클립 창"
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
                            text: "클립보드 한/영 교환"
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                        HotkeyField {
                            id: langField
                            Layout.fillWidth: true
                        }
                        ChipButton {
                            text: "해제"
                            onClicked: langField.setSpec("")
                        }
                    }

                    RowLayout {
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 150
                            text: "선택 영역 한/영 교환"
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                        HotkeyField {
                            id: swapField
                            Layout.fillWidth: true
                        }
                        ChipButton {
                            text: "해제"
                            onClicked: swapField.setSpec("")
                        }
                    }

                    RowLayout {
                        spacing: 6
                        Text {
                            Layout.preferredWidth: 150
                            text: "선택 영역 변환 방향"
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                        ChipButton {
                            text: "자동"
                            checked: settingsWindow.swapDirection === "auto"
                            onClicked: settingsWindow.swapDirection = "auto"
                        }
                        ChipButton {
                            text: "→ 한글"
                            checked: settingsWindow.swapDirection === "ko"
                            onClicked: settingsWindow.swapDirection = "ko"
                        }
                        ChipButton {
                            text: "→ 영문"
                            checked: settingsWindow.swapDirection === "en"
                            onClicked: settingsWindow.swapDirection = "en"
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "자동: 한글이면 영문 자판으로, 영문이면 한글로 알아서 바꿉니다. Ctrl+Alt+L은 드래그한 선택 영역을 복사 없이 바로 바꿔 붙여넣습니다."
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "단축키 상태 · " + app.hotkeyStatus
                        color: app.hotkeyStatus.indexOf("실패") >= 0 ? Theme.red : Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    Text {
                        text: "동작"
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    ToggleCheck { id: autostart; label: "Windows 시작 시 Cloppy 자동 실행" }
                    ToggleCheck { id: closeAfter; label: "붙여넣은 뒤 빠른 창 닫기" }
                    ToggleCheck { id: notifications; label: "알림 표시 (복사·변환·저장 안내)" }

                    RowLayout {
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 150
                            text: "붙여넣기 지연"
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
                            text: "최대 보관 개수"
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
                            text: "시크릿 자동 삭제"
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
                            text: "시간 (0 = 삭제 안 함)"
                            color: Theme.faint
                            font.family: Theme.font
                            font.pixelSize: 10
                        }
                        Item { Layout.fillWidth: true }
                    }

                    Text {
                        text: "캡처 제외 앱"
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
                        text: "개발자"
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    ToggleCheck {
                        id: programmerMode
                        Layout.fillWidth: true
                        label: "프로그래머 모드 (터미널 전용 동작 사용)"
                    }

                    ToggleCheck {
                        id: terminalCleanCopy
                        Layout.fillWidth: true
                        enabled: programmerMode.checked
                        label: "터미널 복사 정리 — ANSI 색상·끝 줄바꿈/공백 제거"
                    }
                    ToggleCheck {
                        id: terminalPasteChord
                        Layout.fillWidth: true
                        enabled: programmerMode.checked
                        label: "터미널에서 Ctrl+Shift+V / Ctrl+Shift+C 사용"
                    }
                    ToggleCheck {
                        id: terminalPlainPaste
                        Layout.fillWidth: true
                        enabled: programmerMode.checked
                        label: "터미널에는 항상 서식 없이 붙여넣기"
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "터미널 앱은 아래 목록으로 판단합니다. 각 항목은 따로 켜고 끌 수 있습니다."
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                    }

                    RowLayout {
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 150
                            text: "터미널 앱"
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
                            text: "텍스트 확장 단축어"
                            color: Theme.faint
                            font.family: Theme.font
                            font.pixelSize: 9
                            font.bold: true
                        }
                        Item { Layout.fillWidth: true }
                        ToggleCheck {
                            id: snippetsEnabled
                            label: "사용"
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "어디서든 단축어를 입력하고 Space를 누르면 문구로 바뀝니다. 예: !mail → 내 메일 주소. 클릭해서 바로 수정할 수 있습니다."
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
                                text: "＋ 단축어 추가"
                                onClicked: settingsWindow.addRow()
                            }
                            Item { Layout.fillWidth: true }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    Text {
                        text: "Windows 기본 클립보드"
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 9
                        font.bold: true
                    }

                    ToggleCheck {
                        id: blockWinV
                        Layout.fillWidth: true
                        label: "Win+V를 가로채 Cloppy를 열고 기본 클립보드 차단"
                    }
                    ToggleCheck {
                        id: disableHistory
                        Layout.fillWidth: true
                        label: "Windows 클립보드 기록 저장 끄기 (레지스트리)"
                    }

                    Rectangle { Layout.fillWidth: true; implicitHeight: 1; color: Theme.border }

                    Text {
                        text: "데이터"
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
                        text: (app.stats.items || 0) + "개 항목 · " + (app.stats.bytesText || "0 B")
                        color: Theme.faint
                        font.family: Theme.font
                        font.pixelSize: 10
                    }

                    Flow {
                        Layout.fillWidth: true
                        spacing: 8
                        ChipButton {
                            text: "데이터 폴더 열기"
                            onClicked: app.openDataFolder()
                        }
                        ChipButton {
                            text: "검색 기록 지우기"
                            onClicked: app.clearSearches()
                        }
                        ChipButton {
                            text: "즐겨찾기 외 삭제"
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
                    text: "텍스트 확장은 Space·Tab·Enter로 실행됩니다."
                    color: Theme.faint
                    font.family: Theme.font
                    font.pixelSize: 9
                    elide: Text.ElideRight
                }
                ChipButton {
                    text: "취소"
                    onClicked: app.closeSettings()
                }
                ChipButton {
                    text: "저장"
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
                text: "클립 삭제"
                color: Theme.text
                font.family: Theme.font
                font.pixelSize: 14
                font.bold: true
            }
            Text {
                Layout.fillWidth: true
                text: "즐겨찾기와 고정한 코드 블록을 제외한 모든 클립을 삭제할까요?\n이 작업은 되돌릴 수 없습니다."
                color: Theme.muted
                font.family: Theme.font
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }
            RowLayout {
                spacing: 8
                Item { Layout.fillWidth: true }
                ChipButton {
                    text: "취소"
                    onClicked: confirmClear.close()
                }
                ChipButton {
                    text: "삭제"
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
