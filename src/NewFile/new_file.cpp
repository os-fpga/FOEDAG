#include "new_file.h"

#include <QTextStream>

using namespace FOEDAG;

NewFile::NewFile(QWidget *parent) : QWidget(parent) {
  QString filter = FILTER_VERILOG + tr(";;") + FILTER_VHDL + tr(";;") +
                   FILTER_TCL + tr(";;") + FILTER_CONSTR + tr(";;") +
                   FILTER_ALL;
  m_fileDialog = new QFileDialog(this, tr("New File"), "default", filter);
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

      QFile file(newFileName);
      if (file.exists()) {
        continue;
      }
      if (!file.open(QFile::WriteOnly | QFile::Text)) {
        continue;
      }
      file.close();
    }
  }
}

void NewFile::StopNewFile() { m_fileDialog->close(); }
