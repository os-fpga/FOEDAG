/**
  * @file Process.h
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or aleksandr.pivovarov.84@gmail.com or
  * https://github.com/w0lek)
  * @date 2024-03-12
  * @copyright Copyright 2021 The Foedag team

  * GPL License

  * Copyright (c) 2021 The Open-Source FPGA Foundation

  * This program is free software: you can redistribute it and/or modify
  * it under the terms of the GNU General Public License as published by
  * the Free Software Foundation, either version 3 of the License, or
  * (at your option) any later version.

  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU General Public License for more details.

  * You should have received a copy of the GNU General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <QProcess>
#include <QTimer>

#include <vector>

namespace FOEDAG {

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

} // namespace FOEDAG