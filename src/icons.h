#pragma once

#include <QIcon>
#include <QPixmap>

QPixmap appPixmap(int size, bool secret = false);
QIcon appIcon(bool secret = false);
QIcon trayIcon(bool secret = false);
