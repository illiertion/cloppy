#pragma once

#include <QObject>
#include <QPair>
#include <QString>
#include <QVector>

#include <atomic>
#include <mutex>
#include <set>
#include <thread>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

class HotkeyManager : public QObject
{
    Q_OBJECT
public:
    explicit HotkeyManager(QObject *parent = nullptr);
    ~HotkeyManager() override;

    void start(const QString &menuHotkey, const QString &langHotkey, const QString &swapHotkey,
               bool blockWinV);
    void update(const QString &menuHotkey, const QString &langHotkey, const QString &swapHotkey,
                bool blockWinV);
    void stop();
    void setCapturing(bool capturing);

    void setExpansions(const QVector<QPair<QString, QString>> &expansions);
    void setExpansionsEnabled(bool enabled);
    void setExpansionBlocked(bool blocked);

    static QString formatHotkey(const QString &spec);

signals:
    void activated();
    void langswap();
    void selectionSwap();
    void expandSnippet(const QString &replacement, int backspaces);
    void status(const QString &text);
    void error(const QString &message);

private:
    struct Spec {
        int mods = 0;
        UINT vk = 0;
        bool valid() const { return vk != 0; }
    };

    static Spec parse(const QString &spec);
    static LRESULT CALLBACK hookProc(int code, WPARAM wParam, LPARAM lParam);
    bool handleKeyDown(DWORD vk, bool injected);
    void handleKeyUp(DWORD vk);
    static int currentModifiers();
    static QString keyChar(DWORD vk);
    static bool foregroundIsSelf();
    void registerHotkeys();
    void unregisterHotkeys();

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_capturing{false};
    std::atomic<bool> m_blockWinV{true};
    std::atomic<void *> m_hook{nullptr};
    DWORD m_threadId = 0;

    std::mutex m_specMutex;
    Spec m_menuSpec;
    Spec m_langSpec;
    Spec m_swapSpec;

    // Registration state is only touched on the hook thread.
    bool m_menuRegistered = false;
    bool m_langRegistered = false;
    bool m_swapRegistered = false;

    std::mutex m_expansionMutex;
    QVector<QPair<QString, QString>> m_expansions;
    std::atomic<bool> m_expansionsEnabled{true};
    std::atomic<bool> m_expansionBlocked{false};
    QString m_typedBuffer; // only touched from the hook thread
    QString m_typedKeys;   // US-layout keys, for Korean triggers

    std::mutex m_downMutex;
    std::set<DWORD> m_down;
};
