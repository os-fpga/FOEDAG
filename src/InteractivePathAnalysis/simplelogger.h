#pragma once

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QTemporaryFile>
#include <QDateTime>
#include <QDebug>

#define ENABLE_LOG_DEBUG_LEVEL

class SimpleLogger {
    const qint64 FILE_BYTES_MAX = 3'000'000;

public:
    static SimpleLogger& instance() {
        static SimpleLogger logger;
        return logger;
    }

    ~SimpleLogger() {
        if (m_file.isOpen()) {
            qint64 fileSize = m_file.size();
            m_file.close();
            if (fileSize >= FILE_BYTES_MAX) {
                int dropBytesNum1 = qint64(0.8*fileSize);
                int dropBytesNum2 = qint64(0.8*FILE_BYTES_MAX);
                qint64 dropBytesNum = std::max(dropBytesNum1, dropBytesNum2);
                trimBeginningOfFile(m_filePath, dropBytesNum);
            }
        }
    }

    void init(const QString& filePath, bool onScreen) {
        m_filePath = filePath;
        m_onScreen = onScreen;
        m_file.setFileName(m_filePath);
        if (!m_file.open(QIODevice::Append | QIODevice::Text)) {
            qWarning() << "Could not open log file:" << m_filePath;
        }
        m_isInitialied = true;
        log("SimpleLogger::init");
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
            qCritical() << "please run SimpleLogger::instance().init() first";
            return;
        }

        QTextStream out(&m_file);
        out << QDateTime::currentDateTime().toString("dd/MM/yyyy-HH:mm:ss: ");
        logInternal(out, args...); // Call the internal function
        out << "\n";
        out.flush(); // Ensure the message is written to the file
    }

private:
    SimpleLogger() {}

    bool m_isInitialied = false;
    bool m_onScreen = false;
    QFile m_file;
    QString m_filePath;

    // Internal function for logging
    template<typename T, typename... Args>
    void logInternal(QTextStream& out, T first, Args... args) {
        out << first << " "; // Write the first argument
        if (m_onScreen) {
            qInfo() << first;
        }
        logInternal(out, args...); // Recursive call for other arguments
    }

    // Base case for recursion
    void logInternal(QTextStream&) {}

    bool trimBeginningOfFile(const QString& fileName, qint64 nBytesToTrim) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            qDebug() << "Failed to open file for reading:" << fileName;
            return false;
        }

        // Skip the first nBytesToTrim bytes
        if (!file.seek(nBytesToTrim)) {
            qDebug() << "Failed to seek to position" << nBytesToTrim << "in file" << fileName;
            file.close();
            return false;
        }

        // Read the rest of the file
        QByteArray data = file.readAll();
        file.close();

        // Use a temporary file to safely write back the trimmed data
        QTemporaryFile tempFile;
        if (!tempFile.open()) {
            qDebug() << "Failed to open temporary file for writing";
            return false;
        }

        // Write the data to the temporary file
        tempFile.write(data);
        tempFile.close();

        // Replace the original file with the temporary file
        if (!QFile::remove(fileName)) {
            qDebug() << "Failed to remove original file:" << fileName;
            return false;
        }

        if (!QFile::rename(tempFile.fileName(), fileName)) {
            qDebug() << "Failed to rename temporary file to original file name:" << fileName;
            return false;
        }

        return true;
    }
};
