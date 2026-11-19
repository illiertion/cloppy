#pragma once

#include <QString>

class HtmlHighlighter
{
public:
    static QString highlight(const QString &text, const QString &language, bool dark);
    static QString plain(const QString &text, bool dark);
};
