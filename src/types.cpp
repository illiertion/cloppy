#include "types.h"

#include "translate.h"
#include <QRegularExpression>

QString humanizeTime(double timestamp)
{
    if (timestamp <= 0)
        return {};
    const QDateTime when = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(timestamp));
    const QDateTime now = QDateTime::currentDateTime();
    const qint64 seconds = when.secsTo(now);
    if (seconds < 45)
        return Tr::text(QStringLiteral("방금"), QStringLiteral("Just now"));
    if (seconds < 3600)
        return Tr::text(QStringLiteral("%1분 전"), QStringLiteral("%1 min ago")).arg(seconds / 60);
    if (when.date() == now.date())
        return when.toString(QStringLiteral("HH:mm"));
    if (when.date() == now.date().addDays(-1))
        return Tr::text(QStringLiteral("어제"), QStringLiteral("Yesterday"));
    if (when.date().year() == now.date().year())
        return when.toString(QStringLiteral("MM/dd"));
    return when.toString(QStringLiteral("yyyy/MM/dd"));
}

QString formatSize(qint64 bytes)
{
    if (bytes < 1024)
        return QStringLiteral("%1 B").arg(bytes);
    if (bytes < 1024 * 1024)
        return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
}

QString collapseWhitespace(const QString &text, int limit)
{
    QString result = text;
    result.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    result = result.trimmed();
    if (result.size() > limit)
        result = result.left(limit - 1) + QChar(0x2026);
    return result;
}

QString firstLine(const QString &text, int limit)
{
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("[\\r\\n]")));
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty())
            return trimmed.left(limit);
    }
    return {};
}

QVariantMap ClipItem::toMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("clipId"), static_cast<double>(id));
    map.insert(QStringLiteral("kind"), kind);
    map.insert(QStringLiteral("content"), content);
    map.insert(QStringLiteral("title"), title);
    map.insert(QStringLiteral("preview"), preview);
    map.insert(QStringLiteral("previewLine"), preview.isEmpty() ? firstLine(content, 160) : collapseWhitespace(preview, 160));
    map.insert(QStringLiteral("imagePath"), imagePath);
    map.insert(QStringLiteral("imageUrl"),
               imagePath.isEmpty() ? QString() : QUrl::fromLocalFile(imagePath).toString());
    map.insert(QStringLiteral("files"), files);
    map.insert(QStringLiteral("filesCount"), files.size());
    map.insert(QStringLiteral("folderId"), static_cast<double>(folderId));
    map.insert(QStringLiteral("favorite"), favorite);
    map.insert(QStringLiteral("secret"), secret);
    map.insert(QStringLiteral("pinned"), pinned);
    map.insert(QStringLiteral("language"), language);
    map.insert(QStringLiteral("tags"), tags);
    map.insert(QStringLiteral("sourceApp"), sourceApp);
    map.insert(QStringLiteral("timeText"), humanizeTime(lastUsedAt > 0 ? lastUsedAt : createdAt));
    map.insert(QStringLiteral("sizeText"), size > 0 ? formatSize(size) : QString());
    map.insert(QStringLiteral("useCount"), useCount);

    QVariantList fileItems;
    fileItems.reserve(files.size());
    for (const QString &path : files) {
        const QFileInfo info(path);
        QVariantMap entry;
        entry.insert(QStringLiteral("path"), path);
        entry.insert(QStringLiteral("name"), info.fileName().isEmpty() ? path : info.fileName());
        entry.insert(QStringLiteral("dir"), info.absolutePath());
        entry.insert(QStringLiteral("exists"), info.exists());
        entry.insert(QStringLiteral("isDir"), info.isDir());
        entry.insert(QStringLiteral("sizeText"), info.isFile() ? formatSize(info.size()) : Tr::text(QStringLiteral("폴더"), QStringLiteral("Folders")));
        fileItems.append(entry);
    }
    map.insert(QStringLiteral("fileItems"), fileItems);
    return map;
}

QVariantMap AssetInfo::toMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("assetId"), static_cast<double>(id));
    map.insert(QStringLiteral("name"), name);
    map.insert(QStringLiteral("path"), path);
    map.insert(QStringLiteral("kind"), kind);
    map.insert(QStringLiteral("size"), static_cast<double>(size));
    map.insert(QStringLiteral("sizeText"), formatSize(size));
    map.insert(QStringLiteral("folderId"), static_cast<double>(folderId));
    map.insert(QStringLiteral("url"), QUrl::fromLocalFile(path).toString());
    map.insert(QStringLiteral("timeText"), humanizeTime(createdAt));
    map.insert(QStringLiteral("ext"), QFileInfo(path).suffix().toUpper());
    return map;
}

QVariantMap Expansion::toMap() const
{
    QVariantMap map;
    map.insert(QStringLiteral("expansionId"), static_cast<double>(id));
    map.insert(QStringLiteral("trigger"), trigger);
    map.insert(QStringLiteral("expansion"), expansion);
    map.insert(QStringLiteral("enabled"), enabled);
    return map;
}
