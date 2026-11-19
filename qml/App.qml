import QtQuick
import Cloppy 1.0

QtObject {
    id: application

    property Binding languageBinding: Binding {
        target: Lang
        property: "language"
        value: app.language
    }

    property var mainWindow: MainWindow {}
    property var popupWindow: PopupWindow {}
    property var settingsWindow: SettingsWindow {}
    property var helpWindow: HelpWindow {}
    property var editWindow: EditWindow {}
}
