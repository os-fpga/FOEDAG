#include "WidgetFactory.h"

#include <QJsonDocument>

using namespace FOEDAG;

QWidget* FOEDAG::createWidget(const QJsonObject& widgetJsonObj) {
  QWidget* retVal = nullptr;

  if (widgetJsonObj.contains("widgetType")) {

  }

  return retVal;
}


QWidget* FOEDAG::createWidget(const QString& widgetJsonStr) {
  QJsonDocument jsonDoc = QJsonDocument::fromJson(widgetJsonStr.toUtf8());
  QJsonObject jsonObj = jsonDoc.object();

  return FOEDAG::createWidget(jsonObj);
}

QComboBox* FOEDAG::createComboBox( const QString& objectName, const QStringList& options, const QString& selectedValue )
{
    QComboBox* widget = new QComboBox();
    widget->setObjectName( objectName );
    widget->insertItems( 0, options );

    // Select a default is selectedValue was set
    int idx = options.indexOf( selectedValue );
    if( idx > -1 )
    {
        widget->setCurrentIndex(idx);
    }

    return widget;
}

QLineEdit* FOEDAG::createLineEdit( const QString& objectName, const QString& text )
{
    QLineEdit* widget = new QLineEdit();
    widget->setObjectName( objectName );

    widget->setText(text);

    return widget;
}