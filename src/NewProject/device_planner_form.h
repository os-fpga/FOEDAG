#ifndef DEVICEPLANNERFORM_H
#define DEVICEPLANNERFORM_H

#include <QStandardItemModel>
#include <QTableView>
#include <QWidget>

enum fpgatype { FT_FPGA, FT_EFPGA };

struct package {
  QString pkgname;
  QString type;
  QString coreVoltage;
  QString luts;
  QString totalIOs;
  QString memoryBits;
  QString dspBlocks;
  QString plls;
  QList<QPair<QString, QString>> itemlist;
};

struct device {
  QString devname;
  QList<package> pkglist;
};

struct series {
  QString sername;
  QList<device> devlist;
};

struct fpga {
  QList<series> serlist;
};

typedef QMap<QString, QMap<QString, package>> FpgaMap;

struct efpga {
  QString type;
  QString ename;
  QString series;
  QString luts;
  QString memoryBits;
  QString dsp;
  QString pins;
  QString pll;
  QString height;
  QString width;
  QString area;
  QString shrinArea;
  QString aspect_Ratio;
  QList<QPair<QString, QString>> itemlist;
};

namespace Ui {
class devicePlannerForm;
}

class devicePlannerForm : public QWidget {
  Q_OBJECT

 public:
  explicit devicePlannerForm(QWidget *parent = nullptr);
  ~devicePlannerForm();

  void getdeviceinfo(QString &servies, QString &device, QString &package);
 private slots:
  void onSeriestextChanged(const QString &arg1);

  void onPackagetextChanged(const QString &arg1);

 private:
  Ui::devicePlannerForm *ui;

  QTableView *m_grid;
  QStandardItemModel *m_model;
  QItemSelectionModel *m_selectmodel;

  fpga m_fpga;
  efpga m_efpga;
  void initformdata_fpga(QString dir);
  void initformdata_efpga(QString dir);
  void updateformview();
  void updateformgrid();
};

#endif  // DEVICEPLANNERFORM_H
