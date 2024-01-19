#include "new_file.h"

#include <QTextStream>

using namespace FOEDAG;

NewFile::NewFile(QWidget *parent) : QWidget(parent) {
  QString filter = FILTER_VERILOG + tr(";;") + FILTER_VHDL + tr(";;") +
                   FILTER_TCL + tr(";;") + FILTER_CONSTR + tr(";;") +
                   FILTER_ALL;
  m_fileDialog = new NewFileDialog(this, tr("New File"), "default", filter);
  m_fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  m_fileDialog->setViewMode(QFileDialog::Detail);
  connect(m_fileDialog, SIGNAL(finished(int)), this, SLOT(SlotFinished(int)));
  connect(m_fileDialog, SIGNAL(fileSelected(QString)), this,
          SLOT(SlotFileSelected(QString)));
  connect(m_fileDialog, SIGNAL(filterSelected(QString)), this,
          SLOT(SlotFilterSelected(QString)));
  m_filter = FILTER_VERILOG;
}

void NewFile::StartNewFile() { m_fileDialog->show(); }

void NewFile::StopNewFile() { m_fileDialog->hide(); }

void NewFile::TclNewFile(const QString &strFileName) {
  QTextStream out(stdout);
  if (strFileName.contains("/")) {
    int ret = CreateFile(strFileName);
    if (ret) {
      out << "New file failed! Please check whether the file path is "
             "correct.\n";
      return;
    }
    out << "New file created successfully! " << strFileName << "\n";
  } else {
    QString newFileName = QDir::homePath() + "/" + strFileName;
    int ret = CreateFile(newFileName);
    if (ret) {
      out << "New file failed! You may not have permission to write.\n";
      return;
    }
    out << "New file created successfully! " << newFileName << "\n";
  }
}

void NewFile::SlotFileSelected(const QString &file) { m_file = file; }

void NewFile::SlotFilterSelected(const QString &filter) { m_filter = filter; }

void NewFile::SlotFinished(int result) {
  if (result) {
    QString filter = "";
    if (m_filter == FILTER_VERILOG) {
      filter = ".v";
    } else if (m_filter == FILTER_VHDL) {
      filter = ".vhd";
    } else if (m_filter == FILTER_TCL) {
      filter = ".tcl";
    } else if (m_filter == FILTER_CONSTR) {
      filter = ".sdc";
    }

    QFileInfo fileInfo(m_file);
    QString suffix = "." + fileInfo.suffix();
    QString newFileName;
    if (suffix.compare(filter, Qt::CaseInsensitive)) {
      newFileName = m_file + filter;
    } else {
      newFileName = m_file;
    }

    if (!CreateFile(newFileName)) {
      emit OpenFile(newFileName);
    }
  }
}

int NewFile::CreateFile(QString strFileName) {
  int ret = 0;
  QFile file(strFileName);
  if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate)) {
    return -1;
  }
  file.close();
  return ret;
}
