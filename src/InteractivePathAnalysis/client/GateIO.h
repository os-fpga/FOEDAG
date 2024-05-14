/**
  * @file GateIO.h
  * @author Oleksandr Pyvovarov (APivovarov@quicklogic.com or
  aleksandr.pivovarov.84@gmail.com or
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

#include <QObject>
#include <chrono>
#include <functional>
#include <map>
#include <optional>
#include <set>

#include "../NCriticalPathParameters.h"
#include "../SimpleLogger.h"
#include "ConvertUtils.h"
#include "TcpSocket.h"

namespace FOEDAG {

namespace client {

/**
 * @brief Primary communication class for managing interactions within the
 * Interactive Path Analysis tool.
 *
 * This central class serves as the core component responsible for communication
 * operations within the Interactive Path Analysis tool. It encapsulates the
 * low-level communication layer, processes received telegrams, and categorizes
 * them based on their nature. Additionally, it handles the configuration and
 * parameterization of requests to the server, utilizing shared data
 * encapsulated within the NCriticalPathParametersPtr. This class acts as a
 * bridge between the tool's interface and underlying communication
 * functionalities, ensuring seamless interactions and data exchange.
 */
class GateIO : public QObject {
  Q_OBJECT

  const int STAT_LOG_INTERVAL_MS = 10000;

  /**
   * @brief Service class to measure size and time of request/reponse pair as a
   * job unit. Also collect statistic about success/fail finish, maximum size
   * and maximum duration.
   */
  class JobStatusStat {
    struct JobStat {
      int jobId = 0;
      int64_t requestSize = 0;
      std::chrono::high_resolution_clock::time_point startTime;
    };

   public:
    void trackRequestCreation(int jobId, int64_t requestSize) {
      auto it = m_pendingJobs.find(jobId);
      if (it == m_pendingJobs.end()) {
        m_pendingJobs[jobId] = JobStat{
            jobId, requestSize, std::chrono::high_resolution_clock::now()};
        m_requestCounter++;
      }
    }
    void trackResponseBroken() { m_brokenResponseCounter++; }

    std::optional<std::pair<int64_t, int64_t>> trackJobFinish(
        int jobId, bool status, int64_t responseSize) {
      auto it = m_pendingJobs.find(jobId);
      if (it != m_pendingJobs.end()) {
        status ? m_successTaskCounter++ : m_failedTaskCounter++;

        const JobStat& job = it->second;

        auto endPoint = std::chrono::high_resolution_clock::now();
        int64_t size = job.requestSize + responseSize;
        int64_t durationMs =
            std::chrono::duration_cast<std::chrono::milliseconds>(endPoint -
                                                                  job.startTime)
                .count();

        if (size > m_maxSize) {
          m_maxSize = size;
        }
        if (durationMs > m_maxDurationMs) {
          m_maxDurationMs = durationMs;
        }

        m_pendingJobs.erase(it);

        return std::make_pair(size, durationMs);
      }
      return std::nullopt;
    }

    int pendingJobsNum() const { return m_pendingJobs.size(); }

    void show(bool skipIfWasShown) {
      std::stringstream ss;
      ss << "*** requests[total:" << m_requestCounter << ",inprogress:[";
      for (const auto& [jobId, value] : m_pendingJobs) {
        ss << jobId << ",";
      }
      ss << "],success:[" << m_successTaskCounter << "]"
         << ",fail:" << m_failedTaskCounter
         << "], responses[broken:" << m_brokenResponseCounter << "]"
         << ", max size:" << getPrettySizeStrFromBytesNum(m_maxSize)
         << ", max duration:" << getPrettyDurationStrFromMs(m_maxDurationMs);

      std::string candidate = ss.str();

      bool toShow = true;
      if (skipIfWasShown && (m_prevShown == candidate)) {
        toShow = false;
      }

      if (toShow) {
        SimpleLogger::instance().log(candidate.c_str());
        m_prevShown = candidate;
      }
    }

   private:
    std::map<int, JobStat> m_pendingJobs;

    int m_requestCounter = 0;
    int m_brokenResponseCounter = 0;
    int m_successTaskCounter = 0;
    int m_failedTaskCounter = 0;

    int64_t m_maxSize = 0;
    int64_t m_maxDurationMs = 0;

    std::string m_prevShown;
  };

 public:
  GateIO(const NCriticalPathParametersPtr&);
  ~GateIO();

  bool isConnected() const;
  void startConnectionWatcher();
  void stopConnectionWatcher();
  void setServerIsRunning(bool flag) { m_socket.setServerIsRunning(flag); }

 public slots:
  void requestPathItemsHighLight(const QString&, const QString&);
  void requestPathList(const QString&);
  void onHightLightModeChanged();
  void onServerPortDetected(int);

 signals:
  void pathListDataReceived(const QString&);
  void highLightModeReceived();
  void connectedChanged(bool);

 private:
  NCriticalPathParametersPtr m_parameters;

  QString m_lastPathItems = comm::CRITICAL_PATH_ITEMS_SELECTION_NONE;
  TcpSocket m_socket;

  JobStatusStat m_jobStatusStat;

  QTimer m_statShowTimer;

  void sendRequest(const comm::TelegramHeader& header, const QByteArray& data,
                   const QString& initiator);
  void handleResponse(const QByteArray&, bool isCompressed);
};

}  // namespace client

}  // namespace FOEDAG
