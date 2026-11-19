#pragma once

#include <QDateTime>
#include <QFileInfo>
#include <QHash>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantMap>

struct CapturePayload {
    QString kind;
    QString content;
    QString title;
    QString preview;
    QString imagePath;
    QString hash;
    QString sourceApp;
    QStringList files;
    qint64 size = 0;
};
Q_DECLARE_METATYPE(CapturePayload)

struct ClipItem {
    qint64 id = 0;
    QString kind = QStringLiteral("text");
    QString content;
    QString title;
    QString preview;
    QString imagePath;
    QStringList files;
    qint64 folderId = -1;
    bool favorite = false;
    bool secret = false;
    bool pinned = false;
    QString sourceApp;
    QString language;
    QString tags;
    double createdAt = 0;
    double lastUsedAt = 0;
    int useCount = 1;
    qint64 size = 0;
    QString hash;

    QVariantMap toMap() const;
};

struct FolderInfo {
    qint64 id = 0;
    QString name;
    QString color = QStringLiteral("#7c5cff");
    int count = 0;
    qint64 parentId = -1;
    int depth = 0;
    QString scope = QStringLiteral("clip");
};

struct AssetInfo {
    qint64 id = 0;
    QString name;
    QString path;
    QString kind;
    qint64 size = 0;
    qint64 folderId = -1;
    double createdAt = 0;

    QVariantMap toMap() const;
};

struct Expansion {
    qint64 id = 0;
    QString trigger;
    QString expansion;
    bool enabled = true;

    QVariantMap toMap() const;
};

struct ClipFilter {
    QString kind;
    bool favoritesOnly = false;
    bool secret = false;
    bool hasFolder = false;
    qint64 folderId = -1;
    bool pinnedOnly = false;
    bool excludePinned = false;
    QString query;
    int limit = 500;
};

struct Counts {
    int all = 0;
    int favorites = 0;
    int secret = 0;
    int snippets = 0;
    QHash<QString, int> kinds;
};

QString humanizeTime(double timestamp);
QString formatSize(qint64 bytes);
QString collapseWhitespace(const QString &text, int limit = 240);
QString firstLine(const QString &text, int limit = 120);
