import QtQuick
import Cloppy 1.0
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: licenseWindow
    objectName: "cloppyLicense"
    title: Lang.t("라이선스", "License")
    visible: app.licenseVisible
    width: Math.min(760, Screen.width - 40)
    height: Math.min(700, Screen.height - 60)
    minimumWidth: 560
    minimumHeight: 420
    color: Theme.bg
    flags: Qt.Dialog
    x: Math.round((Screen.width - width) / 2)
    y: Math.round((Screen.height - height) / 2)

    Binding { target: Theme; property: "dark"; value: app.darkTheme }

    property string currentDoc: "notice"

    onVisibleChanged: {
        if (visible) {
            currentDoc = "notice"
            requestActivate()
        }
    }

    onClosing: function(close) {
        close.accepted = false
        app.closeLicense()
    }

    Shortcut {
        sequence: "Escape"
        onActivated: app.closeLicense()
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
                spacing: 8

                Text {
                    text: Lang.t("라이선스", "License")
                    color: Theme.text
                    font.family: Theme.font
                    font.pixelSize: 23
                    font.bold: true
                }
                Text {
                    text: "Cloppy " + app.appVersion + "  ·  Qt " + app.qtVersion
                    color: Theme.faint
                    font.family: Theme.font
                    font.pixelSize: 10
                }
                Item { Layout.fillWidth: true }

                ChipButton {
                    text: Lang.t("라이선스 폴더 열기", "Open license folder")
                    onClicked: app.openLicenseFolder()
                }
                ChipButton {
                    text: Lang.t("닫기", "Close")
                    primary: true
                    onClicked: app.closeLicense()
                }
            }

            Text {
                Layout.fillWidth: true
                text: Lang.t("Cloppy와 Qt는 GNU LGPL v3 조건으로 배포됩니다. Qt는 DLL로 동적 연결되어 있으므로 Qt 라이브러리를 교체·재연결할 수 있습니다.",
                            "Cloppy and Qt are distributed under the GNU LGPL v3. Qt is dynamically linked, so the Qt libraries can be replaced or relinked.")
                color: Theme.muted
                font.family: Theme.font
                font.pixelSize: 11
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 4

                ChipButton {
                    text: Lang.t("고지 / Qt LGPL", "Notice / Qt LGPL")
                    checked: licenseWindow.currentDoc === "notice"
                    onClicked: licenseWindow.currentDoc = "notice"
                }
                ChipButton {
                    text: Lang.t("Cloppy 라이선스", "Cloppy license")
                    checked: licenseWindow.currentDoc === "license"
                    onClicked: licenseWindow.currentDoc = "license"
                }
                ChipButton {
                    text: "LGPL-3.0"
                    checked: licenseWindow.currentDoc === "lgpl"
                    onClicked: licenseWindow.currentDoc = "lgpl"
                }
                ChipButton {
                    text: "GPL-3.0"
                    checked: licenseWindow.currentDoc === "gpl"
                    onClicked: licenseWindow.currentDoc = "gpl"
                }
                Item { Layout.fillWidth: true }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.surface
                radius: 12
                border.width: 1
                border.color: Theme.border

                ScrollView {
                    id: scroller
                    anchors.fill: parent
                    anchors.margins: 14
                    clip: true
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    Text {
                        width: scroller.availableWidth
                        text: licenseWindow.currentDoc === "notice" ? app.licenseNotice()
                                                                    : app.licenseText(licenseWindow.currentDoc)
                        color: Theme.muted
                        font.family: licenseWindow.currentDoc === "notice" ? Theme.font : "Consolas"
                        font.pixelSize: licenseWindow.currentDoc === "notice" ? 11 : 10
                        lineHeight: 1.35
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }
}