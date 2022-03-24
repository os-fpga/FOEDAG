#include "create_file_dialog.h"

#include <QFileDialog>
#include <QMessageBox>

#include "ui_create_file_dialog.h"

using namespace FOEDAG;

createFileDialog::createFileDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::createFileDialog) {
  ui->setupUi(this);
  // m_type = type;
  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
  QSize size = this->parentWidget()->size();
  this->resize(size.width() * 9 / 16, size.height() * 7 / 8);

  ui->m_labelFileType->setText(tr("File type:"));
  ui->m_labelFileName->setText(tr("File name:"));
  ui->m_labelFileLocation->setText(tr("File location:"));
  ui->m_pushBtnOK->setText(tr("OK"));
  ui->m_pushBtnCancel->setText(tr("Cancel"));
  ui->m_comboxFileLocation->clear();
  ui->m_comboxFileLocation->insertItem(0, tr("<Local to Project>"));
  ui->m_comboxFileLocation->insertItem(1, tr("Choose Location..."));
  ui->m_comboxFileLocation->setStyleSheet("border: 1px solid gray;");
}

createFileDialog::~createFileDialog() { delete ui; }

void createFileDialog::initialDialog(int type) {
  m_type = type;
  if (GT_SOURCE == m_type) {
    setWindowTitle(tr("Create Source File"));
    ui->m_labelDetailed->setText(
        tr("Create a new source file and add it to your project."));

    ui->m_comboxFileType->clear();
    ui->m_comboxFileType->addItem(tr("Verilog"));
    ui->m_comboxFileType->addItem(tr("SystemVerilog"));
    ui->m_comboxFileType->addItem(tr("VHDL"));
  } else if (GT_CONSTRAINTS == m_type) {
    setWindowTitle(tr("Create Constraints File"));
    ui->m_labelDetailed->setText(
        tr("Create a new Constraints file and add it to your project."));
    ui->m_comboxFileType->clear();
    ui->m_comboxFileType->addItem(tr("SDC"));
  }
  // ui->m_comboxFileType->setStyleSheet("border: 1px solid gray;");
}

void createFileDialog::on_m_pushBtnOK_clicked() {
  if ("" == ui->m_lineEditFileName->text()) {
    QMessageBox::information(this, tr("Information"),
                             tr("Please specify a file name"), QMessageBox::Ok);
    return;
  }
  filedata fdata;
  fdata.m_isFolder = false;
  if (GT_SOURCE == m_type) {
    switch (ui->m_comboxFileType->currentIndex()) {
      case 0:
        fdata.m_fileName = ui->m_lineEditFileName->text() + QString(".v");
        fdata.m_fileType = QString("v");
        break;
      case 1:
        fdata.m_fileName = ui->m_lineEditFileName->text() + QString(".sv");
        fdata.m_fileType = QString("sv");
        break;
      case 2:
        fdata.m_fileName = ui->m_lineEditFileName->text() + QString(".vhd");
        fdata.m_fileType = QString("vhd");
        break;
      default:
        break;
    }
  } else if (GT_CONSTRAINTS == m_type) {
    switch (ui->m_comboxFileType->currentIndex()) {
      case 0:
        fdata.m_fileName = ui->m_lineEditFileName->text() + QString(".SDC");
        fdata.m_fileType = QString("SDC");
        break;
      case 1:
        break;
      default:
        break;
    }
  }

  fdata.m_filePath = ui->m_comboxFileLocation->currentText();
  emit sig_updateGrid(fdata);

  this->close();
}

void createFileDialog::on_m_pushBtnCancel_clicked() { this->close(); }

void createFileDialog::on_m_comboxFileLocation_currentIndexChanged(int index) {
  if (1 == index) {
    QString pathName = QFileDialog::getExistingDirectory(
        this, tr("Select Directory"), "",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if ("" == pathName) {
      ui->m_comboxFileLocation->setCurrentIndex(index - 1);
      return;
    }
    // pathName.replace("/", "\\");
    // QAbstractItemView* itemview = new QAbstractItemView(this);
    ui->m_comboxFileLocation->removeItem(2);
    ui->m_comboxFileLocation->insertItem(2, pathName);
    ui->m_comboxFileLocation->setCurrentText(pathName);
  }
}
