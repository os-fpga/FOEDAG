#pragma once

#include <QProcess>
#include <QTimer>

#include <vector>

/**
 * @brief Provides an encapsulation for QProcess functionality.
 * 
 * This class serves as a wrapper around QProcess, offering an improved 
 * interface and additional functionalities, such as error handling and redirection. 
 * It enhances the functionality provided by QProcess, allowing for more convenient 
 * and efficient interaction with process-related functionalities, including 
 * launching and managing external processes within an application context.
 */
class Process : public QProcess
{
    Q_OBJECT
    
    const int PROCESS_WATCHER_INTERVAL_MS = 500;
    const int PROCESS_FINISH_TIMOUT_MS = 5000;
public:
    Process(const QString& name);
    ~Process();

    void addInnerErrorToBypass(const QString& innerError) { m_bypassInnerErrors.push_back(innerError); } 

    bool isRunning() const { return state() != QProcess::NotRunning; }
    void start(const QString& fullCmd);
    void stop();

signals:
    void restarted();
    void runStatusChanged(bool);
    void innerErrorOccurred(QString);

private:
    QString m_name;
    bool m_forwardProcessLog = true;

    bool m_isFirstRun = true;
    QString m_cmd;
    QList<QString> m_args;
    QTimer m_watcherTimer;

    QProcess::ProcessState m_prevState;

    std::vector<QString> m_bypassInnerErrors;

    void restart();
    void checkEvent();
    void stopAndWaitProcess();
};

