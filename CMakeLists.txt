cmake_minimum_required(VERSION 3.22)

project(Cloppy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Sql Widgets)

qt_standard_project_setup()

qt_add_executable(Cloppy WIN32
    assets/cloppy.rc
    src/main.cpp
    src/appcontroller.cpp
    src/appcontroller.h
    src/classifier.cpp
    src/classifier.h
    src/clipboardwatcher.cpp
    src/clipboardwatcher.h
    src/clipmodel.cpp
    src/clipmodel.h
    src/clipstore.cpp
    src/clipstore.h
    src/fileiconprovider.cpp
    src/fileiconprovider.h
    src/highlighter.cpp
    src/highlighter.h
    src/hotkeymanager.cpp
    src/hotkeymanager.h
    src/icons.cpp
    src/icons.h
    src/langswap.cpp
    src/langswap.h
    src/theme.cpp
    src/theme.h
    src/types.cpp
    src/types.h
    src/winutils.cpp
    src/winutils.h
)


qt_add_qml_module(Cloppy
    URI Cloppy
    VERSION 1.0
    RESOURCE_PREFIX "/qt-project.org/imports"
    QML_FILES
        qml/App.qml
        qml/ChipButton.qml
        qml/ClipRow.qml
        qml/EditWindow.qml
        qml/Field.qml
        qml/HelpWindow.qml
        qml/HotkeyField.qml
        qml/KindBadge.qml
        qml/MainWindow.qml
        qml/PopupWindow.qml
        qml/PreviewPane.qml
        qml/ClipSearchField.qml
        qml/SettingsWindow.qml
        qml/StyledMenu.qml
        qml/ToggleCheck.qml
)

target_link_libraries(Cloppy PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Sql
    Qt6::Widgets
)

target_compile_definitions(Cloppy PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

if(MSVC)
    target_compile_options(Cloppy PRIVATE /utf-8)
endif()

qt_add_resources(Cloppy "cloppy_assets"
    PREFIX "/assets"
    BASE assets
    FILES
        assets/cloppy.ico
        assets/cloppy.png
)

set_target_properties(Cloppy PROPERTIES
    WIN32_EXECUTABLE TRUE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
