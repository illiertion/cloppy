#include "clipstore.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSqlError>
#include <QSqlQuery>
#include <QSet>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>
#include <QVariantList>

#include <functional>

#include "classifier.h"

namespace {

const char *kSchema = R"SQL(
CREATE TABLE IF NOT EXISTS items (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    kind TEXT NOT NULL DEFAULT 'text',
    content TEXT NOT NULL DEFAULT '',
    title TEXT NOT NULL DEFAULT '',
    preview TEXT NOT NULL DEFAULT '',
    image_path TEXT NOT NULL DEFAULT '',
    files TEXT NOT NULL DEFAULT '[]',
    folder_id INTEGER,
    favorite INTEGER NOT NULL DEFAULT 0,
    secret INTEGER NOT NULL DEFAULT 0,
    pinned INTEGER NOT NULL DEFAULT 0,
    source_app TEXT NOT NULL DEFAULT '',
    language TEXT NOT NULL DEFAULT '',
    tags TEXT NOT NULL DEFAULT '',
    created_at REAL NOT NULL,
    last_used_at REAL NOT NULL,
    use_count INTEGER NOT NULL DEFAULT 1,
    size INTEGER NOT NULL DEFAULT 0,
    hash TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_items_last_used ON items(last_used_at DESC);
CREATE INDEX IF NOT EXISTS idx_items_hash ON items(hash);
CREATE INDEX IF NOT EXISTS idx_items_kind ON items(kind);
CREATE TABLE IF NOT EXISTS folders (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    color TEXT NOT NULL DEFAULT '#7c5cff',
    parent_id INTEGER,
    scope TEXT NOT NULL DEFAULT 'clip',
    created_at REAL NOT NULL
);
CREATE TABLE IF NOT EXISTS assets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    path TEXT NOT NULL,
    kind TEXT NOT NULL DEFAULT 'other',
    size INTEGER NOT NULL DEFAULT 0,
    folder_id INTEGER,
    created_at REAL NOT NULL
);
CREATE TABLE IF NOT EXISTS expansions (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    trigger TEXT NOT NULL,
    expansion TEXT NOT NULL,
    enabled INTEGER NOT NULL DEFAULT 1,
    created_at REAL NOT NULL
);
CREATE TABLE IF NOT EXISTS searches (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    query TEXT NOT NULL,
    created_at REAL NOT NULL
);
)SQL";

QString sha1Hex(const QByteArray &data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex());
}

QString textHash(const QString &text)
{
    return QStringLiteral("text:") + sha1Hex(text.toUtf8());
}

QString clean(const QString &text)
{
    QString copy = text;
    copy.remove(QChar(0));
    return copy;
}

} // namespace

ClipStore::ClipStore(QObject *parent)
    : QObject(parent)
{
    openDatabase();
}

ClipStore::~ClipStore()
{
    if (m_db.isOpen())
        m_db.close();
    const QString name = m_db.connectionName();
    m_db = QSqlDatabase();
    if (!name.isEmpty())
        QSqlDatabase::removeDatabase(name);
}

bool ClipStore::isOpen() const
{
    return m_db.isOpen();
}

QString ClipStore::imagesDirectory()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString path = QDir(base).filePath(QStringLiteral("images"));
    QDir().mkpath(path);
    return path;
}

QString ClipStore::imagesDir() const
{
    return imagesDirectory();
}

QString ClipStore::databasePath() const
{
    return m_databasePath;
}

void ClipStore::openDatabase()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base);
    m_databasePath = QDir(base).filePath(QStringLiteral("cloppy.db"));

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("cloppy"));
    m_db.setDatabaseName(m_databasePath);
    if (!m_db.open()) {
        qWarning() << "Cloppy: could not open database" << m_databasePath << m_db.lastError().text();
        return;
    }
    QSqlQuery pragma(m_db);
    pragma.exec(QStringLiteral("PRAGMA journal_mode=WAL"));
    pragma.exec(QStringLiteral("PRAGMA synchronous=NORMAL"));

    const QStringList statements = QString::fromLatin1(kSchema).split(QStringLiteral(";"), Qt::SkipEmptyParts);
    for (const QString &raw : statements) {
        const QString statement = raw.trimmed();
        if (statement.isEmpty())
            continue;
        QSqlQuery query(m_db);
        if (!query.exec(statement))
            qWarning() << "Cloppy: schema error" << query.lastError().text();
    }

    // Older databases predate nested folders, pinned snippets and asset folders.
    ensureColumn(QStringLiteral("folders"), QStringLiteral("parent_id"), QStringLiteral("INTEGER"));
    ensureColumn(QStringLiteral("folders"), QStringLiteral("scope"),
                 QStringLiteral("TEXT NOT NULL DEFAULT 'clip'"));
    ensureColumn(QStringLiteral("items"), QStringLiteral("pinned"),
                 QStringLiteral("INTEGER NOT NULL DEFAULT 0"));
}

void ClipStore::ensureColumn(const QString &table, const QString &column, const QString &definition)
{
    bool found = false;
    QSqlQuery info(m_db);
    if (info.exec(QStringLiteral("PRAGMA table_info(%1)").arg(table))) {
        while (info.next()) {
            if (info.value(1).toString() == column)
                found = true;
        }
    }
    if (found)
        return;
    QSqlQuery alter(m_db);
    if (!alter.exec(QStringLiteral("ALTER TABLE %1 ADD COLUMN %2 %3").arg(table, column, definition)))
        qWarning() << "Cloppy: migration failed" << table << column << alter.lastError().text();
}

ClipItem ClipStore::itemFromQuery(const QSqlQuery &query) const
{
    ClipItem item;
    item.id = query.value(QStringLiteral("id")).toLongLong();
    item.kind = query.value(QStringLiteral("kind")).toString();
    item.content = query.value(QStringLiteral("content")).toString();
    item.title = query.value(QStringLiteral("title")).toString();
    item.preview = query.value(QStringLiteral("preview")).toString();
    item.imagePath = query.value(QStringLiteral("image_path")).toString();
    const QString filesJson = query.value(QStringLiteral("files")).toString();
    if (!filesJson.isEmpty() && filesJson != QStringLiteral("[]")) {
        // Store file paths as newline separated text to keep SQLite simple.
        item.files = filesJson.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    }
    const QVariant folder = query.value(QStringLiteral("folder_id"));
    item.folderId = folder.isNull() ? -1 : folder.toLongLong();
    item.favorite = query.value(QStringLiteral("favorite")).toInt() != 0;
    item.secret = query.value(QStringLiteral("secret")).toInt() != 0;
    item.pinned = query.value(QStringLiteral("pinned")).toInt() != 0;
    item.sourceApp = query.value(QStringLiteral("source_app")).toString();
    item.language = query.value(QStringLiteral("language")).toString();
    item.tags = query.value(QStringLiteral("tags")).toString();
    item.createdAt = query.value(QStringLiteral("created_at")).toDouble();
    item.lastUsedAt = query.value(QStringLiteral("last_used_at")).toDouble();
    item.useCount = query.value(QStringLiteral("use_count")).toInt();
    item.size = query.value(QStringLiteral("size")).toLongLong();
    item.hash = query.value(QStringLiteral("hash")).toString();
    return item;
}

qint64 ClipStore::addCapture(const CapturePayload &payload, bool secret, const QString &sourceApp)
{
    if (!m_db.isOpen())
        return 0;

    QString kind = payload.kind;
    const QString content = clean(payload.content);
    QStringList files = payload.files;
    const QString imagePath = payload.imagePath;
    const qint64 size = payload.size;

    QString title = payload.title;
    QString preview = payload.preview;
    QString language;
    QString tags;

    if (kind == QStringLiteral("text")) {
        const Classifier::Result info = Classifier::classify(content);
        kind = info.kind;
        title = info.title;
        preview = info.preview;
        language = info.language;
        tags = info.tags;
    } else if (kind == QStringLiteral("files")) {
        if (files.size() == 1) {
            const QFileInfo info(files.first());
            title = info.isDir() ? QStringLiteral("📁 %1").arg(info.fileName()) : info.fileName();
            language = Classifier::languageForPath(files.first());
        } else if (!files.isEmpty()) {
            title = QStringLiteral("%1 외 %2개").arg(QFileInfo(files.first()).fileName()).arg(files.size() - 1);
        } else {
            title = QStringLiteral("파일");
        }
        preview = files.join(QLatin1Char('\n'));
        tags = QStringLiteral("파일");
    } else if (kind == QStringLiteral("image")) {
        if (title.isEmpty())
            title = QStringLiteral("이미지");
        if (preview.isEmpty())
            preview = QStringLiteral("이미지");
        tags = QStringLiteral("이미지");
    }

    const QString hash = payload.hash.isEmpty()
        ? textHash(kind + content + imagePath + files.join(QLatin1Char('\n')))
        : payload.hash;

    QSqlQuery existing(m_db);
    existing.prepare(QStringLiteral("SELECT id FROM items WHERE hash = ? LIMIT 1"));
    existing.addBindValue(hash);
    existing.exec();
    if (existing.next()) {
        const qint64 id = existing.value(0).toLongLong();
        QSqlQuery update(m_db);
        // Pinned snippets keep their visibility: copying the same text again must
        // never turn them secret or drop them out of their folder.
        update.prepare(QStringLiteral(
            "UPDATE items SET last_used_at = ?, use_count = use_count + 1,"
            " secret = CASE WHEN pinned = 1 THEN secret ELSE ? END,"
            " source_app = CASE WHEN ? != '' THEN ? ELSE source_app END WHERE id = ?"));
        update.addBindValue(QDateTime::currentSecsSinceEpoch());
        update.addBindValue(secret ? 1 : 0);
        update.addBindValue(sourceApp);
        update.addBindValue(sourceApp);
        update.addBindValue(id);
        update.exec();
        return id;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO items(kind, content, title, preview, image_path, files, folder_id, favorite, secret,"
        " source_app, language, tags, created_at, last_used_at, use_count, size, hash)"
        " VALUES(?,?,?,?,?,?,NULL,0,?,?,?,?,?,?,1,?,?)"));
    const auto nonNull = [](const QString &value) {
        return value.isNull() ? QString::fromLatin1("") : value;
    };
    query.addBindValue(nonNull(kind));
    query.addBindValue(nonNull(content));
    query.addBindValue(nonNull(title));
    query.addBindValue(nonNull(preview));
    query.addBindValue(nonNull(imagePath));
    query.addBindValue(nonNull(files.join(QLatin1Char('\n'))));
    query.addBindValue(secret ? 1 : 0);
    query.addBindValue(nonNull(sourceApp));
    query.addBindValue(nonNull(language));
    query.addBindValue(nonNull(tags));
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue(size);
    query.addBindValue(nonNull(hash));
    if (!query.exec()) {
        qWarning() << "Cloppy: insert failed" << query.lastError().text();
        return 0;
    }
    return query.lastInsertId().toLongLong();
}

ClipItem ClipStore::item(qint64 id) const
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT * FROM items WHERE id = ?"));
    query.addBindValue(id);
    if (query.exec() && query.next())
        return itemFromQuery(query);
    return {};
}

QVector<ClipItem> ClipStore::list(const ClipFilter &filter) const
{
    QVector<ClipItem> items;
    if (!m_db.isOpen())
        return items;

    QStringList where;
    QVariantList binds;
    where << QStringLiteral("secret = ?");
    binds << (filter.secret ? 1 : 0);
    if (filter.pinnedOnly)
        where << QStringLiteral("pinned = 1");
    else if (filter.excludePinned)
        where << QStringLiteral("pinned = 0");
    if (!filter.kind.isEmpty()) {
        where << QStringLiteral("kind = ?");
        binds << filter.kind;
    }
    if (filter.favoritesOnly)
        where << QStringLiteral("favorite = 1");
    if (filter.hasFolder) {
        where << QStringLiteral("folder_id = ?");
        binds << filter.folderId;
    }
    const QStringList tokens = filter.query.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    for (const QString &token : tokens) {
        where << QStringLiteral("(content LIKE ? OR title LIKE ? OR tags LIKE ? OR language LIKE ?)");
        const QString pattern = QStringLiteral("%%%1%%").arg(token);
        binds << pattern << pattern << pattern << pattern;
    }

    const QString sql = QStringLiteral("SELECT * FROM items WHERE %1 ORDER BY favorite DESC, last_used_at DESC LIMIT ?")
                            .arg(where.join(QStringLiteral(" AND ")));
    binds << (filter.limit > 0 ? filter.limit : 500);

    QSqlQuery query(m_db);
    query.prepare(sql);
    for (const QVariant &value : binds)
        query.addBindValue(value);
    if (!query.exec()) {
        qWarning() << "Cloppy: list failed" << query.lastError().text();
        return items;
    }
    while (query.next())
        items.append(itemFromQuery(query));
    return items;
}

Counts ClipStore::counts() const
{
    Counts result;
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral(
            "SELECT kind, secret, favorite, pinned, COUNT(*) AS c"
            " FROM items GROUP BY kind, secret, favorite, pinned")))
        return result;
    while (query.next()) {
        const int count = query.value(QStringLiteral("c")).toInt();
        if (query.value(QStringLiteral("secret")).toInt() != 0) {
            result.secret += count;
            continue;
        }
        if (query.value(QStringLiteral("pinned")).toInt() != 0)
            result.snippets += count;
        result.all += count;
        const QString kind = query.value(QStringLiteral("kind")).toString();
        result.kinds[kind] += count;
        if (query.value(QStringLiteral("favorite")).toInt() != 0)
            result.favorites += count;
    }
    return result;
}

QVector<FolderInfo> ClipStore::folders(const QString &scope) const
{
    QVector<FolderInfo> rows;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT f.id, f.name, f.color, f.parent_id, f.scope,"
        " (SELECT COUNT(*) FROM items i WHERE i.folder_id = f.id AND i.secret = 0) AS c"
        " FROM folders f WHERE f.scope = ? ORDER BY f.created_at ASC"));
    query.addBindValue(scope);
    if (!query.exec())
        return rows;
    while (query.next()) {
        FolderInfo folder;
        folder.id = query.value(QStringLiteral("id")).toLongLong();
        folder.name = query.value(QStringLiteral("name")).toString();
        folder.color = query.value(QStringLiteral("color")).toString();
        folder.parentId = query.value(QStringLiteral("parent_id")).isNull()
            ? -1
            : query.value(QStringLiteral("parent_id")).toLongLong();
        folder.count = query.value(QStringLiteral("c")).toInt();
        folder.scope = query.value(QStringLiteral("scope")).toString();
        rows.append(folder);
    }

    QHash<qint64, int> indexById;
    for (int i = 0; i < rows.size(); ++i)
        indexById.insert(rows.at(i).id, i);

    QHash<qint64, QVector<int>> children;
    QVector<int> roots;
    for (int i = 0; i < rows.size(); ++i) {
        const qint64 parentId = rows.at(i).parentId;
        if (parentId > 0 && indexById.contains(parentId))
            children[parentId].append(i);
        else
            roots.append(i);
    }

    QVector<FolderInfo> result;
    QSet<qint64> visited;
    std::function<void(int, int)> append = [&](int rowIndex, int depth) {
        const FolderInfo &row = rows.at(rowIndex);
        if (visited.contains(row.id))
            return;
        visited.insert(row.id);
        FolderInfo copy = row;
        copy.depth = depth;
        result.append(copy);
        for (int child : children.value(row.id))
            append(child, depth + 1);
    };
    for (int root : roots)
        append(root, 0);
    for (int i = 0; i < rows.size(); ++i) {
        if (!visited.contains(rows.at(i).id))
            append(i, 0);
    }
    return result;
}

QVariantMap ClipStore::stats() const
{
    QVariantMap map;
    QSqlQuery query(m_db);
    if (query.exec(QStringLiteral("SELECT COUNT(*) AS c, COALESCE(SUM(size), 0) AS bytes FROM items"))
        && query.next()) {
        map.insert(QStringLiteral("items"), query.value(QStringLiteral("c")).toInt());
        map.insert(QStringLiteral("bytes"), query.value(QStringLiteral("bytes")).toLongLong());
        map.insert(QStringLiteral("bytesText"), formatSize(query.value(QStringLiteral("bytes")).toLongLong()));
    }
    return map;
}

QStringList ClipStore::searches(int limit) const
{
    QStringList result;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("SELECT query FROM searches ORDER BY created_at DESC LIMIT ?"));
    query.addBindValue(limit);
    if (query.exec()) {
        while (query.next())
            result.append(query.value(0).toString());
    }
    return result;
}

void ClipStore::addSearch(const QString &query)
{
    const QString trimmed = query.trimmed();
    if (trimmed.isEmpty())
        return;
    QSqlQuery removeQuery(m_db);
    removeQuery.prepare(QStringLiteral("DELETE FROM searches WHERE query = ?"));
    removeQuery.addBindValue(trimmed);
    removeQuery.exec();

    QSqlQuery insert(m_db);
    insert.prepare(QStringLiteral("INSERT INTO searches(query, created_at) VALUES(?, ?)"));
    insert.addBindValue(trimmed);
    insert.addBindValue(QDateTime::currentSecsSinceEpoch());
    insert.exec();

    QSqlQuery trim(m_db);
    trim.exec(QStringLiteral(
        "DELETE FROM searches WHERE id NOT IN (SELECT id FROM searches ORDER BY created_at DESC LIMIT 25)"));
}

void ClipStore::clearSearches()
{
    QSqlQuery(m_db).exec(QStringLiteral("DELETE FROM searches"));
}

void ClipStore::touch(qint64 id)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE items SET last_used_at = ?, use_count = use_count + 1 WHERE id = ?"));
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue(id);
    query.exec();
}

void ClipStore::setFavorite(qint64 id, bool favorite)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE items SET favorite = ? WHERE id = ?"));
    query.addBindValue(favorite ? 1 : 0);
    query.addBindValue(id);
    query.exec();
}

void ClipStore::setSecret(qint64 id, bool secret)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE items SET secret = ? WHERE id = ?"));
    query.addBindValue(secret ? 1 : 0);
    query.addBindValue(id);
    query.exec();
}

void ClipStore::setFolder(qint64 id, qint64 folderId)
{
    QSqlQuery query(m_db);
    if (folderId > 0) {
        query.prepare(QStringLiteral("UPDATE items SET folder_id = ? WHERE id = ?"));
        query.addBindValue(folderId);
    } else {
        query.prepare(QStringLiteral("UPDATE items SET folder_id = NULL WHERE id = ?"));
    }
    query.addBindValue(id);
    query.exec();
}

void ClipStore::updateContent(qint64 id, const QString &content)
{
    const Classifier::Result info = Classifier::classify(content);
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE items SET content = ?, kind = ?, title = ?, preview = ?, language = ?, tags = ?,"
        " size = ?, hash = ?, last_used_at = ? WHERE id = ?"));
    query.addBindValue(content);
    query.addBindValue(info.kind);
    query.addBindValue(info.title);
    query.addBindValue(info.preview);
    query.addBindValue(info.language);
    query.addBindValue(info.tags);
    query.addBindValue(content.toUtf8().size());
    query.addBindValue(textHash(content));
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue(id);
    query.exec();
}

void ClipStore::remove(qint64 id)
{
    QSqlQuery lookup(m_db);
    lookup.prepare(QStringLiteral("SELECT image_path FROM items WHERE id = ?"));
    lookup.addBindValue(id);
    if (lookup.exec() && lookup.next()) {
        const QString imagePath = lookup.value(0).toString();
        if (!imagePath.isEmpty())
            QFile::remove(imagePath);
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM items WHERE id = ?"));
    query.addBindValue(id);
    query.exec();
}

int ClipStore::clearItems(bool keepFavorites)
{
    QSqlQuery lookup(m_db);
    const QString sql = keepFavorites
        ? QStringLiteral("SELECT image_path FROM items WHERE favorite = 0 AND pinned = 0")
        : QStringLiteral("SELECT image_path FROM items WHERE pinned = 0");
    if (lookup.exec(sql)) {
        while (lookup.next()) {
            const QString path = lookup.value(0).toString();
            if (!path.isEmpty())
                QFile::remove(path);
        }
    }
    QSqlQuery query(m_db);
    if (query.exec(keepFavorites ? QStringLiteral("DELETE FROM items WHERE favorite = 0 AND pinned = 0")
                                 : QStringLiteral("DELETE FROM items WHERE pinned = 0")))
        return query.numRowsAffected();
    return 0;
}

void ClipStore::purge(int maxItems, int secretTtlHours)
{
    const qint64 cutoff = QDateTime::currentSecsSinceEpoch() - static_cast<qint64>(secretTtlHours) * 3600;
    if (secretTtlHours > 0) {
        QSqlQuery expired(m_db);
        expired.prepare(QStringLiteral(
            "SELECT id FROM items WHERE secret = 1 AND pinned = 0 AND last_used_at < ?"));
        expired.addBindValue(cutoff);
        QList<qint64> ids;
        if (expired.exec()) {
            while (expired.next())
                ids.append(expired.value(0).toLongLong());
        }
        for (qint64 id : ids)
            remove(id);
    }
    if (maxItems <= 0)
        return;
    QSqlQuery overflow(m_db);
    overflow.prepare(QStringLiteral(
        "SELECT id FROM items WHERE favorite = 0 AND secret = 0 AND pinned = 0"
        " ORDER BY last_used_at DESC LIMIT -1 OFFSET ?"));
    overflow.addBindValue(maxItems);
    QList<qint64> ids;
    if (overflow.exec()) {
        while (overflow.next())
            ids.append(overflow.value(0).toLongLong());
    }
    for (qint64 id : ids)
        remove(id);
}

qint64 ClipStore::addFolder(const QString &name, const QString &color, qint64 parentId,
                            const QString &scope)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO folders(name, color, parent_id, scope, created_at) VALUES(?,?,?,?,?)"));
    query.addBindValue(name.trimmed().isEmpty() ? QStringLiteral("새 폴더") : name.trimmed());
    query.addBindValue(color);
    if (parentId > 0)
        query.addBindValue(parentId);
    else
        query.addBindValue(QVariant());
    query.addBindValue(scope.isEmpty() ? QStringLiteral("clip") : scope);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec())
        return 0;
    return query.lastInsertId().toLongLong();
}

void ClipStore::renameFolder(qint64 id, const QString &name)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE folders SET name = ? WHERE id = ?"));
    query.addBindValue(name.trimmed().isEmpty() ? QStringLiteral("폴더") : name.trimmed());
    query.addBindValue(id);
    query.exec();
}

void ClipStore::setFolderColor(qint64 id, const QString &color)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE folders SET color = ? WHERE id = ?"));
    query.addBindValue(color);
    query.addBindValue(id);
    query.exec();
}

void ClipStore::moveFolder(qint64 id, qint64 parentId)
{
    if (id <= 0 || id == parentId)
        return;
    if (parentId > 0) {
        QSqlQuery cycle(m_db);
        cycle.prepare(QStringLiteral(
            "WITH RECURSIVE sub(id) AS ("
            " SELECT id FROM folders WHERE id = ?"
            " UNION ALL SELECT f.id FROM folders f JOIN sub s ON f.parent_id = s.id)"
            " SELECT 1 FROM sub WHERE id = ? LIMIT 1"));
        cycle.addBindValue(id);
        cycle.addBindValue(parentId);
        if (cycle.exec() && cycle.next())
            return;
    }
    QSqlQuery query(m_db);
    if (parentId > 0) {
        query.prepare(QStringLiteral("UPDATE folders SET parent_id = ? WHERE id = ?"));
        query.addBindValue(parentId);
    } else {
        query.prepare(QStringLiteral("UPDATE folders SET parent_id = NULL WHERE id = ?"));
    }
    query.addBindValue(id);
    query.exec();
}

void ClipStore::setTitle(qint64 id, const QString &title)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE items SET title = ? WHERE id = ?"));
    query.addBindValue(title.trimmed().isEmpty() ? QStringLiteral("제목 없음") : title.trimmed());
    query.addBindValue(id);
    query.exec();
}

void ClipStore::setPinned(qint64 id, bool pinned)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE items SET pinned = ? WHERE id = ?"));
    query.addBindValue(pinned ? 1 : 0);
    query.addBindValue(id);
    query.exec();
}

qint64 ClipStore::addSnippet(const QString &title, const QString &content,
                             const QString &language, qint64 folderId)
{
    if (!m_db.isOpen())
        return 0;
    const QString body = clean(content);
    const QString lang = language.trimmed().isEmpty()
        ? (Classifier::guessLanguage(body).isEmpty() ? QStringLiteral("text")
                                                     : Classifier::guessLanguage(body))
        : language.trimmed();
    const QString hash = QStringLiteral("snippet:")
        + QString::fromLatin1(QCryptographicHash::hash(QUuid::createUuid().toByteArray(),
                                                       QCryptographicHash::Sha1).toHex());

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO items(kind, content, title, preview, image_path, files, folder_id, favorite,"
        " secret, pinned, source_app, language, tags, created_at, last_used_at, use_count, size, hash)"
        " VALUES('code',?,?,?,'','',?,0,0,1,'코드 블록',?,?,?,?,1,?,?)"));
    query.addBindValue(body);
    query.addBindValue(title.trimmed().isEmpty()
                           ? (firstLine(body, 80).isEmpty() ? QStringLiteral("새 코드 블록")
                                                            : firstLine(body, 80))
                           : title.trimmed());
    query.addBindValue(collapseWhitespace(body, 240));
    if (folderId > 0)
        query.addBindValue(folderId);
    else
        query.addBindValue(QVariant());
    query.addBindValue(lang);
    query.addBindValue(lang);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue(body.toUtf8().size());
    query.addBindValue(hash);
    if (!query.exec()) {
        qWarning() << "Cloppy: snippet insert failed" << query.lastError().text();
        return 0;
    }
    return query.lastInsertId().toLongLong();
}

void ClipStore::updateSnippet(qint64 id, const QString &title, const QString &content,
                              const QString &language, qint64 folderId)
{
    const QString body = clean(content);
    const QString lang = language.trimmed().isEmpty() ? QStringLiteral("text") : language.trimmed();
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "UPDATE items SET title = ?, content = ?, preview = ?, language = ?, tags = ?,"
        " size = ?, folder_id = ?, last_used_at = ? WHERE id = ?"));
    query.addBindValue(title.trimmed().isEmpty() ? QStringLiteral("코드 블록") : title.trimmed());
    query.addBindValue(body);
    query.addBindValue(collapseWhitespace(body, 240));
    query.addBindValue(lang);
    query.addBindValue(lang);
    query.addBindValue(body.toUtf8().size());
    if (folderId > 0)
        query.addBindValue(folderId);
    else
        query.addBindValue(QVariant());
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    query.addBindValue(id);
    query.exec();
}

void ClipStore::deleteFolder(qint64 id)
{
    QList<qint64> ids;
    QSqlQuery descendants(m_db);
    descendants.prepare(QStringLiteral(
        "WITH RECURSIVE sub(id) AS ("
        " SELECT id FROM folders WHERE id = ?"
        " UNION ALL SELECT f.id FROM folders f JOIN sub s ON f.parent_id = s.id)"
        " SELECT id FROM sub"));
    descendants.addBindValue(id);
    if (descendants.exec()) {
        while (descendants.next())
            ids.append(descendants.value(0).toLongLong());
    }
    if (ids.isEmpty())
        return;

    QStringList placeholders;
    for (int i = 0; i < ids.size(); ++i)
        placeholders << QStringLiteral("?");
    const QString list = placeholders.join(QLatin1Char(','));

    QSqlQuery clear(m_db);
    clear.prepare(QStringLiteral("UPDATE items SET folder_id = NULL WHERE folder_id IN (%1)").arg(list));
    for (qint64 folderId : ids)
        clear.addBindValue(folderId);
    clear.exec();

    QSqlQuery clearAssets(m_db);
    clearAssets.prepare(QStringLiteral("UPDATE assets SET folder_id = NULL WHERE folder_id IN (%1)").arg(list));
    for (qint64 folderId : ids)
        clearAssets.addBindValue(folderId);
    clearAssets.exec();

    QSqlQuery remove(m_db);
    remove.prepare(QStringLiteral("DELETE FROM folders WHERE id IN (%1)").arg(list));
    for (qint64 folderId : ids)
        remove.addBindValue(folderId);
    remove.exec();
}

// ----------------------------------------------------------------------- assets
QString ClipStore::assetsDirectory()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString path = QDir(base).filePath(QStringLiteral("assets"));
    QDir().mkpath(path);
    return path;
}

namespace {

QString assetKind(const QString &path)
{
    static const QHash<QString, QString> kinds = {
        {QStringLiteral("png"), QStringLiteral("image")},   {QStringLiteral("jpg"), QStringLiteral("image")},
        {QStringLiteral("jpeg"), QStringLiteral("image")},  {QStringLiteral("gif"), QStringLiteral("image")},
        {QStringLiteral("webp"), QStringLiteral("image")},  {QStringLiteral("svg"), QStringLiteral("image")},
        {QStringLiteral("bmp"), QStringLiteral("image")},   {QStringLiteral("ico"), QStringLiteral("image")},
        {QStringLiteral("ttf"), QStringLiteral("font")},    {QStringLiteral("otf"), QStringLiteral("font")},
        {QStringLiteral("woff"), QStringLiteral("font")},   {QStringLiteral("woff2"), QStringLiteral("font")},
        {QStringLiteral("mp3"), QStringLiteral("audio")},   {QStringLiteral("wav"), QStringLiteral("audio")},
        {QStringLiteral("ogg"), QStringLiteral("audio")},   {QStringLiteral("flac"), QStringLiteral("audio")},
        {QStringLiteral("mp4"), QStringLiteral("video")},   {QStringLiteral("webm"), QStringLiteral("video")},
        {QStringLiteral("mov"), QStringLiteral("video")},
        {QStringLiteral("pdf"), QStringLiteral("doc")},     {QStringLiteral("md"), QStringLiteral("doc")},
        {QStringLiteral("txt"), QStringLiteral("doc")},     {QStringLiteral("docx"), QStringLiteral("doc")},
        {QStringLiteral("zip"), QStringLiteral("archive")}, {QStringLiteral("7z"), QStringLiteral("archive")},
        {QStringLiteral("rar"), QStringLiteral("archive")},
        {QStringLiteral("json"), QStringLiteral("code")},   {QStringLiteral("js"), QStringLiteral("code")},
        {QStringLiteral("ts"), QStringLiteral("code")},     {QStringLiteral("py"), QStringLiteral("code")},
        {QStringLiteral("html"), QStringLiteral("code")},   {QStringLiteral("css"), QStringLiteral("code")},
        {QStringLiteral("sh"), QStringLiteral("code")},     {QStringLiteral("ps1"), QStringLiteral("code")},
    };
    return kinds.value(QFileInfo(path).suffix().toLower(), QStringLiteral("other"));
}

} // namespace

QVector<AssetInfo> ClipStore::assets(qint64 folderId) const
{
    QVector<AssetInfo> result;
    QSqlQuery query(m_db);
    if (folderId > 0) {
        query.prepare(QStringLiteral(
            "SELECT * FROM assets WHERE folder_id = ? ORDER BY created_at DESC"));
        query.addBindValue(folderId);
    } else {
        query.prepare(QStringLiteral("SELECT * FROM assets ORDER BY created_at DESC"));
    }
    if (!query.exec())
        return result;
    while (query.next()) {
        AssetInfo asset;
        asset.id = query.value(QStringLiteral("id")).toLongLong();
        asset.name = query.value(QStringLiteral("name")).toString();
        asset.path = query.value(QStringLiteral("path")).toString();
        asset.kind = query.value(QStringLiteral("kind")).toString();
        asset.size = query.value(QStringLiteral("size")).toLongLong();
        const QVariant folder = query.value(QStringLiteral("folder_id"));
        asset.folderId = folder.isNull() ? -1 : folder.toLongLong();
        asset.createdAt = query.value(QStringLiteral("created_at")).toDouble();
        result.append(asset);
    }
    return result;
}

qint64 ClipStore::addAsset(const QString &sourcePath, qint64 folderId)
{
    const QFileInfo info(sourcePath);
    if (!info.exists() || !info.isFile())
        return 0;

    QString target = QDir(assetsDirectory()).filePath(info.fileName());
    int suffix = 1;
    while (QFile::exists(target)) {
        const QString base = info.completeBaseName().isEmpty() ? info.fileName() : info.completeBaseName();
        const QString ext = info.suffix();
        const QString name = ext.isEmpty()
            ? QStringLiteral("%1-%2").arg(base).arg(suffix)
            : QStringLiteral("%1-%2.%3").arg(base).arg(suffix).arg(ext);
        target = QDir(assetsDirectory()).filePath(name);
        ++suffix;
    }
    if (QFileInfo(sourcePath).absoluteFilePath() != QFileInfo(target).absoluteFilePath()) {
        if (!QFile::copy(sourcePath, target))
            return 0;
    }

    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO assets(name, path, kind, size, folder_id, created_at) VALUES(?,?,?,?,?,?)"));
    query.addBindValue(info.fileName());
    query.addBindValue(target);
    query.addBindValue(assetKind(target));
    query.addBindValue(QFileInfo(target).size());
    if (folderId > 0)
        query.addBindValue(folderId);
    else
        query.addBindValue(QVariant());
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec())
        return 0;
    return query.lastInsertId().toLongLong();
}

void ClipStore::renameAsset(qint64 id, const QString &name)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE assets SET name = ? WHERE id = ?"));
    query.addBindValue(name.trimmed().isEmpty() ? QStringLiteral("에셋") : name.trimmed());
    query.addBindValue(id);
    query.exec();
}

void ClipStore::setAssetFolder(qint64 id, qint64 folderId)
{
    QSqlQuery query(m_db);
    if (folderId > 0) {
        query.prepare(QStringLiteral("UPDATE assets SET folder_id = ? WHERE id = ?"));
        query.addBindValue(folderId);
    } else {
        query.prepare(QStringLiteral("UPDATE assets SET folder_id = NULL WHERE id = ?"));
    }
    query.addBindValue(id);
    query.exec();
}

void ClipStore::removeAsset(qint64 id)
{
    QSqlQuery lookup(m_db);
    lookup.prepare(QStringLiteral("SELECT path FROM assets WHERE id = ?"));
    lookup.addBindValue(id);
    if (lookup.exec() && lookup.next()) {
        const QString path = lookup.value(0).toString();
        if (!path.isEmpty() && QFileInfo(path).absolutePath().compare(assetsDirectory(), Qt::CaseInsensitive) == 0)
            QFile::remove(path);
    }
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM assets WHERE id = ?"));
    query.addBindValue(id);
    query.exec();
}

// ------------------------------------------------------------------- expansions
QVector<Expansion> ClipStore::expansions() const
{
    QVector<Expansion> result;
    QSqlQuery query(m_db);
    if (!query.exec(QStringLiteral("SELECT * FROM expansions ORDER BY id ASC")))
        return result;
    while (query.next()) {
        Expansion expansion;
        expansion.id = query.value(QStringLiteral("id")).toLongLong();
        expansion.trigger = query.value(QStringLiteral("trigger")).toString();
        expansion.expansion = query.value(QStringLiteral("expansion")).toString();
        expansion.enabled = query.value(QStringLiteral("enabled")).toInt() != 0;
        result.append(expansion);
    }
    return result;
}

qint64 ClipStore::addExpansion(const QString &trigger, const QString &expansion, bool enabled)
{
    if (trigger.trimmed().isEmpty())
        return 0;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO expansions(trigger, expansion, enabled, created_at) VALUES(?,?,?,?)"));
    query.addBindValue(trigger.trimmed());
    query.addBindValue(expansion);
    query.addBindValue(enabled ? 1 : 0);
    query.addBindValue(QDateTime::currentSecsSinceEpoch());
    if (!query.exec())
        return 0;
    return query.lastInsertId().toLongLong();
}

void ClipStore::replaceExpansions(const QVariantList &rows)
{
    m_db.transaction();
    QSqlQuery(m_db).exec(QStringLiteral("DELETE FROM expansions"));
    for (const QVariant &row : rows) {
        const QVariantMap map = row.toMap();
        const QString trigger = map.value(QStringLiteral("trigger")).toString().trimmed();
        if (trigger.isEmpty())
            continue;
        addExpansion(trigger, map.value(QStringLiteral("expansion")).toString(),
                     map.value(QStringLiteral("enabled"), true).toBool());
    }
    m_db.commit();
}
