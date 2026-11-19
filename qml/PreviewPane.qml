import QtQuick
import Cloppy 1.0
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    property var item: ({})
    readonly property bool hasItem: item && item.clipId !== undefined

    color: Theme.surface
    radius: Theme.radius
    border.width: 1
    border.color: Theme.border
    clip: true

    function metaText() {
        if (!hasItem)
            return ""
        var parts = [item.timeText, item.sizeText, item.language, item.sourceApp]
        return parts.filter(function(p) { return p && String(p).length > 0 }).join("  ·  ")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 9

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            KindBadge {
                kind: root.hasItem ? root.item.kind : "text"
                implicitWidth: 36
                implicitHeight: 36
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    Layout.fillWidth: true
                    text: root.hasItem ? ((root.item.secret ? "🔒 " : "") + (root.item.title || Lang.t("텍스트", "Text")))
                                       : Lang.t("선택된 항목 없음", "Nothing selected")
                    color: Theme.text
                    font.family: Theme.font
                    font.pixelSize: 13
                    font.bold: true
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    text: root.hasItem ? root.metaText() : Lang.t("클립을 선택하면 여기에 미리보기가 표시됩니다", "Select a clip to see a preview here")
                    color: Theme.faint
                    font.family: Theme.font
                    font.pixelSize: 10
                    elide: Text.ElideRight
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.border
        }

        Text {
            visible: !root.hasItem
            Layout.fillWidth: true
            Layout.fillHeight: true
            text: Lang.t("미리보기", "Preview")
            color: Theme.faint
            font.family: Theme.font
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        ScrollView {
            id: textScroll
            visible: root.hasItem && root.item.kind !== "image"
                     && root.item.kind !== "files" && root.item.kind !== "color"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            TextArea {
                width: textScroll.availableWidth
                readOnly: true
                textFormat: TextEdit.RichText
                wrapMode: TextEdit.Wrap
                color: Theme.text
                selectionColor: Theme.accent
                selectedTextColor: Theme.accentText
                background: null
                padding: 4
                font.family: "Segoe UI"
                font.pixelSize: 13
                text: {
                    if (!root.hasItem)
                        return ""
                    var dark = app.darkTheme
                    if (root.item.kind === "code")
                        return app.highlight(root.item.content, root.item.language)
                    return app.plainHtml(root.item.content)
                }
            }
        }

        Rectangle {
            visible: root.hasItem && root.item.kind === "image"
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 10
            color: Theme.thumb
            clip: true

            Image {
                anchors.fill: parent
                anchors.margins: 8
                source: root.hasItem ? root.item.imageUrl : ""
                fillMode: Image.PreserveAspectFit
                asynchronous: true
                cache: false
            }
        }

        ListView {
            id: filesList
            visible: root.hasItem && root.item.kind === "files"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.hasItem ? (root.item.fileItems || []) : []
            spacing: 2

            delegate: Rectangle {
                id: fileRow
                width: ListView.view.width
                height: 44
                radius: 8
                color: fileHover.hovered ? Theme.surface2 : "transparent"

                property var entry: root.item.fileItems ? root.item.fileItems[index] : ({})

                HoverHandler { id: fileHover }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 9

                    Image {
                        source: fileRow.entry.path
                                ? "image://fileicon/" + encodeURIComponent(fileRow.entry.path) : ""
                        sourceSize: Qt.size(24, 24)
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        Text {
                            Layout.fillWidth: true
                            text: fileRow.entry.name || ""
                            color: Theme.text
                            font.family: Theme.font
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }

                        Text {
                            Layout.fillWidth: true
                            text: fileRow.entry.dir || ""
                            color: Theme.faint
                            font.family: Theme.font
                            font.pixelSize: 9
                            elide: Text.ElideMiddle
                        }
                    }

                    Text {
                        text: fileRow.entry.sizeText || ""
                        color: Theme.muted
                        font.family: Theme.font
                        font.pixelSize: 10
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onDoubleClicked: app.copyItem(root.item.clipId)
                }
            }
        }

        Item {
            visible: root.hasItem && root.item.kind === "color"
            Layout.fillWidth: true
            Layout.fillHeight: true

            Column {
                anchors.centerIn: parent
                spacing: 12

                Rectangle {
                    width: 110
                    height: 110
                    radius: 14
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: root.hasItem ? app.colorFor(root.item.content) : Theme.surface2
                    border.width: 1
                    border.color: Theme.border
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.hasItem ? root.item.content : ""
                    color: Theme.muted
                    font.family: "Consolas"
                    font.pixelSize: 13
                }
            }
        }

        Flow {
            visible: root.hasItem
            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            spacing: 6

            ChipButton {
                text: Lang.t("붙여넣기", "Paste")
                primary: true
                onClicked: app.pasteItem(root.item.clipId, false)
            }
            ChipButton {
                text: Lang.t("복사", "Copy")
                onClicked: app.copyItem(root.item.clipId)
            }
            ChipButton {
                text: root.item.favorite ? Lang.t("★ 즐겨찾기", "★ Favorite") : Lang.t("☆ 즐겨찾기", "☆ Favorite")
                checked: !!root.item.favorite
                onClicked: app.toggleFavorite(root.item.clipId)
            }
            ChipButton {
                text: Lang.t("편집", "Edit")
                onClicked: app.openEdit(root.item.clipId)
            }
            ChipButton {
                text: Lang.t("한/영", "Case")
                onClicked: app.swapLanguage(root.item.clipId)
            }
            ChipButton {
                text: root.item.pinned ? Lang.t("📌 고정 해제", "📌 Unpin") : Lang.t("📌 코드 블록 고정", "📌 Pin as snippet")
                checked: !!root.item.pinned
                onClicked: app.setPinned(root.item.clipId, !root.item.pinned)
            }
            ChipButton {
                text: Lang.t("삭제", "Delete")
                onClicked: app.deleteItem(root.item.clipId)
            }
        }
    }
}
