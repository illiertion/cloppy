#include "theme.h"

Theme::Theme(QObject *parent)
    : QObject(parent)
{
}

void Theme::setDark(bool dark)
{
    if (m_dark == dark)
        return;
    m_dark = dark;
    emit darkChanged();
}

QString Theme::kindColor(const QString &kind) const
{
    if (kind == QLatin1String("code"))
        return QStringLiteral("#4cc9f0");
    if (kind == QLatin1String("link"))
        return QStringLiteral("#3ddc97");
    if (kind == QLatin1String("email"))
        return QStringLiteral("#ffc857");
    if (kind == QLatin1String("color"))
        return QStringLiteral("#f472b6");
    if (kind == QLatin1String("number"))
        return QStringLiteral("#a78bfa");
    if (kind == QLatin1String("path"))
        return QStringLiteral("#fbbf24");
    if (kind == QLatin1String("command"))
        return QStringLiteral("#ff9f68");
    if (kind == QLatin1String("json"))
        return QStringLiteral("#7dd3fc");
    if (kind == QLatin1String("image"))
        return QStringLiteral("#f472b6");
    if (kind == QLatin1String("files"))
        return QStringLiteral("#fbbf24");
    return QStringLiteral("#7c5cff");
}

QString Theme::kindLabel(const QString &kind) const
{
    if (kind == QLatin1String("code"))
        return QStringLiteral("</>");
    if (kind == QLatin1String("link"))
        return QStringLiteral("URL");
    if (kind == QLatin1String("email"))
        return QStringLiteral("@");
    if (kind == QLatin1String("color"))
        return QStringLiteral("RGB");
    if (kind == QLatin1String("number"))
        return QStringLiteral("#");
    if (kind == QLatin1String("path"))
        return QStringLiteral("DIR");
    if (kind == QLatin1String("command"))
        return QStringLiteral(">_");
    if (kind == QLatin1String("json"))
        return QStringLiteral("{}");
    if (kind == QLatin1String("image"))
        return QStringLiteral("IMG");
    if (kind == QLatin1String("files"))
        return QStringLiteral("FILE");
    return QStringLiteral("T");
}
