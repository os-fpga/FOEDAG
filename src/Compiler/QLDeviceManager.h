#include <QWidget>
#include <QString>
#include <QComboBox>


#include <string>
#include <vector>
#include <set>
#include <filesystem>

#ifndef QLDEVICEMANAGER_H
#define QLDEVICEMANAGER_H

namespace FOEDAG {

class CompilerOpenFPGA_ql;

class QLDeviceVariantLayout {
    public:
    std::string name;
    // can add other things here such as num of BRAM/DSP etc. in the future!
};

class QLDeviceVariant {
    public:
    std::string family;
    std::string foundry;
    std::string node;
    std::string voltage_threshold;
    std::string p_v_t_corner;
    std::vector<std::string> device_variant_layouts;
};

class QLDevice {
    public:
    std::string family;
    std::string foundry;
    std::string node;
    std::vector<QLDeviceVariant> device_variants;
    std::filesystem::path device_root_path;
};

class QLDeviceManager : public QWidget {
  Q_OBJECT
 public:
  QLDeviceManager(QWidget *parent = nullptr);
  QLDeviceManager(CompilerOpenFPGA_ql *compiler, QWidget *parent = nullptr);


 public:
 QWidget* createDeviceSelectionWidget();
 void parseDeviceData();

 public slots:
 void familyChanged(const QString& family_qstring);
 void foundrynodeChanged(const QString& foundrynode_qstring);
 void voltagethresholdChanged(const QString& voltagetheshold_qstring);
 void pvtcornerChanged(const QString& pvtcorner_qstring);
 void deviceIndexChanged(int index);
 void handleCancelButtonClicked();

 public:
  std::vector <QLDevice> device_list;
  CompilerOpenFPGA_ql* compiler;
  std::set <std::string> families;
  std::string family;
  std::set <std::string> foundrynodes;
  std::string foundrynode;
  std::set <std::string> voltagethresholds;
  std::string voltagethreshold;
  std::set <std::string> pvtcorners;
  std::string pvtcorner;
  std::set <std::string> layouts;
  std::string layout;
  QLDevice device;

  QComboBox *combo;
  QComboBox *combo2;
  QComboBox *combo3;
  QComboBox *combo4;
  QComboBox *combo5;
};



}

#endif // QLDEVICEMANAGER_H