#include "highlighter.h"

#include "translate.h"
#include <QSet>
#include <QStringList>

namespace {

struct Palette {
    const char *text;
    const char *comment;
    const char *keyword;
    const char *string;
    const char *number;
    const char *type;
    const char *function;
    const char *tag;
    const char *op;
};

const Palette kDark{"#e9ecf5", "#6b7387", "#c792ea", "#a5e075", "#f78c6c", "#ffcb6b", "#82aaff", "#ff6b81", "#89ddff"};
const Palette kLight{"#171a23", "#7b8296", "#7c3aed", "#2e7d32", "#b45309", "#a16207", "#1d4ed8", "#b91c1c", "#0e7490"};

const QSet<QString> &keywords()
{
    static const QSet<QString> set = {
        "if", "else", "for", "while", "return", "function", "func", "fn", "def", "class", "struct",
        "enum", "interface", "type", "package", "import", "from", "export", "const", "let", "var",
        "new", "this", "self", "try", "catch", "finally", "throw", "switch", "case", "default",
        "break", "continue", "public", "private", "protected", "static", "final", "async", "await",
        "yield", "lambda", "pass", "with", "as", "in", "is", "not", "and", "or", "elif", "impl",
        "use", "mod", "match", "where", "select", "insert", "update", "delete", "create", "table",
        "values", "join", "group", "order", "by", "limit", "offset", "end", "then", "do", "begin",
        "namespace", "using", "template", "operator", "virtual", "override", "extends", "implements",
        "raise", "except", "global", "nonlocal", "assert", "del", "typeof", "instanceof", "void",
        "int", "float", "double", "char", "long", "short", "unsigned", "signed", "auto", "string",
        "bool", "boolean", "number", "any", "unknown", "never", "object", "mut", "pub", "crate",
        "module", "require", "echo", "fi", "done", "esac", "local", "readonly", "declare",
    };
    return set;
}

const QSet<QString> &booleans()
{
    static const QSet<QString> set = {
        "true", "false", "null", "none", "nil", "undefined", "True", "False", "None", "NULL", "TRUE", "FALSE",
    };
    return set;
}

const QSet<QString> &types()
{
    static const QSet<QString> set = {
        "QString", "QObject", "QVariant", "QList", "QVector", "QMap", "QHash", "QWidget", "QWindow",
        "String", "Object", "Array", "Promise", "Map", "Set", "List", "Dict", "Tuple", "Optional",
        "Integer", "Boolean", "vec2", "vec3", "vec4", "u8", "u16", "u32", "u64", "i8", "i16", "i32", "i64",
        "f32", "f64", "usize", "isize", "size_t", "uint8_t", "uint32_t", "uint64_t", "int32_t",
    };
    return set;
}

QString span(const char *color, const QString &text)
{
    return QStringLiteral("<span style=\"color:%1\">%2</span>")
        .arg(QString::fromLatin1(color), text.toHtmlEscaped());
}

bool isWordChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_') || ch == QLatin1Char('$');
}

bool isOperator(QChar ch)
{
    static const QString ops = QStringLiteral("+-*/%=<>!&|^~?:");
    return ops.contains(ch);
}

QString safeText(const QString &text)
{
    if (text.size() > 40000)
        return text.left(40000) + Tr::text(QStringLiteral("\n… (미리보기는 40,000자까지 표시)"), QStringLiteral("\n… (preview limited to 40,000 characters)"));
    return text;
}

} // namespace

QString HtmlHighlighter::highlight(const QString &input, const QString &language, bool dark)
{
    const QString text = safeText(input);
    const Palette &palette = dark ? kDark : kLight;
    QString out;
    out.reserve(text.size() + text.size() / 3);

    const QString lang = language.toLower();
    const bool hashComments = lang == "python" || lang == "bash" || lang == "ruby" || lang == "yaml"
        || lang == "toml" || lang == "powershell" || lang == "dockerfile" || lang == "cmake" || lang == "qml";
    const bool slashComments = !hashComments && lang != "sql" && lang != "html" && lang != "xml";
    const bool dashComments = lang == "sql";
    const bool markup = lang == "html" || lang == "xml" || lang == "svg";
    const bool markdown = lang == "markdown";
    int i = 0;
    const int n = text.size();

    while (i < n) {
        const QChar ch = text.at(i);

        if (markup && ch == QLatin1Char('<')) {
            int end = i;
            while (end < n && text.at(end) != QLatin1Char('>'))
                ++end;
            if (end < n)
                ++end;
            out += span(palette.tag, text.mid(i, end - i));
            i = end;
            continue;
        }
        if (hashComments && ch == QLatin1Char('#')) {
            int end = text.indexOf(QLatin1Char('\n'), i);
            if (end < 0)
                end = n;
            out += span(palette.comment, text.mid(i, end - i));
            i = end;
            continue;
        }
        if (dashComments && text.mid(i, 2) == QStringLiteral("--")) {
            int end = text.indexOf(QLatin1Char('\n'), i);
            if (end < 0)
                end = n;
            out += span(palette.comment, text.mid(i, end - i));
            i = end;
            continue;
        }
        if (slashComments && text.mid(i, 2) == QStringLiteral("//")) {
            int end = text.indexOf(QLatin1Char('\n'), i);
            if (end < 0)
                end = n;
            out += span(palette.comment, text.mid(i, end - i));
            i = end;
            continue;
        }
        if (slashComments && text.mid(i, 2) == QStringLiteral("/*")) {
            int end = text.indexOf(QStringLiteral("*/"), i + 2);
            end = end < 0 ? n : end + 2;
            out += span(palette.comment, text.mid(i, end - i));
            i = end;
            continue;
        }
        if (markup && text.mid(i, 4) == QStringLiteral("<!--")) {
            int end = text.indexOf(QStringLiteral("-->"), i + 4);
            end = end < 0 ? n : end + 3;
            out += span(palette.comment, text.mid(i, end - i));
            i = end;
            continue;
        }
        if (ch == QLatin1Char('"') || ch == QLatin1Char('\'') || (ch == QLatin1Char('`') && (lang == "javascript" || lang == "typescript" || lang == "qml"))) {
            if (lang == "python" && text.mid(i, 3) == QString(ch).repeated(3)) {
                int end = text.indexOf(QString(ch).repeated(3), i + 3);
                end = end < 0 ? n : end + 3;
                out += span(palette.string, text.mid(i, end - i));
                i = end;
                continue;
            }
            int j = i + 1;
            while (j < n) {
                const QChar current = text.at(j);
                if (current == QLatin1Char('\\')) {
                    j += 2;
                    continue;
                }
                if (current == ch)
                    break;
                if (current == QLatin1Char('\n') && ch != QLatin1Char('`'))
                    break;
                ++j;
            }
            if (j < n && text.at(j) == ch)
                ++j;
            out += span(palette.string, text.mid(i, j - i));
            i = j;
            continue;
        }
        if (ch.isDigit()) {
            int j = i + 1;
            while (j < n && (text.at(j).isLetterOrNumber() || text.at(j) == QLatin1Char('.') || text.at(j) == QLatin1Char('_')))
                ++j;
            out += span(palette.number, text.mid(i, j - i));
            i = j;
            continue;
        }
        if (ch.isLetter() || ch == QLatin1Char('_') || ch == QLatin1Char('$') || ch.unicode() >= 0x80) {
            int j = i + 1;
            while (j < n && isWordChar(text.at(j)))
                ++j;
            const QString word = text.mid(i, j - i);
            if (markdown && ch == QLatin1Char('#') && (i == 0 || text.at(i - 1) == QLatin1Char('\n'))) {
                int end = text.indexOf(QLatin1Char('\n'), i);
                end = end < 0 ? n : end;
                out += span(palette.keyword, text.mid(i, end - i));
                i = end;
                continue;
            }
            if (keywords().contains(word)) {
                out += span(palette.keyword, word);
            } else if (booleans().contains(word)) {
                out += span(palette.number, word);
            } else if (types().contains(word)) {
                out += span(palette.type, word);
            } else {
                int k = j;
                while (k < n && text.at(k).isSpace() && text.at(k) != QLatin1Char('\n'))
                    ++k;
                if (k < n && text.at(k) == QLatin1Char('('))
                    out += span(palette.function, word);
                else
                    out += word.toHtmlEscaped();
            }
            i = j;
            continue;
        }
        if (isOperator(ch)) {
            int j = i + 1;
            while (j < n && isOperator(text.at(j)))
                ++j;
            out += span(palette.op, text.mid(i, j - i));
            i = j;
            continue;
        }
        out += QString(ch).toHtmlEscaped();
        ++i;
    }

    return QStringLiteral("<pre style=\"white-space:pre-wrap; word-wrap:break-word; margin:0;"
                          " font-family:'Cascadia Mono','Consolas',monospace; font-size:11pt;"
                          " line-height:1.55; color:%1;\">%2</pre>")
        .arg(QString::fromLatin1(palette.text), out);
}

QString HtmlHighlighter::plain(const QString &input, bool dark)
{
    const Palette &palette = dark ? kDark : kLight;
    const QString escaped = safeText(input).toHtmlEscaped();
    return QStringLiteral("<pre style=\"white-space:pre-wrap; word-wrap:break-word; margin:0;"
                          " font-family:'Segoe UI','Malgun Gothic',sans-serif; font-size:11pt;"
                          " line-height:1.55; color:%1;\">%2</pre>")
        .arg(QString::fromLatin1(palette.text), escaped);
}
