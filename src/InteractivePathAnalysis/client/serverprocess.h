#pragma once

#include <QProcess>
#include <QTimer>

namespace client {

class ServerProcess : public QProcess
{
    Q_OBJECT
    
    const int PROCESS_WATCHER_INTERVAL_MS = 500;
    const int PROCESS_FINISH_TIMOUT_MS = 5000;
public:
    ServerProcess(const QString& name);
    ~ServerProcess();

    bool isRunning() const { return state() != QProcess::NotRunning; }
    void start(const QString& fullCmd);
    void stop();

signals:
    void restarted();
    void runStatusChanged(bool);
    void innerErrorOccurred(QString);

private:
    QString m_name;

    bool m_isFirstRun = true;
    QString m_cmd;
    QList<QString> m_args;
    QTimer m_watcherTimer;

    QProcess::ProcessState m_prevState;

    void restart();
    void checkEvent();
    void stopAndWaitProcess();
};

} // namespace client