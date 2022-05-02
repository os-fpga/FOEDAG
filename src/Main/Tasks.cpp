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

#include <QDebug>
#include <QJsonArray>

#include "Settings.h"
#include "WidgetFactory.h"

using namespace FOEDAG;

void FOEDAG::getTasks(Settings* settings) {
  QJsonValue tasksVal = settings->getNested("Settings.Tasks", ".");
  if (tasksVal.isObject()) {
    qDebug() << "\nReading tasks w/ Settings::getNested(\"Settings.Tasks\"):";

    // Step through Tasks
    // Convert value to object and step through object keys
    QJsonObject tasksObj = tasksVal.toObject();
    for (const QString& taskName : tasksObj.keys()) {
      qDebug() << "\tTask: " << taskName;

      // Get task object values
      QJsonValue taskVal = tasksObj.value(taskName);
      if (taskVal.isArray()) {
        // Step through task settings
        for (QJsonValue setting : taskVal.toArray()) {
          if (setting.isObject()) {
            QJsonObject metaObj = setting.toObject();
            // Step through widget settings for current setting
            for (const QString& metaKey : metaObj.keys()) {
              qDebug() << "\t\t" << metaKey << "->" << metaObj.value(metaKey);
            }
          }
          qDebug() << "\n";
        }
      }
    }
  }
}

QWidget* FOEDAG::createTaskWidgets(Settings* settings)
{
  QWidget* widget = new QWidget();
  widget->setObjectName( "tasksWidget" );
  QVBoxLayout* VLayout = new QVBoxLayout();
  widget->setLayout( VLayout );

  // load some fake data
  QString filepath = "/usr/local/share/foedag/etc/settings/settings_test.json";
  settings->loadJsonFile(filepath, "Settings");

  QJsonValue tasksVal = settings->getNested("Settings.Tasks", ".");
  if (tasksVal.isObject()) {
    // Step through Tasks
    // Convert value to object and step through object keys
    QJsonObject tasksObj = tasksVal.toObject();
    for (const QString& taskName : tasksObj.keys()) {

      // Get task object values
      QJsonValue taskVal = tasksObj.value(taskName);
      if (taskVal.isArray()) {
        // Step through task settings
        for (QJsonValue setting : taskVal.toArray()) {
          if (setting.isObject()) {
            QJsonObject metaObj = setting.toObject();

            QWidget* subWidget = FOEDAG::createWidget( metaObj );
            VLayout->addWidget( subWidget );
          }
        }
      }
    }
  }

  return widget;
}