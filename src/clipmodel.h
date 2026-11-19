#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "types.h"

class ClipStore;

class ClipModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        ClipIdRole = Qt::UserRole + 1,
        KindRole,
        TitleRole,
        PreviewLineRole,
        PreviewRole,
        ContentRole,
        ImagePathRole,
        ImageUrlRole,
        FavoriteRole,
        SecretRole,
        PinnedRole,
        LanguageRole,
        TimeTextRole,
        SizeTextRole,
        SourceAppRole,
        FilesCountRole,
        UseCountRole,
        FilesRole,
        FileItemsRole,
        FolderIdRole,
    };

    explicit ClipModel(ClipStore *store, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_items.size(); }
    void setFilter(const ClipFilter &filter);
    ClipFilter filter() const { return m_filter; }
    void reload();

    Q_INVOKABLE QVariantMap get(int row) const;

signals:
    void countChanged();

private:
    ClipStore *m_store;
    ClipFilter m_filter;
    QVector<ClipItem> m_items;
};
