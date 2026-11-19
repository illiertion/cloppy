#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QVariantList>
#include <QVector>

#include "types.h"

class QSqlQuery;

class ClipStore : public QObject
{
    Q_OBJECT
public:
    explicit ClipStore(QObject *parent = nullptr);
    ~ClipStore();

    bool isOpen() const;
    QString databasePath() const;
    QString imagesDir() const;
    static QString imagesDirectory();
    static QString assetsDirectory();

    qint64 addCapture(const CapturePayload &payload, bool secret, const QString &sourceApp);
    ClipItem item(qint64 id) const;
    QVector<ClipItem> list(const ClipFilter &filter) const;
    Counts counts() const;
    QVector<FolderInfo> folders(const QString &scope = QStringLiteral("clip")) const;
    QVariantMap stats() const;
    QStringList searches(int limit = 15) const;

    void addSearch(const QString &query);
    void clearSearches();
    void touch(qint64 id);
    void setFavorite(qint64 id, bool favorite);
    void setSecret(qint64 id, bool secret);
    void setFolder(qint64 id, qint64 folderId);
    void updateContent(qint64 id, const QString &content);
    void remove(qint64 id);
    int clearItems(bool keepFavorites = true);
    void purge(int maxItems, int secretTtlHours);

    qint64 addSnippet(const QString &title, const QString &content, const QString &language,
                      qint64 folderId);
    void updateSnippet(qint64 id, const QString &title, const QString &content,
                       const QString &language, qint64 folderId);
    void setTitle(qint64 id, const QString &title);
    void setPinned(qint64 id, bool pinned);

    qint64 addFolder(const QString &name, const QString &color = QStringLiteral("#7c5cff"),
                     qint64 parentId = -1, const QString &scope = QStringLiteral("clip"));
    void renameFolder(qint64 id, const QString &name);
    void setFolderColor(qint64 id, const QString &color);
    void moveFolder(qint64 id, qint64 parentId);
    void deleteFolder(qint64 id);

    QVector<AssetInfo> assets(qint64 folderId = -1) const;
    qint64 addAsset(const QString &sourcePath, qint64 folderId);
    void renameAsset(qint64 id, const QString &name);
    void setAssetFolder(qint64 id, qint64 folderId);
    void removeAsset(qint64 id);

    QVector<Expansion> expansions() const;
    qint64 addExpansion(const QString &trigger, const QString &expansion, bool enabled = true);
    void replaceExpansions(const QVariantList &rows);

private:
    void openDatabase();
    void ensureColumn(const QString &table, const QString &column, const QString &definition);
    ClipItem itemFromQuery(const QSqlQuery &query) const;

    QSqlDatabase m_db;
    QString m_databasePath;
};
