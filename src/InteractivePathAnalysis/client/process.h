#pragma once

#include <QProcess>
#include <QTimer>

class Process : public QProcess
{
    Q_OBJECT
public:
    Process();
    ~Process();

    void start(const QString& fullCmd);

signals:
    void restarted();
    void runningChanged(bool);

private:
    bool m_isFirstRun = true;
    QString m_cmd;
    QList<QString> m_args;
    QTimer m_watcherTimer;

    QProcess::ProcessState m_prevState;

    void restart();
    void checkEvent();
};

