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

public:
  static QLMetricsManager* instance;
};

}

#endif // QLMETRICSMANAGER_H