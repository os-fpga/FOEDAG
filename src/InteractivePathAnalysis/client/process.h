#pragma once

#include <QProcess>
#include <QTimer>

class Process : public QProcess
{
    Q_OBJECT
public:
    Process(const QString& name);
    ~Process();

    void start(const QString& fullCmd);

signals:
    void restarted();
    void runStatusChanged(bool);

private:
    QString m_name;

    bool m_isFirstRun = true;
    QString m_cmd;
    QList<QString> m_args;
    QTimer m_watcherTimer;

    QProcess::ProcessState m_prevState;

    void restart();
    void checkEvent();
};

