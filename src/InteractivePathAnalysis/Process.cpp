#include "Process.h"

#include "SimpleLogger.h"

namespace FOEDAG {

Process::Process(const QString& name)
    : m_name(name)
{
    if (m_forwardProcessLog) {
        connect(this, &QProcess::readyReadStandardOutput, this, [this](){
            QByteArray output = readAllStandardOutput();
            QList<QByteArray> lines = output.split('\n');
            for (const auto& line: lines) {
               SimpleLogger::instance().log(QString("%1 proc:").arg(m_name), line);
            }
        });
    };

    connect(this, &QProcess::readyReadStandardError, this, [this](){
        QString error{QString::fromUtf8(readAllStandardError())};
        SimpleLogger::instance().error(QString("%1 proc:").arg(m_name), error);
        bool skipUIReportingError = false;
        for (const QString& bypassError: m_bypassInnerErrors) {
            if (error.contains(bypassError)) {
                skipUIReportingError = true;
                break;
            }
        }
        if (!skipUIReportingError) {
            emit innerErrorOccurred(error);
        }
    });

    m_watcherTimer.setInterval(PROCESS_WATCHER_INTERVAL_MS);
    QObject::connect(&m_watcherTimer, &QTimer::timeout, this, &Process::checkEvent);
    m_watcherTimer.start();

    m_prevState = state();
}

Process::~Process()
{
    stopAndWaitProcess();
}

void Process::stopAndWaitProcess()
{
    m_watcherTimer.stop();
    terminate();
    if (!waitForFinished(PROCESS_FINISH_TIMOUT_MS)) {
        kill();
        waitForFinished();
    }
}

void Process::start(const QString& fullCmd)
{
    QList<QString> fragments = fullCmd.split(" ");
    if (!fragments.isEmpty()) {
        m_cmd = fragments[0];

        fragments.pop_front();
        m_args = fragments;

        restart();
    }
}

void Process::stop()
{
    stopAndWaitProcess();
    m_isFirstRun = true;
    m_prevState = QProcess::ProcessState::NotRunning;
    emit runStatusChanged(false);
}

void Process::restart()
{
    setProgram(m_cmd);
    setArguments(m_args);
    SimpleLogger::instance().log("--- running", m_cmd, m_args.join(" "));
    QProcess::start();

    if (m_isFirstRun) {
        m_isFirstRun = false;
    } else {
        emit restarted();
    }

    if (!m_watcherTimer.isActive()) {
        m_watcherTimer.start();
    }
}

void Process::checkEvent()
{
    QProcess::ProcessState curState = state();
    if (m_prevState != curState) {
#ifdef USE_VPR_VIEWER_AUTO_RECOVERY
        if (curState == QProcess::NotRunning) {
            SimpleLogger::instance().log("vpr is not running, restarting");
            restart();
        }
#endif
        emit runStatusChanged(curState != QProcess::NotRunning);
        m_prevState = curState;
    }
}

} // namespace FOEDAG