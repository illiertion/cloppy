# Third-Party Notices

Cloppy is distributed together with third-party software. This file lists
those components, the licenses they are used under, and how Cloppy complies
with them. The full license texts are in the [`LICENSES`](LICENSES)
directory and are also shown inside the app (tray menu → **License**).

Cloppy itself is licensed under the **GNU Lesser General Public License
version 3 (or any later version)** — see [LICENSE](LICENSE).

## Qt (The Qt Company Ltd.)

Cloppy uses the Qt framework under the **GNU Lesser General Public License,
version 3**:

    The Qt Toolkit is Copyright (C) 2017 The Qt Company Ltd.
    Contact: https://www.qt.io/licensing

    You may use, distribute and copy the Qt GUI Toolkit under the terms of
    GNU Lesser General Public License version 3, which supplements GNU
    General Public License Version 3. Both of the licenses are displayed in
    the LICENSES directory.

License texts: [`LICENSES/LGPL-3.0.txt`](LICENSES/LGPL-3.0.txt) and
[`LICENSES/GPL-3.0.txt`](LICENSES/GPL-3.0.txt).

### How Cloppy complies with the LGPL v3 for Qt

- **Dynamic linking.** Cloppy links Qt dynamically. The Qt libraries ship as
  ordinary DLLs (`Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Quick.dll`,
  `Qt6QuickControls2.dll`, `Qt6Qml.dll`, `Qt6Sql.dll`, `Qt6Widgets.dll`, …)
  next to `Cloppy.exe`, together with the QML modules in `qml/`, the plugins
  in `imageformats/`, `sqldrivers/`, `styles/`, `tls/`, `platforms/`, etc.
  Nothing from Qt is statically linked into or merged with `Cloppy.exe`.
- **Relinking and replacement.** Anyone may replace the shipped Qt DLLs and
  QML modules with their own build of Qt 6 (compatible API) and run Cloppy
  with it. Cloppy contains no anti-tampering or code that would prevent
  replacing or relinking the Qt libraries.
- **License texts.** The LGPL v3 and GPL v3 texts accompany every copy of the
  program: in the `LICENSES/` folder of the portable package and the
  installation directory, and inside the app itself (tray menu → License).
- **Corresponding source.** The corresponding source code of Cloppy is
  distributed with the project (source repository / release archive).
  The corresponding source code of Qt is published by The Qt Company:
  - https://download.qt.io/official_releases/qt/ — official Qt releases,
    including the matching `qt-everywhere-src-<version>` source archives
  - https://code.qt.io/cgit/qt/qtbase.git/ — Git repositories of the Qt
    modules (qtbase, qtdeclarative, qtquickcontrols, …)

  Cloppy is developed against Qt 6 (development environment: Qt 6.12.0
  `msvc2022_64`), so the matching `qt-everywhere-src-6.12.0` archive is the
  corresponding source for the shipped Qt libraries.
- **No warranty.** Cloppy and Qt are provided without any warranty, as
  described in the license texts.

### Qt components deployed with Cloppy

| Component | Purpose |
| --- | --- |
| Qt 6 Core, Gui, Qml, Quick, QuickControls2, Sql, Widgets | Application runtime |
| QtQuick, QtQuick.Window, QtQuick.Layouts, QtQuick.Shapes, QtQuick.Effects, QtQuick.Templates | QML modules used by the UI |
| QtQuick.Controls (Basic/Fusion/Material/Universal/Imagine/Windows/FluentWinUI3 styles) | QML controls shipped by windeployqt |
| imageformats, iconengines, platforminputcontexts, platforms, styles | Qt plugins |
| sqldrivers/qsqlite | SQLite driver (clip database) |
| tls (qcertonlybackend, qschannelbackend) | TLS backends |
| Qt translation/tooling DLLs deployed by windeployqt | Qt runtime support |

## SQLite

The clip database is stored in SQLite, accessed through Qt's `qsqlite`
driver. SQLite is in the **public domain** — see https://sqlite.org/copyright.html

## Microsoft Visual C++ Runtime

`vcruntime140.dll`, `vcruntime140_1.dll`, `vcruntime140_threads.dll`,
`vccorlib140.dll` and the `msvcp140*.dll` family from the Microsoft Visual
C++ Redistributable are distributed with Cloppy.

© Microsoft Corporation. License terms:
https://aka.ms/vs/17/release/redist.txt and
https://visualstudio.microsoft.com/license-terms/vs2022-cruntime/

## Qt Installer Framework

`CloppySetup.exe` is produced with the Qt Installer Framework
(https://doc.qt.io/qtinstallerframework/), which is developed by The Qt
Company under the GPL v3 (see [`LICENSES/GPL-3.0.txt`](LICENSES/GPL-3.0.txt)).
The installer only copies the Cloppy payload; the installed application
itself is covered by the licenses listed above.