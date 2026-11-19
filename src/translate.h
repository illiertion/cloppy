#pragma once

#include <QString>

namespace Tr {

void setLanguage(const QString &language); // "en" (default) or "ko"
QString language();

// English is the default; Korean is returned when the app language is Korean.
QString text(const QString &korean, const QString &english);

} // namespace Tr
