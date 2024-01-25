/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <QMap>
#include <QObject>
#include <QSet>
#include <filesystem>

#include "RapidGptContext.h"
#include "RapigGptSettingsWindow.h"

namespace FOEDAG {

class ChatWidget;
class RapidGpt : public QObject {
  Q_OBJECT

 public:
  explicit RapidGpt(const RapidGptSettings &settings,
                    const std::filesystem::path &projectPath,
                    QObject *parent = nullptr);
  QWidget *widget();
  void setSettings(const RapidGptSettings &settings);

  void setProjectPath(const std::filesystem::path &projectPath);

 public slots:
  void fileContext(const QString &file);

 private slots:
  void sendUserText(const QString &text);
  void cleanCurrentHistory();
  void regenerateLast();
  void removeMessageAt(int index);

 private:
  void flush();
  QString GetFileContent() const;
  static QString currentDate();
  void updateChat(const RapidGptContext &context);
  void loadFromFile();
  QString buildPath(const QString &relativePath) const;
  void sendRapidGpt(const QString &text);
  RapidGptContext compileContext() const;

 private:
  ChatWidget *m_chatWidget{nullptr};
  std::filesystem::path m_path{};
  QMap<QString, RapidGptContext> m_files;
  QSet<QString> m_filesSet;
  QString m_currectFile{};
  RapidGptSettings m_settings{};
};

}  // namespace FOEDAG
