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
QSpinBox* createSpinBox( const QString& objectName, int minVal, int maxVal, int stepVal, int defaultVal = -1 );
QRadioButton* createRadioButton( const QString& objectName, const QString& text );
QButtonGroup* createRadioButtons( const QString& objectName, const QStringList& nameList, const QString& selectedValue = "" );
QCheckBox* createCheckBox( const QString& objectName, const QString& text, Qt::CheckState checked );
// QCheckBox* createCheckBox( const QString& objectName, const QString& text, bool checked );

}  // namespace FOEDAG

#endif