#include "QLDeviceManager.h"

#include <QObject>
#include <QWidget>
#include <QFont>
#include <QSpacerItem>
#include <QPalette>
#include <QDialog>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QMessageBox>
#include <QTextEdit>
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>
#include <QJsonArray>
#include <QDirIterator>
#include <QProcess>

#include <iostream>
#include <set>
#include <regex>
#include <filesystem>

#include <CRFileCryptProc.hpp>
#include "CompilerOpenFPGA_ql.h"
#include "Compiler/Compiler.h"
#include "Utils/FileUtils.h"
#include "Utils/LogUtils.h"
#include "Utils/StringUtils.h"
#include "MainWindow/Session.h"
#include "QLSettingsManager.h"
#include "NewProject/ProjectManager/project_manager.h"

extern FOEDAG::Session* GlobalSession;

namespace FOEDAG {


// singleton init
QLDeviceManager* QLDeviceManager::instance = nullptr;


// static access function
QLDeviceManager* QLDeviceManager::getInstance(bool initialize) {

  // creation
  if(instance == nullptr) {
    // std::cout << "create new QLDeviceManager()" << std::endl;
    instance = new QLDeviceManager();
  }

  // init, if needed
  if(initialize == true) {
    instance->parseDeviceData();
  }

  return instance;
}


bool QLDeviceManager::compareLayouts(const std::string& layout_1, const std::string& layout_2) {

  bool firstIsLesser = true;

  // assumption: layout_names are always "widthxheight" -> if not, then default to layout_1 < layout_2
  // return true if layout_1 < layout_2, else false
  try {

    std::string layout_1_lc = StringUtils::toLower(layout_1);
    std::string layout_2_lc = StringUtils::toLower(layout_2);

    std::vector<std::string> tokens_layout_1;
    StringUtils::tokenize(layout_1_lc, "x", tokens_layout_1);
    
    std::vector<std::string> tokens_layout_2;
    StringUtils::tokenize(layout_2_lc, "x", tokens_layout_2);

    if( (tokens_layout_1.size() == 2) && (tokens_layout_2.size() == 2) ) {
      int layout_1_width = std::stoi(tokens_layout_1[0]);
      int layout_1_height = std::stoi(tokens_layout_1[1]);

      int layout_2_width = std::stoi(tokens_layout_2[0]);
      int layout_2_height = std::stoi(tokens_layout_2[1]);

      if(layout_1_width < layout_2_width) {
        firstIsLesser = true;
      }
      else if(layout_1_width == layout_2_width) {
        if(layout_1_height < layout_2_height) {
          firstIsLesser = true;
        }
        else {
          firstIsLesser = false;
        }
      }
      else {
        firstIsLesser = false;
      }
    }
  }
  catch (std::invalid_argument const &e) {
    std::cout << "[compareLayouts] Bad input: std::invalid_argument thrown" << std::endl;
    firstIsLesser = true;
  }
    catch (std::out_of_range const &e) {
    std::cout << "[compareLayouts] Integer overflow: std::out_of_range thrown" << std::endl;
    firstIsLesser = true;
  }

  return firstIsLesser;
}


QLDeviceManager::QLDeviceManager(QObject *parent)
    : QObject(parent) {}


QLDeviceManager::~QLDeviceManager() {
  // std::cout << "~QLDeviceManager()" << std::endl;
  if(device_manager_widget != nullptr) {
    delete device_manager_widget;
  }
}


QWidget* QLDeviceManager::createDeviceSelectionWidget(bool newProjectMode) {

  // whenever a new GUI Widget is created, we mark the mode of operation of this widget:
  this->newProjectMode = newProjectMode;

  // std::cout << "createDeviceSelectionWidget()++, newProjectMode: " << newProjectMode << std::endl;

  if(device_manager_widget != nullptr) {
    device_manager_widget->deleteLater();
  }

  // cleanup GUI related data as well:
  QLDeviceTarget empty_device_target;
  this->device_target_selected = empty_device_target;
  this->families.clear();
  this->family.clear();
  this->foundrynodes.clear();
  this->foundrynode.clear();
  this->foundry.clear();
  this->node.clear();
  this->voltage_thresholds.clear();
  this->voltage_threshold.clear();
  this->p_v_t_corners.clear();
  this->p_v_t_corner.clear();
  this->layouts.clear();
  this->layout.clear();

  device_manager_widget = new QWidget();

  QWidget* dlg = device_manager_widget;

  dlg->setWindowTitle("Device Selection");

  QVBoxLayout* dlg_toplevellayout = new QVBoxLayout();
  dlg->setLayout(dlg_toplevellayout);

  QVBoxLayout* dlg_titleDetailLayout = new QVBoxLayout();
  QLabel* dlg_titleLabel = new QLabel("Device Selection");
  QFont dlg_titleLabelFont;
  dlg_titleLabelFont.setWeight(QFont::Bold);
  dlg_titleLabelFont.setStyleHint(QFont::SansSerif);
  dlg_titleLabelFont.setPointSize(12);
  dlg_titleLabel->setFont(dlg_titleLabelFont);
  dlg_titleLabel->setWordWrap(true);

  QLabel* dlg_detailLabel = new QLabel("Select the target device for this project");
  dlg_detailLabel->setWordWrap(true);

  dlg_titleDetailLayout->addWidget(dlg_titleLabel);
  dlg_titleDetailLayout->addSpacing(15);
  dlg_titleDetailLayout->addWidget(dlg_detailLabel);


  QFrame* dlg_selectionFrame = new QFrame();
  dlg_selectionFrame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  QVBoxLayout* dlg_selectionFrameLayout = new QVBoxLayout();
  dlg_selectionFrame->setLayout(dlg_selectionFrameLayout);
  QGroupBox *devicetypeGroupBox = new QGroupBox(tr("Device Type"));
  QVBoxLayout* devicetypeGroupBoxLayout =  new QVBoxLayout();
  devicetypeGroupBox->setLayout(devicetypeGroupBoxLayout);
  QHBoxLayout* dlg_familylayout = new QHBoxLayout();
  QHBoxLayout* dlg_foundrynodelayout = new QHBoxLayout();
  devicetypeGroupBoxLayout->addLayout(dlg_familylayout);
  devicetypeGroupBoxLayout->addLayout(dlg_foundrynodelayout);
  QGroupBox *devicevariantGroupBox = new QGroupBox(tr("Device Variant"));
  QVBoxLayout* devicevariantGroupBoxLayout =  new QVBoxLayout();
  devicevariantGroupBox->setLayout(devicevariantGroupBoxLayout);
  QHBoxLayout* dlg_voltage_thresholdlayout = new QHBoxLayout();
  QHBoxLayout* dlg_p_v_t_cornerlayout = new QHBoxLayout();
  devicevariantGroupBoxLayout->addLayout(dlg_voltage_thresholdlayout);
  devicevariantGroupBoxLayout->addLayout(dlg_p_v_t_cornerlayout);
  QGroupBox *devicesizeGroupBox = new QGroupBox(tr("Device Size"));
  QVBoxLayout* devicesizeGroupBoxLayout =  new QVBoxLayout();
  devicesizeGroupBox->setLayout(devicesizeGroupBoxLayout);
  QHBoxLayout* dlg_layoutlayout = new QHBoxLayout();
  QHBoxLayout* dlg_resourceslayout = new QHBoxLayout();
  devicesizeGroupBoxLayout->addLayout(dlg_layoutlayout);
  devicesizeGroupBoxLayout->addLayout(dlg_resourceslayout);


  // dlg_selectionFrameLayout->addLayout(dlg_familylayout);
  // dlg_selectionFrameLayout->addLayout(dlg_foundrynodelayout);
  // dlg_selectionFrameLayout->addLayout(dlg_voltage_thresholdlayout);
  // dlg_selectionFrameLayout->addLayout(dlg_p_v_t_cornerlayout);
  // dlg_selectionFrameLayout->addLayout(dlg_layoutlayout);
  dlg_selectionFrameLayout->addWidget(devicetypeGroupBox);
  dlg_selectionFrameLayout->addWidget(devicevariantGroupBox);
  dlg_selectionFrameLayout->addWidget(devicesizeGroupBox);

  QLabel* m_combobox_family_label = new QLabel("Family");
  QLabel* m_combobox_foundry_node_label = new QLabel("Foundry-Node");
  QLabel* m_combobox_voltage_threshold_label = new QLabel("Voltage Threshold");
  QLabel* m_combobox_p_v_t_corner_label = new QLabel("Corner");
  QLabel* m_combobox_layout_label = new QLabel("Layout");
  m_combobox_family = new QComboBox();
  m_combobox_foundry_node = new QComboBox();
  m_combobox_voltage_threshold = new QComboBox();
  m_combobox_p_v_t_corner = new QComboBox();
  m_combobox_layout = new QComboBox();
  m_device_resources_label = new QLabel();
  m_combobox_family->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  m_combobox_foundry_node->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  m_combobox_voltage_threshold->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  m_combobox_p_v_t_corner->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  m_combobox_layout->setSizeAdjustPolicy(QComboBox::AdjustToContents);

  families.clear();
  singularity.clear();
  m_combobox_family->clear();
  for (QLDeviceType device: this->device_list) {
    // ensure that the item being added has not been added before using std::set
    // for performance reasons, keeping a vector as final container for future need of sorting.
    if(singularity.insert(device.family).second == true) {
      families.push_back(device.family);
    }
  }

  for (std::string family: families) {
    m_combobox_family->addItem(QString::fromStdString(family));
  }

  // connect( m_combobox_family, SIGNAL(currentTextChanged(const QString&)), this, SLOT(familyChanged(const QString&)) );
  // QObject::connect( m_combobox_family, &QComboBox::currentTextChanged,
  //                   this, &QLDeviceManager::familyChanged );
  // unfortunately, the QLDeviceManager object seems to live on a thread, where there is no EventQueue!
  // hence the slot never gets called (unless we can derive it from QWidget, in which case it will be on the GUI thread)
  // hence, we resort to using a lambda (instead of say, customizing QWidget for the GUI part itself) as below:
  
  QObject::connect( m_combobox_family, &QComboBox::currentTextChanged, 
                    [this](const QString& currentText){
                        // std::cout << "lambda-oncurrentTextChanged-m_combobox_family: " << currentText.toStdString() << std::endl;
                        this->familyChanged(currentText);
                        } );

  QObject::connect( m_combobox_foundry_node, &QComboBox::currentTextChanged, 
                    [this](const QString& currentText){
                        // std::cout << "lambda-oncurrentTextChanged-m_combobox_foundry_node: " << currentText.toStdString() << std::endl;
                        this->foundrynodeChanged(currentText);
                        } );

  QObject::connect( m_combobox_voltage_threshold, &QComboBox::currentTextChanged, 
                    [this](const QString& currentText){
                        // std::cout << "lambda-oncurrentTextChanged-m_combobox_voltage_threshold: " << currentText.toStdString() << std::endl;
                        this->voltage_thresholdChanged(currentText);
                        } );

  QObject::connect( m_combobox_p_v_t_corner, &QComboBox::currentTextChanged, 
                    [this](const QString& currentText){
                        // std::cout << "lambda-oncurrentTextChanged-m_combobox_p_v_t_corner: " << currentText.toStdString() << std::endl;
                        this->p_v_t_cornerChanged(currentText);
                        } );

  QObject::connect( m_combobox_layout, &QComboBox::currentTextChanged, 
                    [this](const QString& currentText){
                        // std::cout << "lambda-oncurrentTextChanged-m_combobox_layout: " << currentText.toStdString() << std::endl;
                        this->layoutChanged(currentText);
                        } );

  dlg_familylayout->addWidget(m_combobox_family_label);
  dlg_familylayout->addWidget(m_combobox_family);

  dlg_foundrynodelayout->addWidget(m_combobox_foundry_node_label);
  dlg_foundrynodelayout->addWidget(m_combobox_foundry_node);

  dlg_voltage_thresholdlayout->addWidget(m_combobox_voltage_threshold_label);
  dlg_voltage_thresholdlayout->addWidget(m_combobox_voltage_threshold);

  dlg_p_v_t_cornerlayout->addWidget(m_combobox_p_v_t_corner_label);
  dlg_p_v_t_cornerlayout->addWidget(m_combobox_p_v_t_corner);

  dlg_layoutlayout->addWidget(m_combobox_layout_label);
  dlg_layoutlayout->addWidget(m_combobox_layout);
  dlg_resourceslayout->addStretch();
  dlg_resourceslayout->addWidget(m_device_resources_label);

  QHBoxLayout* dlg_buttonslayout = nullptr;
  if(!newProjectMode) {
    m_message_label = new QLabel();
    m_message_label->setWordWrap(true);
    // background/font color
    m_message_label->setAutoFillBackground(true);
    //m_message_label->setStyleSheet("QLabel { background-color : red; color : blue; }");
    QPalette m_message_label_palette;
    m_message_label_palette.setColor(QPalette::Window, Qt::white);
    m_message_label_palette.setColor(QPalette::WindowText, Qt::red);
    m_message_label->setPalette(m_message_label_palette);
    // frame
    m_message_label->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    m_message_label->hide();


    dlg_buttonslayout = new QHBoxLayout();
    m_button_reset = new QPushButton("Reset");
    m_button_reset->setToolTip("Reset Device Selection as in the Settings JSON");
    m_button_reset->setDisabled(true);
    QObject::connect( m_button_reset, &QPushButton::clicked, 
                        [this](){
                            // std::cout << "lambda-clicked-m_button_reset" << std::endl;
                            this->resetButtonClicked();
                            } );
    m_button_apply = new QPushButton("Apply");
    m_button_apply->setToolTip("Apply the new Device Selection to the Settings JSON");
    m_button_apply->setDisabled(true);
    QObject::connect( m_button_apply, &QPushButton::clicked, 
                        [this](){
                            // std::cout << "lambda-clicked-m_button_apply" << std::endl;
                            this->applyButtonClicked();
                            } );
    dlg_buttonslayout->addStretch();
    dlg_buttonslayout->addWidget(m_button_reset);
    dlg_buttonslayout->addWidget(m_button_apply);
  }

  dlg_toplevellayout->addLayout(dlg_titleDetailLayout);
  dlg_toplevellayout->addSpacing(20);
  dlg_toplevellayout->addWidget(dlg_selectionFrame);
  if(!newProjectMode) {
    dlg_toplevellayout->addWidget(m_message_label);
    dlg_toplevellayout->addLayout(dlg_buttonslayout);
  }
  dlg_toplevellayout->addStretch();

  // trigger a self UI update according to the currently selected device:
  triggerUIUpdate();

  // std::cout << "createDeviceSelectionWidget()--, newProjectMode: " << newProjectMode << std::endl;

  return dlg;
}

QLDeviceVariantLayout* QLDeviceManager::findDeviceLayoutVariantPtr(const std::string& family, 
                                                                   const std::string& foundry,
                                                                   const std::string& node,
                                                                   const std::string& voltage_threshold,
                                                                   const std::string& p_v_t_corner,
                                                                   const std::string& layoutName)
{
  for (QLDeviceType& device: this->device_list) {
    if ((device.family == family) && (device.foundry == foundry) && (device.node == node)) {
      for (QLDeviceVariant& device_variant: device.device_variants) {
        if ((device_variant.voltage_threshold == voltage_threshold) && (device_variant.p_v_t_corner == p_v_t_corner)) {
          for (QLDeviceVariantLayout& layout: device_variant.device_variant_layouts) {
            if (layout.name == layoutName) {
              return &layout;
            }
          }
        }
      }
    }
  }
  std::cout << "cannot find layout " << layoutName << " for family " << family << std::endl;
  return nullptr;
};

void QLDeviceManager::giveupDeviceSelectionWidget() {

  if(device_manager_widget != nullptr) {

    // remove it from any associated parent widget
    device_manager_widget->setParent(0);
  }

}


void QLDeviceManager::familyChanged(const QString& family_qstring)
{

  // when 'family' changes, repopulate all the 'foundry - node' entries accordingly

  // std::cout << "familychanged: " << family_qstring.toStdString() << std::endl;
  foundrynodes.clear();
  singularity.clear();
  m_combobox_foundry_node->blockSignals(true);
  m_combobox_foundry_node->clear();

  family = family_qstring.toStdString();

  for (QLDeviceType device: this->device_list) {
    if(device.family == family) {

      std::string _foundrynode = convertToFoundryNode(device.foundry, device.node);
      // ensure that the item being added has not been added before using std::set
      // for performance reasons, keeping a vector as final container for future need of sorting.
      if(singularity.insert(_foundrynode).second == true) {
        foundrynodes.push_back(_foundrynode);
      }
    }
  }

  for (std::string _foundrynode: foundrynodes) {

    m_combobox_foundry_node->addItem(QString::fromStdString(_foundrynode));
  }

  m_combobox_foundry_node->setCurrentIndex(-1);
  m_combobox_foundry_node->blockSignals(false);

  if(currentDeviceTargetUpdateInProgress) {

      std::string _foundrynode = 
        convertToFoundryNode(device_target.device_variant.foundry, device_target.device_variant.node);

      int index = m_combobox_foundry_node->findText(QString::fromStdString(_foundrynode));
      // std::cout << "m_combobox_foundry_node index" << index << std::endl;
      m_combobox_foundry_node->setCurrentIndex(index);
  }
  else {
    m_combobox_foundry_node->setCurrentIndex(0);
  }
}


void QLDeviceManager::foundrynodeChanged(const QString& foundrynode_qstring)
{

  // when 'foundry - node' changes, repopulate all the 'voltage_threshold' entries accordingly

  // std::cout << "foundrynodechanged: " << foundrynode_qstring.toStdString() << std::endl;

  foundrynode = foundrynode_qstring.toStdString();
  std::vector<std::string> foundrynode_vector = convertFromFoundryNode(foundrynode);
  foundry = foundrynode_vector[0];
  node = foundrynode_vector[1];
  voltage_thresholds.clear();
  singularity.clear();
  m_combobox_voltage_threshold->blockSignals(true);
  m_combobox_voltage_threshold->clear();

  for (QLDeviceType device: this->device_list) {
    if (device.family == family) {
      std::string _foundrynode = convertToFoundryNode(device.foundry, device.node);
      if (_foundrynode == foundrynode) {
        for (QLDeviceVariant variant : device.device_variants) {
          // ensure that the item being added has not been added before using std::set
          // for performance reasons, keeping a vector as final container for future need of sorting.
          if(singularity.insert(variant.voltage_threshold).second == true) {
            voltage_thresholds.push_back(variant.voltage_threshold);
          }
        }
      }
    }
  }

  for (std::string _voltage_threshold: voltage_thresholds) {
    m_combobox_voltage_threshold->addItem(QString::fromStdString(_voltage_threshold));
  }

  m_combobox_voltage_threshold->setCurrentIndex(-1);
  m_combobox_voltage_threshold->blockSignals(false);

  if(currentDeviceTargetUpdateInProgress) {
    int index = m_combobox_voltage_threshold->findText(QString::fromStdString(device_target.device_variant.voltage_threshold));
    // std::cout << "m_combobox_voltage_threshold index" << index << std::endl;
    m_combobox_voltage_threshold->setCurrentIndex(index);
  }
  else {
    m_combobox_voltage_threshold->setCurrentIndex(0);
  }
}


void QLDeviceManager::voltage_thresholdChanged(const QString& voltage_threshold_qstring)
{

  // when 'voltage_threshold' changes, repopulate all the 'p_v_t_corner' entries accordingly

  // std::cout << "voltage_thresholdchanged: " << voltage_threshold_qstring.toStdString() << std::endl;

  voltage_threshold = voltage_threshold_qstring.toStdString();
  p_v_t_corners.clear();
  singularity.clear();
  m_combobox_p_v_t_corner->blockSignals(true);
  m_combobox_p_v_t_corner->clear();

  for (QLDeviceType device: this->device_list) {
    if (device.family == family) {
      std::string _foundrynode = convertToFoundryNode(device.foundry, device.node);
      if (_foundrynode == foundrynode) {
        for (QLDeviceVariant variant : device.device_variants) {
          if (variant.voltage_threshold == voltage_threshold) {
            // ensure that the item being added has not been added before using std::set
            // for performance reasons, keeping a vector as final container for future need of sorting.
            if(singularity.insert(variant.p_v_t_corner).second == true) {
              p_v_t_corners.push_back(variant.p_v_t_corner);
            }
          }
        }
      }
    }
  }

  for (std::string _p_v_t_corner: p_v_t_corners) {
    m_combobox_p_v_t_corner->addItem(QString::fromStdString(_p_v_t_corner));
  }

  m_combobox_p_v_t_corner->setCurrentIndex(-1);
  m_combobox_p_v_t_corner->blockSignals(false);

  if(currentDeviceTargetUpdateInProgress) {
    int index = m_combobox_p_v_t_corner->findText(QString::fromStdString(device_target.device_variant.p_v_t_corner));
    // std::cout << "m_combobox_p_v_t_corner index" << index << std::endl;
    m_combobox_p_v_t_corner->setCurrentIndex(index);
  }
  else {
    m_combobox_p_v_t_corner->setCurrentIndex(0);
  }
}


void QLDeviceManager::p_v_t_cornerChanged(const QString& p_v_t_corner_qstring)
{

  // when 'p_v_t_corner' changes, repopulate all the 'layout' entries accordingly

  // std::cout << "p_v_t_cornerchanged: " << p_v_t_corner_qstring.toStdString() << std::endl;

  p_v_t_corner = p_v_t_corner_qstring.toStdString();
  layouts.clear();
  singularity.clear();
  m_combobox_layout->blockSignals(true);
  m_combobox_layout->clear();

  for (QLDeviceType device: this->device_list) {
    if (device.family == family) {
      std::string _foundrynode = convertToFoundryNode(device.foundry, device.node);
      if (_foundrynode == foundrynode) {
        for (QLDeviceVariant variant : device.device_variants) {
          if (variant.voltage_threshold == voltage_threshold) {
            if(variant.p_v_t_corner == p_v_t_corner) {
              for(QLDeviceVariantLayout _layout : variant.device_variant_layouts) {
                // ensure that the item being added has not been added before using std::set
                // for performance reasons, keeping a vector as final container for future need of sorting.
                if(singularity.insert(_layout.name).second == true) {
                  layouts.push_back(_layout.name);
                }
              }
            }
          }
        }
      }
    }
  }

  // sort the layouts:
  std::sort(layouts.begin(), layouts.end(), QLDeviceManager::compareLayouts);

  for (std::string _layout: layouts) {
    m_combobox_layout->addItem(QString::fromStdString(_layout));
  }

  m_combobox_layout->setCurrentIndex(-1);
  m_combobox_layout->blockSignals(false);

  if(currentDeviceTargetUpdateInProgress) {
    int index = m_combobox_layout->findText(QString::fromStdString(device_target.device_variant_layout.name));
    // std::cout << "m_combobox_layout index" << index << std::endl;
    m_combobox_layout->setCurrentIndex(index);
  }
  else {
    m_combobox_layout->setCurrentIndex(0);
  }
}

void QLDeviceManager::layoutChanged(const QString& layout_qstring) {

  // when 'layout' changes, store the value

  // std::cout << "layoutChanged: " << layout_qstring.toStdString() << std::endl;
  // std::cout << "newProjectMode: " << newProjectMode << std::endl;

  layout = layout_qstring.toStdString();

  if(currentDeviceTargetUpdateInProgress) {

    currentDeviceTargetUpdateInProgress = false; // done.
  }

  QLDeviceTarget _device_target = convertToDeviceTarget(family,
                                                        foundry,
                                                        node,
                                                        voltage_threshold,
                                                        p_v_t_corner,
                                                        layout);

  if(isDeviceTargetValid(_device_target)) {

    // store the update: in separate variable, until user says confirm
    device_target_selected = _device_target;

    // std::cout << "selected device: " << std::endl;
    // std::cout << " >> [family]              " << device_target.device_variant.family << std::endl;
    // std::cout << " >> [foundry]             " << device_target.device_variant.foundry << std::endl;
    // std::cout << " >> [node]                " << device_target.device_variant.node << std::endl;
    // std::cout << " >> [voltage_threshold]   " << device_target.device_variant.voltage_threshold << std::endl;
    // std::cout << " >> [p_v_t_corner]        " << device_target.device_variant.p_v_t_corner << std::endl;
    // std::cout << " >> [layout name]         " << device_target.device_variant_layout.name << std::endl;
    // std::cout << " >> [layout width]        " << device_target.device_variant_layout.width << std::endl;
    // std::cout << " >> [layout height]       " << device_target.device_variant_layout.height << std::endl;
  }

  // now check if the selected device_target (via GUI) is different from the currently set device_target,
  // if so, enable the buttons, and show info text to user:
  if(device_manager_widget != nullptr) {
    
    if(newProjectMode) {

        // no apply or reset buttons, update the device_target immediately:
        // device_target should be set to device_target_selected
        setCurrentDeviceTarget(device_target_selected);
        
        // signal to the QLSettingsManager to update the settings JSON
        // to reflect the device_target selection!
        // change this to emit signal later...
        if(settings_manager != nullptr) {
            settings_manager->newProjectMode = newProjectMode;
            settings_manager->updateJSONSettingsForDeviceTarget(device_target_selected);
        }
    }
    else {

        // existing project mode: check if the device_target is changed from currently set device_target
        // and update the UI accordingly.

        std::string device_target_string = convertToDeviceString(device_target);
        std::string device_target_selected_string = convertToDeviceString(device_target_selected);
        // std::cout << "device_target_string: " << device_target_string << std::endl;
        // std::cout << "device_target_selected_string: " << device_target_selected_string << std::endl;
        if(!device_target_string.empty() && !device_target_selected_string.empty()) {
            if(device_target_string != device_target_selected_string) {
                // current device_target of the project differs from that selected in GUI.
                // Allow user to Apply the new target, or reset it to the current device_target.
                m_button_reset->setDisabled(false);
                m_button_apply->setDisabled(false);

                m_message_label->setText("Device Selection has changed!\n\n"
                                        "- Click 'Apply' to set the new Device.\n"
                                        "- Click 'Reset' to reset to the original Device.\n\n"
                                        "Task Settings will not reflect new Device until 'Apply' is executed!");
                m_message_label->show();
                // device_manager_widget->adjustSize();
            }
            else {
                // current device_target of the project is the same as the one selected in the GUI, no changes to apply.
                m_button_reset->setDisabled(true);
                m_button_apply->setDisabled(true);

                m_message_label->setText("");
                m_message_label->hide();
            }
        }
        // else if(!device_target_selected_string.empty()) {
        // // no currently set device target from JSON, probably new project.
        // // then, if the device_target_selected is via GUI, allow user to click apply
        // // so that we can do rest of the settings for selected device target...
        // m_button_apply->setDisabled(false);

        // m_message_label->setText("New Device Selection!\n\n"
        //                         "Click 'Apply' to set the new Device!\n\n"
        //                         "Task Settings will not reflect new Device\n"
        //                         "until 'Apply' is executed!");
        // m_message_label->show();
        // }
    }

    // update the layout's resource information:
    QString archInfo;
    //archInfo += "| ";
    archInfo += "width: <b>" + QString::number(device_target_selected.device_variant_layout.width) + " </b>| ";
    archInfo += "height: <b>" + QString::number(device_target_selected.device_variant_layout.height) + " </b>| ";
    archInfo += "\n";
    archInfo += "clb: <b>" + QString::number(device_target_selected.device_variant_layout.clb) + " </b>| ";
    archInfo += "dsp: <b>" + QString::number(device_target_selected.device_variant_layout.dsp) + " </b>| ";
    archInfo += "bram: <b>" + QString::number(device_target_selected.device_variant_layout.bram) + " </b>| ";
    archInfo += "io: <b>" + QString::number(device_target_selected.device_variant_layout.io) + " </b>| ";
    m_device_resources_label->setText(archInfo);
  }

}


std::vector<std::shared_ptr<LayoutInfoHelper>>
QLDeviceManager::ExtractDeviceAvailableResourcesFromVprLogContent(const std::string& content) const
{
  static QRegularExpression layoutPattern(R"(^Resource usage for device layout (\w+)...$)");
  static QRegularExpression clbLogPattern(R"(^(\d+)\s+blocks of type: clb$)");
  static QRegularExpression dspLogPattern(R"(^(\d+)\s+blocks of type: dsp$)");
  static QRegularExpression bramLogPattern(R"(^(\d+)\s+blocks of type: bram$)");
  static QRegularExpression ioLogPattern(R"(^(\d+)\s+blocks of type: io.+$)");

  auto tryExtractSubInt = [](const QRegularExpression& pattern, const std::string& content) -> int {
    int result = 0;
    auto match = pattern.match(content.c_str());
    if (match.hasMatch() && (match.lastCapturedIndex() == 1)) {
      bool ok;
      int candidate = match.captured(1).toInt(&ok);
      if (ok) {
        result = candidate;
      }
    }
    return result;
  };
  auto tryExtractSubStr = [](const QRegularExpression& pattern, const std::string& content) -> std::string {
    std::string result;
    auto match = pattern.match(content.c_str());
    if (match.hasMatch() && (match.lastCapturedIndex() == 1)) {
      result = match.captured(1).toStdString();
    }
    return result;
  };

  QString buff(content.c_str());
  QList<QString> lines = buff.split("\n");

  std::vector<std::shared_ptr<LayoutInfoHelper>> result;
  std::shared_ptr<LayoutInfoHelper> layoutInfo;
  for (const QString& line: lines) {
    std::string trimmedLine = line.trimmed().toStdString();
    std::string layoutName = tryExtractSubStr(layoutPattern, trimmedLine);
    if (!layoutName.empty() && (layoutName != "auto")) {
      if (layoutInfo) {
        result.push_back(layoutInfo);
      }
      layoutInfo = std::make_shared<LayoutInfoHelper>(layoutName);
      layoutInfo->clb = 0;
      layoutInfo->dsp = 0;
      layoutInfo->bram = 0;
      layoutInfo->io = 0;
      continue;
    }
    int clb = tryExtractSubInt(clbLogPattern, trimmedLine);
    if (layoutInfo && clb) {
      layoutInfo->clb += clb;
      continue;
    }
    int dsp = tryExtractSubInt(dspLogPattern, trimmedLine);
    if (layoutInfo && dsp) {
      layoutInfo->dsp += dsp;
      continue;
    }
    int bram = tryExtractSubInt(bramLogPattern, trimmedLine);
    if (layoutInfo && bram) {
      layoutInfo->bram += bram;
      continue;
    }
    int io = tryExtractSubInt(ioLogPattern, trimmedLine);
    if (layoutInfo && io) {
      layoutInfo->io += io;
      continue;
    }
  }

  if (layoutInfo) { // add last item
    result.push_back(layoutInfo);
  }

  return result;
}


void QLDeviceManager::collectDeviceVariantAvailableResources(const QLDeviceVariant& device_variant) {
  CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());
  if (!compiler) {
      return;
  }

  // create command to ask vpr to spit out resource information for the variant:
  std::filesystem::path architectureFile = GetArchitectureFileForDeviceVariant(device_variant);
  if (architectureFile.empty()) {
    return;
  }

  // use a 'placeholder' blif as vpr requires a blif to run, though we don't use it in 'resource_usage' mode
  std::filesystem::path blif_filepath = std::filesystem::canonical(GlobalSession->Context()->DataPath() /
                                                                   std::filesystem::path("..") /
                                                                   std::filesystem::path("scripts") / 
                                                                   "and2.blif");

  std::string vpr_command =
      ((CompilerOpenFPGA_ql* )GlobalSession->GetCompiler())->m_vprExecutablePath.string() + std::string(" ") +
      architectureFile.string() + std::string(" ") +
      blif_filepath.string() + std::string(" ") +
      std::string("--show_resource_usage on");

  // execute vpr command
  QProcess* process = compiler->ExecuteCommand(vpr_command);

  // non-blocking: once the command executes, use the result and update the device_data structure to store the layout details:
  QObject::connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), [this, process, device_variant](int exitCode) {
    std::vector<std::shared_ptr<LayoutInfoHelper>> layoutsInfo =
        ExtractDeviceAvailableResourcesFromVprLogContent(process->readAllStandardOutput().toStdString());
    process->deleteLater();

    if (exitCode == 0) {
      for (const std::shared_ptr<LayoutInfoHelper>& layoutInfo: layoutsInfo) {
        QLDeviceVariantLayout* device_layout = findDeviceLayoutVariantPtr(device_variant.family, device_variant.foundry, device_variant.node, device_variant.voltage_threshold, device_variant.p_v_t_corner, layoutInfo->name);
        if (device_layout) {
          device_layout->bram = layoutInfo->bram;
          device_layout->dsp = layoutInfo->dsp;
          device_layout->clb = layoutInfo->clb;
          device_layout->io = layoutInfo->io;
        }
      }
      // update GUI with the acquired resource information.
      triggerUIUpdate();
    } else {
      std::cout << "Cannot fetch layout available resources. Process finished with err code " << exitCode << std::endl;
    }
  });

}

void QLDeviceManager::resetButtonClicked() {

  //device_target_selected = device_target;
  
  triggerUIUpdate();
}


void QLDeviceManager::applyButtonClicked() {

  // device_target should be set to device_target_selected
  setCurrentDeviceTarget(device_target_selected);

  triggerUIUpdate();
  
  // signal to the QLSettingsManager to update the settings JSON
  // to reflect the device_target selection!
  // change this to emit signal later...
  if(settings_manager != nullptr) {
    settings_manager->newProjectMode = newProjectMode;
    settings_manager->updateJSONSettingsForDeviceTarget(device_target);
  }
}


void QLDeviceManager::parseDeviceData() {

  std::string family;
  std::string foundry;
  std::string node;

  std::error_code ec;

  // get to the device_data dir path of the installation
  std::filesystem::path root_device_data_dir_path = 
      GlobalSession->Context()->DataPath();

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
              std::vector<QLDeviceVariant> device_variants = listDeviceVariants(family,
                                                                                foundry,
                                                                                node);

              if(device_variants.empty()) {
                // display error, but continue with other devices.
                std::cout << "error in parsing variants for device: " + family + "_" + foundry + "_" + node +"\n" << std::endl;
              }
              else {

                QLDeviceType device;
                device.family = family;
                device.foundry = foundry;
                device.node = node;
                device.device_variants = device_variants;

                for (const auto& device_variant: device.device_variants) {
                  collectDeviceVariantAvailableResources(device_variant);
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
  // for (QLDeviceType device: device_list) {
  //     std::cout << "Device: " + device.family + " " + device.foundry + " " + device.node << std::endl;
  //     for (QLDeviceVariant variant: device.device_variants) {
  //       std::cout << "  Variant: " + variant.family + " " + variant.foundry + " " + variant.node + " " + variant.voltage_threshold + " " + variant.p_v_t_corner << std::endl;
  //       for (QLDeviceVariantLayout layout: variant.device_variant_layouts) {
  //         std::cout << "    " + layout.name + " " + std::to_string(layout.width) + " " + std::to_string(layout.height) << std::endl;
  //       }
  //     }
  //     std::cout << "\n" << std::endl;
  // }
  //DEBUG
}


std::vector<QLDeviceVariant> QLDeviceManager::listDeviceVariants(
    std::string family,
    std::string foundry,
    std::string node) {

  // prep an empty list of device variants for the current 'device'
  std::vector<QLDeviceVariant> device_variants;

  // std::string device_string = DeviceString(family,
  //                                          foundry,
  //                                          node,
  //                                          "",
  //                                          "",
  //                                          "");
  // std::cout << "parsing variants for: " + device_string << std::endl;

  // get to the device_data dir path of the installation
  std::filesystem::path root_device_data_dir_path = 
     GlobalSession->Context()->DataPath();

  // calculate the device_data dir path for specified device
  std::filesystem::path device_data_dir_path = root_device_data_dir_path / family / foundry / node;
  // std::cout << "device_data dir: " + device_data_dir_path.string() << std::endl;

  // [1] check for valid path
  // convert to canonical path, which will also check that the path exists.
  std::error_code ec;
  std::filesystem::path device_data_dir_path_c = 
          std::filesystem::canonical(device_data_dir_path, ec);
  if(ec) {
    // error
    std::cout << "Please check if the path specified exists!" << std::endl;
    std::cout << "path: " + device_data_dir_path.string() << std::endl;
    return device_variants;
  }

  // [2] check dir structure of the device_data_dir_path
  // [2][a] atleast one set of vpr.xml and openfpga.xml files should exist.
  // [2][b] all xmls sets should be in directory structure as below:
  //          - device_data_dir_path/<ANY_DIR_NAME_VT>/<ANY_DIR_NAME_PVT_CORNER> (device_variants)
  //        <ANY_DIR_NAME_VT>(s) represent the Cell Threshold Voltage(s)
  //        <ANY_DIR_NAME_PVT_CORNER>(s) represent the PVT Corner(s) 
  // [2][c] check that we have all the (other)required XML files for the device
  
  // [2][a] search for all vpr.xml/openfpga.xml files, and check the dir paths:
  std::vector<std::filesystem::path> vpr_xml_files;
  std::vector<std::filesystem::path> openfpga_xml_files;
  for (const std::filesystem::directory_entry& dir_entry :
      std::filesystem::recursive_directory_iterator(device_data_dir_path_c,
                                                    std::filesystem::directory_options::skip_permission_denied,
                                                    ec)) {
    if(ec) {
      std::cout << std::string("failed listing contents of ") +
                              device_data_dir_path_c.string() << std::endl;
      return device_variants;
    }

    if(dir_entry.is_regular_file(ec)) {

      // this will match both .xml and .xml.en(encrypted) files
      std::string vpr_xml_pattern = "vpr\\.xml.*";
      std::string openfpga_xml_pattern = "openfpga\\.xml.*";
      
      if (std::regex_match(dir_entry.path().filename().string(),
                            std::regex(vpr_xml_pattern,
                              std::regex::icase))) {
        vpr_xml_files.push_back(dir_entry.path().string());
      }
      if (std::regex_match(dir_entry.path().filename().string(),
                            std::regex(openfpga_xml_pattern,
                              std::regex::icase))) {
        openfpga_xml_files.push_back(dir_entry.path().string());
      }
    }

    if(ec) {
      std::cout << std::string("error while checking: ") +  dir_entry.path().string() << std::endl;
      return device_variants;
    }
  }

  // sort the entries for easier processing
  std::sort(vpr_xml_files.begin(),vpr_xml_files.end());
  std::sort(openfpga_xml_files.begin(),openfpga_xml_files.end());

  // check that we have atleast one set.
  if(vpr_xml_files.size() == 0) {
    std::cout << "No VPR XML files were found in the source device data dir !" << std::endl;
    return device_variants;
  }
  if(openfpga_xml_files.size() == 0) {
    std::cout << "No OPENFPGA XML files were found in the source device data dir !" << std::endl;
    return device_variants;
  }

  // check that we have the same number of entries for both vpr.xml and openfpga.xml
  // as they should be travelling in pairs.
  if(vpr_xml_files.size() != openfpga_xml_files.size()) {
    std::cout << "Mismatched number of VPR XML(s) w.r.t OPENFPGA XML(s) !" << std::endl;
    return device_variants;
  }

  // [2][b] gather all the 'parent' dirs of the XMLs, and check that they are in the expected hierarchy
  std::vector<std::filesystem::path> vpr_xml_file_parent_dirs;
  std::vector<std::filesystem::path> openfpga_xml_file_parent_dirs;
  for(std::filesystem::path xmlpath : vpr_xml_files) {
    vpr_xml_file_parent_dirs.push_back(xmlpath.parent_path());
  }
  for(std::filesystem::path xmlpath : openfpga_xml_files) {
    openfpga_xml_file_parent_dirs.push_back(xmlpath.parent_path());
  }

  // sort the entries for easier processing
  std::sort(vpr_xml_file_parent_dirs.begin(),vpr_xml_file_parent_dirs.end());
  std::sort(openfpga_xml_file_parent_dirs.begin(),openfpga_xml_file_parent_dirs.end());

  // check that we have the same set of dir paths for both XMLs, as they travel in pairs.
  // redundant?
  if(vpr_xml_file_parent_dirs != openfpga_xml_file_parent_dirs) {
    std::cout << "Mismatched number of VPR XML(s) w.r.t OPENFPGA XML(s) !" << std::endl;
    return device_variants;
  }
  // now we can take any one of the file_dirs vector for further steps as they are the same.

  // debug prints
  // std::cout << "vpr xmls" << std::endl;
  // for(auto path : vpr_xml_files) std::cout << path << std::endl;
  // std::cout << std::endl;
  // std::cout << "openfpga xmls" << std::endl;
  // for(auto path : openfpga_xml_files) std::cout << path << std::endl;
  // std::cout << std::endl;
  // std::cout << "vpr xml dirs" << std::endl;
  // for(auto path : vpr_xml_file_parent_dirs) std::cout << path << std::endl;
  // std::cout << std::endl;
  // std::cout << "openfpga xml dirs" << std::endl;
  // for(auto path : openfpga_xml_file_parent_dirs) std::cout << path << std::endl;
  // std::cout << std::endl;

  // now that the dir paths for both xml(s) are identical vectors, take one of them.
  // each dir *should be* of the structure:
  // - source_device_data_dir_path/<voltage_threshold>/<p_v_t_corner> (for variants)
  //          <voltage_threshold> can be any name, usually something like LVT, RVT, ULVT ...
  //          <p_v_t_corner> can be any name, usually something like TYPICAL, BEST, WORST ...
  // from this vector, we can deduce all of the possible device variants, and check correctness of hierarchy
  for (std::filesystem::path dirpath: vpr_xml_file_parent_dirs) {

    // canonicalize to remove any trailing slashes and normalize path to full path
    std::filesystem::path dirpath_c = std::filesystem::canonical(dirpath, ec);
    if(ec) {
      // filesystem error
      return device_variants;
    }
    
    // get the dir-name component of the path, this is the p_v_t_corner
    std::string p_v_t_corner = dirpath_c.filename().string();
    
    // get the dir-name component of the parent of this path, this is the voltage_threshold
    std::string voltage_threshold = dirpath_c.parent_path().filename().string();
    
    // additionally check that p_v_t_corner dir is 2 levels down from the source_device_data_dir_path
    if(!std::filesystem::equivalent(dirpath_c.parent_path().parent_path(), device_data_dir_path_c)) {
      std::cout << dirpath_c.parent_path() << std::endl;
      std::cout << device_data_dir_path_c << std::endl;
      std::cout << "p_v_t_corner dirs with XMLs are not 2 levels down from the source_device_data_dir_path!!!" << std::endl;
      return device_variants;
    }

    // create the variant
    QLDeviceVariant device_variant;
    device_variant.family = family;
    device_variant.foundry = foundry;
    device_variant.node = node;
    device_variant.voltage_threshold = voltage_threshold;
    device_variant.p_v_t_corner = p_v_t_corner;

    // list and store all the layouts available in this device_variant:
    device_variant.device_variant_layouts = listDeviceVariantLayouts(family,
                                                                     foundry,
                                                                     node,
                                                                     voltage_threshold,
                                                                     p_v_t_corner);

    // add the variant to the list
    device_variants.push_back(device_variant);
  }

  // sort the devices found - this needs custom sort function or < overloading for QLDeviceVariant, TODO.
  // std::sort(device_variants.begin(),device_variants.end());

  // debug prints
  // std::cout << std::endl;
  // std::cout << "device variants parsed:" << std::endl;
  // std::cout << "<family>,<foundry>,<node>,[voltage_threshold],[p_v_t_corner]" << std::endl;
  // int index = 1;
  // for (auto device_variant: device_variants) {
  //   std::cout << index << ". " << device_variant << std::endl;
  //   index++;
  // }
  // std::cout << std::endl;

  // [2][c] check other required and optional XML files for the device:
  // required:
  // std::filesystem::path fixed_sim_openfpga_xml = 
  //     device_data_dir_path_c / "aurora" / "fixed_sim_openfpga.xml";
  // std::filesystem::path fixed_sim_openfpga_xml_en = 
  //     device_data_dir_path_c / "aurora" / "fixed_sim_openfpga.xml.en";
  // if(!std::filesystem::exists(fixed_sim_openfpga_xml) &&
  //    !std::filesystem::exists(fixed_sim_openfpga_xml_en)) {
  //   std::cout << "fixed_sim_openfpga.xml not found in source_device_data_dir_path!!!" << std::endl;
  //   return device_variants;
  // }

  // optional: not checking these for now, if needed we can add in later.
  //std::filesystem::path bitstream_annotation_xml = 
  //    source_device_data_dir_path_c / std::string("aurora") / "bitstream_annotation.xml";
  //std::filesystem::path repack_design_constraint_xml = 
  //    source_device_data_dir_path_c / std::string("aurora") / "repack_design_constraint.xml";
  //std::filesystem::path fabric_key_xml = 
  //    source_device_data_dir_path_c / std::string("aurora") / "fabric_key.xml";

  return device_variants;
}


std::vector<QLDeviceVariantLayout> QLDeviceManager::listDeviceVariantLayouts(std::string family,
                                                                             std::string foundry,
                                                                             std::string node,
                                                                             std::string voltage_threshold,
                                                                             std::string p_v_t_corner) {
  std::vector<QLDeviceVariantLayout> device_variant_layouts = {};
  
  std::filesystem::path root_device_data_dir_path = 
      GlobalSession->Context()->DataPath();
  
  std::filesystem::path device_data_dir_path = root_device_data_dir_path / family / foundry / node;

  std::filesystem::path device_variant_dir = device_data_dir_path / voltage_threshold / p_v_t_corner;

  
  std::filesystem::path source_vpr_xml_filepath;
  std::filesystem::path vpr_xml_filepath;

  // check for unencrypted vpr xml file first:
  source_vpr_xml_filepath = device_variant_dir / "vpr.xml";
  
  if (FileUtils::FileExists(source_vpr_xml_filepath)) {
    // use this file as is
    vpr_xml_filepath = source_vpr_xml_filepath;
  }
  else {
    // we should have an encrypted vpr xml file:
    source_vpr_xml_filepath = device_variant_dir / "vpr.xml.en";

    if (!FileUtils::FileExists(source_vpr_xml_filepath)) {
      // this means we don't have a vpr xml file, which is an error!
      std::cout << "vpr xml: " + source_vpr_xml_filepath.string() << std::endl;
      std::cout << "vpr xml not found!" << std::endl;
      ((CompilerOpenFPGA_ql* )GlobalSession->GetCompiler())->CleanTempFiles();
      return device_variant_layouts;
    }

    // decrypt the encrypted vpr xml file. and then use that:
    vpr_xml_filepath = ((CompilerOpenFPGA_ql* )GlobalSession->GetCompiler())->GenerateTempFilePath();

    std::filesystem::path m_cryptdbPath = 
        CRFileCryptProc::getInstance()->getCryptDBFileName(device_data_dir_path.string(),
                                                          family + "_" + foundry + "_" + node);

    if (!CRFileCryptProc::getInstance()->loadCryptKeyDB(m_cryptdbPath.string())) {
      std::cout << "load cryptdb failed!" << std::endl;
      ((CompilerOpenFPGA_ql* )GlobalSession->GetCompiler())->CleanTempFiles();
      return device_variant_layouts;
    }

    if (!CRFileCryptProc::getInstance()->decryptFile(source_vpr_xml_filepath, vpr_xml_filepath)) {
      std::cout << "decryption failed!" << std::endl;
      ((CompilerOpenFPGA_ql* )GlobalSession->GetCompiler())->CleanTempFiles();
      return device_variant_layouts;
    }
  }


  // open file with Qt
  // qDebug() << "vpr xml" << QString::fromStdString(vpr_xml_filepath.string());
  QFile file(vpr_xml_filepath.string().c_str());
  if (!file.open(QFile::ReadOnly)) {
    std::cout << "Cannot open file: " + vpr_xml_filepath.string() << std::endl;
    ((CompilerOpenFPGA_ql* )GlobalSession->GetCompiler())->CleanTempFiles();
    return device_variant_layouts;
  }

  // parse as XML with Qt
  QDomDocument doc;
  if (!doc.setContent(&file)) {
    file.close();
    std::cout << "Incorrect file: " + vpr_xml_filepath.string() << std::endl;
    ((CompilerOpenFPGA_ql* )GlobalSession->GetCompiler())->CleanTempFiles();
    return device_variant_layouts;
  }
  file.close();
  ((CompilerOpenFPGA_ql* )GlobalSession->GetCompiler())->CleanTempFiles(); // the decrypted file is not needed anymore.


  QDomNodeList nodes = doc.elementsByTagName("fixed_layout");
  for(int i = 0; i < nodes.count(); i++) {
      QDomNode node = nodes.at(i);
      if(node.isElement()) {

          // we have a 'layout element'
          QLDeviceVariantLayout layout;

          // get the "name" attribute for the "fixed_layout" tag element
          std::string fixed_layout_name_str = node.toElement().attribute("name", "notfound").toStdString();
          layout.name = fixed_layout_name_str;

          // get the "width" attribute for the "fixed_layout" tag element
          std::string fixed_layout_width_str = node.toElement().attribute("width", "0").toStdString();
          try {
            layout.width = std::stoi(fixed_layout_width_str);
          }
          catch (std::invalid_argument const &e) {
            std::cout << "Bad input: std::invalid_argument thrown" << std::endl;
            layout.width = 0;
          }
          catch (std::out_of_range const &e) {
            std::cout << "Integer overflow: std::out_of_range thrown" << std::endl;
            layout.width = 0;
          }

          // get the "height" attribute for the "fixed_layout" tag element
          std::string fixed_layout_height_str = node.toElement().attribute("height", "0").toStdString();
          try {
            layout.height = std::stoi(fixed_layout_height_str);
          }
          catch (std::invalid_argument const &e) {
            std::cout << "Bad input: std::invalid_argument thrown" << std::endl;
            layout.height = 0;
          }
          catch (std::out_of_range const &e) {
            std::cout << "Integer overflow: std::out_of_range thrown" << std::endl;
            layout.height = 0;
          }

          // obtain resource information for each layout using vpr is done asynchronously.

          device_variant_layouts.push_back(layout);
      }
  }

  return device_variant_layouts;
}

std::string QLDeviceManager::DeviceString(std::string family,
                                          std::string foundry,
                                          std::string node,
                                          std::string voltage_threshold,
                                          std::string p_v_t_corner,
                                          std::string layout_name) {

  // form the string representation of the device
  std::string device_string = family + "_" + foundry + "_" + node;

  if(!voltage_threshold.empty() && !p_v_t_corner.empty()) {
    device_string += "_" + voltage_threshold + "_" + p_v_t_corner;
  }

  if(!layout_name.empty()) {
    device_string += "_" + layout_name;
  }

  return device_string;
}


bool QLDeviceManager::DeviceExists(std::string family,
                                   std::string foundry,
                                   std::string node,
                                   std::string voltage_threshold,
                                   std::string p_v_t_corner,
                                   std::string layout_name) {

  // form the string representation of the device
  std::string device_string = 
      DeviceString(family,foundry,node,voltage_threshold,p_v_t_corner,layout_name);

  return DeviceExists(device_string);
}


bool QLDeviceManager::DeviceExists(std::string device_string) {

  QLDeviceTarget device_target = convertToDeviceTarget(device_string);
  if(isDeviceTargetValid(device_target)) {
       return true;
  }

  return false;
}


bool QLDeviceManager::DeviceExists(QLDeviceTarget device_target) {

  // loop through the device_list and check if a matching device exists.

  for (QLDeviceType device: device_list) {
    for (QLDeviceVariant device_variant: device.device_variants) {
      for (QLDeviceVariantLayout device_variant_layout: device_variant.device_variant_layouts) {
        if(device_target.device_variant.family            == device_variant.family &&
           device_target.device_variant.foundry           == device_variant.foundry &&
           device_target.device_variant.node              == device_variant.node &&
           device_target.device_variant.voltage_threshold == device_variant.voltage_threshold &&
           device_target.device_variant.p_v_t_corner      == device_variant.p_v_t_corner &&
           device_target.device_variant_layout.name       == device_variant_layout.name) {

          return true;
        }
      }
    }
  }

  return false;
}


QLDeviceTarget QLDeviceManager::convertToDeviceTarget(std::string family,
                                                std::string foundry,
                                                std::string node,
                                                std::string voltage_threshold,
                                                std::string p_v_t_corner,
                                                std::string layout_name) {

  // form the string representation of the device
  std::string device_string = 
      DeviceString(family,foundry,node,voltage_threshold,p_v_t_corner,layout_name);

  return convertToDeviceTarget(device_string);
}


QLDeviceTarget QLDeviceManager::convertToDeviceTarget(std::string device_string) {

  QLDeviceTarget device_target;

  // loop through the device_list and check if a matching device exists.

  for (QLDeviceType device: device_list) {
    for (QLDeviceVariant device_variant: device.device_variants) {
      for (QLDeviceVariantLayout device_variant_layout: device_variant.device_variant_layouts) {
        std::string current_device_string = DeviceString(device_variant.family,
                                                         device_variant.foundry,
                                                         device_variant.node,
                                                         device_variant.voltage_threshold,
                                                         device_variant.p_v_t_corner,
                                                         device_variant_layout.name);
        if(current_device_string == device_string) {
          device_target.device_variant_layout = device_variant_layout;
          device_target.device_variant = device_variant;
          return device_target;
        }
      }
    }
  }

  return device_target;
}

bool QLDeviceManager::isDeviceTargetValid(QLDeviceTarget device_target) {

  if(!device_target.device_variant.family.empty() && 
     !device_target.device_variant.foundry.empty() && 
     !device_target.device_variant.node.empty() && 
     !device_target.device_variant.voltage_threshold.empty() && 
     !device_target.device_variant.p_v_t_corner.empty() && 
     !device_target.device_variant_layout.name.empty()) {
       return true;
  }

  return false;
}

void QLDeviceManager::setCurrentDeviceTarget(std::string family,
                                             std::string foundry,
                                             std::string node,
                                             std::string voltage_threshold,
                                             std::string p_v_t_corner,
                                             std::string layout_name) {

  // form the string representation of the device
  std::string device_string = 
      DeviceString(family,foundry,node,voltage_threshold,p_v_t_corner,layout_name);
  
  setCurrentDeviceTarget(device_string);
}

void QLDeviceManager::setCurrentDeviceTarget(std::string device_string) {

  QLDeviceTarget device_target = convertToDeviceTarget(device_string);

  setCurrentDeviceTarget(device_target);

}


void QLDeviceManager::setCurrentDeviceTarget(QLDeviceTarget device_target) {

  // std::cout << "setCurrentDeviceTarget" << std::endl;
  // std::cout << "newProjectMode: " << newProjectMode << std::endl;
  // std::cout << "device_target: " << convertToDeviceString(device_target) << std::endl;

  if(isDeviceTargetValid(device_target)) {

    this->device_target = device_target;
  }
  else {

    // invalid target device in "device_target" - JSON is incorrect, or missing?
    QLDeviceTarget empty_device_target;
    this->device_target = empty_device_target;
  }
}


std::filesystem::path QLDeviceManager::GetArchitectureFileForDeviceVariant(const QLDeviceVariant& device_variant)
{
  std::filesystem::path architectureFile;
  std::filesystem::path device_type_dir_path =
      std::filesystem::path(GlobalSession->Context()->DataPath() /
                            device_variant.family /
                            device_variant.foundry /
                            device_variant.node);

  std::filesystem::path device_variant_dir_path =
      std::filesystem::path(GlobalSession->Context()->DataPath() /
                            device_variant.family /
                            device_variant.foundry /
                            device_variant.node /
                            device_variant.voltage_threshold /
                            device_variant.p_v_t_corner);

  // prefer to use the unencrypted file, if available.
  architectureFile =
      std::filesystem::path(device_variant_dir_path / std::string("vpr.xml"));

  // if not, use the encrypted file after decryption.
  std::error_code ec;
  if (!std::filesystem::exists(architectureFile, ec)) {

    std::filesystem::path vpr_xml_en_path =
          std::filesystem::path(device_variant_dir_path / std::string("vpr.xml.en"));
    architectureFile = ((CompilerOpenFPGA_ql* )GlobalSession->GetCompiler())->GenerateTempFilePath();

    std::filesystem::path cryptdbPath =
        CRFileCryptProc::getInstance()->getCryptDBFileName(device_type_dir_path.string(),
                                                           device_variant.family +
                                                           "_" +
                                                           device_variant.foundry +
                                                           "_" +
                                                           device_variant.node);

    if (!CRFileCryptProc::getInstance()->loadCryptKeyDB(cryptdbPath.string())) {
      std::cout << "load cryptdb failed!" << std::endl;
      return std::string("");
    }

    if (!CRFileCryptProc::getInstance()->decryptFile(vpr_xml_en_path, architectureFile)) {
      std::cout << "decryption failed!" << std::endl;
      return std::string("");
    }
  }

  //Message( std::string("Using vpr.xml for: ") + QLDeviceManager::getInstance()->convertToDeviceString(device_variant) );
  return architectureFile;
}

std::string QLDeviceManager::getCurrentDeviceTargetString() {

  return convertToDeviceString(getCurrentDeviceTarget());

}


QLDeviceTarget QLDeviceManager::getCurrentDeviceTarget() {

  return this->device_target;
}


void QLDeviceManager::triggerUIUpdate() {

  // set the selected device in the GUI:
  // newProjectMode : set the device selected in device_target_newproject
  // existingProjectMode : set the device selected in device_target_selected

  // to trigger the GUI update, we update the 'family' combobox, which cascades and
  // terminates at layoutChanged()

  if(device_manager_widget != nullptr) {
    if(newProjectMode) {
      // trigger GUI to default selection (index0 of all fields...)
      m_combobox_family->blockSignals(true);
      m_combobox_family->setCurrentIndex(-1);
      m_combobox_family->blockSignals(false);
      m_combobox_family->setCurrentIndex(0);
    }
    else {
      // existingProjectMode

      // check if the device_target contains a valid target, if so, set the ball rolling:
      if( DeviceExists(this->device_target) ) {

        int index;
        index = m_combobox_family->findText(QString::fromStdString(this->device_target.device_variant.family));
        // std::cout << "m_combobox_family index" << index << std::endl;
        m_combobox_family->blockSignals(true);
        m_combobox_family->setCurrentIndex(-1);
        m_combobox_family->blockSignals(false);
        currentDeviceTargetUpdateInProgress = true;
        m_combobox_family->setCurrentIndex(index);
      }
      else {
        // invalid device, ignore for now.
        std::cout << "existing-project, but device_target is invalid, json parse problem??" << std::endl;
      }
    }
  }
}


std::string QLDeviceManager::convertToFoundryNode(std::string foundry, std::string node) {

    std::string foundrynode = foundry + " - " + node;
    return foundrynode;
}


std::vector<std::string> QLDeviceManager::convertFromFoundryNode(std::string foundrynode) {

    std::vector<std::string> tokens;
    StringUtils::tokenize(foundrynode, " - ", tokens);
    return tokens;

    // Qt:
    // std::vector<std::string> tokens;
    // QString foundrynode_qstring = QString::fromStdString(foundrynode);
    // QRegExp delimiterExp(" - ");
    // QStringList foundrynodelist = foundrynode_qstring.split(delimiterExp);
    // for(QString token: foundrynodelist) {
    //   tokens.push_back(token.toStdString());
    // }
}

std::string QLDeviceManager::convertToDeviceString(QLDeviceTarget device_target) {

  // form the string representation of the device
  std::string device_string;

  if(isDeviceTargetValid(device_target)) {

    device_string = DeviceString(device_target.device_variant.family ,
                                 device_target.device_variant.foundry,
                                 device_target.device_variant.node,
                                 device_target.device_variant.voltage_threshold,
                                 device_target.device_variant.p_v_t_corner,
                                 device_target.device_variant_layout.name);
  }

  return device_string;

}

// encryptDevice-> take input device data -> produce encrypted version
// addDevice -> call encryptDevice -> take encrypted version -> copy into aurora installation

int QLDeviceManager::encryptDevice(std::string family, std::string foundry, std::string node,
                                     std::string device_data_source, std::string device_data_target) {

    CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

    // encrypt_device <family> <foundry> <node> <source_device_data_dir_path> <target_device_data_dir_path>
    // 1. ensure that the structure in the <source_device_data_dir_path> reflects 
    //      required structure, as specified in the document: <TODO>
    //    basically, all the required files should exist, in the right hierarchy,
    //      and missing optional files would output a warning.
    // 2. encrypt all the files in the <source_device_data_dir_path> in place
    // 3. copy over all the encrypted files & cryption db
    //      from: <source_device_data_dir_path>
    //      to: <INSTALLATION> / device_data / <family> / <foundry> / <node>
    //      and clean up all the encrypted files & cryption db from the <source_device_data_dir_path>


    std::filesystem::path source_device_data_dir_path = device_data_source;

    std::string device = QLDeviceManager::getInstance()->DeviceString(family,
                                                                      foundry,
                                                                      node,
                                                                      "",
                                                                      "",
                                                                      "");

    // convert to canonical path, which will also check that the path exists.
    std::error_code ec;
    std::filesystem::path source_device_data_dir_path_c = 
            std::filesystem::canonical(source_device_data_dir_path, ec);
    if(ec) {
      // error
      compiler->ErrorMessage("Please check if the path specified exists!");
      compiler->ErrorMessage("path: " + source_device_data_dir_path.string());
      return -1;
    }

    std::filesystem::path target_device_data_dir_path;
    // if unspecified, default destination for the encrypted files will be in 
    // a dir parallel to the source dir, with "_en" added to the name of the source dir.
    if (device_data_target.empty()) {
      std::string target_device_data_dir_name = source_device_data_dir_path_c.filename().string() + "_en";
      target_device_data_dir_path = source_device_data_dir_path_c / ".." / target_device_data_dir_name;
    }
    else {
      target_device_data_dir_path = device_data_target;
    }

    // debug prints
    // std::cout << std::endl;
    // std::cout << "family: " << family << std::endl;
    // std::cout << "foundry: " << foundry << std::endl;
    // std::cout << "node: " << node << std::endl;
    // std::cout << "source_device_data_dir_path: " << source_device_data_dir_path_c << std::endl;
    // std::cout << "force: " << std::string(force?"true":"false") << std::endl;
    // std::cout << std::endl;


    if (std::filesystem::exists(target_device_data_dir_path, ec)) {
        compiler->Message("\nWARNING: The target device data dir already exists.");
        compiler->Message("device:      " + device);
        compiler->Message("target path: " + target_device_data_dir_path.string());
        compiler->Message("this will overwrite the target device dir with new files.");
        compiler->Message("\n");
    }
    else {
        compiler->Message("\nNew Device files will be created.");
        compiler->Message("device:      " + device);
        compiler->Message("target path: " + target_device_data_dir_path.string());
        compiler->Message("\n");
    }


    // [2] check dir structure of the source_device_data_dir_path of the device to be added
    // and return the list of device_variants if everything is ok.
    std::vector<std::string> device_variants;
    device_variants = compiler->list_device_variants(family,
                                                     foundry,
                                                     node,
                                                     source_device_data_dir_path_c);

    if(device_variants.empty()) {
      compiler->ErrorMessage(std::string("error parsing device_data in: ") +
                               source_device_data_dir_path_c.string());
        return -1;
    }
    else {
      // save std::ios settings.
      std::ios ios_default_state(nullptr);
      ios_default_state.copyfmt(std::cout);

      std::cout << std::endl;
      std::cout << "device variants parsed:" << std::endl;
      std::cout << "<family>,<foundry>,<node>,[voltage_threshold],[p_v_t_corner]" << std::endl;
      int index = 1;
      for (auto device_variant: device_variants) {
        std::cout << std::setw(4)
                  << std::setfill(' ')
                  << index;
        // restore cout state
        std::cout.copyfmt(ios_default_state);
        std::cout << ". " 
                  << device_variant 
                  << std::endl;
        index++;
      }
      std::cout << std::endl;
    }

    // collect the list of every filepath in the source_device_data_dir that we want to encrypt.
    std::vector<std::filesystem::path> source_device_data_file_list_to_encrypt;
    std::vector<std::filesystem::path> source_device_data_file_list_to_copy;
    for (const std::filesystem::directory_entry& dir_entry :
        std::filesystem::recursive_directory_iterator(source_device_data_dir_path_c,
                                                      std::filesystem::directory_options::skip_permission_denied,
                                                      ec))
    {
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed listing contents of ") +  source_device_data_dir_path.string());
        return -1;
      }

      if(dir_entry.is_regular_file(ec)) {

          // skip entries which are in specific directories in device data:
          // skip '/extra/' or '\extra\'
          if (std::regex_match(dir_entry.path().string(),
                                std::regex(R"(.+[\/\\]extra[\/\\].*)",
                                std::regex::icase))) {
            continue;
          }

          //  to skip '/examples/' or '\examples\' ? skip in future
          // if (std::regex_match(dir_entry.path().string(),
          //                       std::regex(R"(.+[\/\\]examples[\/\\].*)",
          //                       std::regex::icase))) {
          //   continue;
          // }

          // we want xml files for encryption
          if (std::regex_match(dir_entry.path().filename().string(),
                                std::regex(".+\\.xml",
                                std::regex::icase))) {
            // exclude fpga_io_map xml files from encryption
            // include them for copy
            if (std::regex_match(dir_entry.path().filename().string(),
                                  std::regex(".*fpga_io_map\\.xml",
                                  std::regex::icase))) {
              source_device_data_file_list_to_copy.push_back(dir_entry.path().string());
              continue;
            }
            source_device_data_file_list_to_encrypt.push_back(dir_entry.path().string());
          }

          // include pin_table csv files for copy
          if (std::regex_match(dir_entry.path().filename().string(),
                                std::regex(".*pin_table\\.csv",
                                std::regex::icase))) {
            source_device_data_file_list_to_copy.push_back(dir_entry.path().string());
          }

          // include template json files for copy
          if (std::regex_match(dir_entry.path().filename().string(),
                                std::regex(".+_template\\.json",
                                std::regex::icase))) {
            source_device_data_file_list_to_copy.push_back(dir_entry.path().string());
          }

          // include yosys template script for copy (aurora_template_script.ys)
          if (std::regex_match(dir_entry.path().filename().string(),
                                std::regex("aurora_template_script\\.ys",
                                std::regex::icase))) {
            source_device_data_file_list_to_copy.push_back(dir_entry.path().string());
          }

          // include openfpga template script for copy (aurora_template_script.openfpga)
          if (std::regex_match(dir_entry.path().filename().string(),
                                std::regex("aurora_template_script\\.openfpga",
                                std::regex::icase))) {
            source_device_data_file_list_to_copy.push_back(dir_entry.path().string());
          }

          // include verilog files for copy (cells_sim.v etc.)
          if (std::regex_match(dir_entry.path().filename().string(),
                                std::regex(".+\\.v",
                                std::regex::icase))) {
            source_device_data_file_list_to_copy.push_back(dir_entry.path().string());
          }

          // include system verilog files for copy (cells_sim.sv etc.)
          if (std::regex_match(dir_entry.path().filename().string(),
                                std::regex(".+\\.sv",
                                std::regex::icase))) {
            source_device_data_file_list_to_copy.push_back(dir_entry.path().string());
          }

          // include txt files for copy (brams.txt etc.)
          if (std::regex_match(dir_entry.path().filename().string(),
                                std::regex(".+\\.txt",
                                std::regex::icase))) {
            source_device_data_file_list_to_copy.push_back(dir_entry.path().string());
          }

          // include json file for copy (config.json etc.)
          if (std::regex_match(dir_entry.path().filename().string(),
                                std::regex(".+\\.json",
                                std::regex::icase))) {
            source_device_data_file_list_to_copy.push_back(dir_entry.path().string());
          }

          // include md5 file for copy (SOFTWARE_RELEASE.md5 etc.)
          if (std::regex_match(dir_entry.path().filename().string(),
                                std::regex(".+\\.md5",
                                std::regex::icase))) {
            source_device_data_file_list_to_copy.push_back(dir_entry.path().string());
          }
      }

      if(ec) {
        compiler->ErrorMessage(std::string("error while checking: ") +  dir_entry.path().string());
        return -1;
      }
    }

    // debug prints
    // std::sort(source_device_data_file_list_to_encrypt.begin(),source_device_data_file_list_to_encrypt.end());
    // std::cout << "source_device_data_file_list_to_encrypt" << std::endl;
    // for(auto path : source_device_data_file_list_to_encrypt) std::cout << path << std::endl;
    // std::cout << std::endl;

    // encrypt the list of files
    if (!CRFileCryptProc::getInstance()->encryptFiles(source_device_data_file_list_to_encrypt)) {
        compiler->ErrorMessage("encrypt files failed!");
        return -1;
    } else {
        compiler->Message("files encrypted ok.");
    }

    // save cryptdb
    string cryptdb_path_str;
    if (!CRFileCryptProc::getInstance()->saveCryptKeyDB(source_device_data_dir_path_c.string(), 
                                                        family + "_" + foundry + "_" + node,
                                                        cryptdb_path_str)) {
        compiler->ErrorMessage("cryptdb save failed!");
        return -1;
    }
    else {
        compiler->Message("cryptdb saved ok.");
    }

    // [4] copy all encrypted files and cryptdb into the installation target_device_data_dir_path
    //     also, cleanup these files in the source_device_data_dir_path

    // delete the target_device_data_dir_path directory in the installation
    //   so that, we don't have a mix of old remnants and new files.
    std::filesystem::remove_all(target_device_data_dir_path,
                                ec);
    if(ec) {
      // error
      compiler->ErrorMessage(std::string("failed to delete target dir: ") + target_device_data_dir_path.string());
      return -1;
    }

    // create the target_device_data_dir_path directory in the installation
    std::filesystem::create_directories(target_device_data_dir_path,
                                        ec);
    if(ec) {
      // error
      compiler->ErrorMessage(std::string("failed to create target dir: ") + target_device_data_dir_path.string());
      return -1;
    }

    // pass through the list of files we prepared earlier for encryption and process each one
    for(std::filesystem::path source_file_path : source_device_data_file_list_to_encrypt) {

      // corresponding encrypted file path
      std::filesystem::path source_en_file_path = 
          std::filesystem::path(source_file_path.string() + ".en");

      // get the encrypted file path, relative to the source_device_data_dir_path
      std::filesystem::path relative_en_file_path = 
          std::filesystem::relative(source_en_file_path,
                                    source_device_data_dir_path_c,
                                    ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to create relative path: ") + source_en_file_path.string());
        return -1;
      }

      // add the relative encrypted file path to the target_device_data_dir_path
      std::filesystem::path target_en_file_path = 
          target_device_data_dir_path / relative_en_file_path;

      // ensure that the target encrypted file's parent dir is created if not existing:
      std::filesystem::create_directories(target_en_file_path.parent_path(),
                                          ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to create directory: ") + target_en_file_path.parent_path().string());
        return -1;
      }

      // copy the source encrypted file to the target encrypted file path:
      std::cout << "copying:" << relative_en_file_path << std::endl;
      std::filesystem::copy_file(source_en_file_path,
                                 target_en_file_path,
                                 std::filesystem::copy_options::overwrite_existing,
                                 ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to copy: ") + source_en_file_path.string());
        return -1;
      }

      // delete the source encrypted file, as it not needed anymore.
      std::filesystem::remove(source_en_file_path,
                              ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to delete: ") + source_en_file_path.string());
        return -1;
      }
    }

    // now copy the cryptdb file into the installation and delete from the source
    std::filesystem::path source_cryptdb_path = cryptdb_path_str;

    // get the cryptdb file path, relative to the source_device_data_dir_path
    std::filesystem::path relative_cryptdb_path =
        std::filesystem::relative(source_cryptdb_path,
                                  source_device_data_dir_path_c,
                                  ec);
    if(ec) {
      // error
      compiler->ErrorMessage(std::string("failed to create relative path: ") + source_cryptdb_path.string());
        return -1;
    }

    // add the relative encrypted file path to the target_device_data_dir_path
    std::filesystem::path target_cryptdb_path =
        target_device_data_dir_path / relative_cryptdb_path;

    // copy the source cryptdb file to the target cryptdb file path:
    std::cout << "copying:" << relative_cryptdb_path << std::endl;
    std::filesystem::copy_file(source_cryptdb_path,
                               target_cryptdb_path,
                               std::filesystem::copy_options::overwrite_existing,
                               ec);
    if(ec) {
      // error
      compiler->ErrorMessage(std::string("failed to copy: ") + source_cryptdb_path.string());
      return -1;
    }

    // delete the source encrypted file, as it not needed anymore.
    std::filesystem::remove(source_cryptdb_path,
                            ec);
    if(ec) {
      // error
      compiler->ErrorMessage(std::string("failed to delete: ") + source_cryptdb_path.string());
      return -1;
    }

    // pass through the list of files we prepared earlier for copying without encryption and process each one
    for(std::filesystem::path source_file_path : source_device_data_file_list_to_copy) {

      // get the file path, relative to the source_device_data_dir_path
      std::filesystem::path relative_file_path = 
          std::filesystem::relative(source_file_path,
                                    source_device_data_dir_path_c,
                                    ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to create relative path: ") + source_file_path.string());
        return -1;
      }

      // add the relative file path to the target_device_data_dir_path
      std::filesystem::path target_file_path = 
          target_device_data_dir_path / relative_file_path;

      // ensure that the target file's parent dir is created if not existing:
      std::filesystem::create_directories(target_file_path.parent_path(),
                                          ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to create directory: ") + target_file_path.parent_path().string());
        return -1;
      }

      // copy the source file to the target file path:
      std::cout << "copying:" << relative_file_path << std::endl;
      std::filesystem::copy_file(source_file_path,
                                 target_file_path,
                                 std::filesystem::copy_options::overwrite_existing,
                                 ec);
      if(ec) {
        // error
        compiler->ErrorMessage(std::string("failed to copy: ") + source_file_path.string());
        return -1;
      }
    }

    compiler->Message("\ndevice encrypted ok: " + device);
    compiler->Message("\ntarget path: " + target_device_data_dir_path.string());

    return 0;
  }


int QLDeviceManager::addDevice(std::string family, std::string foundry, std::string node,
                                 std::string device_data_source, bool force) {

    // add_device <family> <foundry> <node> <source_device_data_dir_path> [force]
    // this will perform the steps:
    // 1. check if the 'device' already exists in the installation
    //      check if the '<INSTALLATION> / device_data / <family> / <foundry> / <node>' dir path
    //        already exists in installation
    //      if it already exists, we will display an error, and stop.
    //      if 'force' has been specified, we will push out a warning, but proceed further.
    // 2. call encryptDevice() to ensure the source structure is ok, and to encrypt and copy the device to the 
    //    target device data installation dir

    CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

    std::filesystem::path source_device_data_dir_path = device_data_source;

    std::string device = QLDeviceManager::getInstance()->DeviceString(family,
                                                                      foundry,
                                                                      node,
                                                                      "",
                                                                      "",
                                                                      "");

    // convert to canonical path, which will also check that the path exists.
    std::error_code ec;
    std::filesystem::path source_device_data_dir_path_c = 
            std::filesystem::canonical(source_device_data_dir_path, ec);
    if(ec) {
      // error
      compiler->ErrorMessage("Please check if the path specified exists!");
      compiler->ErrorMessage("path: " + source_device_data_dir_path.string());
      return -1;
    }

    // debug prints
    // std::cout << std::endl;
    // std::cout << "family: " << family << std::endl;
    // std::cout << "foundry: " << foundry << std::endl;
    // std::cout << "node: " << node << std::endl;
    // std::cout << "source_device_data_dir_path: " << source_device_data_dir_path_c << std::endl;
    // std::cout << "force: " << std::string(force?"true":"false") << std::endl;
    // std::cout << std::endl;

    // [1] check if installation already has the device added and inform the user accordingly.
    //     (device data dir for this device already exists)
    std::filesystem::path target_device_data_dir_path = 
        std::filesystem::path(compiler->GetSession()->Context()->DataPath() /
                              family /
                              foundry /
                              node);

    if (std::filesystem::exists(target_device_data_dir_path, ec)) {
      if(force) {
        compiler->Message("\nWARNING: The device you are trying to add already exists in the installation.");
        compiler->Message("device:      " + device);
        compiler->Message("target path: " + target_device_data_dir_path.string());
        compiler->Message("'force' has been specified, this will overwrite the target device dir with new files.");
        compiler->Message("\n");
      }
      else {
        compiler->Message("\n");
        compiler->ErrorMessage("The device you are trying to add already exists in the installation.");
        compiler->Message("device:      " + device);
        compiler->Message("target path: " + target_device_data_dir_path.string());
        compiler->Message("Please specify 'force' to overwrite the target device dir with new files.");
        compiler->Message("Please enter command in the format:\n"
                          "    add_device <family> <foundry> <node> <source_device_data_dir_path> [force]");
        compiler->Message("\n");
        return -1;
      }
    }
    else {
        //Message("\nNew Device files will be added to the installation.");
        //Message("device:      " + device);
        //Message("target path: " + target_device_data_dir_path.string());
        //Message("\n");
    }

    int status = encryptDevice(family, foundry, node,
                               device_data_source, target_device_data_dir_path.string());

    if(status == 0) {
      compiler->Message("\ndevice added ok: " + device);
      return 0;
    }

    compiler->ErrorMessage("\nadd device failed: " + device);
    return status;

  }


bool QLDeviceManager::deviceFileIsEncrypted(std::filesystem::path filepath) {

  if(filepath.extension() == ".en") {
    return true;
  }
  else {
    return false;
  }

}


std::filesystem::path QLDeviceManager::deviceTypeDirPath(QLDeviceTarget device_target) {

  CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

  std::filesystem::path device_type_dir_path;

  if( !isDeviceTargetValid(device_target) ) {
    device_target = this->device_target;
  }

  device_type_dir_path = 
      std::filesystem::path(compiler->GetSession()->Context()->DataPath() /
                            device_target.device_variant.family /
                            device_target.device_variant.foundry /
                            device_target.device_variant.node);
  
  return device_type_dir_path;
}


std::filesystem::path QLDeviceManager::deviceVariantDirPath(QLDeviceTarget device_target) {

  CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

  std::filesystem::path device_variant_dir_path;

  if( !isDeviceTargetValid(device_target) ) {
    device_target = this->device_target;
  }

  device_variant_dir_path =
      std::filesystem::path(compiler->GetSession()->Context()->DataPath() /
                            device_target.device_variant.family /
                            device_target.device_variant.foundry /
                            device_target.device_variant.node /
                            device_target.device_variant.voltage_threshold /
                            device_target.device_variant.p_v_t_corner);

  return device_variant_dir_path;
}


std::filesystem::path QLDeviceManager::deviceYosysScriptFile(QLDeviceTarget device_target) {

  CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

  std::filesystem::path empty_path;
  std::filesystem::path aurora_template_script_yosys_path;

  if( !isDeviceTargetValid(device_target) ) {
    device_target = this->device_target;
  }

  // use the device specific yosys script
  // -- hardcoded path --
  // aurora_template_script_yosys_path = 
  //     std::filesystem::path(deviceTypeDirPath(device_target) / std::string("aurora") / std::string("aurora_template_script.ys"));
  // use config.json if it exists
  std::filesystem::path device_target_config_json_filepath = deviceTypeDirPath(device_target) / std::string("config.json");
  if(FileUtils::FileExists(device_target_config_json_filepath)) {

    std::ifstream device_target_config_json_ifstream(device_target_config_json_filepath.string());
    json device_target_config_json = json::parse(device_target_config_json_ifstream);
    // get json value
    std::string json_value;
    if( device_target_config_json.contains("AURORA_YOSYS_TEMPLATE_SCRIPT")  ) {
      json_value = device_target_config_json["AURORA_YOSYS_TEMPLATE_SCRIPT"].get<std::string>();
    }
    aurora_template_script_yosys_path = 
        deviceTypeDirPath(device_target) / json_value;
  }
  // else, we assume that this is a legacy device data directory (< v2.8.0)
  else {

    aurora_template_script_yosys_path = 
        std::filesystem::path(deviceTypeDirPath(device_target) / std::string("aurora_template_script.ys"));
  }

  std::cout << "[zyxw]" << "using ys template: " << aurora_template_script_yosys_path.string() << std::endl;

  if(!FileUtils::FileExists(aurora_template_script_yosys_path)) {

    compiler->ErrorMessage("Cannot find device Yosys Template Script: " + aurora_template_script_yosys_path.string());
    return empty_path;
  }

  return aurora_template_script_yosys_path;
}


std::filesystem::path QLDeviceManager::deviceSettingsTemplateFile(QLDeviceTarget device_target) {

  std::filesystem::path empty_path;
  return empty_path;

}


std::filesystem::path QLDeviceManager::devicePowerTemplateFile(QLDeviceTarget device_target) {

  std::filesystem::path empty_path;
  return empty_path;

}


std::filesystem::path QLDeviceManager::deviceOpenFPGAScriptFile(QLDeviceTarget device_target) {

  CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

  std::filesystem::path empty_path;
  std::filesystem::path aurora_template_script_openfpga_path;

  if( !isDeviceTargetValid(device_target) ) {
    device_target = this->device_target;
  }

  // use the device specific openfpga script

  // -- hardcoded path --
  // aurora_template_script_openfpga_path = 
  //     std::filesystem::path(deviceTypeDirPath(device_target) / std::string("aurora") / std::string("aurora_template_script.openfpga"));
  // use config.json if it exists
  std::filesystem::path device_target_config_json_filepath = deviceTypeDirPath(device_target) / std::string("config.json");
  if(FileUtils::FileExists(device_target_config_json_filepath)) {

    std::ifstream device_target_config_json_ifstream(device_target_config_json_filepath.string());
    json device_target_config_json = json::parse(device_target_config_json_ifstream);
    // get json value
    std::string json_value;
    if( device_target_config_json.contains("AURORA_OPENFPGA_TEMPLATE_SCRIPT")  ) {
      json_value = device_target_config_json["AURORA_OPENFPGA_TEMPLATE_SCRIPT"].get<std::string>();
    }
    aurora_template_script_openfpga_path = 
        deviceTypeDirPath(device_target) / json_value;
  }
  // else, we assume that this is a legacy device data directory (< v2.8.0)
  else {
    aurora_template_script_openfpga_path = 
        std::filesystem::path(compiler->GetSession()->Context()->DataPath() /
                              std::string("..") /
                              std::string("scripts") /
                              std::string("aurora_template_script.openfpga"));
  }

  std::cout << "[zyxw]" << "using openfpga template: " << aurora_template_script_openfpga_path.string() << std::endl;

  if(!FileUtils::FileExists(aurora_template_script_openfpga_path)) {

    compiler->ErrorMessage("Cannot find device OpenFPGA Template Script: " + aurora_template_script_openfpga_path.string());
    return empty_path;
  }

  return aurora_template_script_openfpga_path;
}


std::filesystem::path QLDeviceManager::deviceVPRArchitectureFile(QLDeviceTarget device_target) {

  CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

  std::filesystem::path empty_path;
  std::filesystem::path vpr_architecture_file_path;

  if( !isDeviceTargetValid(device_target) ) {
    device_target = this->device_target;
  }

  // use the device specific vpr architecture file, and note that we may have
  // unencrypted (first priority) or encrypted file

  // use config.json if it exists
  std::filesystem::path device_target_config_json_filepath = deviceTypeDirPath(device_target) / std::string("config.json");
  if(FileUtils::FileExists(device_target_config_json_filepath)) {

    std::ifstream device_target_config_json_ifstream(device_target_config_json_filepath.string());
    json device_target_config_json = json::parse(device_target_config_json_ifstream);
    // get json value
    std::string json_value;
    if( device_target_config_json.contains("CORNER_VPR_ARCH")  ) {

      json_value = device_target_config_json["CORNER_VPR_ARCH"].get<std::string>();
    }
    // check for unencrypted file
    vpr_architecture_file_path = 
        deviceVariantDirPath(device_target) / json_value;
    if(!FileUtils::FileExists(vpr_architecture_file_path)) {

      // check for encrypted file
      vpr_architecture_file_path += ".en";
      if(!FileUtils::FileExists(vpr_architecture_file_path)) {

        compiler->ErrorMessage("Cannot find device vpr architecture file: " + vpr_architecture_file_path.string());
        return empty_path;
      }
    }
  }
  // else, we assume that this is a legacy device data directory (< v2.8.0)
  else {
    // check for unencrypted file
    vpr_architecture_file_path = 
        std::filesystem::path(deviceVariantDirPath(device_target) / std::string("vpr.xml"));
    if(!FileUtils::FileExists(vpr_architecture_file_path)) {

      // check for encrypted file
      vpr_architecture_file_path += ".en";
      if(!FileUtils::FileExists(vpr_architecture_file_path)) {

        compiler->ErrorMessage("Cannot find device vpr architecture file: " + vpr_architecture_file_path.string());
        return empty_path;
      }
    }
  }

  std::cout << "[zyxw]" << "using vpr arch file: " << vpr_architecture_file_path.string() << std::endl;

  return vpr_architecture_file_path;
}


std::filesystem::path QLDeviceManager::deviceOpenFPGAArchitectureFile(QLDeviceTarget device_target) {

  CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

  std::filesystem::path empty_path;
  std::filesystem::path openfpga_architecture_file_path;

  if( !isDeviceTargetValid(device_target) ) {
    device_target = this->device_target;
  }

  // use the device specific openfpga architecture file, and note that we may have
  // unencrypted (first priority) or encrypted file

  // use config.json if it exists
  std::filesystem::path device_target_config_json_filepath = deviceTypeDirPath(device_target) / std::string("config.json");
  if(FileUtils::FileExists(device_target_config_json_filepath)) {

    std::ifstream device_target_config_json_ifstream(device_target_config_json_filepath.string());
    json device_target_config_json = json::parse(device_target_config_json_ifstream);
    // get json value
    std::string json_value;
    if( device_target_config_json.contains("CORNER_OPENFPGA_ARCH")  ) {

      json_value = device_target_config_json["CORNER_OPENFPGA_ARCH"].get<std::string>();
    }
    // check for unencrypted file
    openfpga_architecture_file_path = 
        deviceVariantDirPath(device_target) / json_value;
    if(!FileUtils::FileExists(openfpga_architecture_file_path)) {

      // check for encrypted file
      openfpga_architecture_file_path += ".en";
      if(!FileUtils::FileExists(openfpga_architecture_file_path)) {

        compiler->ErrorMessage("Cannot find device openfpga architecture file: " + openfpga_architecture_file_path.string());
        return empty_path;
      }
    }
  }
  // else, we assume that this is a legacy device data directory (< v2.8.0)
  else {
    // check for unencrypted file
    openfpga_architecture_file_path = 
        std::filesystem::path(deviceVariantDirPath(device_target) / std::string("openfpga.xml"));
    if(!FileUtils::FileExists(openfpga_architecture_file_path)) {

      // check for encrypted file
      openfpga_architecture_file_path += ".en";
      if(!FileUtils::FileExists(openfpga_architecture_file_path)) {

        compiler->ErrorMessage("Cannot find device openfpga architecture file: " + openfpga_architecture_file_path.string());
        return empty_path;
      }
    }
  }

  std::cout << "[zyxw]" << "using openfpga arch file: " << openfpga_architecture_file_path.string() << std::endl;

  return openfpga_architecture_file_path;
}


std::filesystem::path QLDeviceManager::deviceOpenFPGABitstreamAnnotationFile(QLDeviceTarget device_target) {

  CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

  std::filesystem::path empty_path;
  std::filesystem::path bitstream_annotation_file_path;

  if( !isDeviceTargetValid(device_target) ) {
    device_target = this->device_target;
  }

  // use the device specific bitstream annotation file, and note that we may have
  // unencrypted (first priority) or encrypted file

  // use config.json if it exists
  std::filesystem::path device_target_config_json_filepath = deviceTypeDirPath(device_target) / std::string("config.json");
  if(FileUtils::FileExists(device_target_config_json_filepath)) {

    std::ifstream device_target_config_json_ifstream(device_target_config_json_filepath.string());
    json device_target_config_json = json::parse(device_target_config_json_ifstream);
    // get json value
    std::string json_value;
    if( device_target_config_json.contains("BITSTREAM_ANNOTATION")  ) {

      json_value = device_target_config_json["BITSTREAM_ANNOTATION"].get<std::string>();
    }
    // check for unencrypted file
    bitstream_annotation_file_path = 
        deviceTypeDirPath(device_target) / json_value;
    if(!FileUtils::FileExists(bitstream_annotation_file_path)) {

      // check for encrypted file
      bitstream_annotation_file_path += ".en";
      if(!FileUtils::FileExists(bitstream_annotation_file_path)) {

        compiler->ErrorMessage("Cannot find device bitstream annotation file: " + bitstream_annotation_file_path.string());
        return empty_path;
      }
    }
  }
  // else, we assume that this is a legacy device data directory (< v2.8.0)
  else {
    // check for unencrypted file
    bitstream_annotation_file_path = 
        std::filesystem::path(deviceTypeDirPath(device_target) / std::string("bitstream_annotation.xml"));
    if(!FileUtils::FileExists(bitstream_annotation_file_path)) {

      // check for encrypted file
      bitstream_annotation_file_path += ".en";
      if(!FileUtils::FileExists(bitstream_annotation_file_path)) {

        compiler->ErrorMessage("Cannot find device bitstream annotation file: " + bitstream_annotation_file_path.string());
        return empty_path;
      }
    }
  }

  std::cout << "[zyxw]" << "using bitstream annotation file: " << bitstream_annotation_file_path.string() << std::endl;

  return bitstream_annotation_file_path;
}


std::filesystem::path QLDeviceManager::deviceOpenFPGARepackDesignConstraintFile(QLDeviceTarget device_target) {

  CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

  std::filesystem::path empty_path;
  std::filesystem::path repack_design_constraint_file_path;

  if( !isDeviceTargetValid(device_target) ) {
    device_target = this->device_target;
  }

  // use the device specific repack design contraint file, and note that we may have
  // unencrypted (first priority) or encrypted file

  // use config.json if it exists
  std::filesystem::path device_target_config_json_filepath = deviceTypeDirPath(device_target) / std::string("config.json");
  if(FileUtils::FileExists(device_target_config_json_filepath)) {

    std::ifstream device_target_config_json_ifstream(device_target_config_json_filepath.string());
    json device_target_config_json = json::parse(device_target_config_json_ifstream);
    // get json value
    std::string json_value;
    if( device_target_config_json.contains("REPACK_DESIGN_CONSTRAINT")  ) {

      json_value = device_target_config_json["REPACK_DESIGN_CONSTRAINT"].get<std::string>();
    }
    // check for unencrypted file
    repack_design_constraint_file_path = 
        deviceTypeDirPath(device_target) / json_value;
    if(!FileUtils::FileExists(repack_design_constraint_file_path)) {

      // check for encrypted file
      repack_design_constraint_file_path += ".en";
      if(!FileUtils::FileExists(repack_design_constraint_file_path)) {

        compiler->ErrorMessage("Cannot find device repack design contraint file: " + repack_design_constraint_file_path.string());
        return empty_path;
      }
    }
  }
  // else, we assume that this is a legacy device data directory (< v2.8.0)
  else {
    // check for unencrypted file
    repack_design_constraint_file_path = 
        std::filesystem::path(deviceTypeDirPath(device_target) / std::string("repack_design_constraint.xml"));
    if(!FileUtils::FileExists(repack_design_constraint_file_path)) {

      // check for encrypted file
      repack_design_constraint_file_path += ".en";
      if(!FileUtils::FileExists(repack_design_constraint_file_path)) {

        compiler->ErrorMessage("Cannot find device repack design contraint file: " + repack_design_constraint_file_path.string());
        return empty_path;
      }
    }
  }

  std::cout << "[zyxw]" << "using repack design contraint file: " << repack_design_constraint_file_path.string() << std::endl;

  return repack_design_constraint_file_path;
}


std::filesystem::path QLDeviceManager::deviceOpenFPGAFixedSimFile(QLDeviceTarget device_target) {

  CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

  std::filesystem::path empty_path;
  std::filesystem::path fixed_sim_file_path;

  if( !isDeviceTargetValid(device_target) ) {
    device_target = this->device_target;
  }

  // use the device specific fixed sim file, and note that we may have
  // unencrypted (first priority) or encrypted file

  // use config.json if it exists
  std::filesystem::path device_target_config_json_filepath = deviceTypeDirPath(device_target) / std::string("config.json");
  if(FileUtils::FileExists(device_target_config_json_filepath)) {

    std::ifstream device_target_config_json_ifstream(device_target_config_json_filepath.string());
    json device_target_config_json = json::parse(device_target_config_json_ifstream);
    // get json value
    std::string json_value;
    if( device_target_config_json.contains("FIXED_SIM_OPENFPGA")  ) {

      json_value = device_target_config_json["FIXED_SIM_OPENFPGA"].get<std::string>();
    }
    // check for unencrypted file
    fixed_sim_file_path = 
        deviceTypeDirPath(device_target) / json_value;
    if(!FileUtils::FileExists(fixed_sim_file_path)) {

      // check for encrypted file
      fixed_sim_file_path += ".en";
      if(!FileUtils::FileExists(fixed_sim_file_path)) {

        compiler->ErrorMessage("Cannot find device fixed sim file: " + fixed_sim_file_path.string());
        return empty_path;
      }
    }
  }
  // else, we assume that this is a legacy device data directory (< v2.8.0)
  else {
    // check for unencrypted file
    fixed_sim_file_path = 
        std::filesystem::path(deviceTypeDirPath(device_target) / std::string("fixed_sim_openfpga.xml"));
    if(!FileUtils::FileExists(fixed_sim_file_path)) {

      // check for encrypted file
      fixed_sim_file_path += ".en";
      if(!FileUtils::FileExists(fixed_sim_file_path)) {

        compiler->ErrorMessage("Cannot find device fixed sim file: " + fixed_sim_file_path.string());
        return empty_path;
      }
    }
  }

  std::cout << "[zyxw]" << "using fixed sim file: " << fixed_sim_file_path.string() << std::endl;

  return fixed_sim_file_path;
}


std::filesystem::path QLDeviceManager::deviceOpenFPGAFabricKeyFile(QLDeviceTarget device_target) {

  // CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

  std::filesystem::path empty_path;
  std::filesystem::path fabric_key_file_path;

  if( !isDeviceTargetValid(device_target) ) {
    device_target = this->device_target;
  }

  // use the device specific fabric key file, and note that we may have
  // unencrypted (first priority) or encrypted file

  // this is a bit of a special handling case, because we still want to support
  // multiple layouts in a single device for tsmc16 until we decide to change it
  // to become multiple devices instead.
  // so, we will not rely **only** on the 'config.json', but will actually do:
  // 1. (new structure) if config.json found, find fabric file as : "aurora/layoutname_fabric_key.xml", or +.en
  // 2. (new structure) if layoutname_fabric_key.xml.* is not found, find fabric file using config.json value
  // 3. (legacy)if config.json not found, find fabric file as fabric_key/family_foundry_node_vt_corner_layoutname.xml or +.en


  // check config.json if it exists
  std::filesystem::path device_target_config_json_filepath = deviceTypeDirPath(device_target) / std::string("config.json");
  if(FileUtils::FileExists(device_target_config_json_filepath)) {

    std::ifstream device_target_config_json_ifstream(device_target_config_json_filepath.string());
    json device_target_config_json = json::parse(device_target_config_json_ifstream);
    // get json value
    std::string json_value;
    if( device_target_config_json.contains("FABRIC_KEY")  ) {

      json_value = device_target_config_json["FABRIC_KEY"].get<std::string>();
    }

    // 1. check for specific layout's fabric key (**not** using the json value): aurora/layoutname_fabric_key.xml
    // check for unencrypted file
    fabric_key_file_path = 
        deviceTypeDirPath(device_target) /
        std::string("aurora") /
        std::string(device_target.device_variant_layout.name + "_fabric_key.xml");

    if(!FileUtils::FileExists(fabric_key_file_path)) {

      // check for encrypted file
      fabric_key_file_path += ".en";
      if(!FileUtils::FileExists(fabric_key_file_path)) {

        // mark the variable empty to indicate it was not found
        fabric_key_file_path.clear();
      }
    }

    // 2. if layoutname_ specific file not found, find the fabric key using the config.json value
    if(fabric_key_file_path.empty()) {

      // check for unencrypted file
      fabric_key_file_path = 
          deviceTypeDirPath(device_target) / json_value;
      if(!FileUtils::FileExists(fabric_key_file_path)) {

        // check for encrypted file
        fabric_key_file_path += ".en";
        if(!FileUtils::FileExists(fabric_key_file_path)) {

          fabric_key_file_path.clear();
        }
      }
    }
  }
  // else, we assume that this is a legacy device data directory (< v2.8.0)
  else {
    // check for unencrypted file
    fabric_key_file_path = 
        std::filesystem::path(deviceTypeDirPath(device_target) /
                              std::string("fabric_key") /
                              std::string(device_target.device_variant.family + "_" +
                              device_target.device_variant.foundry + "_" +
                              device_target.device_variant.node + "_" +
                              device_target.device_variant.voltage_threshold + "_" +
                              device_target.device_variant.p_v_t_corner + "_" +
                              device_target.device_variant_layout.name + "_" +
                              std::string("fabric_key.xml")));
    if(!FileUtils::FileExists(fabric_key_file_path)) {

      // check for encrypted file
      fabric_key_file_path += ".en";
      if(!FileUtils::FileExists(fabric_key_file_path)) {

        fabric_key_file_path.clear();
      }
    }
  }

  if(fabric_key_file_path.empty()) {
    std::cout << "[zyxw]" << "no fabric key available, use autogenerated one" << std::endl;
  }
  else {
    std::cout << "[zyxw]" << "using fabric key file: " << fabric_key_file_path.string() << std::endl;
  }

  return fabric_key_file_path;
}


std::filesystem::path QLDeviceManager::deviceOpenFPGAPinTableFile(QLDeviceTarget device_target) {

  CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

  std::filesystem::path empty_path;
  std::filesystem::path pin_table_file_path;

  if( !isDeviceTargetValid(device_target) ) {
    device_target = this->device_target;
  }

  // use the device specific pin table file, and note that we may have
  // unencrypted (first priority) or encrypted file

  // this is a bit of a special handling case, because we still want to support
  //  multiple layouts in a single device for tsmc16 until we decide to change it
  //  to become multiple devices instead.
  // additionally, the pin table file can be placed in the 'design directory' which
  //  should override any other file in the device data directory, so we should search there
  //  first (design dir file gets priority)
  // so, we will not rely **only** on the 'config.json', but will actually do:
  // 1. (new structure) if config.json found, find pin table file as : "projectpath/../layoutname_pin_table.csv", or +.en
  //    1a. (new structure, not found in design dir, search device data) -> find pin table file as : "aurora/layoutname_pin_table.csv", or +.en
  // 2. (new structure) if layoutname_pin_table.csv.* is not found, find pin table as "projectpath/../filename-from-json-value", or +.en file 
  //    2a. (new structure, using json, not found in design dir, search device data) -> find pin table using config.json value
  // 3. (legacy) if config.json not found, find pin table file as "projectpath/../family_foundry_node_vt_corner_layoutname_pin_table.csv"
  //    3a. (legacy, not found in design dir, search device data) -> find pin table as : pin_table/family_foundry_node_vt_corner_layoutname_pin_table.csv or +.en


  // check config.json if it exists
  std::filesystem::path device_target_config_json_filepath = deviceTypeDirPath(device_target) / std::string("config.json");
  if(FileUtils::FileExists(device_target_config_json_filepath)) {

    std::ifstream device_target_config_json_ifstream(device_target_config_json_filepath.string());
    json device_target_config_json = json::parse(device_target_config_json_ifstream);
    // get json value
    std::string json_value;
    if( device_target_config_json.contains("PIN_TABLE")  ) {

      json_value = device_target_config_json["PIN_TABLE"].get<std::string>();
    }

    // 1. check for specific layout's pin table (**not** using the json value): layoutname_pin_table.csv in project_path
    // check for unencrypted file
    pin_table_file_path = 
        std::filesystem::path(compiler->ProjManager()->projectPath()) /
        std::string("..") /
        std::string(device_target.device_variant_layout.name + "_pin_table.csv");

    if(!FileUtils::FileExists(pin_table_file_path)) {

      // check for encrypted file
      pin_table_file_path += ".en";
      if(!FileUtils::FileExists(pin_table_file_path)) {

        // mark the variable empty to indicate it was not found
        pin_table_file_path.clear();
      }
    }
    // 1a. not found in design dir, search in the device data: aurora/layoutname_pin_table.csv
    if(pin_table_file_path.empty()) {
      // check for unencrypted file
      pin_table_file_path = 
          deviceTypeDirPath(device_target) /
          std::string("aurora") /
          std::string(device_target.device_variant_layout.name + "_pin_table.csv");

      if(!FileUtils::FileExists(pin_table_file_path)) {

        // check for encrypted file
        pin_table_file_path += ".en";
        if(!FileUtils::FileExists(pin_table_file_path)) {

          // mark the variable empty to indicate it was not found
          pin_table_file_path.clear();
        }
      }
    }

    // 2. if layoutname_ specific file not found, find the pin table csv using the config.json value (filename only) in design dir
    if(pin_table_file_path.empty()) {

      // check for unencrypted file
      pin_table_file_path = 
          std::filesystem::path(compiler->ProjManager()->projectPath()) /
          std::string("..") /
          std::filesystem::path(json_value).filename();
      if(!FileUtils::FileExists(pin_table_file_path)) {

        // check for encrypted file
        pin_table_file_path += ".en";
        if(!FileUtils::FileExists(pin_table_file_path)) {

          pin_table_file_path.clear();
        }
      }
    }
    // 2a. json value filename not found in design dir, search device data
    if(pin_table_file_path.empty()) {

      // check for unencrypted file
      pin_table_file_path = 
          deviceTypeDirPath(device_target) / json_value;
      if(!FileUtils::FileExists(pin_table_file_path)) {

        // check for encrypted file
        pin_table_file_path += ".en";
        if(!FileUtils::FileExists(pin_table_file_path)) {

          pin_table_file_path.clear();
        }
      }
    }
  }
  // else, we assume that this is a legacy device data directory (< v2.8.0)
  else {
    // 3. find file in design dir:
    // check for unencrypted file
    pin_table_file_path = 
        std::filesystem::path(std::filesystem::path(compiler->ProjManager()->projectPath()) /
                              std::string("..") /
                              std::string(device_target.device_variant.family + "_" +
                              device_target.device_variant.foundry + "_" +
                              device_target.device_variant.node + "_" +
                              device_target.device_variant.voltage_threshold + "_" +
                              device_target.device_variant.p_v_t_corner + "_" +
                              device_target.device_variant_layout.name + "_" +
                              std::string("pin_table.csv")));
    if(!FileUtils::FileExists(pin_table_file_path)) {

      // check for encrypted file
      pin_table_file_path += ".en";
      if(!FileUtils::FileExists(pin_table_file_path)) {

        pin_table_file_path.clear();
      }
    }
    // 3a. if not found in design dir, search the device data:
    if(pin_table_file_path.empty()) {
      // check for unencrypted file
      pin_table_file_path = 
          std::filesystem::path(deviceTypeDirPath(device_target) /
                                std::string("pin_table") /
                                std::string(device_target.device_variant.family + "_" +
                                device_target.device_variant.foundry + "_" +
                                device_target.device_variant.node + "_" +
                                device_target.device_variant.voltage_threshold + "_" +
                                device_target.device_variant.p_v_t_corner + "_" +
                                device_target.device_variant_layout.name + "_" +
                                std::string("pin_table.csv")));
      if(!FileUtils::FileExists(pin_table_file_path)) {

        // check for encrypted file
        pin_table_file_path += ".en";
        if(!FileUtils::FileExists(pin_table_file_path)) {

          pin_table_file_path.clear();
        }
      }
    }
  }

  if(pin_table_file_path.empty()) {
    compiler->ErrorMessage("Cannot find device pin table file!");
    return empty_path;
  }
  else {
    std::cout << "[zyxw]" << "using pin table file: " << pin_table_file_path.string() << std::endl;
  }

  return pin_table_file_path;
}


std::filesystem::path QLDeviceManager::deviceOpenFPGAIOMapFile(QLDeviceTarget device_target) {

  CompilerOpenFPGA_ql* compiler = static_cast<CompilerOpenFPGA_ql*>(GlobalSession->GetCompiler());

  std::filesystem::path empty_path;
  std::filesystem::path io_map_file_path;

  if( !isDeviceTargetValid(device_target) ) {
    device_target = this->device_target;
  }

  // use the device specific io map file, and note that we may have
  // unencrypted (first priority) or encrypted file

  // this is a bit of a special handling case, because we still want to support
  //  multiple layouts in a single device for tsmc16 until we decide to change it
  //  to become multiple devices instead.
  // additionally, the io map file can be placed in the 'design directory' which
  //  should override any other file in the device data directory, so we should search there
  //  first (design dir file gets priority)
  // so, we will not rely **only** on the 'config.json', but will actually do:
  // 1. (new structure) if config.json found, find io map file as : "projectpath/../layoutname_fpga_io_map.xml", or +.en
  //    1a. (new structure, not found in design dir, search device data) -> find io map file as : "aurora/layoutname_fpga_io_map.xml", or +.en
  // 2. (new structure) if layoutname_fpga_io_map.xml.* is not found, find io map as "projectpath/../filename-from-json-value", or +.en file 
  //    2a. (new structure, using json, not found in design dir, search device data) -> find io map using config.json value
  // 3. (legacy) if config.json not found, find io map file as "projectpath/../family_foundry_node_vt_corner_layoutname_fpga_io_map.xml"
  //    3a. (legacy, not found in design dir, search device data) -> find io map as : fpga_io_map/family_foundry_node_vt_corner_layoutname_fpga_io_map.xml or +.en


  // check config.json if it exists
  std::filesystem::path device_target_config_json_filepath = deviceTypeDirPath(device_target) / std::string("config.json");
  if(FileUtils::FileExists(device_target_config_json_filepath)) {

    std::ifstream device_target_config_json_ifstream(device_target_config_json_filepath.string());
    json device_target_config_json = json::parse(device_target_config_json_ifstream);
    // get json value
    std::string json_value;
    if( device_target_config_json.contains("FPGA_IO_MAP")  ) {

      json_value = device_target_config_json["FPGA_IO_MAP"].get<std::string>();
    }

    // 1. check for specific layout's io map (**not** using the json value): layoutname_fpga_io_map.xml in project_path
    // check for unencrypted file
    io_map_file_path = 
        std::filesystem::path(compiler->ProjManager()->projectPath()) /
        std::string("..") /
        std::string(device_target.device_variant_layout.name + "_fpga_io_map.xml");

    if(!FileUtils::FileExists(io_map_file_path)) {

      // check for encrypted file
      io_map_file_path += ".en";
      if(!FileUtils::FileExists(io_map_file_path)) {

        // mark the variable empty to indicate it was not found
        io_map_file_path.clear();
      }
    }
    // 1a. not found in design dir, search in the device data: aurora/layoutname_fpga_io_map.xml
    if(io_map_file_path.empty()) {
      // check for unencrypted file
      io_map_file_path = 
          deviceTypeDirPath(device_target) /
          std::string("aurora") /
          std::string(device_target.device_variant_layout.name + "_fpga_io_map.xml");

      if(!FileUtils::FileExists(io_map_file_path)) {

        // check for encrypted file
        io_map_file_path += ".en";
        if(!FileUtils::FileExists(io_map_file_path)) {

          // mark the variable empty to indicate it was not found
          io_map_file_path.clear();
        }
      }
    }

    // 2. if layoutname_ specific file not found, find the io map file using the config.json value (filename only) in design dir
    if(io_map_file_path.empty()) {

      // check for unencrypted file
      io_map_file_path = 
          std::filesystem::path(compiler->ProjManager()->projectPath()) /
          std::string("..") /
          std::filesystem::path(json_value).filename();
      if(!FileUtils::FileExists(io_map_file_path)) {

        // check for encrypted file
        io_map_file_path += ".en";
        if(!FileUtils::FileExists(io_map_file_path)) {

          io_map_file_path.clear();
        }
      }
    }
    // 2a. json value filename not found in design dir, search device data
    if(io_map_file_path.empty()) {

      // check for unencrypted file
      io_map_file_path = 
          deviceTypeDirPath(device_target) / json_value;
      if(!FileUtils::FileExists(io_map_file_path)) {

        // check for encrypted file
        io_map_file_path += ".en";
        if(!FileUtils::FileExists(io_map_file_path)) {

          io_map_file_path.clear();
        }
      }
    }
  }
  // else, we assume that this is a legacy device data directory (< v2.8.0)
  else {
    // 3. find file in design dir:
    // check for unencrypted file
    io_map_file_path = 
        std::filesystem::path(std::filesystem::path(compiler->ProjManager()->projectPath()) /
                              std::string("..") /
                              std::string(device_target.device_variant.family + "_" +
                              device_target.device_variant.foundry + "_" +
                              device_target.device_variant.node + "_" +
                              device_target.device_variant.voltage_threshold + "_" +
                              device_target.device_variant.p_v_t_corner + "_" +
                              device_target.device_variant_layout.name + "_" +
                              std::string("fpga_io_map.xml")));
    if(!FileUtils::FileExists(io_map_file_path)) {

      // check for encrypted file
      io_map_file_path += ".en";
      if(!FileUtils::FileExists(io_map_file_path)) {

        io_map_file_path.clear();
      }
    }
    // 3a. if not found in design dir, search the device data:
    if(io_map_file_path.empty()) {
      // check for unencrypted file
      io_map_file_path = 
          std::filesystem::path(deviceTypeDirPath(device_target) /
                                std::string("fpga_io_map") /
                                std::string(device_target.device_variant.family + "_" +
                                device_target.device_variant.foundry + "_" +
                                device_target.device_variant.node + "_" +
                                device_target.device_variant.voltage_threshold + "_" +
                                device_target.device_variant.p_v_t_corner + "_" +
                                device_target.device_variant_layout.name + "_" +
                                std::string("fpga_io_map.xml")));
      if(!FileUtils::FileExists(io_map_file_path)) {

        // check for encrypted file
        io_map_file_path += ".en";
        if(!FileUtils::FileExists(io_map_file_path)) {

          io_map_file_path.clear();
        }
      }
    }
  }

  if(io_map_file_path.empty()) {
    compiler->ErrorMessage("Cannot find device io map file!");
    return empty_path;
  }
  else {
    std::cout << "[zyxw]" << "using io map file: " << io_map_file_path.string() << std::endl;
  }

  return io_map_file_path;
}


std::filesystem::path QLDeviceManager::deviceVPRRRGraphFile(QLDeviceTarget device_target) {

  std::filesystem::path empty_path;
  return empty_path;

}


std::filesystem::path QLDeviceManager::deviceVPRRouterLookaheadFile(QLDeviceTarget device_target) {

  std::filesystem::path empty_path;
  return empty_path;

}


  // future use (not file access APIs, but used together with them)
std::vector<std::string> QLDeviceManager::deviceCorners(QLDeviceTarget device_target) {

  std::vector<std::string> corners;
  return corners;

}


std::vector<std::filesystem::path> QLDeviceManager::deviceCornerPowerDataFiles(QLDeviceTarget device_target) {

  std::vector<std::filesystem::path> corner_power_data_filepaths;
  return corner_power_data_filepaths;

}

} // namespace FOEDAG
