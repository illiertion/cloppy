#include "hotkeymanager.h"

#include <QDebug>
#include <QStringList>

#include <chrono>
#include <future>

#include "langswap.h"

namespace {

constexpr int kModAlt = 0x0001;
constexpr int kModControl = 0x0002;
constexpr int kModShift = 0x0004;
constexpr int kModWin = 0x0008;

constexpr UINT kMenuHotkeyId = 1;
constexpr UINT kLangHotkeyId = 2;
constexpr UINT kSwapHotkeyId = 3;
constexpr UINT kReRegisterMessage = WM_APP + 1;

HotkeyManager *g_instance = nullptr;

bool isModifierKey(DWORD vk)
{
    return vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU || vk == VK_LWIN || vk == VK_RWIN;
}

bool hasHangul(const QString &text)
{
    for (const QChar c : text) {
        const ushort u = c.unicode();
        if ((u >= 0xAC00 && u <= 0xD7A3) || (u >= 0x3131 && u <= 0x318E))
            return true;
    }
    return false;
}

QString usChar(DWORD vk, bool shift)
{
    if (vk >= 'A' && vk <= 'Z')
        return QString(QChar(shift ? static_cast<ushort>(vk) : static_cast<ushort>(vk + 32)));
    if (vk >= '0' && vk <= '9') {
        if (!shift)
            return QString(QChar(static_cast<ushort>(vk)));
        static const char shifted[] = ")!@#$%^&*(";
        return QString(QChar(shifted[vk - '0']));
    }
    switch (vk) {
    case VK_OEM_1: return shift ? QStringLiteral(":") : QStringLiteral(";");
    case VK_OEM_PLUS: return shift ? QStringLiteral("+") : QStringLiteral("=");
    case VK_OEM_COMMA: return shift ? QStringLiteral("<") : QStringLiteral(",");
    case VK_OEM_MINUS: return shift ? QStringLiteral("_") : QStringLiteral("-");
    case VK_OEM_PERIOD: return shift ? QStringLiteral(">") : QStringLiteral(".");
    case VK_OEM_2: return shift ? QStringLiteral("?") : QStringLiteral("/");
    case VK_OEM_3: return shift ? QStringLiteral("~") : QStringLiteral("`");
    case VK_OEM_4: return shift ? QStringLiteral("{") : QStringLiteral("[");
    case VK_OEM_5: return shift ? QStringLiteral("|") : QStringLiteral("\\");
    case VK_OEM_6: return shift ? QStringLiteral("}") : QStringLiteral("]");
    case VK_OEM_7: return shift ? QStringLiteral("\"") : QStringLiteral("'");
    default: return {};
    }
}

} // namespace

HotkeyManager::HotkeyManager(QObject *parent)
    : QObject(parent)
{
}

HotkeyManager::~HotkeyManager()
{
    stop();
}

HotkeyManager::Spec HotkeyManager::parse(const QString &spec)
{
    Spec result;
    const QStringList parts = spec.toLower().remove(QLatin1Char(' ')).split(QLatin1Char('+'), Qt::SkipEmptyParts);
    QString key;
    for (const QString &part : parts) {
        if (part == QStringLiteral("win") || part == QStringLiteral("meta") || part == QStringLiteral("cmd"))
            result.mods |= kModWin;
        else if (part == QStringLiteral("ctrl") || part == QStringLiteral("control"))
            result.mods |= kModControl;
        else if (part == QStringLiteral("alt"))
            result.mods |= kModAlt;
        else if (part == QStringLiteral("shift"))
            result.mods |= kModShift;
        else
            key = part;
    }
    if (key.isEmpty())
        return {};

    if (key.size() == 1 && key.at(0).isLetter())
        result.vk = key.at(0).toUpper().unicode();
    else if (key.size() == 1 && key.at(0).isDigit())
        result.vk = key.at(0).unicode();
    else if (key == QStringLiteral("space"))
        result.vk = VK_SPACE;
    else if (key == QStringLiteral("tab"))
        result.vk = VK_TAB;
    else if (key == QStringLiteral("esc") || key == QStringLiteral("escape"))
        result.vk = VK_ESCAPE;
    else if (key == QStringLiteral("enter") || key == QStringLiteral("return"))
        result.vk = VK_RETURN;
    else if (key == QStringLiteral("backspace"))
        result.vk = VK_BACK;
    else if (key == QStringLiteral("insert"))
        result.vk = VK_INSERT;
    else if (key == QStringLiteral("delete"))
        result.vk = VK_DELETE;
    else if (key == QStringLiteral("home"))
        result.vk = VK_HOME;
    else if (key == QStringLiteral("end"))
        result.vk = VK_END;
    else if (key == QStringLiteral("pageup"))
        result.vk = VK_PRIOR;
    else if (key == QStringLiteral("pagedown"))
        result.vk = VK_NEXT;
    else if (key == QStringLiteral("up"))
        result.vk = VK_UP;
    else if (key == QStringLiteral("down"))
        result.vk = VK_DOWN;
    else if (key == QStringLiteral("left"))
        result.vk = VK_LEFT;
    else if (key == QStringLiteral("right"))
        result.vk = VK_RIGHT;
    else if (key.startsWith(QLatin1Char('f'))) {
        bool ok = false;
        const int number = key.mid(1).toInt(&ok);
        if (ok && number >= 1 && number <= 24)
            result.vk = VK_F1 + number - 1;
    }
    return result;
}

QString HotkeyManager::formatHotkey(const QString &spec)
{
    const QStringList parts = spec.toLower().split(QLatin1Char('+'), Qt::SkipEmptyParts);
    QStringList out;
    for (const QString &part : parts) {
        const QString token = part.trimmed();
        if (token == QStringLiteral("win") || token == QStringLiteral("meta") || token == QStringLiteral("cmd"))
            out << QStringLiteral("Win");
        else if (token == QStringLiteral("ctrl") || token == QStringLiteral("control"))
            out << QStringLiteral("Ctrl");
        else if (token == QStringLiteral("alt"))
            out << QStringLiteral("Alt");
        else if (token == QStringLiteral("shift"))
            out << QStringLiteral("Shift");
        else if (token.startsWith(QLatin1Char('f')) && token.mid(1).toInt() > 0)
            out << token.toUpper();
        else if (token.size() == 1)
            out << token.toUpper();
        else
            out << token.left(1).toUpper() + token.mid(1);
    }
    return out.join(QLatin1Char('+'));
}

void HotkeyManager::start(const QString &menuHotkey, const QString &langHotkey,
                          const QString &swapHotkey, bool blockWinV)
{
    update(menuHotkey, langHotkey, swapHotkey, blockWinV);
    if (m_running.exchange(true))
        return;
    g_instance = this;

    auto ready = std::make_shared<std::promise<bool>>();
    auto future = ready->get_future();
    m_thread = std::thread([this, ready]() {
        m_threadId = GetCurrentThreadId();
        MSG msg;
        PeekMessageW(&msg, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
        HHOOK hook = SetWindowsHookExW(WH_KEYBOARD_LL, &HotkeyManager::hookProc,
                                       GetModuleHandleW(nullptr), 0);
        m_hook.store(reinterpret_cast<void *>(hook));
        registerHotkeys();
        ready->set_value(hook != nullptr || m_menuRegistered || m_langRegistered
                         || m_swapRegistered);
        if (!hook && !m_menuRegistered && !m_langRegistered && !m_swapRegistered)
            return;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            if (msg.message == WM_HOTKEY) {
                const UINT id = static_cast<UINT>(msg.wParam);
                if (id == kMenuHotkeyId)
                    Q_EMIT activated();
                else if (id == kLangHotkeyId)
                    Q_EMIT langswap();
                else if (id == kSwapHotkeyId)
                    Q_EMIT selectionSwap();
                continue;
            }
            if (msg.message == kReRegisterMessage) {
                registerHotkeys();
                continue;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        unregisterHotkeys();
        if (hook)
            UnhookWindowsHookEx(hook);
        m_hook.store(nullptr);
    });

    if (future.wait_for(std::chrono::seconds(2)) != std::future_status::ready || !future.get()) {
        qWarning() << "Cloppy: failed to install keyboard hook";
        Q_EMIT error(QStringLiteral("전역 단축키를 등록하지 못했습니다. 다른 앱이 같은 조합을 쓰고 있는지 확인해 주세요."));
    } else {
        qInfo() << "Cloppy: global hotkeys ready";
    }
}

void HotkeyManager::registerHotkeys()
{
    unregisterHotkeys();

    Spec menu;
    Spec lang;
    Spec swap;
    {
        std::lock_guard<std::mutex> lock(m_specMutex);
        menu = m_menuSpec;
        lang = m_langSpec;
        swap = m_swapSpec;
    }

    // Windows keeps Win+V reserved, so that one relies on the low-level hook.
    m_menuRegistered = menu.valid()
        && RegisterHotKey(nullptr, kMenuHotkeyId, static_cast<UINT>(menu.mods), menu.vk);
    m_langRegistered = lang.valid() && lang.vk != 0
        && RegisterHotKey(nullptr, kLangHotkeyId, static_cast<UINT>(lang.mods), lang.vk);
    m_swapRegistered = swap.valid() && swap.vk != 0
        && RegisterHotKey(nullptr, kSwapHotkeyId, static_cast<UINT>(swap.mods), swap.vk);

    QStringList ready;
    if (m_menuRegistered)
        ready << QStringLiteral("빠른 창");
    else if (m_hook.load())
        ready << QStringLiteral("빠른 창(후킹)");
    if (m_langRegistered)
        ready << QStringLiteral("클립보드 변환");
    if (m_swapRegistered)
        ready << QStringLiteral("선택 변환");
    const QString text = ready.isEmpty()
        ? QStringLiteral("후킹 모드로 동작 중")
        : QStringLiteral("등록됨: ") + ready.join(QStringLiteral(", "));
    Q_EMIT status(text);
    qInfo() << "Cloppy hotkeys:" << text;
}

void HotkeyManager::unregisterHotkeys()
{
    if (m_menuRegistered)
        UnregisterHotKey(nullptr, kMenuHotkeyId);
    if (m_langRegistered)
        UnregisterHotKey(nullptr, kLangHotkeyId);
    if (m_swapRegistered)
        UnregisterHotKey(nullptr, kSwapHotkeyId);
    m_menuRegistered = false;
    m_langRegistered = false;
    m_swapRegistered = false;
}

void HotkeyManager::update(const QString &menuHotkey, const QString &langHotkey,
                           const QString &swapHotkey, bool blockWinV)
{
    const Spec menu = parse(menuHotkey);
    const Spec lang = parse(langHotkey);
    const Spec swap = parse(swapHotkey);
    std::lock_guard<std::mutex> lock(m_specMutex);
    m_menuSpec = menu.valid() ? menu : Spec{kModWin, static_cast<UINT>('V')};
    m_langSpec = lang;
    m_swapSpec = swap;
    m_blockWinV.store(blockWinV);
    if (m_threadId)
        PostThreadMessageW(m_threadId, kReRegisterMessage, 0, 0);
}

void HotkeyManager::setExpansions(const QVector<QPair<QString, QString>> &expansions)
{
    std::lock_guard<std::mutex> lock(m_expansionMutex);
    m_expansions = expansions;
}

void HotkeyManager::setExpansionsEnabled(bool enabled)
{
    m_expansionsEnabled.store(enabled);
    m_typedBuffer.clear();
    m_typedKeys.clear();
}

void HotkeyManager::setExpansionBlocked(bool blocked)
{
    m_expansionBlocked.store(blocked);
    m_typedBuffer.clear();
    m_typedKeys.clear();
}

void HotkeyManager::stop()
{
    m_running.store(false);
    if (m_thread.joinable()) {
        if (m_threadId)
            PostThreadMessageW(m_threadId, WM_QUIT, 0, 0);
        m_thread.join();
    }
    m_threadId = 0;
    if (g_instance == this)
        g_instance = nullptr;
}

void HotkeyManager::setCapturing(bool capturing)
{
    m_capturing.store(capturing);
    std::lock_guard<std::mutex> lock(m_downMutex);
    m_down.clear();
}

QString HotkeyManager::keyChar(DWORD vk)
{
    BYTE state[256] = {};
    if (!GetKeyboardState(state))
        return {};
    wchar_t buffer[8] = {};
    const int count = ToUnicodeEx(vk, MapVirtualKeyW(vk, MAPVK_VK_TO_VSC), state, buffer, 8, 0,
                                 GetKeyboardLayout(0));
    if (count <= 0)
        return {};
    return QString::fromWCharArray(buffer, count);
}

bool HotkeyManager::foregroundIsSelf()
{
    const HWND hwnd = GetForegroundWindow();
    if (!hwnd)
        return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    return pid == GetCurrentProcessId();
}

int HotkeyManager::currentModifiers()
{
    int mods = 0;
    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        mods |= kModControl;
    if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        mods |= kModShift;
    if (GetAsyncKeyState(VK_MENU) & 0x8000)
        mods |= kModAlt;
    if ((GetAsyncKeyState(VK_LWIN) & 0x8000) || (GetAsyncKeyState(VK_RWIN) & 0x8000))
        mods |= kModWin;
    return mods;
}

bool HotkeyManager::handleKeyDown(DWORD vk, bool injected)
{
    if (m_capturing.load())
        return false;
    if (isModifierKey(vk))
        return false;

    Spec menu;
    Spec lang;
    Spec swap;
    bool block = true;
    {
        std::lock_guard<std::mutex> lock(m_specMutex);
        menu = m_menuSpec;
        lang = m_langSpec;
        swap = m_swapSpec;
        block = m_blockWinV.load();
    }
    const int mods = currentModifiers();

    {
        std::lock_guard<std::mutex> lock(m_downMutex);
        const bool alreadyDown = m_down.count(vk) > 0;
        if (!alreadyDown)
            m_down.insert(vk);
        if (alreadyDown) {
            const bool menuMatch = menu.valid() && vk == menu.vk && mods == menu.mods;
            const bool langMatch = lang.valid() && vk == lang.vk && mods == lang.mods;
            const bool swapMatch = swap.valid() && vk == swap.vk && mods == swap.mods;
            if ((menuMatch && m_menuRegistered) || (langMatch && m_langRegistered)
                || (swapMatch && m_swapRegistered))
                return false;
            return menuMatch || langMatch || swapMatch || (block && vk == 'V' && mods == kModWin);
        }
    }

    // When RegisterHotKey owns the combination, let the key through so Windows
    // delivers WM_HOTKEY; swallowing it here would suppress our own hotkey.
    if (lang.valid() && vk == lang.vk && mods == lang.mods) {
        if (m_langRegistered)
            return false;
        Q_EMIT langswap();
        return true;
    }
    if (swap.valid() && vk == swap.vk && mods == swap.mods) {
        if (m_swapRegistered)
            return false;
        Q_EMIT selectionSwap();
        return true;
    }
    if (menu.valid() && vk == menu.vk && mods == menu.mods) {
        if (m_menuRegistered)
            return false;
        Q_EMIT activated();
        return true;
    }
    if (block && vk == 'V' && mods == kModWin)
        return true;

    // Injected input (mouse macro keys, remote tools) still triggers hotkeys and
    // the Win+V block, but never text expansion, which would feed on itself.
    if (!injected && m_expansionsEnabled.load() && !m_expansionBlocked.load()) {
        if ((mods & (kModControl | kModAlt | kModWin)) != 0) {
            m_typedBuffer.clear();
            m_typedKeys.clear();
        } else if (vk == VK_BACK) {
            if (!m_typedBuffer.isEmpty())
                m_typedBuffer.chop(1);
            if (!m_typedKeys.isEmpty())
                m_typedKeys.chop(1);
        } else if (vk == VK_SPACE || vk == VK_TAB || vk == VK_RETURN) {
            QString replacement;
            int length = 0;
            // Korean IME composes jamo, so also match the raw key letters and
            // their Hangul interpretation before deciding on a trigger.
            const QString hangulForm = hasHangul(m_typedBuffer) ? LangSwap::enToKo(m_typedKeys)
                                                                : QString();
            {
                std::lock_guard<std::mutex> lock(m_expansionMutex);
                for (const auto &entry : m_expansions) {
                    const QString &trigger = entry.first;
                    if (trigger.isEmpty())
                        continue;
                    const bool matches = m_typedBuffer.endsWith(trigger, Qt::CaseInsensitive)
                        || m_typedKeys.endsWith(trigger, Qt::CaseInsensitive)
                        || (!hangulForm.isEmpty() && hangulForm.endsWith(trigger, Qt::CaseInsensitive));
                    if (matches && trigger.size() > length) {
                        length = trigger.size();
                        replacement = entry.second;
                    }
                }
            }
            m_typedBuffer.clear();
            m_typedKeys.clear();
            if (length > 0) {
                Q_EMIT expandSnippet(replacement, length);
                return true;
            }
        } else {
            const QString text = keyChar(vk);
            if (!text.isEmpty() && text.at(0).isPrint()) {
                m_typedBuffer += text;
                if (m_typedBuffer.size() > 64)
                    m_typedBuffer = m_typedBuffer.right(64);
            }
            const QString raw = usChar(vk, (mods & kModShift) != 0);
            if (!raw.isEmpty()) {
                m_typedKeys += raw;
                if (m_typedKeys.size() > 48)
                    m_typedKeys = m_typedKeys.right(48);
            }
        }
    }
    return false;
}

void HotkeyManager::handleKeyUp(DWORD vk)
{
    std::lock_guard<std::mutex> lock(m_downMutex);
    m_down.erase(vk);
}

LRESULT CALLBACK HotkeyManager::hookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION) {
        const auto *data = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
        if (data) {
            HotkeyManager *self = g_instance;
            if (self) {
                const bool injected = (data->flags & LLKHF_INJECTED) != 0;
                if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                    if (self->handleKeyDown(data->vkCode, injected))
                        return 1;
                } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                    self->handleKeyUp(data->vkCode);
                }
            }
        }
    }
    return CallNextHookEx(nullptr, code, wParam, lParam);
}
