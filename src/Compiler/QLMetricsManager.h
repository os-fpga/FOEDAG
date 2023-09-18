#include <string>
#include <vector>
#include <set>
#include <filesystem>
#include <regex>

#include "Compiler/Compiler.h"

#include "nlohmann_json/json.hpp"


#ifndef QLMETRICSMANAGER_H
#define QLMETRICSMANAGER_H

using json = nlohmann::ordered_json;

namespace FOEDAG {

class AuroraMetrics {

  public:
    // name of the metrics parameter
    std::string name;
    // compilation "stage" where the metrics parameter is available
    std::string stage;
    // filename (in project path) to parse
    std::string filename;
    // regex to use, with single capture group
    std::string regex;
    // description of the parameter
    std::string description;
    // type of expected value: string, integer, double
    std::string type;
    // how to match the regex: first, last, add(int/double: addition, string: concatenation),
    std::string match_type;
    // value parsed from the file as string
    std::string string_value;
    // units of the value, if any
    std::string value_units;
    // was the metric found after parsing
    bool found = false;
};


class AuroraBRAMConfigurationData {

  public:
    bool bram_type_is_split;          // split = true, nonsplit = false
    unsigned int port_A1_width;       // split = A1, nonsplit = A
    unsigned int port_B1_width;       // split = B1, nonsplit = B
    unsigned int port_A2_width;       // split = A2, nonsplit = invalid
    unsigned int port_B2_width;       // split = B2, nonsplit = invalid
    unsigned int count;               // how many instances of this
};

// total available blocks can be obtained from architecture information from the layout.
class AuroraUtilization {

  public:
    // level 1 : clb used
    unsigned int clb;
    // level 1 : fles in clb == clb*10
    unsigned int fle;
      // level 2: fle used
      unsigned int clb_fle;
      // level 2: luts used
      unsigned int clb_lut;
        // level 3: luts used in lut4 mode (adder carry chain)
        unsigned int clb_lut_lut4;
        // level 3: luts used in lut5 mode
        unsigned int clb_lut_lut5;
        // level 3: luts used in lut5+ff mode
        unsigned int clb_lut_lut5ff;
        // level 3: luts used in lut6 mode
        unsigned int clb_lut_lut6;
        // level 3: luts used in lut6+ff mode
        unsigned int clb_lut_lut6ff;
      // level 3: ffs used
      unsigned int clb_ff;
        // level 3: ffs used in lut5+ff mode
        unsigned int clb_ff_lut5ff;
        // level 3: ffs used in lut6+ff mode
        unsigned int clb_ff_lut6ff;
        // level 3: ffs used in ff mode
        unsigned int clb_ff_ff;
        // level 3: ffs used in shift register mode
        unsigned int clb_ff_shiftreg;
    // level 1: brams used
    unsigned int bram;
      // level 2: brams used in nonsplit mode
      unsigned int bram_bram_nonsplit;
        // level 3: map of 'bram_config_type : num_used' for nonsplit mode
        std::unordered_map<std::string, unsigned int> bram_bram_nonsplit_map;
      // level 2: brams used in split mode
      unsigned int bram_bram_split;
        // level 3: map of 'bram_config_type : num_used' for split mode
        std::unordered_map<std::string, unsigned int> bram_bram_split_map;
      // level 2: fifos used in nonsplit mode
      unsigned int bram_fifo_nonsplit;
        // level 3: map of 'fifo_config_type : num_used' for nonsplit mode
        std::unordered_map<std::string, unsigned int> bram_fifo_nonsplit_map;
      // level 2: fifos used in split mode
      unsigned int bram_fifo_split;
        // level 3: map of 'fifo_config_type : num_used' for split mode
        std::unordered_map<std::string, unsigned int> bram_fifo_split_map;
    // level 1: dsp used
    unsigned int dsp;
      // level 2: map of 'dsp_type : num_used'
      std::unordered_map<std::string, unsigned int> dsp_map;
    // level 1: io used
    unsigned int io;
      // level 2: io used as input
      unsigned int io_input;
      // level 2: io used as output
      unsigned int io_output;
};


class QLMetricsManager : public QObject {
  Q_OBJECT

private:
  QLMetricsManager(QObject* parent = nullptr);

public:
  void parseMetricsForAction(Compiler::Action action);
  void parseRoutingReportForDetailedUtilization();

public:
  static QLMetricsManager* getInstance();
  static std::string getStringValue(std::string stage, std::string name);
  static int getIntValue(std::string stage, std::string name);
  static double getDoubleValue(std::string stage, std::string name);
  static void addParsedMetrics(std::vector<AuroraMetrics>& metrics_list);

private:
    bool parseJSON();
    std::vector<AuroraMetrics> buildMetricsListForAction(Compiler::Action action);


public:
  json metrics_json;
  std::vector<AuroraMetrics> aurora_metrics_list;
  std::string rpt_delimiter = ";";
  AuroraUtilization aurora_routing_utilization;

public:
  static QLMetricsManager* instance;
};

}

#endif // QLMETRICSMANAGER_H