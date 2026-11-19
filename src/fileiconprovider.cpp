#include "fileiconprovider.h"

#include <QFileInfo>
#include <QUrl>

FileIconProvider::FileIconProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap FileIconProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QString path = QUrl::fromPercentEncoding(id.toUtf8());
    const QSize target = requestedSize.isValid() ? requestedSize : QSize(32, 32);
    const QString key = path + QStringLiteral("|%1x%2").arg(target.width()).arg(target.height());
    if (m_cache.contains(key)) {
        const QPixmap cached = m_cache.value(key);
        if (size)
            *size = cached.size();
        return cached;
    }

    QPixmap pixmap = m_provider.icon(QFileInfo(path)).pixmap(target);
    if (pixmap.isNull())
        pixmap = QPixmap(target);
    m_cache.insert(key, pixmap);
    if (size)
        *size = pixmap.size();
    return pixmap;
}
