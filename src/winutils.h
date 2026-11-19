#pragma once

#include <QString>

namespace WinUtils {

quintptr foregroundWindow();
QString foregroundProcessName();
QString processNameForWindow(quintptr handle);
void setForeground(quintptr handle);
unsigned long clipboardSequence();
bool modifiersDown();
void sendPaste();
void sendCopy();
void sendTerminalPaste();
void sendTerminalCopy();
void sendBackspaces(int count);
void typeText(const QString &text);
void replaceTyped(int backspaces, const QString &text);
bool foregroundIsSelf();
bool runInTerminal(const QString &command);
void openPath(const QString &path);
void revealInExplorer(const QString &path);

bool clipboardHistoryEnabled();
void setClipboardHistory(bool enabled);
bool autostartEnabled();
void setAutostart(bool enabled);

} // namespace WinUtils
