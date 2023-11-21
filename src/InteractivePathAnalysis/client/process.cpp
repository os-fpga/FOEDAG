#include "process.h"

#include "../simplelogger.h"

#define PRINT_PROC_LOGS

Process::Process(const QString& name)
    : m_name(name)
{
    #ifdef PRINT_PROC_LOGS
        connect(this, &QProcess::readyReadStandardOutput, this, [this](){
            QByteArray output = readAllStandardOutput();
            QList<QByteArray> d = output.split('\n');
            for (const auto& e: d) {
                SimpleLogger::instance().log(QString("%1 proc:").arg(m_name), e);
            }
        });
    #endif

    m_watcherTimer.setInterval(PROCESS_WATCHER_INTERVAL_MS);
    QObject::connect(&m_watcherTimer, &QTimer::timeout, this, &Process::checkEvent);
    m_watcherTimer.start();

    m_prevState = state();
}

Process::~Process()
{
    SimpleLogger::instance().debug("~~~ ~Process() START");
    m_watcherTimer.stop();
    terminate();
    if (!waitForFinished(PROCESS_FINISH_TIMOUT_MS)) {
        kill();
        waitForFinished();
    }
    SimpleLogger::instance().debug("~~~ ~Process() END");
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

