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
#include <QObject>
#include <QString>
#include <ostream>

namespace FOEDAG {

class ProjectManager;
class SourcesForm;

class TclCommandIntegration : public QObject {
  Q_OBJECT
 public:
  TclCommandIntegration(ProjectManager *projManager, SourcesForm *form);
  bool TclSetTopModule(int argc, const char *argv[], std::ostream &out);
  bool TclCreateFileSet(int argc, const char *argv[], std::ostream &out);
  bool TclCreateFileSet(const QString &name, std::ostream &out);
  bool TclAddOrCreateDesignFiles(int argc, const char *argv[],
                                 std::ostream &out);
  bool TclAddOrCreateDesignFiles(const QString &files, int lang,
                                 std::ostream &out);
  bool TclAddDesignFiles(const QString &commands, const QString &libs,
                         const QString &files, int lang, std::ostream &out);
  bool TclAddSimulationFiles(const QString &commands, const QString &libs,
                             const QString &files, int lang, std::ostream &out);
  bool TclAddOrCreateConstrFiles(const QString &file, std::ostream &out);
  bool TclAddConstrFiles(const QString &file, std::ostream &out);
  bool TclSetActive(int argc, const char *argv[], std::ostream &out);
  bool TclSetAsTarget(int argc, const char *argv[], std::ostream &out);
  bool TclCreateProject(int argc, const char *argv[], std::ostream &out);
  bool TclCreateProject(const QString &name, const QString &type,
                        std::ostream &out);
  bool TclCloseProject();
  bool TclClearSimulationFiles(std::ostream &out);

  bool TclshowChatGpt(const std::string &request, const std::string &content);

  ProjectManager *GetProjectManager();
  void saveSettings();

 signals:
  void newDesign(const QString &);
  void closeDesign();
  void showChatGpt(const QString &r, const QString &data);
  void chatGptStatus(bool);
  void saveSettingsSignal();

 private:
  void createNewDesign(const QString &design, int projectType = 0);

 private:
  bool validate() const;
  void update();
  static void error(int res, const QString &filename, std::ostream &out);

 private:
  ProjectManager *m_projManager;
  SourcesForm *m_form;
};

}  // namespace FOEDAG
