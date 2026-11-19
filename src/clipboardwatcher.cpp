#include "clipboardwatcher.h"

#include "translate.h"
#include <QBuffer>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QMimeData>
#include <QRegularExpression>
#include <QUrl>

#include "clipstore.h"
#include "winutils.h"

namespace {

QString sha1Hex(const QByteArray &data)
{
    return QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex());
}

QString cleanTerminalText(const QString &text)
{
    QString out = text;
    out.remove(QRegularExpression(QStringLiteral("\x1B\\[[0-9;?]*[ -/]*[@-~]")));
    out.remove(QRegularExpression(QStringLiteral("\x1B\\][^\x07]*(?:\x07|\x1B\\\\)")));
    out.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    out.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    while (!out.isEmpty()
           && (out.endsWith(QLatin1Char('\n')) || out.endsWith(QLatin1Char(' '))
               || out.endsWith(QLatin1Char('\t'))))
        out.chop(1);
    return out;
}

} // namespace

ClipboardWatcher::ClipboardWatcher(QObject *parent)
    : QObject(parent)
    , m_clipboard(QGuiApplication::clipboard())
{
    if (m_clipboard)
        connect(m_clipboard, &QClipboard::dataChanged, this, &ClipboardWatcher::check);
    m_timer.setInterval(700);
    connect(&m_timer, &QTimer::timeout, this, &ClipboardWatcher::check);
    m_timer.start();
}

void ClipboardWatcher::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

void ClipboardWatcher::guard(const QString &hash)
{
    m_guardHash = hash;
    m_lastHash = hash;
}

void ClipboardWatcher::setProgrammerMode(bool enabled)
{
    m_programmerMode = enabled;
}

void ClipboardWatcher::setTerminalApps(const QStringList &apps)
{
    m_terminalApps.clear();
    for (const QString &app : apps) {
        const QString trimmed = app.trimmed();
        if (!trimmed.isEmpty())
            m_terminalApps.append(trimmed.toLower());
    }
}

bool ClipboardWatcher::skip(const QString &hash)
{
    if (hash == m_lastHash)
        return true;
    m_lastHash = hash;
    return false;
}

void ClipboardWatcher::check()
{
    if (!m_enabled || !m_clipboard)
        return;
    const QMimeData *mime = m_clipboard->mimeData();
    if (!mime)
        return;

    CapturePayload payload;
    payload.sourceApp = WinUtils::foregroundProcessName();

    if (mime->hasImage()) {
        const QImage image = m_clipboard->image();
        if (image.isNull())
            return;
        QByteArray bytes;
        QBuffer buffer(&bytes);
        buffer.open(QIODevice::WriteOnly);
        if (!image.save(&buffer, "PNG"))
            return;
        buffer.close();
        const QString digest = sha1Hex(bytes);
        const QString hash = QStringLiteral("image:") + digest;
        if (skip(hash))
            return;
        const QString path = QDir(ClipStore::imagesDirectory()).filePath(digest + QStringLiteral(".png"));
        if (!QFile::exists(path)) {
            QFile file(path);
            if (file.open(QIODevice::WriteOnly))
                file.write(bytes);
        }
        payload.kind = QStringLiteral("image");
        payload.imagePath = path;
        payload.hash = hash;
        payload.size = bytes.size();
        payload.title = Tr::text(QStringLiteral("이미지 %1×%2"), QStringLiteral("Image %1×%2")).arg(image.width()).arg(image.height());
        payload.preview = QStringLiteral("%1 × %2 PNG").arg(image.width()).arg(image.height());
        if (payload.sourceApp.isEmpty())
            payload.sourceApp = Tr::text(QStringLiteral("클립보드"), QStringLiteral("Clips"));
        Q_EMIT captured(payload);
        return;
    }

    if (mime->hasUrls()) {
        QStringList paths;
        const QList<QUrl> urls = mime->urls();
        for (const QUrl &url : urls) {
            const QString local = url.toLocalFile();
            if (!local.isEmpty())
                paths.append(QDir::cleanPath(local));
        }
        if (!paths.isEmpty()) {
            const QString hash = QStringLiteral("files:") + sha1Hex(paths.join(QLatin1Char('\n')).toUtf8());
            if (skip(hash))
                return;
            qint64 size = 0;
            for (const QString &path : paths) {
                const QFileInfo info(path);
                if (info.isFile())
                    size += info.size();
            }
            payload.kind = QStringLiteral("files");
            payload.content = paths.join(QLatin1Char('\n'));
            payload.files = paths;
            payload.hash = hash;
            payload.size = size;
            if (payload.sourceApp.isEmpty())
                payload.sourceApp = Tr::text(QStringLiteral("탐색기"), QStringLiteral("Explorer"));
            Q_EMIT captured(payload);
            return;
        }
    }

    if (mime->hasText()) {
        QString text = m_clipboard->text();
        if (text.isEmpty() || text.trimmed().isEmpty())
            return;
        if (text.size() > 500000)
            text = text.left(500000);
        if (m_programmerMode && m_terminalApps.contains(payload.sourceApp.toLower())) {
            text = cleanTerminalText(text);
            if (text.trimmed().isEmpty())
                return;
        }
        const QString hash = QStringLiteral("text:") + sha1Hex(text.toUtf8());
        if (skip(hash))
            return;
        payload.kind = QStringLiteral("text");
        payload.content = text;
        payload.hash = hash;
        payload.size = text.toUtf8().size();
        Q_EMIT captured(payload);
    }
}
