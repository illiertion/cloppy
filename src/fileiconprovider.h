#pragma once

#include <QFileIconProvider>
#include <QHash>
#include <QQuickImageProvider>

class FileIconProvider : public QQuickImageProvider
{
public:
    FileIconProvider();

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QFileIconProvider m_provider;
    QHash<QString, QPixmap> m_cache;
};
