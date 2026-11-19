import QtQuick
import Cloppy 1.0
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: helpWindow
    objectName: "cloppyHelp"
    title: Lang.t("Cloppy 사용법", "Cloppy Help")
    visible: app.helpVisible
    width: Math.min(780, Screen.width - 40)
    height: Math.min(720, Screen.height - 60)
    minimumWidth: 600
    minimumHeight: 440
    color: Theme.bg
    flags: Qt.Dialog
    x: Math.round((Screen.width - width) / 2)
    y: Math.round((Screen.height - height) / 2)

    Binding { target: Theme; property: "dark"; value: app.darkTheme }

    component Section: ColumnLayout {
        property string title: ""
        property string body: ""
        Layout.fillWidth: true
        spacing: 5

        Text {
            text: title
            color: Theme.accent
            font.family: Theme.font
            font.pixelSize: 14
            font.bold: true
        }

        Text {
            Layout.fillWidth: true
            visible: body.length > 0
            text: body
            color: Theme.muted
            font.family: Theme.font
            font.pixelSize: 11
            lineHeight: 1.4
            wrapMode: Text.WordWrap
        }
    }

    component KeyRow: RowLayout {
        property string keys: ""
        property string desc: ""
        Layout.fillWidth: true
        spacing: 10

        Rectangle {
            implicitWidth: Math.max(96, keyText.implicitWidth + 16)
            implicitHeight: 23
            radius: 6
            color: Theme.surface2
            border.width: 1
            border.color: Theme.border

            Text {
                id: keyText
                anchors.centerIn: parent
                text: keys
                color: Theme.text
                font.family: "Consolas"
                font.pixelSize: 10
                font.bold: true
            }
        }

        Text {
            Layout.fillWidth: true
            text: desc
            color: Theme.muted
            font.family: Theme.font
            font.pixelSize: 11
            wrapMode: Text.WordWrap
        }
    }

    component Bullet: RowLayout {
        property string text: ""
        Layout.fillWidth: true
        spacing: 8

        Text {
            text: "·"
            color: Theme.accent
            font.pixelSize: 13
        }

        Text {
            Layout.fillWidth: true
            text: parent.text
            color: Theme.muted
            font.family: Theme.font
            font.pixelSize: 11
            lineHeight: 1.4
            wrapMode: Text.WordWrap
        }
    }

    onClosing: function(close) {
        close.accepted = false
        app.closeHelp()
    }

    Shortcut {
        sequence: "Escape"
        onActivated: app.closeHelp()
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
                    text: Lang.t("Cloppy 사용법", "Cloppy Help")
                    color: Theme.text
                    font.family: Theme.font
                    font.pixelSize: 23
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                ChipButton {
                    text: Lang.t("설정 열기", "Open settings")
                    onClicked: app.openSettings()
                }
                Text {
                    text: "✕"
                    color: Theme.muted
                    font.pixelSize: 17
                    padding: 4
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: app.closeHelp()
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                text: Lang.t("복사한 모든 것을 모으고, 코드 블록과 에셋까지 폴더로 정리하는 Windows 클립보드 매니저입니다.", "A Windows clipboard manager that collects everything you copy and organizes snippets and assets into folders.")
                color: Theme.muted
                font.family: Theme.font
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: Theme.border
            }

            ScrollView {
                id: helpScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    width: helpScroll.availableWidth - 8
                    spacing: 14

                    Section {
                        title: Lang.t("시작하기", "Getting started")
                        body: Lang.t("Win+V 로 빠른 창을 열고, Ctrl+, 로 설정을 엽니다. 복사한 내용은 자동으로 기록되며, 메인 창 왼쪽 위의 시크릿 클립으로 숨김 기록을 따로 봅니다.", "Press Win+V to open the quick window and Ctrl+, for settings. Everything you copy is recorded automatically; secret clips are kept separately in the lock at the top left.")
                    }

                    Section {
                        title: Lang.t("단축키", "Shortcuts")
                        body: Lang.t("설정 → 전역 단축키에서 모두 바꾸거나 해제할 수 있습니다. 설정에는 현재 등록 상태도 표시됩니다.", "All global shortcuts can be changed or cleared in Settings, which also shows whether they are registered.")
                    }
                    KeyRow { keys: "Win+V"; desc: Lang.t("빠른 클립 창 열기 / 닫기. Windows 기본 클립보드는 열리지 않습니다.", "Open or close the quick clip window. The Windows clipboard panel will not open.") }
                    KeyRow { keys: "Ctrl+Alt+L"; desc: Lang.t("드래그해 선택한 텍스트를 그 자리에서 바로 한/영 교환 (기본값).", "Convert the selected text in place (default).") }
                    KeyRow { keys: "Win+Alt+L"; desc: Lang.t("현재 클립보드에 들어 있는 내용 한/영 교환.", "Convert the current clipboard contents.") }
                    KeyRow { keys: "Ctrl+1 / 2 / 3"; desc: Lang.t("메인 창 위쪽 탭과 같은 전환입니다(클립 / 코드 블록 / 에셋).", "Same switching as the tabs at the top of the main window (clips / snippets / assets).") }
                    KeyRow { keys: "Ctrl+, / Ctrl+F"; desc: Lang.t("설정 열기 / 검색.", "Open settings / search.") }
                    KeyRow { keys: "Ctrl+N / Ctrl+Shift+N"; desc: Lang.t("새 폴더 / 새 코드 블록.", "New folder / new snippet.") }
                    KeyRow { keys: "Enter / Del"; desc: Lang.t("선택한 클립 복사 / 삭제.", "Copy / delete the selected clip.") }
                    KeyRow { keys: "Ctrl+E / Ctrl+L / Ctrl+D"; desc: Lang.t("편집 / 한영 교환 / 즐겨찾기.", "Edit / convert case / favorite.") }
                    KeyRow { keys: Lang.t("Ctrl+1~9 (팝업)", "Ctrl+1~9 (popup)"); desc: Lang.t("팝업에서 1~9번째 클립을 바로 붙여넣기.", "Paste the 1st-9th clip directly from the popup.") }

                    Section {
                        title: Lang.t("빠른 창 (Win+V)", "Quick window (Win+V)")
                        body: Lang.t("기본 크기 398 × 500이며 오른쪽/아래/모서리를 끌어 조절할 수 있습니다. 바꾼 크기는 기억됩니다. 위쪽 [클립] [코드] [에셋] 버튼으로 보기를 전환하고, 설정에서 고른 폴더가 폴더 버튼으로 표시됩니다. 에셋은 클릭하면 경로가 복사됩니다.", "The default size is 398 x 500 and you can resize it by dragging the right edge, bottom edge or corner; the size is remembered. Use the [Clips] [Snippets] [Assets] buttons at the top to switch views. Folders chosen in Settings appear as folder buttons, and clicking an asset copies its path.")
                    }

                    Section {
                        title: Lang.t("클립 보기", "Clip view")
                        body: Lang.t("메인 창 위쪽의 [클립보드] [코드 블록] [에셋] 탭으로 보기를 바꿉니다. 한 번 클릭하면 선택되고 오른쪽에 미리보기가 표시되며, 더블클릭하면 클립보드로 복사됩니다. Win+V 팝업에서는 한 번 클릭하면 이전 창에 바로 붙여넣습니다.", "Switch views with the [Clips] [Snippets] [Assets] tabs at the top of the main window. A single click selects and shows a preview on the right; a double click copies it. In the Win+V popup a single click pastes into the previous window.")
                    }
                    Bullet { text: Lang.t("우클릭 → 변환하여 붙여넣기: JSON(압축/정리), HTML(줄바꿈 <br>), Markdown 코드블록, 한 줄로, 따옴표 문자열, Base64.", "Right-click -> Paste as: JSON (compact/pretty), HTML (<br> line breaks), Markdown code block, single line, quoted string, Base64.") }
                    Bullet { text: Lang.t("우클릭 → 폴더로 이동으로 클립을 정리합니다.", "Right-click -> Move to folder to organize clips.") }
                    Bullet { text: Lang.t("‘코드 블록으로 고정’을 누르면 자동 정리에서 제외되고 코드 블록 보기에도 보관됩니다. 고정해도 클립 목록에서 사라지지 않습니다.", "Pinning as a snippet keeps the item out of automatic cleanup and also stores it in the snippet view. Pinned items stay in the clip list.") }

                    Section {
                        title: Lang.t("코드 블록 (Ctrl+2)", "Snippets (Ctrl+2)")
                        body: Lang.t("코드 조각에 제목과 언어를 붙여 폴더에 보관합니다. 클립보드 기록과 별도로 관리되며, 자동 삭제·‘즐겨찾기 외 삭제’에서 지워지지 않습니다.", "Keep code snippets with a title and language in folders. They are managed separately from history and are never removed by auto-cleanup or Delete all but favorites.")
                    }
                    Bullet { text: Lang.t("‘+ 새 코드 블록’ 또는 Ctrl+Shift+N 으로 만듭니다.", "Create one with + New snippet or Ctrl+Shift+N.") }
                    Bullet { text: Lang.t("F2 또는 우클릭 → 코드 블록 편집으로 제목·언어·폴더를 고칩니다.", "Press F2 or right-click -> Edit snippet to change its title, language or folder.") }
                    Bullet { text: Lang.t("일반 클립을 고정하면 같은 항목이 코드 블록 보기에도 나타납니다.", "Pinning a normal clip also shows it in the snippet view.") }

                    Section {
                        title: Lang.t("에셋 (Ctrl+3)", "Assets (Ctrl+3)")
                        body: Lang.t("이미지, 폰트, 코드 파일 등을 앱 데이터 폴더에 복사해 보관합니다. 에셋 전용 폴더와 하위 폴더로 정리하고, 미리보기·열기·경로 복사·삭제를 사용할 수 있습니다.", "Copy images, fonts and code files into the app data folder. Organize them with asset folders and subfolders, and preview, open, copy paths or delete them.")
                    }

                    Section {
                        title: Lang.t("텍스트 확장", "Text expansion")
                        body: Lang.t("설정 → 텍스트 확장 단축어에 등록한 문구는 Cloppy 창 안에서도, 다른 앱에서도 입력 후 Space(또는 Tab/Enter)로 바뀝니다. 기본 예: !mail → me@example.com. 한글 단축어(!메일)도 지원하고, Cloppy 창 안에서도 동작합니다. 설정에서 주소를 고치거나 항목을 추가·삭제하고, 사용 토글로 전체를 끌 수 있습니다.", "Triggers added in Settings -> Text expansion are replaced after Space (or Tab/Enter), both in other apps and inside Cloppy windows. Default example: !mail -> me@example.com. Korean triggers (!메일) work too. Edit or remove entries, or turn the whole feature off.")
                    }

                    Section {
                        title: Lang.t("개발자 모드", "Developer mode")
                        body: Lang.t("설정 → 개발자에서 프로그래머 모드를 켜고, 아래 동작을 각각 켜고 끌 수 있습니다.", "Turn on programmer mode in Settings -> Developer and enable each behavior separately.")
                    }
                    Bullet { text: Lang.t("터미널 복사 정리: ANSI 색상 코드와 끝 줄바꿈/공백을 제거합니다.", "Clean terminal copies: strip ANSI color codes and trailing newlines/spaces.") }
                    Bullet { text: Lang.t("터미널에서 Ctrl+Shift+V / Ctrl+Shift+C 사용: Windows Terminal, cmd, PowerShell 등에서 붙여넣기/복사합니다.", "Use Ctrl+Shift+V / Ctrl+Shift+C in terminals such as Windows Terminal, cmd and PowerShell.") }
                    Bullet { text: Lang.t("터미널에는 항상 서식 없이 붙여넣기.", "Always paste into terminals without formatting.") }
                    Bullet { text: Lang.t("터미널 앱 목록은 직접 편집할 수 있습니다.", "The terminal app list can be edited.") }

                    Section {
                        title: Lang.t("한/영 변환 방향", "Conversion direction")
                        body: Lang.t("설정 → 전역 단축키에서 방향을 고릅니다. 자동은 한글이면 영문 자판으로, 영문이면 한글로 바꿉니다. ‘→ 한글’은 dkssud → 안녕, ‘→ 영문’은 안녕 → dkssud 처럼 항상 한 방향으로 변환합니다.", "Choose the direction in Settings -> Global shortcuts. Auto converts Hangul to the English layout and vice versa; -> Hangul always converts one way (dkssud -> 안녕) and -> English the other (안녕 -> dkssud).")
                    }

                    Section {
                        title: Lang.t("폴더", "Folders")
                        body: Lang.t("왼쪽 폴더 옆 + 또는 Ctrl+N 으로 만듭니다. 폴더를 우클릭하면 하위 폴더 만들기, 이름·색상 편집, 다른 폴더로 이동, 삭제가 가능합니다. 폴더를 삭제해도 항목은 지워지지 않고 폴더에서만 빠집니다.", "Create folders with + next to the folder header or Ctrl+N. Right-click a folder to create a subfolder, edit its name or color, move it, or delete it. Deleting a folder keeps its items and only removes them from the folder.")
                    }
                    Bullet { text: Lang.t("설정 → 빠른 창 폴더에서 고른 폴더가 Win+V 빠른 창에 버튼으로 나타나 바로 그 폴더만 볼 수 있습니다. 여러 개를 고를 수 있고, 고르지 않으면 표시되지 않습니다.", "Folders chosen in Settings -> Quick window folders appear as buttons in the Win+V window so you can view just that folder. Choose several; none are shown by default.") }

                    Section {
                        title: Lang.t("문제 해결", "Troubleshooting")
                        body: Lang.t("Win+V를 눌렀는데 Windows 기본 클립보드가 열리면, 다른 클립보드 도구를 종료하고 설정의 ‘Win+V 차단’이 켜져 있는지 확인하세요. 마우스 매크로로 Win+V를 보내도 Cloppy가 열립니다.", "If the Windows clipboard panel opens instead of Cloppy, close other clipboard tools and check that Block Win+V is enabled in Settings. Win+V sent from a mouse macro also opens Cloppy.")
                    }
                    Bullet { text: Lang.t("단축키가 동작하지 않으면 설정의 ‘단축키 상태’를 확인하고, 다른 앱이 같은 조합을 쓰고 있지 않은지 확인하세요.", "If a shortcut does not work, check Hotkey status in Settings and make sure no other app uses the same combination.") }
                    Bullet { text: Lang.t("관리자 권한으로 실행 중인 앱에는 붙여넣기가 제한될 수 있습니다.", "Pasting into apps running as administrator may be restricted.") }
                    Bullet { text: Lang.t("변환이 안 되면 텍스트를 실제로 선택(드래그)한 상태인지 확인하세요. 선택 영역이 없으면 복사할 내용이 없습니다.", "If conversion does nothing, make sure text is actually selected (dragged). With no selection there is nothing to copy.") }

                    Section {
                        title: Lang.t("저장 위치", "Storage")
                        body: Lang.t("%APPDATA%\\Cloppy\\Cloppy\\cloppy.db (기록)\n%APPDATA%\\Cloppy\\Cloppy\\images (이미지)\n%APPDATA%\\Cloppy\\Cloppy\\assets (에셋)\n%APPDATA%\\Cloppy\\Cloppy\\cloppy.log (로그)", "%APPDATA%\\Cloppy\\Cloppy\\cloppy.db (history)\n%APPDATA%\\Cloppy\\Cloppy\\images (images)\n%APPDATA%\\Cloppy\\Cloppy\\assets (assets)\n%APPDATA%\\Cloppy\\Cloppy\\cloppy.log (log)")
                    }

                    Item { Layout.preferredHeight: 8 }
                }
            }
        }
    }
}
