#include "process.h"

#include <QDebug>

Process::Process()
{
    m_watcherTimer.setInterval(500);
    QObject::connect(&m_watcherTimer, &QTimer::timeout, this, &Process::checkEvent);
    m_watcherTimer.start();

    m_prevState = state();
}

Process::~Process()
{
    QProcess::close();
}

void Process::start(const QString& fullCmd)
{
    QList<QString> fragments = fullCmd.split(" ");
    m_cmd = fragments[0];

    fragments.pop_front();
    m_args = fragments;

    restart();
}

void Process::restart()
{
    setProgram(m_cmd);
    setArguments(m_args);
    qInfo() << "--- run" << m_cmd << m_args;
    QProcess::start();

    if (m_isFirstRun) {
        m_isFirstRun = false;
    } else {
        emit restarted();
    }
}

void Process::checkEvent()
{
    //qInfo() << "vpr process id" << processId();
    QProcess::ProcessState curState = state();
    if (m_prevState != curState) {
#ifdef USE_VPR_VIEWER_AUTO_RECOVERY
        if (curState == QProcess::NotRunning) {
            qInfo() << "vpr is not running, restarting";
            restart();
        }
#endif
        emit runningChanged(curState != QProcess::NotRunning);
        m_prevState = curState;
    }
}

