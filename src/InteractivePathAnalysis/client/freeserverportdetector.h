#pragma once

#include <QTcpServer>
#include "../simplelogger.h"

class FreeServerPortDetector {
    const int PORT_NUM_MAX = 65535;
    const int PORT_NUM_START = 60555;
public:
    int detectAvailablePortNum() {
        while(isBusy(m_portNum) && (m_portNum < PORT_NUM_MAX)) {
            m_portNum++;
        }
        return m_portNum;
    }

private:
    int m_portNum = PORT_NUM_START;

    bool isBusy(int portNum) {
        SimpleLogger::instance().log("test server portNum", portNum);
        QTcpServer server;
        if (server.listen(QHostAddress::LocalHost, portNum)) {
            server.close();
            SimpleLogger::instance().log("found free server portNum", portNum);
            return false;
        } else {
            SimpleLogger::instance().log("portNum", portNum, "is busy");
            return true;
        }
    }
};
