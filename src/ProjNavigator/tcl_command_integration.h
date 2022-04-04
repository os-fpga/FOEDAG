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
#include <QString>
#include <ostream>

namespace FOEDAG {

class ProjectManager;
class SourcesForm;
class TclCommandIntegration {
 public:
  TclCommandIntegration(ProjectManager *projManager, SourcesForm *form);
  bool TclSetTopModule(int argc, const char *argv[], std::ostream &out);
  bool TclCreateFileSet(int argc, const char *argv[], std::ostream &out);
  bool TclAddOrCreateFiles(int argc, const char *argv[], std::ostream &out);
  bool TclSetActive(int argc, const char *argv[], std::ostream &out);
  bool TclSetAsTarget(int argc, const char *argv[], std::ostream &out);

 private:
  bool validate() const;
  void TclHelper(std::ostream &out);
  bool TclCheckType(QString strType);

 private:
  ProjectManager *m_projManager;
  SourcesForm *m_form;
};

}  // namespace FOEDAG
