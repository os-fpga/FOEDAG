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
#include <filesystem>
#include <ostream>

namespace FOEDAG {

class ProjectManager;
class SourcesForm;
class IPGenerator;

class TclCommandIntegration : public QObject {
  Q_OBJECT
 public:
  TclCommandIntegration(ProjectManager *projManager, SourcesForm *form);
  bool TclSetTopModule(int argc, const char *argv[], std::ostream &out);
  bool TclCreateFileSet(int argc, const char *argv[], std::ostream &out);
  bool TclCreateFileSet(const std::string &name, std::ostream &out);
  bool TclAddOrCreateDesignFiles(int argc, const char *argv[],
                                 std::ostream &out);
  bool TclAddDesignFiles(const std::string &commands, const std::string &libs,
                         const std::vector<std::string> &files, int lang,
                         std::ostream &out);
  bool TclAddSimulationFiles(const std::string &commands,
                             const std::string &libs,
                             const std::vector<std::string> &files, int lang,
                             std::ostream &out);
  bool TclVerifySynthPorts(std::ostream &out);
  bool TclAddConstrFiles(const std::string &file, std::ostream &out);
  bool TclSetActive(int argc, const char *argv[], std::ostream &out);
  bool TclSetAsTarget(int argc, const char *argv[], std::ostream &out);
  bool TclCreateProject(int argc, const char *argv[], std::ostream &out);
  bool TclCreateProject(const std::string &name, const std::string &type,
                        bool cleanup, std::ostream &out);
  bool TclCloseProject();
  bool TclClearSimulationFiles(std::ostream &out);

  bool TclSetTopTestBench(int argc, const char *argv[], std::ostream &out);
  bool TclAddIpToDesign(const std::string &ipName, std::ostream &out);

  ProjectManager *GetProjectManager();
  void saveSettings();
  static std::vector<std::string> GetClockList(
      const std::filesystem::path &path, bool &vhdl, bool only_inputs);
  void updateHierarchyView();
  void updateReportsView();

  void setIPGenerator(IPGenerator *gen);

 signals:
  void newDesign(const QString &);
  void closeDesign();
  void saveSettingsSignal();
  void updateHierarchy();
  void updateReports();

 private:
  void createNewDesign(const QString &design, int projectType = 0);
  static bool isVHDL(const std::string &str);

 private:
  bool validate() const;
  void update();
  static void error(int res, const QString &filename, std::ostream &out);

 private:
  ProjectManager *m_projManager;
  SourcesForm *m_form;
  IPGenerator *m_IPGenerator{nullptr};
};

}  // namespace FOEDAG
