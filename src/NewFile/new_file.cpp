#include "new_file.h"

#include <QTextStream>

using namespace FOEDAG;

NewFile::NewFile(QWidget *parent) : QWidget(parent) {
  QString filter = FILTER_VERILOG + tr(";;") + FILTER_VHDL + tr(";;") +
                   FILTER_TCL + tr(";;") + FILTER_CONSTR + tr(";;") +
                   FILTER_ALL;
  m_fileDialog = new QFileDialog(this, tr("New File"), "default", filter);
  m_fileDialog->setAcceptMode(QFileDialog::AcceptSave);
  m_fileDialog->setViewMode(QFileDialog::Detail);
}

void NewFile::StartNewFile() {
  int ret = m_fileDialog->exec();
  if (ret) {
    QString selFilter = m_fileDialog->selectedNameFilter();
    QString filter = "";
    if (selFilter == FILTER_VERILOG) {
      filter = ".v";
    } else if (selFilter == FILTER_VHDL) {
      filter = ".vhd";
    } else if (selFilter == FILTER_TCL) {
      filter = ".tcl";
    } else if (selFilter == FILTER_CONSTR) {
      filter = ".sdc";
    }

    QStringList strlist = m_fileDialog->selectedFiles();
    foreach (auto item, strlist) {
      QFileInfo fileInfo(item);
      QString suffix = "." + fileInfo.suffix();
      QString newFileName;
      if (suffix.compare(filter, Qt::CaseInsensitive)) {
        newFileName = item + filter;
      } else {
        newFileName = item;
      }

      if (!CreateFile(newFileName)) {
        emit OpenFile(newFileName);
      }
    }
  }
}

void NewFile::StopNewFile() { m_fileDialog->close(); }

int NewFile::CreateFile(QString strFileName) {
  int ret = 0;
  QFile file(strFileName);
  if (file.exists()) {
    return ret;
  }
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    return -1;
  }
  file.close();
  return ret;
}
