#pragma once

#include <QString>

namespace Classifier {

struct Result {
    QString kind = QStringLiteral("text");
    QString language;
    QString title;
    QString preview;
    QString tags;
};

Result classify(const QString &text);
bool looksLikeCode(const QString &text);
QString guessLanguage(const QString &text);
QString languageForPath(const QString &path);
QString previewLine(const QString &text, int limit = 160);

} // namespace Classifier
