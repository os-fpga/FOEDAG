#include "QLDeviceManager.h"

#include <QObject>
#include <QWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>

#include <iostream>
#include <set>
#include "CompilerOpenFPGA_ql.h"
#include "Compiler/Compiler.h"
#include "Utils/StringUtils.h"
#include "MainWindow/Session.h"

namespace FOEDAG {

QLDeviceManager::QLDeviceManager(QWidget *parent)
    : QWidget(parent) {
}

QLDeviceManager::QLDeviceManager(CompilerOpenFPGA_ql *compiler, QWidget *parent)
    : QWidget(parent) {

      this->compiler = compiler;
}

QWidget* QLDeviceManager::createDeviceSelectionWidget() {

  parseDeviceData();

  QWidget* dlg = this;

  //QWidget* dlg = new QWidget();
  dlg->setWindowTitle("Device Selection");
  //dlg->setAttribute(Qt::WA_DeleteOnClose);

  QVBoxLayout* dlg_toplevellayout = new QVBoxLayout();
  dlg->setLayout(dlg_toplevellayout);

  QHBoxLayout* dlg_familylayout = new QHBoxLayout();
  QHBoxLayout* dlg_foundrynodelayout = new QHBoxLayout();
  QHBoxLayout* dlg_voltagethresholdlayout = new QHBoxLayout();
  QHBoxLayout* dlg_pvtcornerlayout = new QHBoxLayout();
  QHBoxLayout* dlg_layoutlayout = new QHBoxLayout();

  QLabel* combolabel = new QLabel("Family");
  QLabel* combo2label = new QLabel("Foundry-Node");
  QLabel* combo3label = new QLabel("Voltage Threshold");
  QLabel* combo4label = new QLabel("Corner");
  QLabel* combo5label = new QLabel("Layout");
  combo = new QComboBox();
  combo2 = new QComboBox();
  combo3 = new QComboBox();
  combo4 = new QComboBox();
  combo5 = new QComboBox();
  combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  combo2->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  combo3->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  combo4->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  combo5->setSizeAdjustPolicy(QComboBox::AdjustToContents);

  families.clear();
  combo->clear();
  for (QLDevice device: this->device_list) {
    families.insert(device.family);
  }

  for (std::string family: families) {
    combo->addItem(QString::fromStdString(family));
  }


  connect( combo, SIGNAL(currentTextChanged(const QString&)), this, SLOT(familyChanged(const QString&)) );
  connect( combo2, SIGNAL(currentTextChanged(const QString&)), this, SLOT(foundrynodeChanged(const QString&)) );
  connect( combo3, SIGNAL(currentTextChanged(const QString&)), this, SLOT(voltagethresholdChanged(const QString&)) );
  connect( combo4, SIGNAL(currentTextChanged(const QString&)), this, SLOT(pvtcornerChanged(const QString&)) );
  //connect( combo2, SIGNAL(currentTextChanged(const QString&)), this, SLOT(deviceChanged(const QString&)) );
  //connect( combo3, SIGNAL(currentTextChanged(const QString&)), this, SLOT(deviceChanged(const QString&)) );

  combo->setCurrentIndex(-1);
  combo->blockSignals(false);
  combo->setCurrentIndex(0);

  // connect( combo, SIGNAL(currentIndexChanged(int)), this, SLOT(deviceIndexChanged(int)) );
  // connect( combo2, SIGNAL(currentIndexChanged(int)), this, SLOT(deviceIndexChanged(int)) );
  // connect( combo3, SIGNAL(currentIndexChanged(int)), this, SLOT(deviceIndexChanged(int)) );

  dlg_familylayout->addWidget(combolabel);
  dlg_familylayout->addStretch();
  dlg_familylayout->addWidget(combo);

  dlg_foundrynodelayout->addWidget(combo2label);
  dlg_foundrynodelayout->addStretch();
  dlg_foundrynodelayout->addWidget(combo2);

  dlg_voltagethresholdlayout->addWidget(combo3label);
  dlg_voltagethresholdlayout->addStretch();
  dlg_voltagethresholdlayout->addWidget(combo3);

  dlg_pvtcornerlayout->addWidget(combo4label);
  dlg_pvtcornerlayout->addStretch();
  dlg_pvtcornerlayout->addWidget(combo4);

  dlg_layoutlayout->addWidget(combo5label);
  dlg_layoutlayout->addStretch();
  dlg_layoutlayout->addWidget(combo5);

  dlg_toplevellayout->addLayout(dlg_familylayout);
  dlg_toplevellayout->addLayout(dlg_foundrynodelayout);
  dlg_toplevellayout->addLayout(dlg_voltagethresholdlayout);
  dlg_toplevellayout->addLayout(dlg_pvtcornerlayout);
  dlg_toplevellayout->addLayout(dlg_layoutlayout);


  // QHBoxLayout* dlg_buttonslayout = new QHBoxLayout();
  // dlg_toplevellayout->addLayout(dlg_buttonslayout); // second a row of buttons for actions
  // QPushButton *button_select = new QPushButton("Select");
  // button_select->setToolTip("Select Device");
  // QPushButton *button_cancel = new QPushButton("Cancel");
  // button_cancel->setToolTip("Cancel Selection");
  // connect( button_cancel, SIGNAL(clicked()), this, SLOT(handleCancelButtonClicked()) );
  // dlg_buttonslayout->addWidget(button_cancel);
  // dlg_buttonslayout->addStretch();
  // dlg_buttonslayout->addWidget(button_select);

  return this;
}

void QLDeviceManager::familyChanged(const QString& family_qstring)
{

  compiler->Message("familychanged");
  foundrynodes.clear();
  combo2->clear();

  family = family_qstring.toStdString();

  for (QLDevice device: this->device_list) {
    if( device.family == family) {
      std::string foundrynodet = device.foundry + " - " + device.node;
      foundrynodes.insert(foundrynodet);
    }
  }

  for (std::string foundrynodet: foundrynodes) {
    combo2->addItem(QString::fromStdString(foundrynodet));
  }
}

void QLDeviceManager::foundrynodeChanged(const QString& foundrynode_qstring)
{

  compiler->Message("foundrynodechanged");

  foundrynode = foundrynode_qstring.toStdString();
  voltagethresholds.clear();
  combo3->clear();

  for (QLDevice device: this->device_list) {
    if (device.family == family) {
      std::string foundrynodet = device.foundry + " - " + device.node;
      if (foundrynodet == foundrynode) {
        for (QLDeviceVariant variant : device.device_variants) {
          if (!variant.voltage_threshold.empty()) {
            voltagethresholds.insert(variant.voltage_threshold);
          }
        }
      }
    }
  }

  for (std::string voltagethresholdt: voltagethresholds) {
    combo3->addItem(QString::fromStdString(voltagethresholdt));
  }
}

void QLDeviceManager::voltagethresholdChanged(const QString& voltagethreshold_qstring)
{

  compiler->Message("voltagethresholdchanged");

  voltagethreshold = voltagethreshold_qstring.toStdString();
  pvtcorners.clear();
  combo4->clear();

  for (QLDevice device: this->device_list) {
    if (device.family == family) {
      std::string foundrynodet = device.foundry + " - " + device.node;
      if (foundrynodet == foundrynode) {
        for (QLDeviceVariant variant : device.device_variants) {
          if (variant.voltage_threshold == voltagethreshold) {
            if(!variant.p_v_t_corner.empty()) {
              pvtcorners.insert(variant.p_v_t_corner);
            }
          }
        }
      }
    }
  }

  for (std::string p_v_t_cornert: pvtcorners) {
    combo4->addItem(QString::fromStdString(p_v_t_cornert));
  }
}

void QLDeviceManager::pvtcornerChanged(const QString& pvtcorner_qstring)
{

  compiler->Message("pvtcornerchanged");

  pvtcorner = pvtcorner_qstring.toStdString();
  layouts.clear();
  combo5->clear();

  for (QLDevice device: this->device_list) {
    if (device.family == family) {
      std::string foundrynodet = device.foundry + " - " + device.node;
      if (foundrynodet == foundrynode) {
        for (QLDeviceVariant variant : device.device_variants) {
          if (variant.voltage_threshold == voltagethreshold) {
            if(variant.p_v_t_corner == pvtcorner) {
              for(std::string layoutt : variant.device_variant_layouts) {
                layouts.insert(layoutt);
              }
            }
          }
        }
      }
    }
  }

  for (std::string layoutt: layouts) {
    combo5->addItem(QString::fromStdString(layoutt));
  }
}


void QLDeviceManager::deviceIndexChanged(int index)
{
  // QMessageBox msgBox;
  // msgBox.setText("slot index: " + QString::number(index));
  // msgBox.exec();
  // compiler->Message("slot-index");
}

void QLDeviceManager::handleCancelButtonClicked()
{
  // QMessageBox msgBox;
  // msgBox.setText("slot clicked");
  // msgBox.exec();
  // compiler->Message("slot-index");
}

void QLDeviceManager::parseDeviceData() {

  std::string family;
  std::string foundry;
  std::string node;

  std::error_code ec;

  // get to the device_data dir path of the installation
  std::filesystem::path root_device_data_dir_path = 
      compiler->GetSession()->Context()->DataPath();

  // clear the list before parsing
  device_list.clear();

  // each dir in the device_data is a family
  //    for each family, check for foundry dirs
  //        for each foundry, check for node 
  //            for each family-foundry-node dir, check the device_variants
  
  // look at the directories inside the device_data_dir_path for 'family' entries
  for (const std::filesystem::directory_entry& dir_entry_family : 
                    std::filesystem::directory_iterator(root_device_data_dir_path)) {
    
    if(dir_entry_family.is_directory()) {
      
      // we would see family at this level
      family = dir_entry_family.path().filename().string();

      // look at the directories inside the 'family' dir for 'foundry' entries
      for (const std::filesystem::directory_entry& dir_entry_foundry : 
                    std::filesystem::directory_iterator(dir_entry_family.path())) {

        if(dir_entry_foundry.is_directory()) {
      
          // we would see foundry at this level
          foundry = dir_entry_foundry.path().filename().string();

          // look at the directories inside the 'foundry' dir for 'node' entries
          for (const std::filesystem::directory_entry& dir_entry_node : 
                          std::filesystem::directory_iterator(dir_entry_foundry.path())) {

            if(dir_entry_node.is_directory()) {
            
              // we would see devices at this level
              node = dir_entry_node.path().filename().string();

              // get all the device_variants for this device:
              std::vector<std::string> device_variants;

              device_variants = compiler->list_device_variants(family,
                                                     foundry,
                                                     node,
                                                     dir_entry_node.path());
              if(device_variants.empty()) {
                // display error, but continue with other devices.
                compiler->Message("error in parsing variants for device: " + family + "," + foundry + "," + node +"\n");
              }
              else {

                QLDevice device;
                device.family = family;
                device.foundry = foundry;
                device.node = node;

                for (std::string device_variant: device_variants) {
                    QLDeviceVariant variant;
                    std::vector<std::string> tokens;
                    StringUtils::tokenize(device_variant, ",", tokens);
                    variant.family = tokens[0];
                    variant.foundry = tokens[1];
                    variant.node = tokens[2];
                    if(tokens.size() == 5) {
                      variant.voltage_threshold = tokens[3];
                      variant.p_v_t_corner = tokens[4];
                    }
                    variant.device_variant_layouts = compiler->ListLayouts(variant.family,
                                                                  variant.foundry,
                                                                  variant.node,
                                                                  variant.voltage_threshold,
                                                                  variant.p_v_t_corner);

                    device.device_variants.push_back(variant);
                }

                device_list.push_back(device);
                
              }
            }
          }
        }
      }
    }
  }

  // DEBUG
  for (QLDevice device: device_list) {
      compiler->Message("Device: " + device.family + " " + device.foundry + " " + device.node);
      for (QLDeviceVariant variant: device.device_variants) {
        compiler->Message("  Variant: " + variant.family + " " + variant.foundry + " " + variant.node + " " + variant.voltage_threshold + " " + variant.p_v_t_corner);
        for (std::string layout: variant.device_variant_layouts) {
          compiler->Message("    " + layout);
        }
      }
      compiler->Message("\n");
  }

  //QLDeviceManager *devicemanager = new QLDeviceManager(this);
  //showSelectionDialog(this->device_list);
  // Message("done");
}

} // namespace FOEDAG