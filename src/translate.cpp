#include "translate.h"

#include <QMutex>
#include <QMutexLocker>

namespace {

QMutex g_mutex;
QString g_language = QStringLiteral("en");

} // namespace

namespace Tr {

void setLanguage(const QString &language)
{
    QMutexLocker locker(&g_mutex);
    g_language = language == QStringLiteral("ko") ? QStringLiteral("ko") : QStringLiteral("en");
}

QString language()
{
    QMutexLocker locker(&g_mutex);
    return g_language;
}

QString text(const QString &korean, const QString &english)
{
    QMutexLocker locker(&g_mutex);
    return g_language == QStringLiteral("ko") ? korean : english;
}

} // namespace Tr
