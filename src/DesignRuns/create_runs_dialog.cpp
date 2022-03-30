#include "create_runs_dialog.h"

#include <QDesktopWidget>

#include "runs_summary_form.h"
#include "select_design_type_form.h"
#include "ui_create_runs_dialog.h"

using namespace FOEDAG;

CreateRunsDialog::CreateRunsDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::CreateRunsDialog) {
  ui->setupUi(this);
  setWindowTitle(tr("Create New Design Runs"));
  setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
  m_formIndex = CRFI_SELECTTYPE;

  // One thirds of desktop size
  QDesktopWidget dw;
  int w = dw.width() / 4;
  int h = dw.height() / 3;
  setGeometry(w, h, w, h);

  m_selectTypeForm = new SelectDesignTypeForm(this);
  ui->m_stackedWidget->insertWidget(1, m_selectTypeForm);
  m_createSynthRun = new CreateRunsForm(this);
  m_createSynthRun->InitForm(RT_SYNTH);
  ui->m_stackedWidget->insertWidget(2, m_createSynthRun);
  m_createImpleRun = new CreateRunsForm(this);
  m_createImpleRun->InitForm(RT_IMPLE);
  ui->m_stackedWidget->insertWidget(3, m_createImpleRun);
  m_runsSummaryForm = new RunsSummaryForm(this);
  ui->m_stackedWidget->insertWidget(4, m_runsSummaryForm);
  ui->m_stackedWidget->adjustSize();

  m_pm = new ProjectManager(this);

  UpdateDialogView();
}

CreateRunsDialog::~CreateRunsDialog() { delete ui; }

void FOEDAG::CreateRunsDialog::on_m_btnOK_clicked() {
  int itype = m_selectTypeForm->getDesignType();
  if (m_formIndex == CRFI_RUNSUMARY) {
    int ret = 0;
    if (DRT_SYNTH == itype) {
      ret = CreateSynthRuns();
    } else if (DRT_IMPLE == itype) {
      ret = CreateImpleRuns();
    } else {
      ret = CreateSynthRuns();
      ret = CreateImpleRuns();
    }
    if (0 == ret) {
      m_pm->FinishedProject();
      emit RefreshRuns();
    }
    this->hide();
  } else if (m_formIndex == CRFI_SELECTTYPE) {
    if (DRT_IMPLE == itype) {
      m_formIndex++;
    }
    static int type = DRT_SYNTH;
    if (type != itype) {
      m_createSynthRun->ClearData();
      m_createImpleRun->ClearData();
      type = itype;
    }
  } else if (m_formIndex == CRFI_SYNTHRUN) {
    if (DRT_SYNTH == itype) {
      m_formIndex++;
      int iSynthCount = m_createSynthRun->getRunDataSize();
      m_runsSummaryForm->setRunsCount(iSynthCount, -1);
    } else {
      QList<rundata> listRun = m_createSynthRun->getRunDataList();
      QStringList listNewSynth;
      foreach (auto rd, listRun) { listNewSynth.append(rd.m_runName); }
      m_createImpleRun->setNewSynth(listNewSynth);
    }
  } else if (m_formIndex == CRFI_IMPLERUN) {
    if (DRT_IMPLE == itype) {
      int iImpleCount = m_createImpleRun->getRunDataSize();
      m_runsSummaryForm->setRunsCount(-1, iImpleCount);
    } else {
      int iSynthCount = m_createSynthRun->getRunDataSize();
      int iImpleCount = m_createImpleRun->getRunDataSize();
      m_runsSummaryForm->setRunsCount(iSynthCount, iImpleCount);
    }
  }
  m_formIndex++;
  UpdateDialogView();
}

void CreateRunsDialog::on_m_btnCancel_clicked() { this->hide(); }

void CreateRunsDialog::on_m_btnBack_clicked() {
  int itype = m_selectTypeForm->getDesignType();
  if (m_formIndex == CRFI_RUNSUMARY) {
    if (DRT_SYNTH == itype) {
      m_formIndex--;
    }
  } else if (m_formIndex == CRFI_IMPLERUN) {
    if (DRT_IMPLE == itype) {
      m_formIndex--;
    }
  }
  m_formIndex--;
  UpdateDialogView();
}

void CreateRunsDialog::UpdateDialogView() {
  if (m_formIndex == CRFI_SELECTTYPE) {
    ui->m_btnBack->setEnabled(false);
    ui->m_btnOK->setText(tr("Next"));
  } else if (m_formIndex == CRFI_RUNSUMARY) {
    ui->m_btnBack->setEnabled(true);
    ui->m_btnOK->setText(tr("Finish"));
  } else {
    ui->m_btnBack->setEnabled(true);
    ui->m_btnOK->setText(tr("Next"));
  }
  ui->m_stackedWidget->setCurrentIndex(m_formIndex);
}

int CreateRunsDialog::CreateSynthRuns() {
  int ret = 0;
  QList<rundata> listRun = m_createSynthRun->getRunDataList();
  if (listRun.size()) {
    foreach (auto rd, listRun) {
      ret = m_pm->setSynthRun(rd.m_runName);
      if (0 == ret) {
        QList<QPair<QString, QString>> listParam;
        QPair<QString, QString> pair;
        pair.first = PROJECT_PART_DEVICE;
        pair.second = rd.m_device;
        listParam.append(pair);
        ret = m_pm->setSynthesisOption(listParam);

        ret = m_pm->setRunSrcSet(rd.m_srcSet);
        ret = m_pm->setRunConstrSet(rd.m_constrSet);
      }
    }
  }
  return ret;
}

int CreateRunsDialog::CreateImpleRuns() {
  int ret = 0;
  QList<rundata> listRun = m_createImpleRun->getRunDataList();
  if (listRun.size()) {
    foreach (auto rd, listRun) {
      ret = m_pm->setImpleRun(rd.m_runName);
      if (0 == ret) {
        ret = m_pm->setRunSynthRun(rd.m_synthName);

        ret = m_pm->setRunSrcSet(rd.m_srcSet);
        ret = m_pm->setRunConstrSet(rd.m_constrSet);
      }
    }
  }
  return ret;
}
