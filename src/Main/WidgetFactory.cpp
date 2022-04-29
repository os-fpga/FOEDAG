#include "WidgetFactory.h"

#include <QJsonDocument>
#include <QDebug> // sma remove

using namespace FOEDAG;

QWidget* FOEDAG::createWidget(const QJsonObject& widgetJsonObj) {
  QWidget* retVal = nullptr;

  // QHash<QString,int>

  if (widgetJsonObj.contains("widgetType")) {
    QString type = widgetJsonObj.value("widgetType").toString();
    type = type.toLower();

    if( type == "input" )
    {
      qDebug() << "input requested" << widgetJsonObj.value("label") << " " << widgetJsonObj.value("default");
    }

    // fprintf( stderr, "widgetType: %s\n", qPrintable(widgetJsonObj.value("widgetType")));
  }

  return retVal;
}

QWidget* FOEDAG::createWidget(const QString& widgetJsonStr) {
  QJsonDocument jsonDoc = QJsonDocument::fromJson(widgetJsonStr.toUtf8());
  QJsonObject jsonObj = jsonDoc.object();

  return FOEDAG::createWidget(jsonObj);
}

QComboBox* FOEDAG::createComboBox(const QString& objectName,
                                  const QStringList& options,
                                  const QString& selectedValue) {
  QComboBox* widget = new QComboBox();
  widget->setObjectName(objectName);
  widget->insertItems(0, options);

  // Select a default is selectedValue was set
  int idx = options.indexOf(selectedValue);
  if (idx > -1) {
    widget->setCurrentIndex(idx);
  }

  return widget;
}

QLineEdit* FOEDAG::createLineEdit(const QString& objectName,
                                  const QString& text) {
  QLineEdit* widget = new QLineEdit();
  widget->setObjectName(objectName);

  widget->setText(text);

  return widget;
}

QSpinBox* FOEDAG::createSpinBox(const QString& objectName, int minVal,
                                int maxVal, int stepVal, int defaultVal) {
  QSpinBox* widget = new QSpinBox();
  widget->setObjectName(objectName);

  widget->setMinimum(minVal);
  widget->setMaximum(maxVal);
  widget->setSingleStep(stepVal);
  if (defaultVal != -1) {
    widget->setValue(defaultVal);
  }

  return widget;
}

QRadioButton* FOEDAG::createRadioButton(const QString& objectName,
                                     const QString& text) {
  QRadioButton* widget = new QRadioButton();
  widget->setObjectName(objectName);

  widget->setText( text );

  return widget;
}

QButtonGroup* FOEDAG::createRadioButtons(const QString& objectName,
                                     const QStringList& nameList, const QString& selectedValue) {
  QButtonGroup* widget = new QButtonGroup();
  widget->setObjectName(objectName);

  for( const QString& name : nameList )
  {
    QRadioButton* radioBtn = new QRadioButton();
    radioBtn->setObjectName( objectName + "_" + name );
    radioBtn->setText( name );
    widget->addButton(radioBtn);
  }

  return widget;
}

QCheckBox* FOEDAG::createCheckBox(const QString& objectName,
                                     const QString& text, Qt::CheckState checked) {
  QCheckBox* widget = new QCheckBox();
  widget->setObjectName(objectName);

  widget->setText( text );
  widget->setCheckState(checked);

  return widget;
}

// QCheckBox* FOEDAG::createCheckBox(const QString& objectName,
//                                      const QString& text, bool checked) {
//   return createCheckBox( objectName, text, checked );
// }
