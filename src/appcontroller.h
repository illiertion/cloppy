#pragma once

#include <QObject>
#include <QPoint>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "types.h"

class ClipModel;
class ClipStore;
class ClipboardWatcher;
class HotkeyManager;
class QAction;
class QMenu;
class QSystemTrayIcon;
class QWindow;

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool popupVisible READ popupVisible NOTIFY popupVisibleChanged)
    Q_PROPERTY(bool mainVisible READ mainVisible NOTIFY mainVisibleChanged)
    Q_PROPERTY(int popupX READ popupX NOTIFY popupGeometryChanged)
    Q_PROPERTY(int popupY READ popupY NOTIFY popupGeometryChanged)
    Q_PROPERTY(bool darkTheme READ darkTheme NOTIFY settingsChanged)
    Q_PROPERTY(QString language READ language NOTIFY languageChanged)
    Q_PROPERTY(QVariantMap settings READ settingsMap NOTIFY settingsChanged)
    Q_PROPERTY(QString hotkeyText READ hotkeyText NOTIFY settingsChanged)
    Q_PROPERTY(QString langswapText READ langswapText NOTIFY settingsChanged)
    Q_PROPERTY(QString selectionHotkeyText READ selectionHotkeyText NOTIFY settingsChanged)
    Q_PROPERTY(QString hotkeyStatus READ hotkeyStatus NOTIFY settingsChanged)
    Q_PROPERTY(bool secretMode READ secretMode NOTIFY secretModeChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(QVariantMap counts READ counts NOTIFY dataChanged)
    Q_PROPERTY(QVariantList folders READ folders NOTIFY dataChanged)
    Q_PROPERTY(QVariantList assetFolders READ assetFolders NOTIFY dataChanged)
    Q_PROPERTY(QVariantList assets READ assets NOTIFY dataChanged)
    Q_PROPERTY(QVariantList expansions READ expansions NOTIFY expansionsChanged)
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY dataChanged)
    Q_PROPERTY(QStringList searchHistory READ searchHistory NOTIFY searchHistoryChanged)
    Q_PROPERTY(QString viewMode READ viewMode NOTIFY viewChanged)
    Q_PROPERTY(QString mainKind READ mainKind NOTIFY mainFilterChanged)
    Q_PROPERTY(qint64 mainFolder READ mainFolder NOTIFY mainFilterChanged)
    Q_PROPERTY(bool mainSecret READ mainSecret NOTIFY mainFilterChanged)
    Q_PROPERTY(QString mainTitle READ mainTitle NOTIFY mainFilterChanged)
    Q_PROPERTY(QString mainQuery READ mainQuery NOTIFY mainFilterChanged)
    Q_PROPERTY(qint64 assetFolder READ assetFolder NOTIFY viewChanged)
    Q_PROPERTY(QString popupKind READ popupKind NOTIFY popupFilterChanged)
    Q_PROPERTY(QString popupQuery READ popupQuery NOTIFY popupFilterChanged)
    Q_PROPERTY(qint64 popupFolder READ popupFolder NOTIFY popupFilterChanged)
    Q_PROPERTY(QVariantList quickFolders READ quickFolders NOTIFY popupFilterChanged)
    Q_PROPERTY(QString popupView READ popupView NOTIFY popupFilterChanged)
    Q_PROPERTY(QVariantList popupAssets READ popupAssets NOTIFY popupFilterChanged)
    Q_PROPERTY(int popupWidth READ popupWidth NOTIFY popupGeometryChanged)
    Q_PROPERTY(int popupHeight READ popupHeight NOTIFY popupGeometryChanged)
    Q_PROPERTY(bool popupPreview READ popupPreview NOTIFY settingsChanged)
    Q_PROPERTY(bool settingsVisible READ settingsVisible NOTIFY settingsVisibleChanged)
    Q_PROPERTY(bool helpVisible READ helpVisible NOTIFY helpVisibleChanged)
    Q_PROPERTY(bool licenseVisible READ licenseVisible NOTIFY licenseVisibleChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    Q_PROPERTY(QString qtVersion READ qtVersion CONSTANT)
    Q_PROPERTY(bool editVisible READ editVisible NOTIFY editVisibleChanged)
    Q_PROPERTY(QString editText READ editText NOTIFY editVisibleChanged)
    Q_PROPERTY(QString editTitle READ editTitle NOTIFY editVisibleChanged)
    Q_PROPERTY(QString dataPath READ dataPath NOTIFY dataChanged)

public:
    explicit AppController(ClipStore *store, QObject *parent = nullptr);
    ~AppController() override;

    ClipModel *popupModel() { return m_popupModel; }
    ClipModel *mainModel() { return m_mainModel; }

    bool popupVisible() const { return m_popupVisible; }
    bool mainVisible() const { return m_mainVisible; }
    int popupX() const { return m_popupX; }
    int popupY() const { return m_popupY; }
    bool darkTheme() const { return m_darkTheme; }
    QString language() const { return m_language; }
    QVariantMap settingsMap() const;
    QString hotkeyText() const;
    QString langswapText() const;
    QString selectionHotkeyText() const;
    QString hotkeyStatus() const { return m_hotkeyStatus; }
    bool secretMode() const { return m_secretMode; }
    bool paused() const { return m_paused; }
    QVariantMap counts() const { return m_countsCache; }
    QVariantList folders() const { return m_foldersCache; }
    QVariantList assetFolders() const { return m_assetFoldersCache; }
    QVariantList assets() const { return m_assetsCache; }
    QVariantList expansions() const { return m_expansionsCache; }
    QVariantMap stats() const { return m_statsCache; }
    QStringList searchHistory() const { return m_searchCache; }
    QString viewMode() const { return m_viewMode; }
    QString mainKind() const { return m_mainKind; }
    qint64 mainFolder() const { return m_mainFolder; }
    bool mainSecret() const { return m_mainSecret; }
    QString mainTitle() const;
    QString mainQuery() const { return m_mainQuery; }
    qint64 assetFolder() const { return m_assetFolder; }
    QString popupKind() const { return m_popupKind; }
    QString popupQuery() const { return m_popupQuery; }
    qint64 popupFolder() const { return m_popupFolder; }
    QVariantList quickFolders() const { return m_quickFoldersCache; }
    QString popupView() const { return m_popupView; }
    QVariantList popupAssets() const { return m_popupAssetsCache; }
    int popupWidth() const { return m_popupWidth; }
    int popupHeight() const { return m_popupHeight; }
    bool popupPreview() const { return m_showPreview; }
    bool settingsVisible() const { return m_settingsVisible; }
    bool helpVisible() const { return m_helpVisible; }
    bool licenseVisible() const { return m_licenseVisible; }
    QString appVersion() const;
    QString qtVersion() const;
    bool editVisible() const { return m_editVisible; }
    QString editText() const { return m_editText; }
    QString editTitle() const { return m_editTitle; }
    QString dataPath() const;

    Q_INVOKABLE void openPopup();
    Q_INVOKABLE void closePopup();
    Q_INVOKABLE void openMain();
    Q_INVOKABLE void hideMain();

    Q_INVOKABLE void pasteItem(qint64 id, bool plain);
    Q_INVOKABLE void pasteAs(qint64 id, const QString &format);
    Q_INVOKABLE void copyItem(qint64 id);
    Q_INVOKABLE void toggleFavorite(qint64 id);
    Q_INVOKABLE void toggleSecret(qint64 id);
    Q_INVOKABLE void setPinned(qint64 id, bool pinned);
    Q_INVOKABLE void deleteItem(qint64 id);
    Q_INVOKABLE void openEdit(qint64 id);
    Q_INVOKABLE void saveEdit(const QString &text);
    Q_INVOKABLE void closeEdit();
    Q_INVOKABLE void swapLanguage(qint64 id);
    Q_INVOKABLE void swapSelection();
    Q_INVOKABLE void copyCodeBlock(qint64 id);
    Q_INVOKABLE void runTerminal(qint64 id);
    Q_INVOKABLE void openItem(qint64 id);
    Q_INVOKABLE void revealItem(qint64 id);

    Q_INVOKABLE void setViewMode(const QString &mode);
    Q_INVOKABLE void setAssetFolder(qint64 folderId);
    Q_INVOKABLE void addAssets();
    Q_INVOKABLE void removeAsset(qint64 id);
    Q_INVOKABLE void renameAsset(qint64 id, const QString &name);
    Q_INVOKABLE void moveAsset(qint64 assetId, qint64 folderId);
    Q_INVOKABLE void openAsset(qint64 id);
    Q_INVOKABLE void copyAssetPath(qint64 id);

    Q_INVOKABLE void addSnippet(const QString &title, const QString &content,
                                const QString &language, qint64 folderId);
    Q_INVOKABLE void updateSnippet(qint64 id, const QString &title, const QString &content,
                                   const QString &language, qint64 folderId);
    Q_INVOKABLE void saveExpansions(const QVariantList &rows);

    Q_INVOKABLE void setPopupKind(const QString &kind);
    Q_INVOKABLE void setPopupQuery(const QString &query);
    Q_INVOKABLE void setPopupFolder(qint64 folderId);
    Q_INVOKABLE void setPopupView(const QString &view);
    Q_INVOKABLE void setPopupSize(int width, int height);
    Q_INVOKABLE QPoint cursorPos() const;
    Q_INVOKABLE void setMainKind(const QString &kind);
    Q_INVOKABLE void setMainFolder(qint64 folderId);
    Q_INVOKABLE void setMainSecretView(bool enabled);
    Q_INVOKABLE void setMainQuery(const QString &query);
    Q_INVOKABLE void commitSearch(const QString &query);

    Q_INVOKABLE void setSecretMode(bool enabled);
    Q_INVOKABLE void toggleSecretMode();
    Q_INVOKABLE void setPaused(bool paused);
    Q_INVOKABLE void setPopupPreview(bool visible);
    Q_INVOKABLE void setLanguage(const QString &language);
    Q_INVOKABLE void setCapturing(bool capturing);

    Q_INVOKABLE void openSettings();
    Q_INVOKABLE void closeSettings();
    Q_INVOKABLE void openHelp();
    Q_INVOKABLE void closeHelp();
    Q_INVOKABLE void saveSettings(const QVariantMap &values);
    Q_INVOKABLE void addFolder(const QString &name, qint64 parentId, const QString &color,
                               const QString &scope);
    Q_INVOKABLE void renameFolder(qint64 id, const QString &name);
    Q_INVOKABLE void setFolderColor(qint64 id, const QString &color);
    Q_INVOKABLE void moveFolder(qint64 id, qint64 parentId);
    Q_INVOKABLE void deleteFolder(qint64 id);
    Q_INVOKABLE void moveToFolder(qint64 itemId, qint64 folderId);
    Q_INVOKABLE void clearSearches();
    Q_INVOKABLE void clearItems();
    Q_INVOKABLE void clearAllExceptPinned();
    Q_INVOKABLE void confirmClearAll();
    Q_INVOKABLE void openDataFolder();

    Q_INVOKABLE void openLicense();
    Q_INVOKABLE void closeLicense();
    Q_INVOKABLE QString licenseText(const QString &which) const;
    Q_INVOKABLE QString licenseNotice() const;
    Q_INVOKABLE void openLicenseFolder();

    Q_INVOKABLE QString formatHotkey(const QString &spec) const;
    Q_INVOKABLE QString highlight(const QString &text, const QString &language) const;
    Q_INVOKABLE QString plainHtml(const QString &text) const;
    Q_INVOKABLE QString swapText(const QString &text) const;
    Q_INVOKABLE QString colorFor(const QString &text) const;
    Q_INVOKABLE QString transformText(const QString &text, const QString &format) const;
    Q_INVOKABLE void quit();

signals:
    void popupVisibleChanged();
    void mainVisibleChanged();
    void popupGeometryChanged();
    void settingsChanged();
    void languageChanged();
    void secretModeChanged();
    void pausedChanged();
    void dataChanged();
    void expansionsChanged();
    void searchHistoryChanged();
    void mainFilterChanged();
    void popupFilterChanged();
    void viewChanged();
    void settingsVisibleChanged();
    void helpVisibleChanged();
    void licenseVisibleChanged();
    void editVisibleChanged();

private:
    void loadSettings();
    void applyRuntimeSettings(bool initial);
    void buildTray();
    void updateTrayTooltip();
    void notify(const QString &title, const QString &body);
    bool isTerminalApp(const QString &processName) const;

    void refreshAll();
    void refreshMeta();
    void rebuildQuickFolders();
    void reloadPopup();
    void reloadMain();
    ClipFilter popupFilter() const;
    ClipFilter mainFilter() const;

    void onCaptured(const CapturePayload &payload);
    bool writeClipboard(const ClipItem &item, bool plain);
    quintptr currentPasteTarget() const;
    quintptr mainWindowHandle() const;
    QWindow *findWindow(const QString &objectName) const;
    void focusWindow(const QString &objectName);
    void schedulePaste(quintptr target);
    void swapGlobalClipboard();
    void startSelectionSwap();
    void continueSelectionSwap();
    void checkSelectionSwap();
    void updateExpansionBlock();

    ClipStore *m_store;
    ClipModel *m_popupModel;
    ClipModel *m_mainModel;
    ClipboardWatcher *m_watcher;
    HotkeyManager *m_hotkeys;
    QSystemTrayIcon *m_tray = nullptr;
    QMenu *m_trayMenu = nullptr;
    QAction *m_openAction = nullptr;
    QAction *m_secretAction = nullptr;
    QAction *m_pauseAction = nullptr;

    bool m_popupVisible = false;
    bool m_mainVisible = false;
    int m_popupX = 0;
    int m_popupY = 0;
    bool m_settingsVisible = false;
    bool m_helpVisible = false;
    bool m_licenseVisible = false;
    bool m_editVisible = false;
    qint64 m_editId = 0;
    QString m_editText;
    QString m_editTitle;
    qint64 m_mainFolder = -1;
    bool m_mainSecret = false;
    QString m_mainKind = QStringLiteral("all");
    QString m_mainQuery;
    QString m_viewMode = QStringLiteral("clips");
    qint64 m_assetFolder = -1;
    QString m_popupKind;
    QString m_popupQuery;
    QString m_popupView = QStringLiteral("clips");
    QVariantList m_popupAssetsCache;
    int m_popupWidth = 398;
    int m_popupHeight = 500;
    qint64 m_popupFolder = -1;
    QString m_popupFolders;
    QVariantList m_quickFoldersCache;
    bool m_secretMode = false;
    bool m_paused = false;
    bool m_quitting = false;
    int m_captureCount = 0;

    quintptr m_targetWindow = 0;
    quintptr m_mainReturnTarget = 0;

    bool m_darkTheme = true;
    QString m_language = QStringLiteral("en");
    QString m_hotkey;
    QString m_langHotkey;
    QString m_swapHotkey;
    bool m_blockWinV = true;
    bool m_disableHistory = true;
    bool m_autostart = false;
    bool m_showPreview = true;
    bool m_closeAfterPaste = true;
    bool m_notifications = true;
    bool m_keepOrder = true;
    bool m_programmerMode = false;
    bool m_terminalCleanCopy = true;
    bool m_terminalPasteChord = true;
    bool m_terminalPlainPaste = true;
    bool m_snippetsEnabled = true;
    QString m_swapDirection = QStringLiteral("auto");
    QString m_hotkeyStatus;
    qint64 m_lastSelectionSwap = 0;
    quintptr m_swapTarget = 0;
    QString m_swapSaved;
    unsigned long m_swapSequence = 0;
    bool m_swapTerminal = false;
    bool m_swapDone = false;
    int m_swapTries = 0;
    int m_swapWaitTicks = 0;
    int m_maxItems = 3000;
    int m_secretTtl = 24;
    int m_pasteDelay = 120;
    QString m_ignoreApps;
    QString m_terminalApps;

    QVariantMap m_countsCache;
    QVariantList m_foldersCache;
    QVariantList m_assetFoldersCache;
    QVariantList m_assetsCache;
    QVariantList m_expansionsCache;
    QVariantMap m_statsCache;
    QStringList m_searchCache;
};
