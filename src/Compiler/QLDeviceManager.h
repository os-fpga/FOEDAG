#include <QWidget>
#include <QString>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>


#include <string>
#include <vector>
#include <set>
#include <filesystem>

#ifndef QLDEVICEMANAGER_H
#define QLDEVICEMANAGER_H

namespace FOEDAG {

class QLSettingsManager;


struct LayoutInfoHelper {
LayoutInfoHelper(const std::string& name): name(name){}
std::string name;
int clb;
int dsp;
int bram;
int io;
};

class QLDeviceVariantLayout {
    public:
    std::string name;
    int width = 0;
    int height = 0;
    int bram = 0;
    int dsp = 0;
    int clb = 0;
    int io = 0;
};

class QLDeviceVariant {
    public:
    std::string family;
    std::string foundry;
    std::string node;
    std::string voltage_threshold;
    std::string p_v_t_corner;
    std::vector<QLDeviceVariantLayout> device_variant_layouts;
};

class QLDeviceType {
    public:
    std::string family;
    std::string foundry;
    std::string node;
    std::vector<QLDeviceVariant> device_variants;
    std::filesystem::path device_root_path;
};


class QLDeviceTarget  {
  public:
    QLDeviceVariant device_variant;
    QLDeviceVariantLayout device_variant_layout;
};


class QLDeviceManager : public QObject {
  Q_OBJECT
 public:
  static QLDeviceManager* getInstance(bool initialize=false);
  static bool compareLayouts(const std::string& layout_1, const std::string& layout_2);
  ~QLDeviceManager();

 private:
  QLDeviceManager(QObject *parent = nullptr);
  QLDeviceVariantLayout* findDeviceLayoutVariantPtr(const std::string& family, 
                                                    const std::string& foundry,
                                                    const std::string& node,
                                                    const std::string& voltage_threshold,
                                                    const std::string& p_v_t_corner,
                                                    const std::string& layoutName);
  void collectDeviceVariantAvailableResources(const QLDeviceVariant& device_variant);
  std::vector<std::shared_ptr<LayoutInfoHelper>> ExtractDeviceAvailableResourcesFromVprLogContent(const std::string&) const;

 public:
  void initialize();
  QWidget* createDeviceSelectionWidget(bool newProjectMode);
  void giveupDeviceSelectionWidget();
  void parseDeviceData();
  std::vector<QLDeviceVariant> listDeviceVariants(std::string family,
                                                 std::string foundry,
                                                 std::string node);
  std::vector<QLDeviceVariantLayout> listDeviceVariantLayouts(std::string family,
                                                            std::string foundry,
                                                            std::string node,
                                                            std::string voltage_threshold,
                                                            std::string p_v_t_corner);
  std::string DeviceString(std::string family,
                           std::string foundry,
                           std::string node,
                           std::string voltage_threshold,
                           std::string p_v_t_corner,
                           std::string layout_name);
  bool DeviceExists(std::string family,
                    std::string foundry,
                    std::string node,
                    std::string voltage_threshold,
                    std::string p_v_t_corner,
                    std::string layout_name);
  bool DeviceExists(std::string device_string);
  bool DeviceExists(QLDeviceTarget device_target);
  QLDeviceTarget convertToDeviceTarget(std::string family,
                                 std::string foundry,
                                 std::string node,
                                 std::string voltage_threshold,
                                 std::string p_v_t_corner,
                                 std::string layout_name);
  QLDeviceTarget convertToDeviceTarget(std::string device_string);
  std::string convertToDeviceString(QLDeviceTarget device_target);
  bool isDeviceTargetValid(QLDeviceTarget device_target);
  void setCurrentDeviceTarget(std::string family,
                              std::string foundry,
                              std::string node,
                              std::string voltage_threshold,
                              std::string p_v_t_corner,
                              std::string layout_name);
  void setCurrentDeviceTarget(std::string device_string);
  void setCurrentDeviceTarget(QLDeviceTarget device_target);
  std::filesystem::path GetArchitectureFileForDeviceVariant(const QLDeviceVariant& device_variant);
  // only for GUI usage:
  std::string convertToFoundryNode(std::string foundry, std::string node);
  std::vector<std::string> convertFromFoundryNode(std::string foundrynode);

 public:
 void triggerUIUpdate();
 void familyChanged(const QString& family_qstring);
 void foundrynodeChanged(const QString& foundrynode_qstring);
 void voltage_thresholdChanged(const QString& voltagetheshold_qstring);
 void p_v_t_cornerChanged(const QString& p_v_t_corner_qstring);
 void layoutChanged(const QString& layout_qstring);
 void resetButtonClicked();
 void applyButtonClicked();

 public:
  // singleton instance of ourself
  static QLDeviceManager* instance;

  // hold a reference to singleton QLSettingsManager
  QLSettingsManager* settings_manager = nullptr;

  // hieracrchical list of all devices available in the installation
  std::vector <QLDeviceType> device_list;

  // flat list of all device targets (future?)
  //std::vector <QLDeviceTarget> device_target_list;

  // hold the current device_target
  QLDeviceTarget device_target;


  // GUI objects and state maintenance
  QWidget* device_manager_widget = nullptr;

  // hold the 'selected' device_target via GUI
  QLDeviceTarget device_target_selected;

  // hold the 'selected' device parameters via GUI
  std::vector <std::string> families;
  std::string family;
  std::vector <std::string> foundrynodes;
  std::string foundrynode;
  std::string foundry;
  std::string node;
  std::vector <std::string> voltage_thresholds;
  std::string voltage_threshold;
  std::vector <std::string> p_v_t_corners;
  std::string p_v_t_corner;
  std::vector <std::string> layouts;
  std::string layout;
  
  QComboBox* m_combobox_family;
  QComboBox* m_combobox_foundry_node;
  QComboBox* m_combobox_voltage_threshold;
  QComboBox* m_combobox_p_v_t_corner;
  QComboBox* m_combobox_layout;

  QLabel* m_widget_device_available_resources;

  QPushButton* m_button_reset;
  QPushButton* m_button_apply;
  QLabel* m_message_label;

  bool currentDeviceTargetUpdateInProgress = false;
  bool newProjectMode = false;
};



}

#endif // QLDEVICEMANAGER_H
