#pragma once

#include "../NCriticalPathParameters.h"
#include "TcpSocket.h"

#include <QObject>

#include <map>
#include <chrono>

namespace client {

/**
 * @brief Primary communication class for managing interactions within the Interactive Path Analysis tool.
 * 
 * This central class serves as the core component responsible for communication operations 
 * within the Interactive Path Analysis tool. It encapsulates the low-level communication layer, 
 * processes received telegrams, and categorizes them based on their nature. Additionally, it 
 * handles the configuration and parameterization of requests to the server, utilizing shared 
 * data encapsulated within the NCriticalPathParametersPtr. This class acts as a bridge between 
 * the tool's interface and underlying communication functionalities, ensuring seamless interactions 
 * and data exchange.
 */
class GateIO : public QObject
{
    Q_OBJECT

/**
 * @brief Service class to measure size and time of request/reponse pair as a job unit
*/
    class CommInspector {
    public:
        void onJobStart(int jobId, int64_t size) {
            auto startPoint = std::chrono::high_resolution_clock::now();
            m_data[jobId] = std::make_pair(size, startPoint);
        }
        std::optional<std::pair<int64_t, std::int64_t>> onJobFinished(int jobId, int64_t responseSize) {
            std::optional<std::pair<int64_t, int64_t>> result;
            auto it = m_data.find(jobId);
            if (it != m_data.end()) {
                auto [requestSize, startPoint] = it->second;

                auto endPoint = std::chrono::high_resolution_clock::now();
                int64_t size = requestSize + responseSize;
                int64_t durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(endPoint - startPoint).count();

                m_data.erase(it);

                result = std::make_pair(size, durationMs);
            }
            return result;
        }

    private:
        std::map<int, std::pair<int64_t, std::chrono::high_resolution_clock::time_point>> m_data;
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

    CommInspector m_commInspector;

    void sendRequest(QByteArray&, const QString&);
    void handleResponse(const QByteArray&);
};

} // namespace client