# Cloppy

[한국어 문서](README.ko.md)

A clipboard manager for Windows. Cloppy collects everything you copy and lets you organize clips, code blocks, assets, and folders, then paste them instantly from the quick window. Built with C++ / Qt 6 / Qt Quick.

## Key Features

- **Clipboard History**: Automatically saves text, links, images, and files. Duplicate merging, pinning (📌), favorites, secret mode, and automatic cleanup
- **Stable List Order**: Copying or pasting an existing clip again keeps its place in the list (Settings → Behavior). "Clear all (keep pinned)" wipes history from the tray menu, the quick window and the main window
- **Quick Window (Win+V)**: Opens the Cloppy panel instead of the default Windows clipboard. Switch between Clips / Code / Assets, filter by folder, search, navigate with the keyboard, and resize the window (size is remembered)
- **Code Blocks**: Store code snippets with a title, language, and folder. Protected from automatic cleanup
- **Assets**: Copies images, fonts, and code files into the app folder for storage. Preview, open, or copy their paths
- **Folders**: Create folders and subfolders for clips and assets, with customizable colors and item moving
- **Text Expansion**: `!mail` → `me@example.com` and similar shortcuts. Korean shortcuts such as `!메일` are supported, and text expansion also works inside the Cloppy window
- **Korean/English Conversion**: Convert selected text instantly with `Ctrl+Alt+L`, or convert clipboard contents with `Win+Alt+L`
- **Developer Mode**: Clean up terminal copies by removing ANSI escape sequences, terminal paste shortcuts, and paste without formatting
- **Paste with Conversion (developer tools)**: Minify/format JSON, add HTML line breaks (`<br>`), HTML entity escape/undo, string escape/undo, quoted/JSON strings, URL encode/decode, Base64 encode/decode, Markdown code block and inline code, single line, comma join, whitespace trim, `camelCase` / `PascalCase` / `snake_case` / `kebab-case` / `CONSTANT_CASE`, comment each line with `//` or `#`
- **English/Korean UI**: English by default. Switch to Korean under Settings → Appearance → Language. Changes apply immediately
- **In-App Help**: Open from the `? Help` sidebar item, `F1`, or the system tray menu

## Requirements

- Windows 10/11 64-bit
- Qt 6.5 or later (development environment: Qt 6.12.0 `msvc2022_64`)
- Visual Studio 2022 or later (MSVC, CMake, and Ninja included)
- Qt Installer Framework 4.x (only required for creating the installer)

## Build

Edit the `QT_DIR` and `VS_DIR` paths in `build.bat` to match your environment, then run:

```bat
build.bat
```

Output: `build\Cloppy.exe` (required DLLs and QML modules are deployed alongside it using `windeployqt`)

## Portable Package

```bat
package.bat
```

Output: the `dist\` folder. Run `dist\Cloppy.exe`; the entire folder is required.

## Installer (Qt Installer Framework)

```bat
package.bat
installer\build-installer.bat
```

Output: `CloppySetup.exe`. The installer creates Start Menu and Desktop shortcuts. The default installation path is `%USERPROFILE%\Cloppy`.

If your IFW installation is located elsewhere, edit `IFW_DIR` in `installer\build-installer.bat`.

## Shortcuts

| Shortcut                           | Action                                                |
| ---------------------------------- | ----------------------------------------------------- |
| `Win+V`                            | Open/close the Quick Window                           |
| `Ctrl+,` / `F1`                    | Settings / Help                                       |
| `Ctrl+Alt+L`                       | Convert selected text between Korean and English      |
| `Win+Alt+L`                        | Convert clipboard contents between Korean and English |
| `Ctrl+1 / 2 / 3`                   | View Clips / Code Blocks / Assets                     |
| `Ctrl+F`, `Ctrl+N`, `Ctrl+Shift+N` | Search, New Folder, New Code Block                    |
| `Enter` / `Del`                    | Copy / Delete                                         |

All global shortcuts can be changed or disabled in Settings.

## Data Locations

```text
%APPDATA%\Cloppy\Cloppy\cloppy.db      Clips, folders, and settings database (SQLite)
%APPDATA%\Cloppy\Cloppy\images\        Image clips
%APPDATA%\Cloppy\Cloppy\assets\        Asset files
%APPDATA%\Cloppy\Cloppy\cloppy.log     Log file
```

## Language

The UI language is English by default. You can switch to Korean under Settings → Appearance → Language.

The selected language is stored in QSettings (`language`). QML uses the `Lang.t(Korean, English)` singleton for translations, while C++ notifications use `Tr::text()`.

## Structure

```text
src/        C++ (AppController, ClipStore, HotkeyManager, WinUtils, LangSwap ...)
qml/        Qt Quick UI (MainWindow, PopupWindow, SettingsWindow, HelpWindow ...)
assets/     Icons
docs/       Usage notes shipped with the program
LICENSES/   LGPL-3.0 and GPL-3.0 license texts
installer/  Qt Installer Framework configuration
build.bat / package.bat / run.bat
```

## License

Cloppy is free software licensed under the **GNU Lesser General Public
License, version 3 (or any later version)**. See [LICENSE](LICENSE) for the
project license and [LICENSES](LICENSES) for the full license texts
(`LGPL-3.0.txt`, `GPL-3.0.txt`).

Cloppy is built with **Qt**, which is used under the **GNU LGPL v3**:

    The Qt Toolkit is Copyright (C) 2017 The Qt Company Ltd.
    Contact: https://www.qt.io/licensing

Qt is linked dynamically: the Qt DLLs and QML modules that ship next to
`Cloppy.exe` may be replaced or relinked by anyone. Every distribution of
Cloppy (portable zip and installer) includes the license texts in the
`LICENSES` folder, and the app shows them from the tray menu → License.
The corresponding Qt source code is available from
<https://download.qt.io/official_releases/qt/> and <https://code.qt.io/>.

Bundled third-party components (Qt, SQLite, Microsoft VC++ runtime, Qt
Installer Framework) and their terms are listed in
[THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md).