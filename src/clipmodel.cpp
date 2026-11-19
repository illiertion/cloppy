#include "clipmodel.h"

#include "classifier.h"
#include "clipstore.h"

ClipModel::ClipModel(ClipStore *store, QObject *parent)
    : QAbstractListModel(parent)
    , m_store(store)
{
}

int ClipModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant ClipModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const ClipItem &item = m_items.at(index.row());
    switch (role) {
    case ClipIdRole:
        return item.id;
    case KindRole:
        return item.kind;
    case TitleRole:
        return item.title;
    case PreviewLineRole:
        return item.preview.isEmpty() ? Classifier::previewLine(item.content, 160) : item.preview;
    case PreviewRole:
        return item.preview;
    case ContentRole:
        return item.content;
    case ImagePathRole:
        return item.imagePath;
    case ImageUrlRole:
        return item.imagePath.isEmpty() ? QString() : QUrl::fromLocalFile(item.imagePath).toString();
    case FavoriteRole:
        return item.favorite;
    case SecretRole:
        return item.secret;
    case PinnedRole:
        return item.pinned;
    case LanguageRole:
        return item.language;
    case TimeTextRole:
        return humanizeTime(item.lastUsedAt > 0 ? item.lastUsedAt : item.createdAt);
    case SizeTextRole:
        return item.size > 0 ? formatSize(item.size) : QString();
    case SourceAppRole:
        return item.sourceApp;
    case FilesCountRole:
        return item.files.size();
    case UseCountRole:
        return item.useCount;
    case FilesRole:
        return item.files;
    case FileItemsRole:
        return item.toMap().value(QStringLiteral("fileItems"));
    case FolderIdRole:
        return item.folderId;
    default:
        return {};
    }
}

QHash<int, QByteArray> ClipModel::roleNames() const
{
    return {
        {ClipIdRole, "clipId"},
        {KindRole, "kind"},
        {TitleRole, "title"},
        {PreviewLineRole, "previewLine"},
        {PreviewRole, "preview"},
        {ContentRole, "content"},
        {ImagePathRole, "imagePath"},
        {ImageUrlRole, "imageUrl"},
        {FavoriteRole, "favorite"},
        {SecretRole, "secret"},
        {PinnedRole, "pinned"},
        {LanguageRole, "language"},
        {TimeTextRole, "timeText"},
        {SizeTextRole, "sizeText"},
        {SourceAppRole, "sourceApp"},
        {FilesCountRole, "filesCount"},
        {UseCountRole, "useCount"},
        {FilesRole, "files"},
        {FileItemsRole, "fileItems"},
        {FolderIdRole, "folderId"},
    };
}

void ClipModel::setFilter(const ClipFilter &filter)
{
    m_filter = filter;
}

void ClipModel::reload()
{
    beginResetModel();
    m_items = m_store ? m_store->list(m_filter) : QVector<ClipItem>();
    endResetModel();
    Q_EMIT countChanged();
}

QVariantMap ClipModel::get(int row) const
{
    if (row < 0 || row >= m_items.size())
        return {};
    return m_items.at(row).toMap();
}
