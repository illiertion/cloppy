#include "appcontroller.h"

#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QCursor>
#include <QColor>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMenu>
#include <QMimeData>
#include <QRegularExpression>
#include <QScreen>
#include <QSettings>

#include <memory>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>
#include <QWindow>

#include "classifier.h"
#include "clipboardwatcher.h"
#include "clipmodel.h"
#include "clipstore.h"
#include "highlighter.h"
#include "hotkeymanager.h"
#include "icons.h"
#include "langswap.h"
#include "winutils.h"

namespace {

QString sha1Hex(const QByteArray &data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex());
}

QString textHash(const QString &text)
{
    return QStringLiteral("text:") + sha1Hex(text.toUtf8());
}

QString filesHash(const QStringList &paths)
{
    return QStringLiteral("files:") + sha1Hex(paths.join(QLatin1Char('\n')).toUtf8());
}

QString kindLabel(const QString &kind)
{
    static const QHash<QString, QString> labels = {
        {QStringLiteral("all"), QStringLiteral("전체 클립")},
        {QStringLiteral("favorites"), QStringLiteral("즐겨찾기")},
        {QStringLiteral("code"), QStringLiteral("코드")},
        {QStringLiteral("link"), QStringLiteral("링크")},
        {QStringLiteral("image"), QStringLiteral("이미지")},
        {QStringLiteral("files"), QStringLiteral("파일")},
        {QStringLiteral("command"), QStringLiteral("명령어")},
        {QStringLiteral("color"), QStringLiteral("색상")},
        {QStringLiteral("text"), QStringLiteral("텍스트")},
        {QStringLiteral("email"), QStringLiteral("이메일")},
        {QStringLiteral("path"), QStringLiteral("경로")},
    };
    return labels.value(kind, QStringLiteral("전체 클립"));
}

} // namespace

AppController::AppController(ClipStore *store, QObject *parent)
    : QObject(parent)
    , m_store(store)
    , m_popupModel(new ClipModel(store, this))
    , m_mainModel(new ClipModel(store, this))
    , m_watcher(new ClipboardWatcher(this))
    , m_hotkeys(new HotkeyManager(this))
{
    loadSettings();
    buildTray();
    applyRuntimeSettings(true);

    connect(m_watcher, &ClipboardWatcher::captured, this, &AppController::onCaptured);
    connect(m_hotkeys, &HotkeyManager::activated, this, &AppController::openPopup);
    connect(m_hotkeys, &HotkeyManager::langswap, this, &AppController::swapGlobalClipboard);
    connect(m_hotkeys, &HotkeyManager::selectionSwap, this, &AppController::swapSelection);
    connect(m_hotkeys, &HotkeyManager::expandSnippet, this,
            [this](const QString &replacement, int backspaces) {
                Q_UNUSED(this)
                QTimer::singleShot(10, this, [replacement, backspaces]() {
                    WinUtils::replaceTyped(backspaces, replacement);
                });
            });
    connect(m_hotkeys, &HotkeyManager::status, this, [this](const QString &text) {
        m_hotkeyStatus = text;
        Q_EMIT settingsChanged();
    });
    connect(m_hotkeys, &HotkeyManager::error, this, [this](const QString &message) {
        m_hotkeyStatus = QStringLiteral("등록 실패");
        Q_EMIT settingsChanged();
        notify(QStringLiteral("단축키 오류"), message);
    });

    QSettings expansionSettings;
    if (!expansionSettings.value(QStringLiteral("expansionSeeded"), false).toBool()) {
        m_store->addExpansion(QStringLiteral("!mail"), QStringLiteral("me@example.com"));
        expansionSettings.setValue(QStringLiteral("expansionSeeded"), true);
    }
    applyRuntimeSettings(false);

    refreshAll();
    m_store->purge(m_maxItems, m_secretTtl);
    m_hotkeys->start(m_hotkey, m_langHotkey, m_swapHotkey, m_blockWinV);

    QSettings settings;
    const bool firstRun = !settings.value(QStringLiteral("firstRunDone"), false).toBool();
    if (firstRun)
        settings.setValue(QStringLiteral("firstRunDone"), true);

    QTimer::singleShot(350, this, [this, firstRun]() {
        openMain();
        if (firstRun) {
            notify(QStringLiteral("Cloppy가 시작되었습니다"),
                   QStringLiteral("%1 로 빠른 클립 창을 열 수 있습니다. 설정에서 단축키를 바꿀 수 있어요.")
                       .arg(hotkeyText()));
        }
    });
}

AppController::~AppController()
{
    if (m_hotkeys)
        m_hotkeys->stop();
    delete m_trayMenu;
    m_trayMenu = nullptr;
}

// --------------------------------------------------------------------- settings
void AppController::loadSettings()
{
    QSettings settings;
    m_hotkey = settings.value(QStringLiteral("hotkey"), QStringLiteral("win+v")).toString();
    m_langHotkey = settings.value(QStringLiteral("langswapHotkey"), QStringLiteral("win+alt+l")).toString();

    // The selection converter is bound to Ctrl+Alt+L unless the user changes it.
    m_swapHotkey = settings.value(QStringLiteral("selectionHotkey"), QString()).toString();
    if (!settings.value(QStringLiteral("selectionHotkeyMigrated"), false).toBool()
        || m_swapHotkey.isEmpty()) {
        m_swapHotkey = QStringLiteral("ctrl+alt+l");
        settings.setValue(QStringLiteral("selectionHotkey"), m_swapHotkey);
        settings.setValue(QStringLiteral("selectionHotkeyMigrated"), true);
    }
    // The clipboard converter used to be able to sit on the same combination,
    // which made Ctrl+Alt+L convert the clipboard instead of the selection.
    if (!m_langHotkey.isEmpty() && m_langHotkey.compare(m_swapHotkey, Qt::CaseInsensitive) == 0) {
        m_langHotkey = QStringLiteral("win+alt+l");
        settings.setValue(QStringLiteral("langswapHotkey"), m_langHotkey);
    }
    if (!m_hotkey.isEmpty() && m_hotkey.compare(m_swapHotkey, Qt::CaseInsensitive) == 0)
        m_hotkey = QStringLiteral("win+v");
    m_swapDirection = settings.value(QStringLiteral("swapDirection"), QStringLiteral("auto")).toString();
    m_terminalCleanCopy = settings.value(QStringLiteral("terminalCleanCopy"), true).toBool();
    m_terminalPasteChord = settings.value(QStringLiteral("terminalPasteChord"), true).toBool();
    m_terminalPlainPaste = settings.value(QStringLiteral("terminalPlainPaste"), true).toBool();
    m_notifications = settings.value(QStringLiteral("notifications"), true).toBool();
    m_programmerMode = settings.value(QStringLiteral("programmerMode"), false).toBool();
    m_snippetsEnabled = settings.value(QStringLiteral("snippetsEnabled"), true).toBool();
    m_terminalApps = settings
                         .value(QStringLiteral("terminalApps"),
                                QStringLiteral("WindowsTerminal.exe,wt.exe,cmd.exe,powershell.exe,"
                                               "pwsh.exe,conhost.exe,alacritty.exe,"
                                               "wezterm-gui.exe,mintty.exe"))
                         .toString();
    m_darkTheme = settings.value(QStringLiteral("darkTheme"), true).toBool();
    m_blockWinV = settings.value(QStringLiteral("blockWinV"), true).toBool();
    m_disableHistory = settings.value(QStringLiteral("disableWinHistory"), true).toBool();
    m_autostart = settings.value(QStringLiteral("autostart"), false).toBool();
    m_showPreview = settings.value(QStringLiteral("showPreviewInPopup"), true).toBool();
    m_closeAfterPaste = settings.value(QStringLiteral("closeAfterPaste"), true).toBool();
    m_maxItems = settings.value(QStringLiteral("maxItems"), 3000).toInt();
    m_secretTtl = settings.value(QStringLiteral("secretTtlHours"), 24).toInt();
    m_pasteDelay = settings.value(QStringLiteral("pasteDelayMs"), 120).toInt();
    m_ignoreApps = settings
                       .value(QStringLiteral("ignoreApps"),
                              QStringLiteral("1password.exe,bitwarden.exe,keepass.exe"))
                       .toString();
    m_popupFolders = settings.value(QStringLiteral("popupFolders"), QString()).toString();
    m_popupWidth = qBound(360, settings.value(QStringLiteral("popupWidth"), 398).toInt(), 1400);
    m_popupHeight = qBound(420, settings.value(QStringLiteral("popupHeight"), 500).toInt(), 1400);
}

QVariantMap AppController::settingsMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("hotkey"), m_hotkey);
    map.insert(QStringLiteral("langswapHotkey"), m_langHotkey);
    map.insert(QStringLiteral("selectionHotkey"), m_swapHotkey);
    map.insert(QStringLiteral("swapDirection"), m_swapDirection);
    map.insert(QStringLiteral("notifications"), m_notifications);
    map.insert(QStringLiteral("programmerMode"), m_programmerMode);
    map.insert(QStringLiteral("terminalCleanCopy"), m_terminalCleanCopy);
    map.insert(QStringLiteral("terminalPasteChord"), m_terminalPasteChord);
    map.insert(QStringLiteral("terminalPlainPaste"), m_terminalPlainPaste);
    map.insert(QStringLiteral("snippetsEnabled"), m_snippetsEnabled);
    map.insert(QStringLiteral("terminalApps"), m_terminalApps);
    map.insert(QStringLiteral("darkTheme"), m_darkTheme);
    map.insert(QStringLiteral("blockWinV"), m_blockWinV);
    map.insert(QStringLiteral("disableWinHistory"), m_disableHistory);
    map.insert(QStringLiteral("autostart"), m_autostart);
    map.insert(QStringLiteral("showPreviewInPopup"), m_showPreview);
    map.insert(QStringLiteral("closeAfterPaste"), m_closeAfterPaste);
    map.insert(QStringLiteral("maxItems"), m_maxItems);
    map.insert(QStringLiteral("secretTtlHours"), m_secretTtl);
    map.insert(QStringLiteral("pasteDelayMs"), m_pasteDelay);
    map.insert(QStringLiteral("ignoreApps"), m_ignoreApps);
    map.insert(QStringLiteral("popupFolders"), m_popupFolders);
    map.insert(QStringLiteral("databasePath"), m_store->databasePath());
    return map;
}

QString AppController::hotkeyText() const
{
    return HotkeyManager::formatHotkey(m_hotkey);
}

QString AppController::langswapText() const
{
    return HotkeyManager::formatHotkey(m_langHotkey);
}

QString AppController::selectionHotkeyText() const
{
    return HotkeyManager::formatHotkey(m_swapHotkey);
}

QString AppController::dataPath() const
{
    return m_store->databasePath();
}

void AppController::applyRuntimeSettings(bool initial)
{
    Q_UNUSED(initial)
    m_hotkeys->update(m_hotkey, m_langHotkey, m_swapHotkey, m_blockWinV);

    QVector<QPair<QString, QString>> expansions;
    for (const Expansion &entry : m_store->expansions()) {
        if (entry.enabled && !entry.trigger.isEmpty())
            expansions.append(qMakePair(entry.trigger, entry.expansion));
    }
    m_hotkeys->setExpansions(expansions);
    m_hotkeys->setExpansionsEnabled(m_snippetsEnabled);

    QStringList terminalApps = m_terminalApps.split(QLatin1Char(','), Qt::SkipEmptyParts);
    m_watcher->setProgrammerMode(m_programmerMode && m_terminalCleanCopy);
    m_watcher->setTerminalApps(terminalApps);
    if (m_openAction)
        m_openAction->setText(QStringLiteral("빠른 창 열기   %1").arg(hotkeyText()));
    updateTrayTooltip();

    if (WinUtils::clipboardHistoryEnabled() == m_disableHistory)
        WinUtils::setClipboardHistory(!m_disableHistory);
    if (m_autostart != WinUtils::autostartEnabled())
        WinUtils::setAutostart(m_autostart);

    Q_EMIT settingsChanged();
}

void AppController::saveSettings(const QVariantMap &values)
{
    auto text = [&values](const char *key, const QString &fallback) {
        const QVariant value = values.value(QString::fromLatin1(key));
        return value.isValid() ? value.toString() : fallback;
    };
    auto flag = [&values](const char *key, bool fallback) {
        const QVariant value = values.value(QString::fromLatin1(key));
        return value.isValid() ? value.toBool() : fallback;
    };
    auto number = [&values](const char *key, int fallback) {
        const QVariant value = values.value(QString::fromLatin1(key));
        return value.isValid() ? value.toInt() : fallback;
    };

    m_hotkey = text("hotkey", m_hotkey);
    m_langHotkey = text("langswapHotkey", m_langHotkey);
    m_swapHotkey = text("selectionHotkey", m_swapHotkey);
    if (!m_langHotkey.isEmpty() && m_langHotkey.compare(m_swapHotkey, Qt::CaseInsensitive) == 0) {
        m_langHotkey = QStringLiteral("win+alt+l");
        notify(QStringLiteral("단축키 조정"),
               QStringLiteral("선택 변환과 같은 조합이라 클립보드 변환을 Win+Alt+L로 되돌렸습니다."));
    }
    if (!m_hotkey.isEmpty() && m_hotkey.compare(m_swapHotkey, Qt::CaseInsensitive) == 0)
        m_hotkey = QStringLiteral("win+v");
    m_swapDirection = text("swapDirection", m_swapDirection);
    m_notifications = flag("notifications", m_notifications);
    m_programmerMode = flag("programmerMode", m_programmerMode);
    m_terminalCleanCopy = flag("terminalCleanCopy", m_terminalCleanCopy);
    m_terminalPasteChord = flag("terminalPasteChord", m_terminalPasteChord);
    m_terminalPlainPaste = flag("terminalPlainPaste", m_terminalPlainPaste);
    m_snippetsEnabled = flag("snippetsEnabled", m_snippetsEnabled);
    m_terminalApps = text("terminalApps", m_terminalApps);
    m_darkTheme = flag("darkTheme", m_darkTheme);
    m_blockWinV = flag("blockWinV", m_blockWinV);
    m_disableHistory = flag("disableWinHistory", m_disableHistory);
    m_autostart = flag("autostart", m_autostart);
    m_showPreview = flag("showPreviewInPopup", m_showPreview);
    m_closeAfterPaste = flag("closeAfterPaste", m_closeAfterPaste);
    m_maxItems = qMax(100, number("maxItems", m_maxItems));
    m_secretTtl = qMax(0, number("secretTtlHours", m_secretTtl));
    m_pasteDelay = qBound(0, number("pasteDelayMs", m_pasteDelay), 2000);
    m_ignoreApps = text("ignoreApps", m_ignoreApps);
    m_popupFolders = text("popupFolders", m_popupFolders);

    QSettings settings;
    settings.setValue(QStringLiteral("hotkey"), m_hotkey);
    settings.setValue(QStringLiteral("langswapHotkey"), m_langHotkey);
    settings.setValue(QStringLiteral("selectionHotkey"), m_swapHotkey);
    settings.setValue(QStringLiteral("swapDirection"), m_swapDirection);
    settings.setValue(QStringLiteral("notifications"), m_notifications);
    settings.setValue(QStringLiteral("programmerMode"), m_programmerMode);
    settings.setValue(QStringLiteral("terminalCleanCopy"), m_terminalCleanCopy);
    settings.setValue(QStringLiteral("terminalPasteChord"), m_terminalPasteChord);
    settings.setValue(QStringLiteral("terminalPlainPaste"), m_terminalPlainPaste);
    settings.setValue(QStringLiteral("snippetsEnabled"), m_snippetsEnabled);
    settings.setValue(QStringLiteral("terminalApps"), m_terminalApps);
    settings.setValue(QStringLiteral("darkTheme"), m_darkTheme);
    settings.setValue(QStringLiteral("blockWinV"), m_blockWinV);
    settings.setValue(QStringLiteral("disableWinHistory"), m_disableHistory);
    settings.setValue(QStringLiteral("autostart"), m_autostart);
    settings.setValue(QStringLiteral("showPreviewInPopup"), m_showPreview);
    settings.setValue(QStringLiteral("closeAfterPaste"), m_closeAfterPaste);
    settings.setValue(QStringLiteral("maxItems"), m_maxItems);
    settings.setValue(QStringLiteral("secretTtlHours"), m_secretTtl);
    settings.setValue(QStringLiteral("pasteDelayMs"), m_pasteDelay);
    settings.setValue(QStringLiteral("ignoreApps"), m_ignoreApps);
    settings.setValue(QStringLiteral("popupFolders"), m_popupFolders);

    applyRuntimeSettings(false);
    m_store->purge(m_maxItems, m_secretTtl);
    closeSettings();
    refreshAll();
    notify(QStringLiteral("설정 저장됨"), QStringLiteral("변경한 설정을 적용했습니다."));
}

QString AppController::formatHotkey(const QString &spec) const
{
    return HotkeyManager::formatHotkey(spec);
}

QString AppController::highlight(const QString &text, const QString &language) const
{
    return HtmlHighlighter::highlight(text, language, m_darkTheme);
}

QString AppController::plainHtml(const QString &text) const
{
    return HtmlHighlighter::plain(text, m_darkTheme);
}

QString AppController::swapText(const QString &text) const
{
    return LangSwap::swapDirected(text, m_swapDirection);
}

// ------------------------------------------------------------------------ tray
void AppController::buildTray()
{
    m_tray = new QSystemTrayIcon(trayIcon(false), this);
    m_tray->setToolTip(QStringLiteral("Cloppy — %1").arg(hotkeyText()));

    m_trayMenu = new QMenu();
    m_openAction = m_trayMenu->addAction(QStringLiteral("빠른 창 열기"));
    connect(m_openAction, &QAction::triggered, this, &AppController::openPopup);
    QAction *mainAction = m_trayMenu->addAction(QStringLiteral("메인 창 열기"));
    connect(mainAction, &QAction::triggered, this, &AppController::openMain);
    QAction *swapAction = m_trayMenu->addAction(
        QStringLiteral("선택 영역 한/영 교환   %1").arg(selectionHotkeyText()));
    connect(swapAction, &QAction::triggered, this, &AppController::swapSelection);
    m_trayMenu->addSeparator();

    m_secretAction = m_trayMenu->addAction(QStringLiteral("시크릿 모드"));
    m_secretAction->setCheckable(true);
    connect(m_secretAction, &QAction::toggled, this, &AppController::setSecretMode);

    m_pauseAction = m_trayMenu->addAction(QStringLiteral("클립보드 감시 일시정지"));
    m_pauseAction->setCheckable(true);
    connect(m_pauseAction, &QAction::toggled, this, &AppController::setPaused);

    m_trayMenu->addSeparator();
    QAction *settingsAction = m_trayMenu->addAction(QStringLiteral("설정…"));
    connect(settingsAction, &QAction::triggered, this, &AppController::openSettings);
    QAction *helpAction = m_trayMenu->addAction(QStringLiteral("사용법"));
    connect(helpAction, &QAction::triggered, this, &AppController::openHelp);
    m_trayMenu->addSeparator();
    QAction *quitAction = m_trayMenu->addAction(QStringLiteral("종료"));
    connect(quitAction, &QAction::triggered, this, &AppController::quit);

    m_tray->setContextMenu(m_trayMenu);
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger)
            openPopup();
        else if (reason == QSystemTrayIcon::DoubleClick)
            openMain();
    });
    m_tray->show();
}

void AppController::updateTrayTooltip()
{
    if (!m_tray)
        return;
    QString tooltip = QStringLiteral("Cloppy — %1").arg(hotkeyText());
    if (m_secretMode)
        tooltip += QStringLiteral(" · 시크릿 모드");
    m_tray->setToolTip(tooltip);
}

void AppController::notify(const QString &title, const QString &body)
{
    if (!m_notifications)
        return;
    if (m_tray && m_tray->isVisible())
        m_tray->showMessage(title, body, QSystemTrayIcon::Information, 1800);
}

bool AppController::isTerminalApp(const QString &processName) const
{
    if (processName.isEmpty())
        return false;
    const QStringList apps = m_terminalApps.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &app : apps) {
        if (processName.compare(app.trimmed(), Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

// --------------------------------------------------------------------- popup
void AppController::openPopup()
{
    if (m_popupVisible) {
        closePopup();
        return;
    }
    m_targetWindow = WinUtils::foregroundWindow();
    if (m_targetWindow && m_targetWindow == mainWindowHandle())
        m_targetWindow = m_mainReturnTarget;

    m_popupQuery.clear();
    m_popupKind.clear();
    m_popupView = QStringLiteral("clips");
    m_popupFolder = -1;
    m_popupModel->setFilter(popupFilter());
    m_popupModel->reload();
    Q_EMIT popupFilterChanged();

    const QPoint cursor = QCursor::pos();
    QScreen *screen = QGuiApplication::screenAt(cursor);
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    const QRect area = screen ? screen->availableGeometry() : QRect(0, 0, 1920, 1080);
    const int width = m_popupWidth;
    const int height = m_popupHeight;
    int x = cursor.x() - 80;
    int y = cursor.y() + 26;
    x = qBound(area.left() + 8, x, area.right() - width - 8);
    if (y + height > area.bottom() - 8)
        y = qMax(area.top() + 8, cursor.y() - height - 18);
    m_popupX = x;
    m_popupY = y;
    Q_EMIT popupGeometryChanged();

    m_popupVisible = true;
    Q_EMIT popupVisibleChanged();
    QTimer::singleShot(40, this, [this]() { focusWindow(QStringLiteral("cloppyPopup")); });
}

void AppController::closePopup()
{
    if (!m_popupVisible)
        return;
    m_popupVisible = false;
    Q_EMIT popupVisibleChanged();
}

void AppController::openMain()
{
    if (m_popupVisible)
        closePopup();
    const quintptr fg = WinUtils::foregroundWindow();
    if (fg && fg != mainWindowHandle())
        m_mainReturnTarget = fg;
    if (!m_mainVisible) {
        m_mainVisible = true;
        Q_EMIT mainVisibleChanged();
    }
    qInfo() << "Cloppy: openMain, mainVisible =" << m_mainVisible;
    QTimer::singleShot(50, this, [this]() { focusWindow(QStringLiteral("cloppyMain")); });
}

void AppController::hideMain()
{
    if (!m_mainVisible)
        return;
    m_mainVisible = false;
    Q_EMIT mainVisibleChanged();
}

QWindow *AppController::findWindow(const QString &objectName) const
{
    const auto windows = QGuiApplication::topLevelWindows();
    for (QWindow *window : windows) {
        if (window && window->objectName() == objectName)
            return window;
    }
    return nullptr;
}

quintptr AppController::mainWindowHandle() const
{
    if (QWindow *window = findWindow(QStringLiteral("cloppyMain")))
        return static_cast<quintptr>(window->winId());
    return 0;
}

void AppController::focusWindow(const QString &objectName)
{
    QWindow *window = findWindow(objectName);
    if (!window)
        return;
    window->requestActivate();
    WinUtils::setForeground(static_cast<quintptr>(window->winId()));
}

// -------------------------------------------------------------------- filters
ClipFilter AppController::popupFilter() const
{
    ClipFilter filter;
    filter.kind = m_popupKind;
    filter.query = m_popupQuery;
    filter.limit = 250;
    filter.secret = false;
    if (m_popupView == QStringLiteral("snippets")) {
        filter.pinnedOnly = true;
    } else {
        filter.excludePinned = true;
    }
    filter.hasFolder = m_popupFolder > 0;
    filter.folderId = m_popupFolder;
    return filter;
}

ClipFilter AppController::mainFilter() const
{
    ClipFilter filter;
    filter.query = m_mainQuery;
    filter.limit = 500;
    if (m_viewMode == QStringLiteral("snippets")) {
        filter.pinnedOnly = true;
        if (m_mainFolder > 0) {
            filter.hasFolder = true;
            filter.folderId = m_mainFolder;
        }
        if (!m_mainKind.isEmpty() && m_mainKind != QStringLiteral("all")
            && m_mainKind != QStringLiteral("favorites")) {
            filter.kind = m_mainKind;
        }
        return filter;
    }
    if (m_mainSecret) {
        filter.secret = true;
    } else if (m_mainFolder > 0) {
        filter.hasFolder = true;
        filter.folderId = m_mainFolder;
    } else if (m_mainKind == QStringLiteral("favorites")) {
        filter.favoritesOnly = true;
    } else if (!m_mainKind.isEmpty() && m_mainKind != QStringLiteral("all")) {
        filter.kind = m_mainKind;
    }
    return filter;
}

void AppController::reloadPopup()
{
    m_popupAssetsCache.clear();
    const QString needle = m_popupQuery.trimmed();
    for (const AssetInfo &asset : m_store->assets(-1)) {
        if (!needle.isEmpty() && !asset.name.contains(needle, Qt::CaseInsensitive))
            continue;
        m_popupAssetsCache.append(asset.toMap());
    }
    m_popupModel->setFilter(popupFilter());
    m_popupModel->reload();
}

void AppController::reloadMain()
{
    m_mainModel->setFilter(mainFilter());
    m_mainModel->reload();
}

void AppController::refreshAll()
{
    reloadPopup();
    reloadMain();
    refreshMeta();
}

void AppController::refreshMeta()
{
    const Counts counts = m_store->counts();
    m_countsCache.clear();
    m_countsCache.insert(QStringLiteral("all"), counts.all);
    m_countsCache.insert(QStringLiteral("favorite"), counts.favorites);
    m_countsCache.insert(QStringLiteral("secret"), counts.secret);
    m_countsCache.insert(QStringLiteral("snippet"), counts.snippets);
    for (auto it = counts.kinds.cbegin(); it != counts.kinds.cend(); ++it)
        m_countsCache.insert(it.key(), it.value());

    const auto folderList = [this](const QString &scope) {
        QVariantList list;
        const QVector<FolderInfo> folders = m_store->folders(scope);
        for (const FolderInfo &folder : folders) {
            QVariantMap map;
            map.insert(QStringLiteral("id"), static_cast<double>(folder.id));
            map.insert(QStringLiteral("name"), folder.name);
            map.insert(QStringLiteral("color"), folder.color);
            map.insert(QStringLiteral("count"), folder.count);
            map.insert(QStringLiteral("parentId"), static_cast<double>(folder.parentId));
            map.insert(QStringLiteral("depth"), folder.depth);
            map.insert(QStringLiteral("scope"), folder.scope);
            list.append(map);
        }
        return list;
    };
    m_foldersCache = folderList(QStringLiteral("clip"));
    m_assetFoldersCache = folderList(QStringLiteral("asset"));

    m_assetsCache.clear();
    const QVector<AssetInfo> allAssets = m_store->assets(-1);
    QHash<qint64, int> assetCounts;
    for (const AssetInfo &asset : allAssets) {
        if (asset.folderId > 0)
            assetCounts[asset.folderId] += 1;
        if (m_assetFolder < 0 || asset.folderId == m_assetFolder)
            m_assetsCache.append(asset.toMap());
    }
    for (int i = 0; i < m_assetFoldersCache.size(); ++i) {
        QVariantMap map = m_assetFoldersCache.at(i).toMap();
        map.insert(QStringLiteral("count"),
                   assetCounts.value(static_cast<qint64>(map.value(QStringLiteral("id")).toDouble()), 0));
        m_assetFoldersCache[i] = map;
    }

    m_expansionsCache.clear();
    for (const Expansion &entry : m_store->expansions())
        m_expansionsCache.append(entry.toMap());

    m_statsCache = m_store->stats();
    QVariantMap stats = m_statsCache;
    stats.insert(QStringLiteral("path"), m_store->databasePath());
    m_statsCache = stats;

    m_searchCache = m_store->searches(15);
    rebuildQuickFolders();
    Q_EMIT dataChanged();
    Q_EMIT expansionsChanged();
    Q_EMIT searchHistoryChanged();
}

void AppController::rebuildQuickFolders()
{
    // Folders are listed in the popup in the order chosen in settings.
    QVariantList list;
    const QStringList parts = m_popupFolders.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        bool ok = false;
        const qint64 id = part.trimmed().toLongLong(&ok);
        if (!ok || id <= 0)
            continue;
        for (const QVariant &entry : m_foldersCache) {
            const QVariantMap map = entry.toMap();
            if (static_cast<qint64>(map.value(QStringLiteral("id")).toDouble()) == id) {
                list.append(map);
                break;
            }
        }
    }
    if (list == m_quickFoldersCache)
        return;
    m_quickFoldersCache = list;
    Q_EMIT popupFilterChanged();
}

void AppController::setViewMode(const QString &mode)
{
    const QString normalized = (mode == QStringLiteral("snippets") || mode == QStringLiteral("assets"))
        ? mode
        : QStringLiteral("clips");
    if (m_viewMode == normalized)
        return;
    m_viewMode = normalized;
    m_mainFolder = -1;
    m_mainKind = QStringLiteral("all");
    m_mainSecret = false;
    m_assetFolder = -1;
    reloadMain();
    refreshMeta();
    Q_EMIT viewChanged();
    Q_EMIT mainFilterChanged();
}

void AppController::setAssetFolder(qint64 folderId)
{
    if (m_assetFolder == folderId)
        return;
    m_assetFolder = folderId;
    refreshMeta();
    Q_EMIT viewChanged();
}

QString AppController::mainTitle() const
{
    if (m_viewMode == QStringLiteral("assets"))
        return QStringLiteral("에셋");
    if (m_viewMode == QStringLiteral("snippets")) {
        if (m_mainFolder > 0) {
            for (const FolderInfo &folder : m_store->folders()) {
                if (folder.id == m_mainFolder)
                    return folder.name;
            }
        }
        return QStringLiteral("코드 블록");
    }
    if (m_mainSecret)
        return QStringLiteral("시크릿 클립");
    if (m_mainFolder > 0) {
        const QVector<FolderInfo> folders = m_store->folders();
        for (const FolderInfo &folder : folders) {
            if (folder.id == m_mainFolder)
                return folder.name;
        }
        return QStringLiteral("폴더");
    }
    return kindLabel(m_mainKind);
}

void AppController::setPopupKind(const QString &kind)
{
    if (m_popupKind == kind)
        return;
    m_popupKind = kind;
    reloadPopup();
    Q_EMIT popupFilterChanged();
}

void AppController::setPopupQuery(const QString &query)
{
    if (m_popupQuery == query)
        return;
    m_popupQuery = query;
    reloadPopup();
    Q_EMIT popupFilterChanged();
}

void AppController::setPopupFolder(qint64 folderId)
{
    if (m_popupFolder == folderId)
        return;
    m_popupFolder = folderId;
    reloadPopup();
    Q_EMIT popupFilterChanged();
}

void AppController::setPopupView(const QString &view)
{
    const QString normalized = (view == QStringLiteral("snippets") || view == QStringLiteral("assets"))
        ? view
        : QStringLiteral("clips");
    if (m_popupView == normalized)
        return;
    m_popupView = normalized;
    m_popupKind.clear();
    qInfo() << "Cloppy: popup view =" << m_popupView;
    reloadPopup();
    Q_EMIT popupFilterChanged();
}

void AppController::setPopupSize(int width, int height)
{
    const int w = qBound(360, width, 1400);
    const int h = qBound(420, height, 1400);
    if (m_popupWidth == w && m_popupHeight == h)
        return;
    m_popupWidth = w;
    m_popupHeight = h;
    QSettings settings;
    settings.setValue(QStringLiteral("popupWidth"), m_popupWidth);
    settings.setValue(QStringLiteral("popupHeight"), m_popupHeight);
    Q_EMIT popupGeometryChanged();
}

QPoint AppController::cursorPos() const
{
    return QCursor::pos();
}

void AppController::setMainKind(const QString &kind)
{
    m_mainKind = kind.isEmpty() ? QStringLiteral("all") : kind;
    m_mainFolder = -1;
    m_mainSecret = false;
    reloadMain();
    Q_EMIT mainFilterChanged();
}

void AppController::setMainFolder(qint64 folderId)
{
    m_mainFolder = folderId;
    m_mainKind.clear();
    m_mainSecret = false;
    reloadMain();
    Q_EMIT mainFilterChanged();
}

void AppController::setPinned(qint64 id, bool pinned)
{
    m_store->setPinned(id, pinned);
    refreshAll();
    Q_EMIT mainFilterChanged();
    notify(pinned ? QStringLiteral("코드 블록으로 고정됨")
                  : QStringLiteral("고정 해제됨"),
           pinned ? QStringLiteral("자동 정리에서 제외되고 코드 블록 보기에 보관됩니다.")
                  : QStringLiteral("이제 일반 클립으로 정리될 수 있습니다."));
}

void AppController::setMainSecretView(bool enabled)
{
    m_mainSecret = enabled;
    if (enabled) {
        m_mainFolder = -1;
        m_mainKind.clear();
    } else if (m_mainKind.isEmpty()) {
        m_mainKind = QStringLiteral("all");
    }
    reloadMain();
    Q_EMIT mainFilterChanged();
}

void AppController::setMainQuery(const QString &query)
{
    if (m_mainQuery == query)
        return;
    m_mainQuery = query;
    reloadMain();
    Q_EMIT mainFilterChanged();
}

void AppController::commitSearch(const QString &query)
{
    m_store->addSearch(query);
    refreshMeta();
}

// ----------------------------------------------------------------- clipboard
void AppController::onCaptured(const CapturePayload &payload)
{
    const QStringList ignored = m_ignoreApps.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &name : ignored) {
        if (payload.sourceApp.compare(name.trimmed(), Qt::CaseInsensitive) == 0)
            return;
    }
    m_store->addCapture(payload, m_secretMode, payload.sourceApp);
    ++m_captureCount;
    if (m_captureCount % 25 == 0)
        m_store->purge(m_maxItems, m_secretTtl);
    refreshAll();
}

bool AppController::writeClipboard(const ClipItem &item, bool plain)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return false;

    if (item.kind == QStringLiteral("image") && !item.imagePath.isEmpty()
        && QFile::exists(item.imagePath)) {
        const QImage image(item.imagePath);
        if (!image.isNull()) {
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            image.save(&buffer, "PNG");
            m_watcher->guard(QStringLiteral("image:") + sha1Hex(bytes));
            clipboard->setImage(image);
            return true;
        }
    }

    QString text = item.content;
    if (text.isEmpty())
        text = item.files.join(QLatin1Char('\n'));

    if (item.kind == QStringLiteral("files") && !plain && !item.files.isEmpty()) {
        auto *mime = new QMimeData();
        QList<QUrl> urls;
        for (const QString &path : item.files)
            urls.append(QUrl::fromLocalFile(path));
        mime->setUrls(urls);
        mime->setText(item.files.join(QLatin1Char('\n')));
        m_watcher->guard(filesHash(item.files));
        clipboard->setMimeData(mime);
        return true;
    }

    m_watcher->guard(textHash(text));
    clipboard->setText(text);
    return true;
}

quintptr AppController::currentPasteTarget() const
{
    if (m_popupVisible)
        return m_targetWindow;
    const quintptr foreground = WinUtils::foregroundWindow();
    const quintptr mainHandle = mainWindowHandle();
    if (foreground && foreground != mainHandle)
        return foreground;
    return m_targetWindow;
}

void AppController::schedulePaste(quintptr target)
{
    if (!target)
        return;
    const int delay = qMax(20, m_pasteDelay);
    const bool terminal = m_programmerMode && m_terminalPasteChord
        && isTerminalApp(WinUtils::processNameForWindow(target));
    QTimer::singleShot(delay, this, [this, target, terminal]() {
        WinUtils::setForeground(target);
        QTimer::singleShot(90, this, [terminal]() {
            if (terminal)
                WinUtils::sendTerminalPaste();
            else
                WinUtils::sendPaste();
        });
    });
}

void AppController::pasteItem(qint64 id, bool plain)
{
    const ClipItem item = m_store->item(id);
    if (item.id == 0)
        return;
    const bool fromPopup = m_popupVisible;
    const quintptr target = fromPopup ? m_targetWindow : currentPasteTarget();
    const bool forcePlain = plain
        || (m_programmerMode && m_terminalPlainPaste
            && isTerminalApp(WinUtils::processNameForWindow(target)));
    if (!writeClipboard(item, forcePlain))
        return;
    m_store->touch(id);
    if (fromPopup)
        closePopup();
    refreshAll();
    schedulePaste(target);
}

void AppController::pasteAs(qint64 id, const QString &format)
{
    const ClipItem item = m_store->item(id);
    if (item.id == 0)
        return;
    QString source = item.content;
    if (source.isEmpty())
        source = item.files.join(QLatin1Char('\n'));
    const QString transformed = transformText(source, format);
    if (transformed.isEmpty())
        return;

    const bool fromPopup = m_popupVisible;
    const quintptr target = fromPopup ? m_targetWindow : currentPasteTarget();
    m_watcher->guard(textHash(transformed));
    QGuiApplication::clipboard()->setText(transformed);
    m_store->touch(id);
    if (fromPopup)
        closePopup();
    refreshAll();
    schedulePaste(target);
    notify(QStringLiteral("변환하여 붙여넣기"), format);
}

QString AppController::transformText(const QString &text, const QString &format) const
{
    const QString key = format.toLower();
    if (key.isEmpty() || key == QStringLiteral("plain"))
        return text;

    if (key == QStringLiteral("json") || key == QStringLiteral("json-pretty")) {
        QJsonParseError error;
        const QJsonDocument document = QJsonDocument::fromJson(text.trimmed().toUtf8(), &error);
        if (error.error == QJsonParseError::NoError && !document.isNull()) {
            return QString::fromUtf8(document.toJson(key == QStringLiteral("json-pretty")
                                                        ? QJsonDocument::Indented
                                                        : QJsonDocument::Compact));
        }
        QString escaped = text;
        escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
        escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
        escaped.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
        escaped.replace(QLatin1Char('\r'), QStringLiteral("\\r"));
        escaped.replace(QLatin1Char('\t'), QStringLiteral("\\t"));
        return QStringLiteral("\"%1\"").arg(escaped);
    }

    if (key == QStringLiteral("html")) {
        QString escaped = text.toHtmlEscaped();
        escaped.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
        escaped.replace(QLatin1Char('\n'), QStringLiteral("<br>"));
        return escaped;
    }

    if (key == QStringLiteral("markdown")) {
        QString body = text;
        if (body.endsWith(QLatin1Char('\n')))
            body.chop(1);
        return QStringLiteral("```\n%1\n```").arg(body);
    }

    if (key == QStringLiteral("quote")) {
        QString escaped = text;
        escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
        escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
        escaped.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
        return QStringLiteral("\"%1\"").arg(escaped);
    }

    if (key == QStringLiteral("base64"))
        return QString::fromLatin1(text.toUtf8().toBase64());

    if (key == QStringLiteral("oneline")) {
        QString result = text;
        result.replace(QRegularExpression(QStringLiteral("[\\r\\n]+")), QStringLiteral(" "));
        return result.simplified();
    }

    return text;
}

QString AppController::colorFor(const QString &text) const
{
    const QString value = text.trimmed();
    if (value.isEmpty())
        return QStringLiteral("transparent");

    static const QRegularExpression rgbPattern(
        QStringLiteral("^rgba?\\(\\s*([0-9.]+%?)\\s*,\\s*([0-9.]+%?)\\s*,\\s*([0-9.]+%?)"
                       "(?:\\s*,\\s*([0-9.]+%?))?\\s*\\)$"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch rgb = rgbPattern.match(value);
    if (rgb.hasMatch()) {
        const auto channel = [](const QString &part, int max) {
            if (part.endsWith(QLatin1Char('%')))
                return qBound(0, qRound(part.left(part.size() - 1).toDouble() * max / 100.0), max);
            return qBound(0, qRound(part.toDouble()), max);
        };
        const int alpha = rgb.captured(4).isEmpty()
            ? 255
            : (rgb.captured(4).endsWith(QLatin1Char('%'))
                   ? qBound(0, qRound(rgb.captured(4).left(rgb.captured(4).size() - 1).toDouble() * 2.55), 255)
                   : qBound(0, qRound(rgb.captured(4).toDouble() * 255), 255));
        const QColor color(channel(rgb.captured(1), 255), channel(rgb.captured(2), 255),
                           channel(rgb.captured(3), 255), alpha);
        if (color.isValid())
            return color.name(QColor::HexArgb);
    }

    const QColor direct(value);
    if (direct.isValid())
        return direct.name(QColor::HexArgb);
    return QStringLiteral("transparent");
}

void AppController::copyItem(qint64 id)
{
    const ClipItem item = m_store->item(id);
    if (item.id == 0)
        return;
    if (!writeClipboard(item, false))
        return;
    m_store->touch(id);
    if (m_popupVisible)
        closePopup();
    refreshAll();
    notify(QStringLiteral("클립보드에 복사됨"), item.title);
}

void AppController::swapGlobalClipboard()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return;
    const QString text = clipboard->text();
    const QString swapped = LangSwap::swapDirected(text, m_swapDirection);
    if (text.isEmpty() || swapped == text) {
        notify(QStringLiteral("한/영 교환"), QStringLiteral("변환할 한글 또는 영문 텍스트가 없습니다."));
        return;
    }
    m_watcher->guard(textHash(swapped));
    clipboard->setText(swapped);
    notify(QStringLiteral("한/영 교환"), swapped.left(80));
}

void AppController::swapSelection()
{
    qInfo() << "Cloppy: selection swap hotkey triggered";
    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return;
    const quintptr target = WinUtils::foregroundWindow();
    if (!target || target == mainWindowHandle())
        return;

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastSelectionSwap < 600)
        return;
    m_lastSelectionSwap = now;

    m_swapTarget = target;
    m_swapSaved = clipboard->text();
    m_swapSequence = WinUtils::clipboardSequence();
    m_swapTerminal = m_programmerMode && m_terminalPasteChord
        && isTerminalApp(WinUtils::processNameForWindow(target));
    m_swapDone = false;
    m_swapTries = 0;
    m_swapWaitTicks = 0;

    QTimer::singleShot(60, this, &AppController::continueSelectionSwap);
}

void AppController::continueSelectionSwap()
{
    if (m_swapDone)
        return;
    // The hotkey is still held down right after WM_HOTKEY; sending Ctrl+C now
    // would become Ctrl+Alt+C, so wait until the modifiers are released.
    if (WinUtils::modifiersDown() && m_swapWaitTicks < 15) {
        ++m_swapWaitTicks;
        QTimer::singleShot(40, this, &AppController::continueSelectionSwap);
        return;
    }

    if (m_swapTerminal)
        WinUtils::sendTerminalCopy();
    else
        WinUtils::sendCopy();

    for (int delay : {140, 260, 400, 560, 740, 920})
        QTimer::singleShot(delay, this, &AppController::checkSelectionSwap);
}

void AppController::checkSelectionSwap()
{
    if (m_swapDone)
        return;
    ++m_swapTries;

    if (WinUtils::clipboardSequence() == m_swapSequence) {
        if (m_swapTries >= 6) {
            m_swapDone = true;
            notify(QStringLiteral("선택 영역 한/영 교환"),
                   QStringLiteral("선택한 텍스트를 복사하지 못했습니다. 텍스트를 선택한 뒤 다시 눌러 주세요."));
        }
        return;
    }

    QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return;

    const QString text = clipboard->text();
    const QString swapped = LangSwap::swapDirected(text, m_swapDirection);
    if (text.isEmpty() || swapped == text) {
        m_swapDone = true;
        if (!m_swapSaved.isEmpty()) {
            m_watcher->guard(textHash(m_swapSaved));
            clipboard->setText(m_swapSaved);
        }
        notify(QStringLiteral("선택 영역 한/영 교환"),
               QStringLiteral("변환할 한글/영문을 찾지 못했습니다."));
        return;
    }

    m_swapDone = true;
    const QString saved = m_swapSaved;
    const quintptr target = m_swapTarget;
    const bool terminal = m_swapTerminal;

    m_watcher->guard(textHash(swapped));
    clipboard->setText(swapped);
    m_store->addCapture(CapturePayload{QStringLiteral("text"), swapped, {}, {}, {}, textHash(swapped),
                                       WinUtils::foregroundProcessName(), {}, swapped.toUtf8().size()},
                        false, WinUtils::foregroundProcessName());
    refreshAll();
    QTimer::singleShot(70, this, [this, target, terminal]() {
        WinUtils::setForeground(target);
        QTimer::singleShot(60, this, [this, terminal]() {
            if (terminal)
                WinUtils::sendTerminalPaste();
            else
                WinUtils::sendPaste();
        });
    });
    QTimer::singleShot(700, this, [this, clipboard, saved, swapped]() {
        if (!saved.isEmpty() && clipboard->text() == swapped) {
            m_watcher->guard(textHash(saved));
            clipboard->setText(saved);
        }
    });
    notify(QStringLiteral("선택 영역 한/영 교환"), swapped.left(80));
}

// -------------------------------------------------------------------- actions
void AppController::toggleFavorite(qint64 id)
{
    const ClipItem item = m_store->item(id);
    if (item.id == 0)
        return;
    m_store->setFavorite(id, !item.favorite);
    refreshAll();
}

void AppController::toggleSecret(qint64 id)
{
    const ClipItem item = m_store->item(id);
    if (item.id == 0)
        return;
    m_store->setSecret(id, !item.secret);
    refreshAll();
}

void AppController::deleteItem(qint64 id)
{
    m_store->remove(id);
    refreshAll();
}

void AppController::openEdit(qint64 id)
{
    const ClipItem item = m_store->item(id);
    if (item.id == 0)
        return;
    if (item.kind == QStringLiteral("image") || item.kind == QStringLiteral("files")) {
        notify(QStringLiteral("편집할 수 없음"), QStringLiteral("이미지와 파일 항목은 내용을 편집할 수 없습니다."));
        return;
    }
    m_editId = id;
    m_editText = item.content;
    m_editTitle = item.title;
    m_editVisible = true;
    updateExpansionBlock();
    Q_EMIT editVisibleChanged();
}

void AppController::saveEdit(const QString &text)
{
    if (m_editId > 0 && text != m_editText) {
        m_store->updateContent(m_editId, text);
        refreshAll();
    }
    closeEdit();
}

void AppController::closeEdit()
{
    if (!m_editVisible)
        return;
    m_editVisible = false;
    m_editId = 0;
    m_editText.clear();
    m_editTitle.clear();
    updateExpansionBlock();
    Q_EMIT editVisibleChanged();
}

void AppController::swapLanguage(qint64 id)
{
    const ClipItem item = m_store->item(id);
    if (item.id == 0)
        return;
    QString text = item.content;
    if (text.isEmpty())
        text = item.files.join(QLatin1Char('\n'));
    const QString swapped = LangSwap::swapDirected(text, m_swapDirection);
    if (swapped.isEmpty() || swapped == text) {
        notify(QStringLiteral("한/영 교환"), QStringLiteral("변환할 한글 또는 영문이 없습니다."));
        return;
    }
    const bool fromPopup = m_popupVisible;
    const quintptr target = m_targetWindow;
    m_store->updateContent(id, swapped);
    m_watcher->guard(textHash(swapped));
    QGuiApplication::clipboard()->setText(swapped);
    m_store->touch(id);
    if (fromPopup)
        closePopup();
    refreshAll();
    notify(QStringLiteral("한/영 교환"), swapped.left(80));
    if (fromPopup)
        schedulePaste(target);
}

void AppController::copyCodeBlock(qint64 id)
{
    const ClipItem item = m_store->item(id);
    if (item.id == 0)
        return;
    const QString language = item.language.isEmpty()
        ? (item.kind == QStringLiteral("code") ? QStringLiteral("text") : QStringLiteral("text"))
        : item.language;
    QString body = item.content;
    if (body.isEmpty())
        body = item.files.join(QLatin1Char('\n'));
    const QString block = QStringLiteral("```%1\n%2\n```").arg(language, body.trimmed());
    m_watcher->guard(textHash(block));
    QGuiApplication::clipboard()->setText(block);
    notify(QStringLiteral("코드 블록 복사"), QStringLiteral("```%1 블록으로 복사했습니다.").arg(language));
}

void AppController::runTerminal(qint64 id)
{
    const ClipItem item = m_store->item(id);
    if (item.id == 0)
        return;
    QString command = item.content.trimmed();
    if (command.isEmpty() && !item.files.isEmpty())
        command = QStringLiteral("cd /d \"%1\"").arg(QFileInfo(item.files.first()).absolutePath());
    if (command.isEmpty())
        return;
    if (WinUtils::runInTerminal(command)) {
        if (m_popupVisible)
            closePopup();
        notify(QStringLiteral("터미널에서 실행"), command.left(80));
    }
}

void AppController::openItem(qint64 id)
{
    const ClipItem item = m_store->item(id);
    if (item.id == 0)
        return;
    if (item.kind == QStringLiteral("link")) {
        QDesktopServices::openUrl(QUrl(item.content.trimmed()));
    } else if (!item.files.isEmpty()) {
        WinUtils::openPath(item.files.first());
    } else if (item.kind == QStringLiteral("path") && QFileInfo::exists(item.content.trimmed())) {
        WinUtils::openPath(item.content.trimmed());
    } else {
        notify(QStringLiteral("열 수 없음"), QStringLiteral("경로나 링크 항목이 아닙니다."));
    }
}

void AppController::revealItem(qint64 id)
{
    const ClipItem item = m_store->item(id);
    if (item.id == 0)
        return;
    if (!item.files.isEmpty()) {
        WinUtils::revealInExplorer(item.files.first());
    } else if (QFileInfo::exists(item.content.trimmed())) {
        WinUtils::revealInExplorer(item.content.trimmed());
    }
}

// ------------------------------------------------------------------- settings
void AppController::openSettings()
{
    if (m_settingsVisible)
        return;
    m_settingsVisible = true;
    updateExpansionBlock();
    Q_EMIT settingsVisibleChanged();
}

void AppController::closeSettings()
{
    if (!m_settingsVisible)
        return;
    m_settingsVisible = false;
    updateExpansionBlock();
    Q_EMIT settingsVisibleChanged();
}

void AppController::openHelp()
{
    if (m_popupVisible)
        closePopup();
    if (m_helpVisible)
        return;
    m_helpVisible = true;
    updateExpansionBlock();
    Q_EMIT helpVisibleChanged();
}

void AppController::closeHelp()
{
    if (!m_helpVisible)
        return;
    m_helpVisible = false;
    updateExpansionBlock();
    Q_EMIT helpVisibleChanged();
}

void AppController::setCapturing(bool capturing)
{
    m_hotkeys->setCapturing(capturing);
}

void AppController::updateExpansionBlock()
{
    // Expansion works everywhere, including Cloppy's own windows, except while
    // a dialog is open that edits triggers or snippets.
    m_hotkeys->setExpansionBlocked(m_settingsVisible || m_editVisible);
}

void AppController::setPopupPreview(bool visible)
{
    if (m_showPreview == visible)
        return;
    m_showPreview = visible;
    QSettings().setValue(QStringLiteral("showPreviewInPopup"), m_showPreview);
    Q_EMIT settingsChanged();
}

void AppController::setSecretMode(bool enabled)
{
    if (m_secretMode == enabled) {
        if (m_secretAction) {
            m_secretAction->blockSignals(true);
            m_secretAction->setChecked(enabled);
            m_secretAction->blockSignals(false);
        }
        return;
    }
    m_secretMode = enabled;
    if (m_secretAction) {
        m_secretAction->blockSignals(true);
        m_secretAction->setChecked(enabled);
        m_secretAction->blockSignals(false);
    }
    if (m_tray)
        m_tray->setIcon(trayIcon(enabled));
    updateTrayTooltip();
    Q_EMIT secretModeChanged();
    notify(enabled ? QStringLiteral("시크릿 모드 켜짐") : QStringLiteral("시크릿 모드 꺼짐"),
           enabled ? QStringLiteral("이제 복사하는 내용은 숨김으로 저장됩니다.")
                   : QStringLiteral("일반 모드로 복사 내용이 기록됩니다."));
}

void AppController::toggleSecretMode()
{
    setSecretMode(!m_secretMode);
}

void AppController::setPaused(bool paused)
{
    if (m_paused == paused) {
        if (m_pauseAction) {
            m_pauseAction->blockSignals(true);
            m_pauseAction->setChecked(paused);
            m_pauseAction->blockSignals(false);
        }
        return;
    }
    m_paused = paused;
    m_watcher->setEnabled(!paused);
    if (m_pauseAction) {
        m_pauseAction->blockSignals(true);
        m_pauseAction->setChecked(paused);
        m_pauseAction->blockSignals(false);
    }
    Q_EMIT pausedChanged();
    notify(paused ? QStringLiteral("감시 일시정지") : QStringLiteral("감시 재개"),
           paused ? QStringLiteral("복사 내용을 기록하지 않습니다.")
                  : QStringLiteral("다시 클립보드를 기록합니다."));
}

// -------------------------------------------------------------------- folders
void AppController::addFolder(const QString &name, qint64 parentId, const QString &color,
                              const QString &scope)
{
    if (name.trimmed().isEmpty())
        return;
    m_store->addFolder(name, color, parentId, scope.isEmpty() ? QStringLiteral("clip") : scope);
    refreshMeta();
}

void AppController::addSnippet(const QString &title, const QString &content,
                               const QString &language, qint64 folderId)
{
    if (content.trimmed().isEmpty())
        return;
    m_store->addSnippet(title, content, language, folderId);
    refreshAll();
    Q_EMIT mainFilterChanged();
    notify(QStringLiteral("코드 블록 저장됨"), title);
}

void AppController::updateSnippet(qint64 id, const QString &title, const QString &content,
                                  const QString &language, qint64 folderId)
{
    if (id <= 0)
        return;
    m_store->updateSnippet(id, title, content, language, folderId);
    refreshAll();
    Q_EMIT mainFilterChanged();
}

void AppController::saveExpansions(const QVariantList &rows)
{
    m_store->replaceExpansions(rows);
    applyRuntimeSettings(false);
    refreshMeta();
}

void AppController::addAssets()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        nullptr, QStringLiteral("에셋 추가"), QDir::homePath(), QStringLiteral("모든 파일 (*.*)"));
    if (files.isEmpty())
        return;
    int added = 0;
    for (const QString &path : files) {
        if (m_store->addAsset(path, m_assetFolder) > 0)
            ++added;
    }
    refreshMeta();
    notify(QStringLiteral("에셋 추가"), QStringLiteral("%1개 파일을 추가했습니다.").arg(added));
}

void AppController::removeAsset(qint64 id)
{
    m_store->removeAsset(id);
    refreshMeta();
}

void AppController::renameAsset(qint64 id, const QString &name)
{
    m_store->renameAsset(id, name);
    refreshMeta();
}

void AppController::moveAsset(qint64 assetId, qint64 folderId)
{
    m_store->setAssetFolder(assetId, folderId);
    refreshMeta();
}

void AppController::openAsset(qint64 id)
{
    for (const AssetInfo &asset : m_store->assets(-1)) {
        if (asset.id == id) {
            WinUtils::openPath(asset.path);
            return;
        }
    }
}

void AppController::copyAssetPath(qint64 id)
{
    for (const AssetInfo &asset : m_store->assets(-1)) {
        if (asset.id == id) {
            m_watcher->guard(textHash(asset.path));
            QGuiApplication::clipboard()->setText(asset.path);
            notify(QStringLiteral("에셋 경로 복사"), asset.name);
            return;
        }
    }
}

void AppController::renameFolder(qint64 id, const QString &name)
{
    m_store->renameFolder(id, name);
    refreshAll();
    Q_EMIT mainFilterChanged();
}

void AppController::setFolderColor(qint64 id, const QString &color)
{
    m_store->setFolderColor(id, color);
    refreshMeta();
}

void AppController::moveFolder(qint64 id, qint64 parentId)
{
    m_store->moveFolder(id, parentId);
    refreshMeta();
}

void AppController::deleteFolder(qint64 id)
{
    m_store->deleteFolder(id);
    refreshMeta();
    bool stillExists = false;
    for (const FolderInfo &folder : m_store->folders()) {
        if (folder.id == m_mainFolder)
            stillExists = true;
    }
    if (m_mainFolder > 0 && !stillExists) {
        m_mainFolder = -1;
        m_mainKind = QStringLiteral("all");
        Q_EMIT mainFilterChanged();
    }
    refreshAll();
}

void AppController::moveToFolder(qint64 itemId, qint64 folderId)
{
    m_store->setFolder(itemId, folderId);
    refreshAll();
}

void AppController::clearSearches()
{
    m_store->clearSearches();
    refreshMeta();
}

void AppController::clearItems()
{
    m_store->clearItems(true);
    refreshAll();
    notify(QStringLiteral("클립 정리"), QStringLiteral("즐겨찾기를 제외한 클립을 삭제했습니다."));
}

void AppController::openDataFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(m_store->databasePath()).absolutePath()));
}

void AppController::quit()
{
    if (m_quitting)
        return;
    m_quitting = true;
    m_watcher->setEnabled(false);
    m_hotkeys->stop();
    if (m_tray)
        m_tray->hide();
    QCoreApplication::quit();
}
