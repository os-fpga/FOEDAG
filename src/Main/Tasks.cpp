#include "Tasks.h"

#include <QDebug>
#include <QJsonArray>

#include "Settings.h"

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