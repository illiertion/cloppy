#pragma once

#include <QString>

namespace LangSwap {

QString enToKo(const QString &text);
QString koToEn(const QString &text);
QString swap(const QString &text);
QString swapDirected(const QString &text, const QString &direction);
QString swappedOrEmpty(const QString &text);

} // namespace LangSwap
