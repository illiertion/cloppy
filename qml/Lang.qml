pragma Singleton
import QtQuick

QtObject {
    id: root

    // Mirrors AppController.language: "en" (default) or "ko".
    property string language: "en"

    function t(korean, english) {
        return language === "ko" ? korean : english
    }
}
