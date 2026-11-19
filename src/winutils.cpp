#include "winutils.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QThread>
#include <QUrl>
#include <QVector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {

QString toNativePath(const QString &path)
{
    return QDir::toNativeSeparators(path);
}

} // namespace

namespace WinUtils {

quintptr foregroundWindow()
{
    return reinterpret_cast<quintptr>(GetForegroundWindow());
}

QString foregroundProcessName()
{
    return processNameForWindow(reinterpret_cast<quintptr>(GetForegroundWindow()));
}

QString processNameForWindow(quintptr handle)
{
    const HWND hwnd = reinterpret_cast<HWND>(handle);
    if (!hwnd)
        return {};
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid)
        return {};
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process)
        return {};
    wchar_t buffer[1024];
    DWORD size = 1024;
    QString result;
    if (QueryFullProcessImageNameW(process, 0, buffer, &size))
        result = QDir::fromNativeSeparators(QString::fromWCharArray(buffer, static_cast<int>(size)));
    CloseHandle(process);
    return QFileInfo(result).fileName();
}

void setForeground(quintptr handle)
{
    if (!handle)
        return;
    // Let a background process take the foreground.
    keybd_event(VK_MENU, 0, 0, 0);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
    SetForegroundWindow(reinterpret_cast<HWND>(handle));
}

namespace {

void sendChord(WORD key, bool control, bool shift, bool alt)
{
    QVector<INPUT> inputs;
    const auto keyEvent = [&inputs](WORD vk, bool up) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = vk;
        if (up)
            input.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.append(input);
    };
    if (control)
        keyEvent(VK_CONTROL, false);
    if (shift)
        keyEvent(VK_SHIFT, false);
    if (alt)
        keyEvent(VK_MENU, false);
    keyEvent(key, false);
    keyEvent(key, true);
    if (alt)
        keyEvent(VK_MENU, true);
    if (shift)
        keyEvent(VK_SHIFT, true);
    if (control)
        keyEvent(VK_CONTROL, true);
    if (!inputs.isEmpty())
        SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}

} // namespace

unsigned long clipboardSequence()
{
    return GetClipboardSequenceNumber();
}

bool modifiersDown()
{
    return (GetAsyncKeyState(VK_CONTROL) & 0x8000)
        || (GetAsyncKeyState(VK_MENU) & 0x8000)
        || (GetAsyncKeyState(VK_SHIFT) & 0x8000)
        || (GetAsyncKeyState(VK_LWIN) & 0x8000)
        || (GetAsyncKeyState(VK_RWIN) & 0x8000);
}

void sendPaste()
{
    sendChord('V', true, false, false);
}

void sendCopy()
{
    sendChord('C', true, false, false);
}

void sendTerminalPaste()
{
    sendChord('V', true, true, false);
}

void sendTerminalCopy()
{
    sendChord('C', true, true, false);
}

void sendBackspaces(int count)
{
    QVector<INPUT> inputs;
    for (int i = 0; i < count; ++i) {
        INPUT down = {};
        down.type = INPUT_KEYBOARD;
        down.ki.wVk = VK_BACK;
        inputs.append(down);
        INPUT up = {};
        up.type = INPUT_KEYBOARD;
        up.ki.wVk = VK_BACK;
        up.ki.dwFlags = KEYEVENTF_KEYUP;
        inputs.append(up);
    }
    if (!inputs.isEmpty())
        SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}

void typeText(const QString &text)
{
    QVector<INPUT> inputs;
    const auto unicode = [&inputs](ushort code) {
        INPUT down = {};
        down.type = INPUT_KEYBOARD;
        down.ki.wVk = 0;
        down.ki.wScan = code;
        down.ki.dwFlags = KEYEVENTF_UNICODE;
        inputs.append(down);
        INPUT up = down;
        up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        inputs.append(up);
    };
    for (const QChar ch : text) {
        if (ch == QLatin1Char('\n')) {
            INPUT down = {};
            down.type = INPUT_KEYBOARD;
            down.ki.wVk = VK_RETURN;
            inputs.append(down);
            INPUT up = down;
            up.ki.dwFlags = KEYEVENTF_KEYUP;
            inputs.append(up);
        } else if (ch != QLatin1Char('\r')) {
            unicode(ch.unicode());
        }
    }
    if (!inputs.isEmpty())
        SendInput(static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT));
}

void replaceTyped(int backspaces, const QString &text)
{
    if (backspaces > 0)
        sendBackspaces(backspaces);
    QThread::msleep(35);
    typeText(text);
}

bool foregroundIsSelf()
{
    const HWND hwnd = GetForegroundWindow();
    if (!hwnd)
        return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    return pid == GetCurrentProcessId();
}

bool runInTerminal(const QString &command)
{
    if (command.trimmed().isEmpty())
        return false;
    return QProcess::startDetached(QStringLiteral("cmd.exe"),
                                   {QStringLiteral("/c"), QStringLiteral("start"),
                                    QStringLiteral("Cloppy"), QStringLiteral("cmd.exe"),
                                    QStringLiteral("/k"), command});
}

void openPath(const QString &path)
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(toNativePath(path)));
}

void revealInExplorer(const QString &path)
{
    QProcess::startDetached(QStringLiteral("explorer.exe"),
                            {QStringLiteral("/select,"), toNativePath(path)});
}

bool clipboardHistoryEnabled()
{
    QSettings registry(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Clipboard"),
                       QSettings::NativeFormat);
    return registry.value(QStringLiteral("EnableClipboardHistory"), 1).toInt() != 0;
}

void setClipboardHistory(bool enabled)
{
    QSettings registry(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Clipboard"),
                       QSettings::NativeFormat);
    registry.setValue(QStringLiteral("EnableClipboardHistory"), enabled ? 1 : 0);
}

bool autostartEnabled()
{
    QSettings registry(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                       QSettings::NativeFormat);
    return registry.contains(QStringLiteral("Cloppy"));
}

void setAutostart(bool enabled)
{
    QSettings registry(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                       QSettings::NativeFormat);
    if (enabled) {
        const QString command = QStringLiteral("\"%1\"")
                                    .arg(QDir::toNativeSeparators(QCoreApplication::applicationFilePath()));
        registry.setValue(QStringLiteral("Cloppy"), command);
    } else {
        registry.remove(QStringLiteral("Cloppy"));
    }
}

} // namespace WinUtils
