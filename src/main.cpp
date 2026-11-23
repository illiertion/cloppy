#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QMessageBox>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>
#include <qqml.h>

#include "appcontroller.h"
#include "clipmodel.h"
#include "clipstore.h"
#include "fileiconprovider.h"
#include "icons.h"
#include "theme.h"
#include "translate.h"

static QString g_logPath;

static void cloppyMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &message)
{
    const char *level = "info";
    switch (type) {
    case QtDebugMsg: level = "debug"; break;
    case QtInfoMsg: level = "info"; break;
    case QtWarningMsg: level = "warning"; break;
    case QtCriticalMsg: level = "critical"; break;
    case QtFatalMsg: level = "fatal"; break;
    }
    QFile file(g_logPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << QDateTime::currentDateTime().toString(Qt::ISODate) << " [" << level << "] "
               << message << '\n';
    }
    if (type == QtCriticalMsg || type == QtFatalMsg)
        fprintf(stderr, "[%s] %s\n", level, message.toUtf8().constData());
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QQuickStyle::setStyle(QStringLiteral("Basic"));
    Tr::setLanguage(QSettings().value(QStringLiteral("language"), QStringLiteral("en")).toString());

    app.setOrganizationName(QStringLiteral("Cloppy"));
    app.setOrganizationDomain(QStringLiteral("cloppy.local"));
    app.setApplicationName(QStringLiteral("Cloppy"));
    app.setApplicationDisplayName(QStringLiteral("Cloppy"));
#ifdef CLOPPY_VERSION
    app.setApplicationVersion(QString::fromLatin1(CLOPPY_VERSION));
#endif
    app.setQuitOnLastWindowClosed(false);
    app.setWindowIcon(appIcon());

    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    g_logPath = QDir(dataDir).filePath(QStringLiteral("cloppy.log"));
    qInstallMessageHandler(cloppyMessageHandler);

    QLockFile lock(QDir(dataDir).filePath(QStringLiteral("cloppy.lock")));
    if (!lock.tryLock(100)) {
        QMessageBox::information(nullptr, QStringLiteral("Cloppy"),
                                 Tr::text(QStringLiteral("Cloppy가 이미 실행 중입니다.\n트레이 아이콘을 확인해 주세요."), QStringLiteral("Cloppy is already running.\nCheck the tray icon.")));
        return 0;
    }

    ClipStore store;
    if (!store.isOpen()) {
        QMessageBox::critical(nullptr, QStringLiteral("Cloppy"),
                              Tr::text(QStringLiteral("SQLite 데이터베이스를 열 수 없습니다.\n%1"), QStringLiteral("Could not open the SQLite database.\n%1"))
                                  .arg(store.databasePath()));
        return 1;
    }

    AppController controller(&store);

    Theme theme;
    qmlRegisterSingletonInstance("Cloppy", 1, 0, "Theme", &theme);

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("fileicon"), new FileIconProvider);
    engine.rootContext()->setContextProperty(QStringLiteral("app"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("popupModel"),
                                             static_cast<QObject *>(controller.popupModel()));
    engine.rootContext()->setContextProperty(QStringLiteral("mainModel"),
                                             static_cast<QObject *>(controller.mainModel()));
    engine.loadFromModule(QStringLiteral("Cloppy"), QStringLiteral("App"));
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Cloppy: QML root object failed to load";
        return 1;
    }
    qInfo() << "Cloppy started";

    return app.exec();
}
