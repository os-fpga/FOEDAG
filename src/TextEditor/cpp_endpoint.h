#ifndef CPP_ENDPOINT_H
#define CPP_ENDPOINT_H

#include <QObject>
#include <QVariant>

class QVariant;

class CPPEndPoint : public QObject {
  Q_OBJECT

 public:
  CPPEndPoint(QObject* parent = nullptr, QString filePath = "");

 public:
  Q_INVOKABLE void log(QVariant s);

  // expose 'qtVersion' as a property
  // JS accesses 'qtVersion' property -> Qt calls the getQtVersion() API
  // CONSTANT -> no notify signal
  Q_PROPERTY(QVariant qtVersion READ getQtVersion CONSTANT);
  Q_INVOKABLE QVariant getQtVersion();
  Q_INVOKABLE void qtVersion(QVariant v);

  // expose filepath property to JS
  // JS accesses 'filePath' property -> Qt calls the getFilePath() API
  // Qt updates 'filePath' property -> JS receives the
  // signalToJS_FilePathChanged signal
  Q_PROPERTY(
      QVariant filePath READ getFilePath NOTIFY signalToJS_FilePathChanged);
  Q_INVOKABLE QVariant getFilePath();
  Q_INVOKABLE void filePath(QVariant v);

  // JS calls this function and passes in the content of the file to be saved.
  Q_INVOKABLE void saveFileContent(QVariant fileContent);

  // JS calls this function to notify whether the file has any unsaved changes:
  Q_INVOKABLE void fileContentModified(QVariant fileContentModified);

 signals:
  // to Monaco Text Editor JS
  void signalToJS_FilePathChanged(const QString filepath);
  void signalToJS_SaveFile();
  void signalToJS_SetHighlightSelection(int lineFrom, int lineTo);
  void signalToJS_ClearHighlightSelection();
  void signalToJS_SetHighlightWarning(int lineFrom, int lineTo);
  void signalToJS_ClearHighlightWarning();
  void signalToJS_SetHighlightError(int lineFrom, int lineTo);
  void signalToJS_ClearHighlightError();

  // to Monaco Text Editor C++
  void signalToCPP_FileModified(bool fileModified);
  void signalToCPP_SaveFileContentFromJS(QVariant fileContent);

 public:
  bool m_fileIsModified;

 private:
  QList<QVariant> m_qtVersion;
  QString m_filePath;
};

#endif  // #ifndef CPP_ENDPOINT_H
