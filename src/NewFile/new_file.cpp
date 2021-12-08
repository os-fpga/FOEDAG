#include "new_file.h"

#include <QTextStream>

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
    QStringList strlist = m_fileDialog->selectedFiles();
    foreach (auto item, strlist) {}
  }
}

void NewFile::StopNewFile() { m_fileDialog->close(); }
