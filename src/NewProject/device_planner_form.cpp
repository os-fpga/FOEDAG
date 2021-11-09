#include "device_planner_form.h"

#include <QDir>
#include <QFile>
#include <QHeaderView>
#include <QMessageBox>
#include <QTextStream>
#include <QtXml/QDomDocument>

#include "ui_device_planner_form.h"

devicePlannerForm::devicePlannerForm(QWidget *parent)
    : QWidget(parent), ui(new Ui::devicePlannerForm) {
  ui->setupUi(this);
  ui->m_labelTitle->setText(tr("Select Device"));
  ui->m_labelDetail->setText(
      tr("Select the series and device you want to target for compilation."));

  QString path = QDir::currentPath() + "/device/";
  ui->comboBox->setStyleSheet("border: 1px solid gray;");
  ui->m_comboBoxSeries->setStyleSheet("border: 1px solid gray;");
  ui->m_comboBoxPackage->setStyleSheet("border: 1px solid gray;");

  m_grid = new QTableView(this);

  // Set properties
  m_grid->verticalHeader()->hide();
  m_grid->verticalHeader()->setDefaultSectionSize(30);
  m_grid->horizontalHeader()->setMinimumHeight(30);
  m_grid->horizontalHeader()->setStretchLastSection(
      true);  // Last column adaptive width
  m_grid->setEditTriggers(QTableView::NoEditTriggers);
  m_grid->setSelectionBehavior(QTableView::SelectRows);
  m_grid->setSelectionMode(
      QTableView::SingleSelection);       // Single line selection
  m_grid->setAlternatingRowColors(true);  // Color separation between lines

  m_grid->setStyleSheet(
      "QTableView {border: 1px solid rgb(230,230,230);}\
                          QTableView::item:selected{color:black;background: #63B8FF;}");
  m_grid->setColumnWidth(0, 80);
  m_model = new QStandardItemModel();
  m_grid->setModel(m_model);

  QVBoxLayout *vbox = new QVBoxLayout(ui->m_groupBoxGrid);
  vbox->addWidget(m_grid);
  vbox->setContentsMargins(0, 0, 0, 0);
  vbox->setSpacing(1);
  ui->m_groupBoxGrid->setLayout(vbox);

  connect(ui->m_comboBoxSeries, &QComboBox::currentTextChanged, this,
          &devicePlannerForm::onSeriestextChanged);
  connect(ui->m_comboBoxPackage, &QComboBox::currentTextChanged, this,
          &devicePlannerForm::onPackagetextChanged);
}

devicePlannerForm::~devicePlannerForm() { delete ui; }

void devicePlannerForm::getdeviceinfo(QString &servies, QString &device,
                                      QString &package) {
  QString pkgname = ui->m_comboBoxPackage->currentText();
  QString sername = ui->m_comboBoxSeries->currentText();
}

void devicePlannerForm::initformdata_fpga(QString dir) {
  QFile file(dir + "device_fpga.xml");
  if (!file.open(QFile::ReadOnly)) {
    QMessageBox::warning(this, tr("error"), tr("device_fpga.xml read error!"),
                         QMessageBox::Ok);
    return;
  }

  QDomDocument doc;
  if (!doc.setContent(&file)) {
    file.close();
    QMessageBox::warning(this, tr("error"),
                         tr("device_fpga.xml setContent failed!"),
                         QMessageBox::Ok);
    return;
  }
  file.close();

  QDomElement root = doc.documentElement();
  QDomNode node = root.firstChild();
  while (!node.isNull()) {
    if (node.isElement()) {
      QDomElement e1 = node.toElement();
      // qDebug()<<e1.tagName()<<" "<<e1.attribute("name");
      series series;
      series.sername = e1.attribute("name");
      QDomNode node2 = e1.firstChild();
      while (!node2.isNull()) {
        if (node2.isElement()) {
          QDomElement e2 = node2.toElement();
          device dev;
          // qDebug()<<e2.tagName()<<" "<<e2.attribute("name");
          dev.devname = e2.attribute("name");
          QDomNode node3 = e2.firstChild();
          while (!node3.isNull()) {
            if (node3.isElement()) {
              package pkg;
              QDomElement e3 = node3.toElement();
              // qDebug()<<e3.tagName()<<" "<<e3.attribute("name");
              pkg.pkgname = e3.attribute("name");
              QDomNodeList list = e3.childNodes();
              for (int i = 0; i < list.count(); i++) {
                QDomNode n = list.at(i);
                if (node.isElement()) {
                  // qDebug()<<n.nodeName()<<":"<<n.toElement().text();
                  if ("Type" == n.nodeName()) {
                    pkg.type = n.toElement().text();
                  } else if ("CoreVoltage" == n.nodeName()) {
                    pkg.coreVoltage = n.toElement().text();
                  } else if ("LUTs" == n.nodeName()) {
                    pkg.luts = n.toElement().text();
                  } else if ("TotalIOs" == n.nodeName()) {
                    pkg.totalIOs = n.toElement().text();
                  } else if ("MemoryBits" == n.nodeName()) {
                    pkg.memoryBits = n.toElement().text();
                  } else if ("DspBlocks" == n.nodeName()) {
                    pkg.dspBlocks = n.toElement().text();
                  } else if ("PLLs" == n.nodeName()) {
                    pkg.plls = n.toElement().text();
                  }

                  // If you only need to display, you don't need to parse out
                  // each field
                  QPair<QString, QString> pair(n.nodeName(),
                                               n.toElement().text());
                  pkg.itemlist.append(pair);
                }
              }
              dev.pkglist.append(pkg);
            }
            node3 = node3.nextSibling();
          }
          series.devlist.append(dev);
        }
        node2 = node2.nextSibling();
      }
      m_fpga.serlist.append(series);
    }
    node = node.nextSibling();
  }
}

void devicePlannerForm::initformdata_efpga(QString dir) {
  QFile file(dir + "device.xml");
  if (!file.open(QFile::ReadOnly)) {
    QMessageBox::warning(this, tr("error"), tr("device.xml read error!"),
                         QMessageBox::Ok);
    return;
  }

  QDomDocument doc;
  if (!doc.setContent(&file)) {
    file.close();
    QMessageBox::warning(this, tr("error"), tr("device.xml setContent failed!"),
                         QMessageBox::Ok);
    return;
  }
  file.close();

  QDomElement root = doc.documentElement();

  QDomNode node = root.firstChild();
  while (!node.isNull()) {
    if (node.isElement()) {
      QDomElement e = node.toElement();
      // qDebug()<<e.tagName()<<" "<<e.attribute("Name") << e.attribute("Type")
      // << e.attribute("Series");
      m_efpga.ename = e.attribute("Name");
      m_efpga.type = e.attribute("Type");
      m_efpga.series = e.attribute("Series");

      QDomNodeList list = e.childNodes();
      for (int i = 0; i < list.count(); i++) {
        QDomNode n = list.at(i);
        if (node.isElement()) {
          // qDebug()<<n.nodeName()<<":"<<n.toElement().text();
          if ("LUTs" == n.nodeName()) {
            m_efpga.luts = n.toElement().text();
          } else if ("Memory_Bits" == n.nodeName()) {
            m_efpga.memoryBits = n.toElement().text();
          } else if ("DSP" == n.nodeName()) {
            m_efpga.dsp = n.toElement().text();
          } else if ("Pins" == n.nodeName()) {
            m_efpga.pins = n.toElement().text();
          } else if ("PLL" == n.nodeName()) {
            m_efpga.pll = n.toElement().text();
          } else if ("Height" == n.nodeName()) {
            m_efpga.height = n.toElement().text();
          } else if ("Width" == n.nodeName()) {
            m_efpga.width = n.toElement().text();
          } else if ("Area" == n.nodeName()) {
            m_efpga.area = n.toElement().text();
          } else if ("ShrinkArea" == n.nodeName()) {
            m_efpga.shrinArea = n.toElement().text();
          } else if ("Aspect_Ratio" == n.nodeName()) {
            m_efpga.aspect_Ratio = n.toElement().text();
          }

          // If you only need to display, you don't need to parse out each field
          QPair<QString, QString> pair(n.nodeName(), n.toElement().text());
          m_efpga.itemlist.append(pair);
        }
      }
    }
    node = node.nextSibling();
  }
}

void devicePlannerForm::updateformview() {
  disconnect(ui->m_comboBoxSeries, &QComboBox::currentTextChanged, this,
             &devicePlannerForm::onSeriestextChanged);
  disconnect(ui->m_comboBoxPackage, &QComboBox::currentTextChanged, this,
             &devicePlannerForm::onPackagetextChanged);

  updateformgrid();
  connect(ui->m_comboBoxSeries, &QComboBox::currentTextChanged, this,
          &devicePlannerForm::onSeriestextChanged);
  connect(ui->m_comboBoxPackage, &QComboBox::currentTextChanged, this,
          &devicePlannerForm::onPackagetextChanged);
}

void devicePlannerForm::updateformgrid() {
  QString pkgname = ui->m_comboBoxPackage->currentText();
  QString sername = ui->m_comboBoxSeries->currentText();
  m_model->clear();
}

void devicePlannerForm::onSeriestextChanged(const QString &arg1) {
  updateformgrid();
}

void devicePlannerForm::onPackagetextChanged(const QString &arg1) {
  updateformgrid();
}
