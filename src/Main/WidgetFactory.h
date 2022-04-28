// sma find where to get current copyright text from

#ifndef WIDGET_FACTORY_H
#define WIDGET_FACTORY_H

#include <QtWidgets>
#include <QJsonObject>


namespace FOEDAG {

// class WidgetFactory {
//  private:

//  public:
//   WidgetFactory();

//   static QWidget* createWidget( const QJsonObject& widgetJsonObj );
//   static QWidget* createWidget( const QString& widgetJsonStr );
// };

QWidget* createWidget( const QJsonObject& widgetJsonObj );
QWidget* createWidget( const QString& widgetJsonStr );
QComboBox* createComboBox( const QString& objectName, const QStringList& options, const QString& selectedValue = "" );
QLineEdit* createLineEdit( const QString& objectName, const QString& text = "" );


}  // namespace FOEDAG

#endif