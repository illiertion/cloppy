import QtQuick
import Cloppy 1.0

Item {
    id: root
    width: ListView.view ? ListView.view.width : 300
    height: 66
    property bool selected: ListView.isCurrentItem
    property bool hovered: hover.hovered
    property bool activateOnClick: false
    required property int clipId
    required property string kind
    required property string title
    required property string previewLine
    required property string imagePath
    required property string imageUrl
    required property bool favorite
    required property bool secret
    required property bool pinned
    required property string language
    required property string timeText
    required property string sizeText
    required property int useCount
    required property int filesCount
    required property int index
    signal contextRequested(int rowIndex)
    signal activated(int clipId)

    HoverHandler { id: hover }

    Rectangle {
        id: card
        anchors.fill: parent
        anchors.margins: 4
        radius: 12
        color: root.selected ? Theme.accentSoft : (root.hovered ? Theme.surface2 : "transparent")
        border.width: root.selected || root.hovered ? 1 : 0
        border.color: root.selected ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.65)
                                    : Theme.border

        Rectangle {
            visible: root.selected
            width: 3
            height: 24
            radius: 2
            anchors.left: parent.left
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.accent
        }
    }

    Item {
        id: badge
        width: 38
        height: 38
        anchors.left: parent.left
        anchors.leftMargin: 15
        anchors.verticalCenter: parent.verticalCenter

        Rectangle {
            anchors.fill: parent
            radius: 10
            color: Theme.thumb
            visible: root.kind === "image" && root.imagePath !== ""
            clip: true

            Image {
                anchors.fill: parent
                source: root.imageUrl
                sourceSize: Qt.size(76, 76)
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: false
            }
        }

        KindBadge {
            anchors.fill: parent
            kind: root.kind
            visible: !(root.kind === "image" && root.imagePath !== "")
        }
    }

    Column {
        anchors.left: badge.right
        anchors.leftMargin: 12
        anchors.right: rightColumn.left
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        spacing: 3

        Text {
            width: parent.width
            text: (root.favorite ? "★ " : "") + (root.secret ? "🔒 " : "")
                  + (root.pinned ? "📌 " : "") + root.title
            color: Theme.text
            font.family: Theme.font
            font.pixelSize: 13
            font.bold: true
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            text: root.kind === "files"
                  ? (root.filesCount + "개 파일 · " + root.previewLine)
                  : (root.language ? "[" + root.language + "] " + root.previewLine : root.previewLine)
            color: Theme.muted
            font.family: Theme.font
            font.pixelSize: 11
            elide: Text.ElideRight
        }
    }

    Column {
        id: rightColumn
        anchors.right: parent.right
        anchors.rightMargin: 18
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        Text {
            anchors.right: parent.right
            text: root.timeText + (root.useCount > 1 ? " · " + root.useCount + "회" : "")
            color: Theme.faint
            font.family: Theme.font
            font.pixelSize: 10
        }

        Text {
            anchors.right: parent.right
            text: root.kind === "code" && root.language ? root.language : root.sizeText
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
            if (ListView.view)
                ListView.view.currentIndex = root.index
            if (mouse.button === Qt.RightButton)
                root.contextRequested(root.index)
            else if (root.activateOnClick)
                root.activated(root.clipId)
        }
        onDoubleClicked: root.activated(root.clipId)
    }
}
