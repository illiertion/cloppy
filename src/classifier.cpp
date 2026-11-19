#include "classifier.h"

#include "translate.h"
#include "types.h"

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QUrl>

namespace {

const QRegularExpression kUrl(
    QStringLiteral("^(?:(?:https?|ftp)://[^\\s]+|www\\.[a-z0-9.-]+\\.[a-z]{2,}(?:/\\S*)?)$"),
    QRegularExpression::CaseInsensitiveOption);

const QRegularExpression kEmail(
    QStringLiteral("^[^\\s@<>()\\[\\]]+@[^\\s@<>()\\[\\]]+\\.[A-Za-z]{2,}$"));

const QRegularExpression kColor(
    QStringLiteral("^(?:#(?:[0-9a-fA-F]{3}|[0-9a-fA-F]{6}|[0-9a-fA-F]{8})|rgba?\\([^)]*\\)|hsla?\\([^)]*\\))$"));

const QRegularExpression kPath(
    QStringLiteral("^(?:[A-Za-z]:[\\\\/][^\\r\\n]*|\\\\\\\\[^\\\\/\\r\\n]+[\\\\/][^\\r\\n]+|~[\\\\/][^\\r\\n]+|\\.{1,2}[\\\\/][^\\r\\n]+)$"));

const QRegularExpression kCommand(
    QStringLiteral("^(?:sudo\\s+|npm\\s+|npx\\s+|yarn\\s+|pnpm\\s+|bun\\s+|git\\s+|pip\\s+|pip3\\s+|python\\s+|py\\s+"
                   "|node\\s+|deno\\s+|docker\\s+|docker-compose\\s+|kubectl\\s+|helm\\s+|cd\\s+|ls\\b|dir\\b|mkdir\\s+"
                   "|rm\\s+|del\\s+|copy\\s+|move\\s+|curl\\s+|wget\\s+|ssh\\s+|scp\\s+|winget\\s+|choco\\s+|scoop\\s+"
                   "|code\\s+|cmake\\s+|make\\s+|cargo\\s+|go\\s+run\\b|mvn\\s+|gradle\\s+|cls\\b|powershell\\s+|pwsh\\s+)"),
    QRegularExpression::CaseInsensitiveOption);

const QRegularExpression kCodeMarkers(QStringLiteral(
    "[{};]|=>|==|!=|//|/\\*|<!--|^\\s{2,}\\S|<\\w+[^>]*>|\\b(?:def|class|func|fn|function|import|from|"
    "const|let|var|return|public|private|package|namespace|SELECT|echo)\\b"),
    QRegularExpression::MultilineOption);

const QRegularExpression kCodeStart(QStringLiteral(
    "^\\s*(?:def|class|function|func|fn|import|from|const|let|var|return|#include|public|private|package)\\b"),
    QRegularExpression::MultilineOption);

struct LanguageHint {
    const char *language;
    const char *pattern;
    QRegularExpression::PatternOptions options;
};

const LanguageHint kLanguageHints[] = {
    {"python", "\\b(?:def|class)\\s+\\w+|^\\s*(?:from\\s+\\w+\\s+)?import\\s+\\w+|if __name__ ==|print\\(", QRegularExpression::MultilineOption},
    {"javascript", "\\b(?:const|let|var)\\s+\\w+\\s*=|\\bfunction\\s+\\w*\\s*\\(|=>|console\\.log\\(", QRegularExpression::NoPatternOption},
    {"typescript", "\\binterface\\s+\\w+\\s*\\{|\\btype\\s+\\w+\\s*=|:\\s*(?:string|number|boolean)\\b", QRegularExpression::NoPatternOption},
    {"html", "</?(?:html|div|span|body|head|p|a|img|ul|li|table|section|header|main)\\b", QRegularExpression::CaseInsensitiveOption},
    {"css", "(?:^|\\n)\\s*[.#]?[\\w-]+\\s*\\{[^}]*:[^}]*\\}", QRegularExpression::DotMatchesEverythingOption},
    {"sql", "\\b(?:SELECT|INSERT INTO|UPDATE|DELETE FROM|CREATE TABLE)\\b", QRegularExpression::CaseInsensitiveOption},
    {"java", "\\bpublic\\s+(?:static\\s+)?(?:class|void|int|String)\\b", QRegularExpression::NoPatternOption},
    {"csharp", "\\busing\\s+System\\b|\\bnamespace\\s+\\w+|\\bpublic\\s+class\\s+\\w+", QRegularExpression::NoPatternOption},
    {"cpp", "#include\\s*<|std::|int\\s+main\\s*\\(", QRegularExpression::NoPatternOption},
    {"go", "^\\s*package\\s+\\w+|func\\s+\\w+\\s*\\(|:=|fmt\\.Print", QRegularExpression::MultilineOption},
    {"rust", "\\bfn\\s+\\w+\\s*\\(|let\\s+mut\\s+|println!\\(|use\\s+std::", QRegularExpression::NoPatternOption},
    {"php", "<\\?php|\\$\\w+\\s*=", QRegularExpression::NoPatternOption},
    {"ruby", "^\\s*def\\s+\\w+|\\bputs\\s+|require\\s+'", QRegularExpression::MultilineOption},
    {"bash", "^#!.*(?:bash|sh)|^\\s*(?:echo|export|alias)\\s+|\\$\\(", QRegularExpression::MultilineOption},
    {"powershell", "\\bGet-\\w+|\\bSet-\\w+|\\$\\w+\\s*=\\s*Get-", QRegularExpression::CaseInsensitiveOption},
    {"yaml", "^\\s*[\\w.-]+:\\s+\\S+", QRegularExpression::MultilineOption},
    {"markdown", "^#{1,6}\\s+\\S|^\\s*[-*]\\s+\\S|```", QRegularExpression::MultilineOption},
    {"dockerfile", "^\\s*FROM\\s+\\S+|^\\s*RUN\\s+\\S+", QRegularExpression::MultilineOption | QRegularExpression::CaseInsensitiveOption},
};

QString extensionLanguage(const QString &path)
{
    static const QHash<QString, QString> map = {
        {QStringLiteral(".py"), QStringLiteral("python")},
        {QStringLiteral(".js"), QStringLiteral("javascript")},
        {QStringLiteral(".jsx"), QStringLiteral("javascript")},
        {QStringLiteral(".ts"), QStringLiteral("typescript")},
        {QStringLiteral(".tsx"), QStringLiteral("typescript")},
        {QStringLiteral(".json"), QStringLiteral("json")},
        {QStringLiteral(".html"), QStringLiteral("html")},
        {QStringLiteral(".htm"), QStringLiteral("html")},
        {QStringLiteral(".css"), QStringLiteral("css")},
        {QStringLiteral(".scss"), QStringLiteral("css")},
        {QStringLiteral(".java"), QStringLiteral("java")},
        {QStringLiteral(".cs"), QStringLiteral("csharp")},
        {QStringLiteral(".cpp"), QStringLiteral("cpp")},
        {QStringLiteral(".cc"), QStringLiteral("cpp")},
        {QStringLiteral(".c"), QStringLiteral("c")},
        {QStringLiteral(".h"), QStringLiteral("c")},
        {QStringLiteral(".hpp"), QStringLiteral("cpp")},
        {QStringLiteral(".go"), QStringLiteral("go")},
        {QStringLiteral(".rs"), QStringLiteral("rust")},
        {QStringLiteral(".rb"), QStringLiteral("ruby")},
        {QStringLiteral(".php"), QStringLiteral("php")},
        {QStringLiteral(".sql"), QStringLiteral("sql")},
        {QStringLiteral(".sh"), QStringLiteral("bash")},
        {QStringLiteral(".ps1"), QStringLiteral("powershell")},
        {QStringLiteral(".yml"), QStringLiteral("yaml")},
        {QStringLiteral(".yaml"), QStringLiteral("yaml")},
        {QStringLiteral(".md"), QStringLiteral("markdown")},
        {QStringLiteral(".xml"), QStringLiteral("xml")},
        {QStringLiteral(".toml"), QStringLiteral("toml")},
        {QStringLiteral(".qml"), QStringLiteral("qml")},
        {QStringLiteral(".cmake"), QStringLiteral("cmake")},
    };
    return map.value(QFileInfo(path).suffix().prepend('.').toLower());
}

bool validJson(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.size() < 2)
        return false;
    const QChar first = trimmed.at(0);
    if (first != QLatin1Char('{') && first != QLatin1Char('['))
        return false;
    QJsonParseError error;
    QJsonDocument::fromJson(trimmed.toUtf8(), &error);
    return error.error == QJsonParseError::NoError;
}

} // namespace

namespace Classifier {

Result classify(const QString &text)
{
    Result result;
    QString content = text;
    content.remove(QChar(0));
    const QString trimmed = content.trimmed();
    const bool singleLine = !trimmed.contains(QLatin1Char('\n'));

    if (trimmed.isEmpty()) {
        result.title = Tr::text(QStringLiteral("빈 텍스트"), QStringLiteral("Empty text"));
        return result;
    }

    if (singleLine && kColor.match(trimmed).hasMatch()) {
        result.kind = QStringLiteral("color");
        result.tags = Tr::text(QStringLiteral("색상"), QStringLiteral("Colors"));
    } else if (singleLine && kUrl.match(trimmed).hasMatch()) {
        result.kind = QStringLiteral("link");
        QUrl url(trimmed.contains(QStringLiteral("://")) ? trimmed : QStringLiteral("http://") + trimmed);
        if (!url.host().isEmpty())
            result.tags = url.host();
    } else if (singleLine && kEmail.match(trimmed).hasMatch()) {
        result.kind = QStringLiteral("email");
        result.tags = Tr::text(QStringLiteral("이메일"), QStringLiteral("Email"));
    } else if (singleLine && kPath.match(trimmed).hasMatch()) {
        result.kind = QStringLiteral("path");
        result.tags = Tr::text(QStringLiteral("경로"), QStringLiteral("Path"));
        QString ext = QFileInfo(trimmed).suffix();
        if (!ext.isEmpty())
            result.tags += QStringLiteral(",.") + ext;
    } else if (singleLine && kCommand.match(trimmed).hasMatch()) {
        result.kind = QStringLiteral("command");
        result.language = trimmed.startsWith(QStringLiteral("powershell"), Qt::CaseInsensitive)
                || trimmed.startsWith(QStringLiteral("pwsh"), Qt::CaseInsensitive)
                || trimmed.startsWith(QStringLiteral("Get-"), Qt::CaseInsensitive)
            ? QStringLiteral("powershell")
            : QStringLiteral("bash");
        result.tags = Tr::text(QStringLiteral("명령어"), QStringLiteral("Commands"));
    } else if (validJson(trimmed)) {
        result.kind = QStringLiteral("code");
        result.language = QStringLiteral("json");
        result.tags = QStringLiteral("json");
    } else if (looksLikeCode(content)) {
        result.kind = QStringLiteral("code");
        result.language = guessLanguage(content);
        if (!result.language.isEmpty())
            result.tags = result.language;
    } else if (trimmed.startsWith(QStringLiteral("#!")) && trimmed.contains(QLatin1Char('\n'))) {
        result.kind = QStringLiteral("code");
        result.language = QStringLiteral("bash");
    } else {
        result.kind = QStringLiteral("text");
    }

    result.title = firstLine(content, 120);
    if (result.title.isEmpty())
        result.title = Tr::text(QStringLiteral("텍스트"), QStringLiteral("Text"));
    result.preview = collapseWhitespace(content, 240);
    return result;
}

bool looksLikeCode(const QString &text)
{
    if (text.size() < 8)
        return false;
    if (!text.contains(QLatin1Char('\n'))) {
        static const QRegularExpression single(
            QStringLiteral("[{};]|=>|::|</\\w+>|\\b(?:console\\.log|print|printf|alert|echo)\\s*\\(|^\\w+(?:\\.\\w+)*\\([^)]*\\)\\s*$"));
        return single.match(text).hasMatch();
    }
    int score = 0;
    auto matches = kCodeMarkers.globalMatch(text);
    while (matches.hasNext()) {
        matches.next();
        ++score;
    }
    if (kCodeStart.match(text).hasMatch())
        ++score;
    if (text.trimmed().startsWith(QStringLiteral("#!")))
        score += 3;
    return score >= 3;
}

QString guessLanguage(const QString &text)
{
    for (const LanguageHint &hint : kLanguageHints) {
        const QRegularExpression pattern(QString::fromLatin1(hint.pattern), hint.options);
        if (pattern.match(text).hasMatch())
            return QString::fromLatin1(hint.language);
    }
    return {};
}

QString languageForPath(const QString &path)
{
    return extensionLanguage(path);
}

QString previewLine(const QString &text, int limit)
{
    QStringList parts;
    const QStringList lines = text.split(QRegularExpression(QStringLiteral("[\\r\\n]")));
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (!trimmed.isEmpty()) {
            parts.append(trimmed);
            if (parts.size() == 2)
                break;
        }
    }
    QString joined = parts.join(QStringLiteral("  "));
    if (joined.size() > limit)
        joined = joined.left(limit - 1) + QChar(0x2026);
    return joined;
}

} // namespace Classifier
