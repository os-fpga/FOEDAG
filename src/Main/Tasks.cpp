#include "Tasks.h"

#include <QDebug>
#include <QJsonArray>

#include "Settings.h"
#include "WidgetFactory.h"

using namespace FOEDAG;

Tasks* Tasks::m_instance = nullptr;
Tasks::Tasks() { fprintf(stderr, "Tasks Created\n"); }

Tasks* Tasks::getInstance() {
  if (!m_instance) {
    m_instance = new Tasks();
  }
  return m_instance;
}

void Tasks::getTasks() {
  QJsonValue tasksVal = Settings::getNested("Settings.Tasks", ".");
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

QWidget* FOEDAG::createTaskWidgets()
{
  // sma add object names etc when converting this to a real function
  QWidget* widget = new QWidget();
  QHBoxLayout* HLayout = new QHBoxLayout();
  widget->setLayout( HLayout );

  // load some fake data
  QString filepath = "/usr/local/share/foedag/etc/settings/settings_test.json";
  FOEDAG::Settings::loadJsonFile(filepath, "Settings");

  QJsonValue tasksVal = Settings::getNested("Settings.Tasks", ".");
  if (tasksVal.isObject()) {
    // qDebug() << "\nReading tasks w/ Settings::getNested(\"Settings.Tasks\"):";

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

            QWidget* subWidget = FOEDAG::createWidget( metaObj );
            HLayout->addWidget( subWidget );
            // // Step through widget settings for current setting
            // for (const QString& metaKey : metaObj.keys()) {
            //   qDebug() << "\t\t" << metaKey << "->" << metaObj.value(metaKey);
            // }
          }
          // qDebug() << "\n";
        }
      }
    }
  }

  return widget;
}