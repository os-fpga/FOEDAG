#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#define ENABLE_LOG_DEBUG_LEVEL

class SimpleLogger {
public:
    static SimpleLogger& instance() {
        static SimpleLogger logger;
        return logger;
    }

    ~SimpleLogger() {
        m_file.close();
    }

    void init(const QString& filePath, bool onScreen) {
        m_onScreen = onScreen;
        m_file.setFileName(filePath);
        if (!m_file.open(QIODevice::Append | QIODevice::Text)) {
            qWarning() << "Could not open log file:" << filePath;
        }
        m_isInitialied = true;
    }

    template<typename... Args>
    void error(Args... args) {
        log("ERROR: ", args...);
    }

    template<typename... Args>
    void warn(Args... args) {
        log("WARNING: ", args...);
    }

    template<typename... Args>
    void debug(Args... args) {
#ifdef ENABLE_LOG_DEBUG_LEVEL
        log(args...);
#endif
    }

    template<typename... Args>
    void log(Args... args) {
        if (!m_isInitialied) {
            qCritical() << "please run SimpleLogger::instance().init(filePath) first";
            return;
        }

        QTextStream out(&m_file);
        logInternal(out, args...); // Call the internal function
        out << "\n";
        out.flush(); // Ensure the message is written to the file
    }

private:
    SimpleLogger() {}

    bool m_isInitialied = false;
    bool m_onScreen = false;
    QFile m_file;
    QString m_fileName = "interactive_critical_path_analysis.log";

    // Internal function for logging
    template<typename T, typename... Args>
    void logInternal(QTextStream& out, T first, Args... args) {
        out << first; // Write the first argument
        if (m_onScreen) {
            qInfo() << first;
        }
        logInternal(out, args...); // Recursive call for other arguments
    }

    // Base case for recursion
    void logInternal(QTextStream& out) {}
};
