/**
  * @file SimpleLogger.h
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

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QTemporaryFile>
#include <QDateTime>
#include <QDebug>

#define ENABLE_LOG_DEBUG_LEVEL

namespace FOEDAG {

/**
 * @brief Basic logger for recording information to either a separate file or the screen.
 * 
 * This logger supports logging data to a designated file or displaying it on the screen. 
 * Additionally, it includes a feature to truncate log files if they exceed a specified size limit.
 */
class SimpleLogger {
    const qint64 FILE_BYTES_MAX = 10'000'000;

public:
    static SimpleLogger& instance() {
        static SimpleLogger logger;
        return logger;
    }

    ~SimpleLogger() {
        setEnabled(false);
    }

    void setEnabled(bool isEnabled) {
        if (m_isEnabled != isEnabled) {
            if (isEnabled) {
                if (!m_file.isOpen()) {
                    if (!m_file.open(QIODevice::Append | QIODevice::Text)) {
                        qWarning() << "Could not open log file:" << m_filePath;
                    }
                }
            } else {
                if (m_file.isOpen()) {
                    m_file.close();
                }
            }
            m_isEnabled = isEnabled;
        }
    }

    void setOnScreenEnabled(bool onScreen) { m_onScreen = onScreen; }
    void setFilePath(const QString& filePath) {
        m_filePath = filePath;
        if (m_file.isOpen()) {
            m_file.close();
        }
        m_file.setFileName(m_filePath);
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
        if (!m_isEnabled) {
            return;
        }

        /// construct message
        QString source;
        QTextStream out_str(&source);
        out_str << QDateTime::currentDateTime().toString("dd/MM/yyyy-HH:mm:ss: ");
        logInternal(out_str, args...); // Call the internal function
        ///

        if (!m_file.isOpen()) {
            if (!m_file.open(QIODevice::Append | QIODevice::Text)) {
                qWarning() << "Could not open log file:" << m_filePath;
            }
        }

        if (m_file.isOpen()) {
            qint64 fileSize = m_file.size();
            if (fileSize >= FILE_BYTES_MAX) {
                /// close file
                m_file.close();
                ///

                int dropBytesNum = qint64(0.5*FILE_BYTES_MAX);
                trimBeginningOfFile(m_filePath, dropBytesNum);

                /// open file
                if (!m_file.open(QIODevice::Append | QIODevice::Text)) {
                    qWarning() << "Could not open log file:" << m_filePath;
                }
                ///
            }

            /// write message to file
            QTextStream out_file(&m_file);
            out_file << *out_str.string() << "\n";
            out_file.flush(); // Ensure the message is written to the file
            ///
        } else {
            qWarning() << "Could not write" << *out_str.string() << "to log file, file is not opened" << m_filePath;
        }

        if (m_onScreen) {
            qInfo() << *out_str.string();
        }
    }

private:
    SimpleLogger() {}

    bool m_isEnabled = false;
    bool m_isInitialied = false;
    bool m_onScreen = false;
    QFile m_file;
    QString m_filePath;

    // Internal function for logging
    template<typename T, typename... Args>
    void logInternal(QTextStream& out, T first, Args... args) {
        out << first << " "; // Write the first argument
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

} // namespace FOEDAG