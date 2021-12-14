#ifndef RUNGRID_H
#define RUNGRID_H

#include <QObject>
#include <QWidget>

namespace FOEDAG {

enum RunsType { RT_SYNTH, RT_IMPLE};

typedef struct tagFileData {
  bool m_isFolder;
  QString m_fileType;
  QString m_fileName;
  QString m_filePath;
} FILEDATA;

typedef FILEDATA filedata;

class RunsGrid : public QWidget
{
    Q_OBJECT
public:
    explicit RunsGrid(QWidget *parent = nullptr);

signals:

};
}
#endif // RUNGRID_H
