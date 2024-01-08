#ifndef CPP_ENDPOINT_H
#define CPP_ENDPOINT_H

#include <QObject>
#include <QVariant>


class QVariant;


class CPPEndPoint: public QObject
{
    Q_OBJECT

public:
    CPPEndPoint(QObject* parent=nullptr, QString filePath="");


public:
    Q_INVOKABLE void log(QVariant s);
    Q_INVOKABLE QVariant getAppVersion();
    Q_INVOKABLE void saveFileContent(QVariant fileContent);

    // callback of C++ which JS can use to call on a hover event on some element
    Q_INVOKABLE void hoveredOnElement(QVariant elementName);

    // expose 'intValue' as a property, which will invoke getIntValue() to get the value
    Q_PROPERTY(int intValue READ getIntValue NOTIFY signalToJS_IntValueChanged);
    Q_INVOKABLE int getIntValue();

    // expose 'qtVersion' as a property, which will invoke getQtVersion() to get the value
    Q_PROPERTY(QVariant qtVersion READ getQtVersion CONSTANT);
    Q_INVOKABLE QVariant getQtVersion();

    // expose filepath property to JS
    Q_PROPERTY(QVariant filePath READ getFilePath);
    Q_INVOKABLE QVariant getFilePath();


signals:

    // to Monaco Text Editor JS
    void signalToJS_IntValueChanged(int);
    void signalToJS_UpdateFilePath(const QString filepath);
    void signalToJS_SaveFile();

    // to Monaco Text Editor C++
    void signalToCPP_SaveFileContentFromJS(QVariant fileContent);


private:
    int m_intValue = 413;
    QList<int> m_qtVersion;
    QString m_filePath;
};

#endif // #ifndef CPP_ENDPOINT_H
