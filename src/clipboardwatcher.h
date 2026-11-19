#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

#include "types.h"

class QClipboard;

class ClipboardWatcher : public QObject
{
    Q_OBJECT
public:
    explicit ClipboardWatcher(QObject *parent = nullptr);

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    void guard(const QString &hash);
    void setProgrammerMode(bool enabled);
    void setTerminalApps(const QStringList &apps);

signals:
    void captured(const CapturePayload &payload);

private slots:
    void check();

private:
    bool skip(const QString &hash);

    QClipboard *m_clipboard = nullptr;
    QTimer m_timer;
    QString m_lastHash;
    QString m_guardHash;
    bool m_enabled = true;
    bool m_programmerMode = false;
    QStringList m_terminalApps;
};
