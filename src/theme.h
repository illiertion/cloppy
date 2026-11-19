#pragma once

#include <QColor>
#include <QObject>
#include <QString>

class Theme : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool dark READ dark WRITE setDark NOTIFY darkChanged)
    Q_PROPERTY(QColor bg READ bg NOTIFY darkChanged)
    Q_PROPERTY(QColor surface READ surface NOTIFY darkChanged)
    Q_PROPERTY(QColor surface2 READ surface2 NOTIFY darkChanged)
    Q_PROPERTY(QColor surface3 READ surface3 NOTIFY darkChanged)
    Q_PROPERTY(QColor border READ border NOTIFY darkChanged)
    Q_PROPERTY(QColor text READ text NOTIFY darkChanged)
    Q_PROPERTY(QColor muted READ muted NOTIFY darkChanged)
    Q_PROPERTY(QColor faint READ faint NOTIFY darkChanged)
    Q_PROPERTY(QColor accent READ accent NOTIFY darkChanged)
    Q_PROPERTY(QColor accentSoft READ accentSoft NOTIFY darkChanged)
    Q_PROPERTY(QColor accentText READ accentText CONSTANT)
    Q_PROPERTY(QColor green READ green CONSTANT)
    Q_PROPERTY(QColor amber READ amber CONSTANT)
    Q_PROPERTY(QColor red READ red CONSTANT)
    Q_PROPERTY(QColor blue READ blue CONSTANT)
    Q_PROPERTY(QColor thumb READ thumb NOTIFY darkChanged)
    Q_PROPERTY(int radius READ radius CONSTANT)
    Q_PROPERTY(QString font READ font CONSTANT)

public:
    explicit Theme(QObject *parent = nullptr);

    bool dark() const { return m_dark; }
    void setDark(bool dark);

    QColor bg() const { return m_dark ? QColor(QStringLiteral("#0a0c12")) : QColor(QStringLiteral("#f5f6fb")); }
    QColor surface() const { return m_dark ? QColor(QStringLiteral("#12151f")) : QColor(QStringLiteral("#ffffff")); }
    QColor surface2() const { return m_dark ? QColor(QStringLiteral("#1a1e2c")) : QColor(QStringLiteral("#eef0f8")); }
    QColor surface3() const { return m_dark ? QColor(QStringLiteral("#232838")) : QColor(QStringLiteral("#e3e6f2")); }
    QColor border() const { return m_dark ? QColor(QStringLiteral("#272d40")) : QColor(QStringLiteral("#dde1ee")); }
    QColor text() const { return m_dark ? QColor(QStringLiteral("#eef2ff")) : QColor(QStringLiteral("#151827")); }
    QColor muted() const { return m_dark ? QColor(QStringLiteral("#98a1b8")) : QColor(QStringLiteral("#5f667c")); }
    QColor faint() const { return m_dark ? QColor(QStringLiteral("#626b82")) : QColor(QStringLiteral("#969db0")); }
    QColor accent() const { return m_dark ? QColor(QStringLiteral("#7c6bff")) : QColor(QStringLiteral("#6a55f5")); }
    QColor accentSoft() const { return m_dark ? QColor(QStringLiteral("#2a2452")) : QColor(QStringLiteral("#ece8ff")); }
    QColor accentText() const { return QColor(QStringLiteral("#ffffff")); }
    QColor green() const { return QColor(QStringLiteral("#3ddc97")); }
    QColor amber() const { return QColor(QStringLiteral("#ffc857")); }
    QColor red() const { return QColor(QStringLiteral("#ff6b81")); }
    QColor blue() const { return QColor(QStringLiteral("#4cc9f0")); }
    QColor thumb() const { return m_dark ? QColor(QStringLiteral("#0c0f17")) : QColor(QStringLiteral("#f0f2f8")); }
    int radius() const { return 14; }
    QString font() const { return QStringLiteral("Malgun Gothic"); }

    Q_INVOKABLE QString kindColor(const QString &kind) const;
    Q_INVOKABLE QString kindLabel(const QString &kind) const;

signals:
    void darkChanged();

private:
    bool m_dark = true;
};
