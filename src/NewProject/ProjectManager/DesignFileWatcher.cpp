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
#include "DesignFileWatcher.h"

#include "Compiler/Compiler.h"
#include "MainWindow/Session.h"
#include "project_manager.h"

extern FOEDAG::Session* GlobalSession;

namespace FOEDAG {

// ProjectMananager isn't a singleton class so storing a file watcher within it
// was problematic as the main_window had trouble connecting to all the ProjMan
// ptrs. As an alternative, we'll create a singleton for the filewatcher itself
DesignFileWatcher* DesignFileWatcher::Instance() {
  static DesignFileWatcher watcher{};
  return &watcher;
}
//  return designFileWatcher(); }

void DesignFileWatcher::init() {
  m_fileWatcher = new QFileSystemWatcher{};
  QObject::connect(m_fileWatcher, &QFileSystemWatcher::fileChanged, this,
                   &DesignFileWatcher::designFilesChanged);
}

void DesignFileWatcher::emitDesignCreated() { emit designCreated(); }

void DesignFileWatcher::setFiles(const QStringList& filePaths) {
  if (!isValid()) return;
  // Do nothing if new file list is the same
  if (filePaths != m_watchFiles) {
    // QT prints to terminal if you call removePaths on an empty filewatcher
    // so we have to check for empty first
    if (!m_fileWatcher->files().empty()) {
      // Remove old watchers
      m_fileWatcher->removePaths(m_fileWatcher->files());
    }

    // Add each file to the watcher
    // Note that some systems potentially have a max file limit, see qt docs
    // for details https://doc.qt.io/qt-5/qfilesystemwatcher.html#details
    for (auto file : filePaths) {
      // Convert paths incase they have PROJECT_OSRCDIR relative paths
      file.replace(PROJECT_OSRCDIR, Project::Instance()->projectPath());
      // Add file to watcher
      m_fileWatcher->addPath(file);
    }
    m_watchFiles = filePaths;

    // Consider any change to the design file list (filePaths) a design change
    emit designFilesChanged();
  }
}

void DesignFileWatcher::setDevice(const std::string& device) {
  if (m_device != device) {
    m_device = device;
    emit designFilesChanged();
  }
}

void DesignFileWatcher::updateDesignFileWatchers(ProjectManager* pManager) {
  if (!isValid()) return;
  QStringList files;

  // Watch Design Files
  files += pManager->getDesignFiles();
  // Watch Sim Files
  files += pManager->getSimulationFiles(pManager->getSimulationActiveFileSet());
  // Watch Constraint Files
  for (const auto& file : pManager->getConstrFiles()) {
    files += QString::fromStdString(file);
  }

  // Watch IP Files
  Compiler* compiler{};
  IPGenerator* ipGen{};
  // QSystemFileWatcher works on directories, but not recursively and QT
  // has intenionally ignored a bug in their directory file watching
  // that doesn't alert when an immediate child file changes.
  // https://bugreports.qt.io/browse/QTBUG-24693
  // As such we have to add a filewatcher to each ip src file and the cache json
  if (GlobalSession && (compiler = GlobalSession->GetCompiler()) &&
      (ipGen = compiler->GetIPGenerator())) {
    // Step through our IP Instance
    for (auto instance : ipGen->IPInstances()) {
      auto ipPaths = ipGen->GetDesignAndCacheFiles(instance);
      // Loop through each design file of the IP
      for (const auto& file : ipPaths) {
        // Store file
        files += QString::fromStdString(file.string());
      }
    }
  }

  setFiles(files);
  setDevice(pManager->getTargetDevice());
}

bool DesignFileWatcher::isValid() const { return m_fileWatcher != nullptr; }

}  // namespace FOEDAG
