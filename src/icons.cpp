#include "icons.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

QPixmap appPixmap(int size, bool secret)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient gradient(0, 0, size, size);
    if (secret) {
        gradient.setColorAt(0, QColor(QStringLiteral("#ff6b81")));
        gradient.setColorAt(1, QColor(QStringLiteral("#a31d3f")));
    } else {
        gradient.setColorAt(0, QColor(QStringLiteral("#8b6dff")));
        gradient.setColorAt(1, QColor(QStringLiteral("#5436e0")));
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawRoundedRect(QRectF(0, 0, size, size), size * 0.24, size * 0.24);

    painter.setBrush(QColor(Qt::white));
    painter.drawRoundedRect(QRectF(size * 0.22, size * 0.16, size * 0.56, size * 0.72),
                            size * 0.08, size * 0.08);
    painter.drawRoundedRect(QRectF(size * 0.36, size * 0.10, size * 0.28, size * 0.14),
                            size * 0.05, size * 0.05);
    painter.setBrush(secret ? QColor(QStringLiteral("#a31d3f")) : QColor(QStringLiteral("#5436e0")));
    painter.drawRoundedRect(QRectF(size * 0.34, size * 0.42, size * 0.32, size * 0.045), 2, 2);
    painter.drawRoundedRect(QRectF(size * 0.34, size * 0.53, size * 0.32, size * 0.045), 2, 2);
    painter.drawRoundedRect(QRectF(size * 0.34, size * 0.64, size * 0.20, size * 0.045), 2, 2);
    painter.end();
    return pixmap;
}

QIcon appIcon(bool secret)
{
    if (!secret) {
        QIcon resourceIcon(QStringLiteral(":/assets/cloppy.ico"));
        if (!resourceIcon.isNull())
            return resourceIcon;
    }
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256})
        icon.addPixmap(appPixmap(size, secret));
    return icon;
}

QIcon trayIcon(bool secret)
{
    return appIcon(secret);
}
