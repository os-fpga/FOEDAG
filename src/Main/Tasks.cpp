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

#include "Tasks.h"

#include <QDir>
#include <QFileInfo>
#include <QTextStream>

#include "Foedag.h"
#include "Settings.h"
#include "WidgetFactory.h"

using namespace FOEDAG;

#define TASKS_KEY "Tasks"

#define TASKS_DEBUG false

auto TASKS_DBG_PRINT = [](std::string printStr) {
  if (TASKS_DEBUG) {
    std::cout << printStr << std::flush;
  }
};

QDialog* FOEDAG::createTaskDialog(const QString& taskName) {
  FOEDAG::Settings* settings = GlobalSession->GetSettings();
  QDialog* dlg = nullptr;
  if (settings) {
    // Get widget parameters from json settings
    json& widgetsJson = settings->getJson()[TASKS_KEY][taskName.toStdString()];

    // Create dialog
    dlg = createSettingsDialog(widgetsJson, "Edit " + taskName + " Settings");

    QObject::connect(dlg, &QDialog::accepted, [dlg, taskName]() {
      // Find the settings widget contained by the dialog
      QRegularExpression regex(".*" + QString(SETTINGS_WIDGET_SUFFIX));
      auto settingsWidget = dlg->findChildren<QWidget*>(regex);

      // If we found a settings widget
      if (settingsWidget.count() > 0) {
        // Look up changed value json
        QString patch = settingsWidget[0]->property("userPatch").toString();
        if (!patch.isEmpty()) {
          // Create the parent json structure and add userPatch data
          json cleanJson;
          cleanJson[TASKS_KEY][taskName.toStdString()] =
              json::parse(patch.toStdString());

          // Create user settings dir
          QString userDir = getTaskUserSettingsPath();
          // A user setting dir only exists when a project has been loaded so
          // ignore saving when there isn't a project
          if (!userDir.isEmpty()) {
            QFileInfo filepath(userDir + TASKS_KEY + "_" + taskName + ".json");
            QDir dir;
            dir.mkpath(filepath.dir().path());

            // Save settings for this specific Task category
            QFile file(filepath.filePath());
            if (file.open(QFile::WriteOnly)) {
              QTextStream out(&file);
              out << QString::fromStdString(cleanJson.dump());

              TASKS_DBG_PRINT("Saving Tasks: user values saved to " +
                              filepath.filePath().toStdString() + "\n");
            }
          } else {
            TASKS_DBG_PRINT(
                "Saving Tasks: No user settings path, skipping save.\n");
          }
        }
      }
    });
  }

  return dlg;
}

QString FOEDAG::getTaskUserSettingsPath() {
  QString path;
  QString projPath =
      GlobalSession->GetCompiler()->ProjManager()->getProjectPath();
  QString projName =
      GlobalSession->GetCompiler()->ProjManager()->getProjectName();
  QString separator = QString::fromStdString(
      std::string(1, std::filesystem::path::preferred_separator));

  if (!projPath.isEmpty() && !projName.isEmpty()) {
    path = projPath + separator + projName + ".settings/";
  }

  return path;
}

void FOEDAG::handleTaskDialogRequested(const QString& category) {
  QDialog* dlg = createTaskDialog(category);
  if (dlg) {
    dlg->exec();
  }
}
