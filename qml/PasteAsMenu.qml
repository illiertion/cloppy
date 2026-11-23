import QtQuick
import Cloppy 1.0
import QtQuick.Controls

// "Paste as" transformations. Developer-oriented tools: HTML <br> insertion,
// entity/string escaping, URL and Base64 encoding, Markdown, line joining and
// naming-style conversions. Shared by the popup and the main window.
StyledMenu {
    id: pasteAsMenu
    property var targetId: 0

    StyledMenu {
        title: "JSON"
        MenuItem { text: Lang.t("압축", "Compact"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "json") }
        MenuItem { text: Lang.t("정리", "Pretty"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "json-pretty") }
        MenuItem { text: Lang.t("문자열로", "As string"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "json-string") }
    }

    StyledMenu {
        title: "HTML"
        MenuItem { text: Lang.t("줄바꿈 <br>", "<br> line breaks"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "html") }
        MenuItem { text: Lang.t("엔티티 변환", "Escape entities"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "html-escape") }
        MenuItem { text: Lang.t("엔티티 해제", "Undo entities"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "html-unescape") }
    }

    StyledMenu {
        title: Lang.t("문자열", "String")
        MenuItem { text: Lang.t("이스케이프", "Escape"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "escape") }
        MenuItem { text: Lang.t("이스케이프 해제", "Unescape"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "unescape") }
        MenuItem { text: Lang.t("따옴표 문자열", "Quoted string"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "quote") }
    }

    StyledMenu {
        title: Lang.t("인코딩", "Encoding")
        MenuItem { text: "URL encode"; onTriggered: app.pasteAs(pasteAsMenu.targetId, "url-encode") }
        MenuItem { text: "URL decode"; onTriggered: app.pasteAs(pasteAsMenu.targetId, "url-decode") }
        MenuItem { text: "Base64"; onTriggered: app.pasteAs(pasteAsMenu.targetId, "base64") }
        MenuItem { text: Lang.t("Base64 디코딩", "Base64 decode"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "base64-decode") }
    }

    StyledMenu {
        title: "Markdown"
        MenuItem { text: Lang.t("코드블록", "Code block"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "markdown") }
        MenuItem { text: Lang.t("인라인 코드", "Inline code"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "md-inline") }
    }

    StyledMenu {
        title: Lang.t("정리", "Clean up")
        MenuItem { text: Lang.t("한 줄로", "Single line"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "oneline") }
        MenuItem { text: Lang.t("쉼표로 연결", "Join with commas"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "join-comma") }
        MenuItem { text: Lang.t("공백 정리", "Trim whitespace"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "trim") }
    }

    StyledMenu {
        title: Lang.t("이름 규칙", "Naming")
        MenuItem { text: "camelCase"; onTriggered: app.pasteAs(pasteAsMenu.targetId, "camel") }
        MenuItem { text: "PascalCase"; onTriggered: app.pasteAs(pasteAsMenu.targetId, "pascal") }
        MenuItem { text: "snake_case"; onTriggered: app.pasteAs(pasteAsMenu.targetId, "snake") }
        MenuItem { text: "kebab-case"; onTriggered: app.pasteAs(pasteAsMenu.targetId, "kebab") }
        MenuItem { text: "CONSTANT_CASE"; onTriggered: app.pasteAs(pasteAsMenu.targetId, "const") }
    }

    StyledMenu {
        title: Lang.t("주석", "Comments")
        MenuItem { text: Lang.t("각 줄 // 주석", "// on each line"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "comment-slash") }
        MenuItem { text: Lang.t("각 줄 # 주석", "# on each line"); onTriggered: app.pasteAs(pasteAsMenu.targetId, "comment-hash") }
    }
}