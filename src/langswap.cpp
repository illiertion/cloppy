#include "langswap.h"

#include <QHash>
#include <QRegularExpression>
#include <QSet>

namespace {

const QString CHO = QStringLiteral("ㄱㄲㄴㄷㄸㄹㅁㅂㅃㅅㅆㅇㅈㅉㅊㅋㅌㅍㅎ");
const QString JUNG = QStringLiteral("ㅏㅐㅑㅒㅓㅔㅕㅖㅗㅘㅙㅚㅛㅜㅝㅞㅟㅠㅡㅢㅣ");
const QString JONG = QStringLiteral(" ㄱㄲㄳㄴㄵㄶㄷㄹㄺㄻㄼㄽㄾㄿㅀㅁㅂㅄㅅㅆㅇㅈㅊㅋㅌㅍㅎ");

const QHash<QChar, QChar> &keyToJamo()
{
    static const QHash<QChar, QChar> map = {
        {u'q', u'ㅂ'}, {u'w', u'ㅈ'}, {u'e', u'ㄷ'}, {u'r', u'ㄱ'}, {u't', u'ㅅ'},
        {u'y', u'ㅛ'}, {u'u', u'ㅕ'}, {u'i', u'ㅑ'}, {u'o', u'ㅐ'}, {u'p', u'ㅔ'},
        {u'a', u'ㅁ'}, {u's', u'ㄴ'}, {u'd', u'ㅇ'}, {u'f', u'ㄹ'}, {u'g', u'ㅎ'},
        {u'h', u'ㅗ'}, {u'j', u'ㅓ'}, {u'k', u'ㅏ'}, {u'l', u'ㅣ'},
        {u'z', u'ㅋ'}, {u'x', u'ㅌ'}, {u'c', u'ㅊ'}, {u'v', u'ㅍ'}, {u'b', u'ㅠ'},
        {u'n', u'ㅜ'}, {u'm', u'ㅡ'},
        {u'Q', u'ㅃ'}, {u'W', u'ㅉ'}, {u'E', u'ㄸ'}, {u'R', u'ㄲ'}, {u'T', u'ㅆ'},
        {u'O', u'ㅒ'}, {u'P', u'ㅖ'},
    };
    return map;
}

const QHash<QChar, QChar> &jamoToKey()
{
    static QHash<QChar, QChar> map;
    if (map.isEmpty()) {
        const auto &source = keyToJamo();
        for (auto it = source.cbegin(); it != source.cend(); ++it)
            map.insert(it.value(), it.key());
    }
    return map;
}

const QHash<QString, QString> &vowelCombine()
{
    static const QHash<QString, QString> map = {
        {QStringLiteral("ㅗㅏ"), QStringLiteral("ㅘ")},
        {QStringLiteral("ㅗㅐ"), QStringLiteral("ㅙ")},
        {QStringLiteral("ㅗㅣ"), QStringLiteral("ㅚ")},
        {QStringLiteral("ㅜㅓ"), QStringLiteral("ㅝ")},
        {QStringLiteral("ㅜㅔ"), QStringLiteral("ㅞ")},
        {QStringLiteral("ㅜㅣ"), QStringLiteral("ㅟ")},
        {QStringLiteral("ㅡㅣ"), QStringLiteral("ㅢ")},
    };
    return map;
}

const QHash<QString, QString> &finalCombine()
{
    static const QHash<QString, QString> map = {
        {QStringLiteral("ㄱㅅ"), QStringLiteral("ㄳ")},
        {QStringLiteral("ㄴㅈ"), QStringLiteral("ㄵ")},
        {QStringLiteral("ㄴㅎ"), QStringLiteral("ㄶ")},
        {QStringLiteral("ㄹㄱ"), QStringLiteral("ㄺ")},
        {QStringLiteral("ㄹㅁ"), QStringLiteral("ㄻ")},
        {QStringLiteral("ㄹㅂ"), QStringLiteral("ㄼ")},
        {QStringLiteral("ㄹㅅ"), QStringLiteral("ㄽ")},
        {QStringLiteral("ㄹㅌ"), QStringLiteral("ㄾ")},
        {QStringLiteral("ㄹㅍ"), QStringLiteral("ㄿ")},
        {QStringLiteral("ㄹㅎ"), QStringLiteral("ㅀ")},
        {QStringLiteral("ㅂㅅ"), QStringLiteral("ㅄ")},
    };
    return map;
}

QSet<QChar> vowelSet()
{
    QSet<QChar> set;
    for (const QChar ch : JUNG)
        set.insert(ch);
    return set;
}

QSet<QChar> finalSet()
{
    QSet<QChar> set;
    for (int i = 1; i < JONG.size(); ++i)
        set.insert(JONG.at(i));
    return set;
}

QString compose(const QString &cho, const QString &jung, const QString &jong)
{
    if (cho.isEmpty() && jung.isEmpty())
        return {};
    if (cho.isEmpty())
        return jung + jong;
    if (jung.isEmpty())
        return cho + jong;
    const int choIndex = CHO.indexOf(cho);
    const int jungIndex = JUNG.indexOf(jung);
    const int jongIndex = jong.isEmpty() ? 0 : JONG.indexOf(jong);
    if (choIndex < 0 || jungIndex < 0 || jongIndex < 0)
        return cho + jung + jong;
    const uint code = 0xAC00 + (choIndex * 21 + jungIndex) * 28 + jongIndex;
    return QString(QChar(static_cast<ushort>(code)));
}

QStringList splitVowel(const QString &vowel)
{
    for (auto it = vowelCombine().cbegin(); it != vowelCombine().cend(); ++it) {
        if (it.value() == vowel)
            return {it.key().left(1), it.key().mid(1)};
    }
    return {vowel};
}

QStringList splitFinal(const QString &final)
{
    for (auto it = finalCombine().cbegin(); it != finalCombine().cend(); ++it) {
        if (it.value() == final)
            return {it.key().left(1), it.key().mid(1)};
    }
    return {final};
}

} // namespace

namespace LangSwap {

QString enToKo(const QString &text)
{
    QString out;
    QString cho, jung, jong;
    const auto flush = [&]() {
        const QString syllable = compose(cho, jung, jong);
        if (!syllable.isEmpty())
            out += syllable;
        cho.clear();
        jung.clear();
        jong.clear();
    };
    const auto &mapping = keyToJamo();
    const QSet<QChar> vowels = vowelSet();
    const QSet<QChar> finals = finalSet();

    for (const QChar ch : text) {
        const auto it = mapping.constFind(ch);
        if (it == mapping.cend()) {
            flush();
            out += ch;
            continue;
        }
        const QString jamo(it.value());

        if (vowels.contains(it.value())) {
            if (cho.isEmpty() && jung.isEmpty()) {
                out += jamo;
            } else if (jung.isEmpty()) {
                jung = jamo;
            } else if (jong.isEmpty()) {
                const QString combined = vowelCombine().value(jung + jamo);
                if (!combined.isEmpty()) {
                    jung = combined;
                } else {
                    flush();
                    out += jamo;
                }
            } else {
                if (CHO.contains(jong)) {
                    const QString moved = jong;
                    jong.clear();
                    flush();
                    cho = moved;
                    jung = jamo;
                } else {
                    flush();
                    out += jamo;
                }
            }
        } else {
            if (cho.isEmpty() && jung.isEmpty()) {
                cho = jamo;
            } else if (jung.isEmpty()) {
                flush();
                cho = jamo;
            } else if (jong.isEmpty()) {
                if (finals.contains(it.value()))
                    jong = jamo;
                else {
                    flush();
                    cho = jamo;
                }
            } else {
                const QString combined = finalCombine().value(jong + jamo);
                if (!combined.isEmpty())
                    jong = combined;
                else {
                    flush();
                    cho = jamo;
                }
            }
        }
    }
    flush();
    return out;
}

QString koToEn(const QString &text)
{
    QString out;
    const auto &mapping = jamoToKey();
    for (const QChar ch : text) {
        const ushort code = ch.unicode();
        if (code >= 0xAC00 && code <= 0xD7A3) {
            const int index = code - 0xAC00;
            const QString cho = CHO.mid(index / (21 * 28), 1);
            const QString jung = JUNG.mid((index % (21 * 28)) / 28, 1);
            const int jongIndex = index % 28;
            const QString jong = jongIndex == 0 ? QString() : JONG.mid(jongIndex, 1);
            QStringList jamo;
            if (!cho.isEmpty())
                jamo.append(cho);
            if (!jung.isEmpty())
                jamo.append(splitVowel(jung));
            if (!jong.isEmpty())
                jamo.append(splitFinal(jong));
            for (const QString &part : jamo) {
                const QChar jamoChar = part.at(0);
                out += mapping.value(jamoChar, jamoChar);
            }
        } else {
            out += mapping.value(ch, ch);
        }
    }
    return out;
}

QString swap(const QString &text)
{
    if (text.isEmpty())
        return text;
    static const QRegularExpression hangul(QStringLiteral("[\\x{AC00}-\\x{D7A3}]"));
    static const QRegularExpression latin(QStringLiteral("[A-Za-z]"));
    const int hangulCount = text.count(hangul);
    const int latinCount = text.count(latin);
    if (hangulCount == 0 && latinCount == 0)
        return text;
    return hangulCount >= latinCount && hangulCount > 0 ? koToEn(text) : enToKo(text);
}

QString swapDirected(const QString &text, const QString &direction)
{
    if (text.isEmpty())
        return text;
    if (direction == QStringLiteral("ko"))
        return enToKo(text);
    if (direction == QStringLiteral("en"))
        return koToEn(text);
    return swap(text);
}

QString swappedOrEmpty(const QString &text)
{
    const QString result = swap(text);
    return result == text ? QString() : result;
}

} // namespace LangSwap
