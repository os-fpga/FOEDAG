#include "create_file_dialog.h"

#include <QFileDialog>
#include <QMessageBox>

#include "Compiler/CompilerDefines.h"
#include "ui_create_file_dialog.h"

using namespace FOEDAG;

constexpr auto DefaultLocation{"<Local to Project>"};

createFileDialog::createFileDialog(const QString &projectPath, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::createFileDialog),
      m_projectPath(projectPath) {
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
  ui->m_comboxFileLocation->insertItem(0, tr(DefaultLocation));
  ui->m_comboxFileLocation->insertItem(1, tr("Choose Location..."));
  ui->m_comboxFileLocation->setStyleSheet("border: 1px solid gray;");
}

createFileDialog::~createFileDialog() { delete ui; }

bool createFileDialog::verifyFileName(const QString &fileName,
                                      QWidget *parent) {
  if (fileName.contains(" ")) {
    QMessageBox::warning(
        parent, tr("Warning"),
        QString("\"%1\": file name with space not supported.").arg(fileName),
        QMessageBox::Ok);
    return false;
  }
  return true;
}

void createFileDialog::initialDialog(int type) {
  m_type = type;
  if (GT_SOURCE == m_type || GT_SIM == m_type) {
    setWindowTitle(tr("Create Source File"));
    ui->m_labelDetailed->setText(
        tr("Create a new source file and add it to your project."));

    ui->m_comboxFileType->clear();
    ui->m_comboxFileType->addItem(tr("Verilog"), Verilog);
    ui->m_comboxFileType->addItem(tr("SystemVerilog"), SystemVerilog);
    ui->m_comboxFileType->addItem(tr("VHDL"), VHDL);
    if (GT_SIM == m_type) ui->m_comboxFileType->addItem(tr("CPP"), Cpp);
  } else if (GT_CONSTRAINTS == m_type) {
    setWindowTitle(tr("Create Constraints File"));
    ui->m_labelDetailed->setText(
        tr("Create a new Constraints file and add it to your project."));
    ui->m_comboxFileType->clear();
    ui->m_comboxFileType->addItem(tr("sdc"), Sdc);
    ui->m_comboxFileType->addItem(tr("pin"), Pin);
  }
  // ui->m_comboxFileType->setStyleSheet("border: 1px solid gray;");
}

void createFileDialog::on_m_pushBtnOK_clicked() {
  auto fileName = ui->m_lineEditFileName->text();
  if (fileName.isEmpty()) {
    QMessageBox::information(this, tr("Information"),
                             tr("Please specify a file name"), QMessageBox::Ok);
    return;
  }
  if (!verifyFileName(fileName, this)) return;

  filedata fdata;
  fdata.m_isFolder = false;
  if (GT_SOURCE == m_type || GT_SIM == m_type) {
    switch (ui->m_comboxFileType->currentData().toInt()) {
      case Verilog:
        fdata.m_fileName = AppendExtension(fileName, QString(".v"));
        fdata.m_fileType = QString("v");
        break;
      case SystemVerilog:
        fdata.m_fileName = AppendExtension(fileName, QString(".sv"));
        fdata.m_fileType = QString("sv");
        break;
      case VHDL:
        fdata.m_fileName = AppendExtension(fileName, QString(".vhd"));
        fdata.m_fileType = QString("vhd");
        break;
      case Cpp:
        fdata.m_fileName = AppendExtension(fileName, QString(".cpp"));
        fdata.m_fileType = QString("cpp");
        break;
      default:
        break;
    }
    fdata.m_language = FromFileType(fdata.m_fileType);
  } else if (GT_CONSTRAINTS == m_type) {
    auto ext = ui->m_comboxFileType->currentText();
    fdata.m_fileName = AppendExtension(fileName, QString(".%1").arg(ext));
    fdata.m_fileType = ext;
  }

  fdata.m_filePath = ui->m_comboxFileLocation->currentText();

  if (FileExists(fdata)) {
    QMessageBox::information(
        this, tr("Information"),
        tr("File already exists. Please specify another file name"),
        QMessageBox::Ok);
    return;
  }

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

bool createFileDialog::FileExists(const filedata &fData) const {
  QString location{fData.m_filePath};
  if (location == DefaultLocation) {
    location = m_projectPath;
    if (location.isEmpty()) return false;  // project haven't created yet.
  }
  QFileInfo fileInfo(QDir(location), fData.m_fileName);
  return fileInfo.exists();
}

QString createFileDialog::AppendExtension(const QString &fileName,
                                          const QString &ext) {
  if (fileName.endsWith(ext, Qt::CaseInsensitive)) return fileName;
  return QString{"%1%2"}.arg(fileName, ext);
}
