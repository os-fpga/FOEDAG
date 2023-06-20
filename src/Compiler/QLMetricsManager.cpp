
#include "Compiler/Compiler.h"
#include "Utils/FileUtils.h"
#include "Utils/LogUtils.h"
#include "Utils/StringUtils.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "QLMetricsManager.h"


extern FOEDAG::Session* GlobalSession;

namespace FOEDAG {

// singleton init
QLMetricsManager* QLMetricsManager::instance = NULL;

///////////////////////////////////////////////// static functions

QLMetricsManager* QLMetricsManager::getInstance() {

  if(instance == NULL) {

    instance = new QLMetricsManager();

    instance->parseJSON();
  }

  return instance;
}

std::string QLMetricsManager::getStringValue(std::string category, std::string subcategory, std::string name) {

  std::string string_value;

  // std::cout << "getStringValue(): " << category << ", " << subcategory << ", " << name << std::endl;

  for (AuroraMetrics metric: QLMetricsManager::getInstance()->aurora_metrics_list) {

    // std::cout << "\n\nmetric:" << std::endl;
    // std::cout << "    " << metric.category << ", " << metric.subcategory << ", " << metric.name << ", " << metric.found << ", " << metric.string_value << std::endl;

    if(metric.category == category &&
       metric.subcategory == subcategory &&
       metric.name == name &&
       metric.found == true) {

        string_value = metric.string_value;
        // std::cout << "metric.string_value: " << metric.string_value << std::endl;
        break;
    }
  }

  return string_value;
}


int QLMetricsManager::getIntValue(std::string category, std::string subcategory, std::string name) {

  int int_value = 0;

  std::string string_value = QLMetricsManager::getStringValue(category, subcategory, name);

  if(string_value.empty()) {
    return int_value;
  }

  try {
    int_value = std::stoi(string_value);
    // std::cout << "int_value: " << int_value << std::endl;
  }
  catch (std::invalid_argument const &e) {
    std::cout << "[metrics json] Bad input: std::invalid_argument thrown: " << string_value << std::endl;
    int_value = 0;
  }
  catch (std::out_of_range const &e) {
    std::cout << "[metrics json] Integer overflow: std::out_of_range thrown: " << string_value << std::endl;
    int_value = 0;
  }

  return int_value;
}


double QLMetricsManager::getDoubleValue(std::string category, std::string subcategory, std::string name) {

  double double_value = 0;

  std::string string_value = QLMetricsManager::getStringValue(category, subcategory, name);

  if(string_value.empty()) {
    return double_value;
  }

  try {
    double_value = std::stod(string_value);
    // std::cout << "double_value: " << double_value << std::endl;
  }
  catch (std::invalid_argument const &e) {
    std::cout << "[metrics json] Bad input: std::invalid_argument thrown: " << string_value << std::endl;
    double_value = 0;
  }
  catch (std::out_of_range const &e) {
    std::cout << "[metrics json] Integer overflow: std::out_of_range thrown: " << string_value << std::endl;
    double_value = 0;
  }

  return double_value;
}


void QLMetricsManager::addParsedMetrics(std::vector<AuroraMetrics>& metrics_list) {

  // add the list of metrics, to the full metrics list:
  QLMetricsManager::getInstance()->aurora_metrics_list.insert(std::end(QLMetricsManager::getInstance()->aurora_metrics_list),
                                                              std::begin(metrics_list),
                                                              std::end(metrics_list));

  // std::cout << "\n\naddParsedMetrics, new total number of metrics: " << QLMetricsManager::getInstance()->aurora_metrics_list.size() << "\n\n" << std::endl;
}


///////////////////////////////////////////////// object functions

QLMetricsManager::QLMetricsManager(QObject *parent)
    : QObject(parent) {}

bool QLMetricsManager::parseJSON() {

  std::string metrics_json_filename = "aurora_metrics.json";
  std::filesystem::path scripts_path = GlobalSession->Context()->DataPath() / ".." / "scripts";
  std::filesystem::path metrics_json_filepath = scripts_path / metrics_json_filename;

  if(FileUtils::FileExists(metrics_json_filepath)) {
    try {
        // std::cout << "metrics_json_filepath" << metrics_json_filepath << std::endl;
        std::ifstream metrics_json_ifstream(metrics_json_filepath.string());
        metrics_json = json::parse(metrics_json_ifstream);
    }
    catch (json::parse_error& e) {
        // output exception information
        std::cout << "message: " << e.what() << '\n'
                  << "exception id: " << e.id << '\n'
                  << "byte position of error: " << e.byte << std::endl;
        metrics_json = json::object();
    }
  }
  else {
    std::cout << "[warning] expected metrics json file does not exist: " << metrics_json_filepath.string() << std::endl;
  }

  if(metrics_json.empty()) {

    return false;
  }

  return true;
}


std::vector<AuroraMetrics> QLMetricsManager::buildMetricsListForAction(Compiler::Action action) {

  // create a metrics list for the current action
  std::vector<AuroraMetrics> metrics_list;
  
  if(!metrics_json.empty()) {

    for (auto [category_name, category_json] : metrics_json.items()) {

      for (auto [subcategory_name, subcategory_json] : category_json.items()) {

        for (auto [parameter_name, parameter_json] : subcategory_json.items()) {

          // filter the metrics according to the stage and metrics classification:
          if(action == Compiler::Action::Synthesis) {
            if(category_name != "yosys_metrics") {
              continue;
            }
            if(subcategory_name != "synthesis") {
              continue;
            }
          }
          else if(action == Compiler::Action::Pack) {
            if(category_name != "vpr_metrics") {
              continue;
            }
            if( (subcategory_name != "packing") && (subcategory_name != "general") ) {
              continue;
            }
          }
          else if(action == Compiler::Action::Detailed) {
            if(category_name != "vpr_metrics") {
              continue;
            }
            if( (subcategory_name != "placement") && (subcategory_name != "general") ) {
              continue;
            }
          }
          else if(action == Compiler::Action::Routing) {
            if(category_name != "vpr_metrics") {
              continue;
            }
            if( (subcategory_name != "routing") && (subcategory_name != "general") ) {
              continue;
            }
          }

          // create a metric object
          AuroraMetrics metric;
          metric.category = category_name;        // yosys, vpr
          metric.subcategory = subcategory_name;  // synthesis, packing, placement, routing
          metric.name = parameter_name;
          metric.filename = parameter_json["filename"];
          metric.filename = std::regex_replace(metric.filename, std::regex("\\$\\{PROJECT_NAME\\}"), GlobalSession->GetCompiler()->ProjManager()->projectName());
          std::string action_log_file;
          if(action == Compiler::Action::Synthesis)       { action_log_file = SYNTHESIS_LOG;  }
          else if(action == Compiler::Action::Pack)       { action_log_file = PACKING_LOG;    }
          else if(action == Compiler::Action::Detailed)   { action_log_file = PLACEMENT_LOG;  }
          else if(action == Compiler::Action::Routing)    { action_log_file = ROUTING_LOG;    }
          else if(action == Compiler::Action::Bitstream)  { action_log_file = BITSTREAM_LOG;  }
          metric.filename = std::regex_replace(metric.filename, std::regex("\\$\\{ACTION_LOG_FILE\\}"), action_log_file);
          metric.regex = parameter_json["regex"];
          metric.description = parameter_json["description"];
          metric.type = parameter_json["type"];
          metric.match_type = parameter_json["match_type"];

          metrics_list.push_back(metric);
        }
      }
    }
  }

  // std::cout << "buildMetricsListForAction : " << metrics_list.size() << " metrics to parse" << std::endl;

  return metrics_list;
}


void QLMetricsManager::parseMetricsForAction(Compiler::Action action) {

  // parse the logs according to the step that was completed, assuming the logs are available.
  // obtain the metrics that we need using the logs and regex
  // produce a metrics report log
  // save the parsed metrics list into the QLMetricsManager for later API access by other
  // components.

  // std::string stage;
  // if(action == Compiler::Action::Synthesis)         { stage = "synthesis";  }
  // else if(action == Compiler::Action::Pack)         { stage = "packing";       }
  // else if(action == Compiler::Action::Detailed)     { stage = "placement";      }
  // else if(action == Compiler::Action::Routing)      { stage = "routing";      }
  // else if(action == Compiler::Action::Bitstream)    { stage = "bitstream";   }
  // else                                              { stage = "!!unknown!!"; }
  // std::cout << "\n\n\n>>> ParseLogs() for: " << stage << std::endl;

  // build the list of metrics for this action, from the JSON:
  std::vector<AuroraMetrics> metrics_list = buildMetricsListForAction(action);


  // step 1: for all the metrics, collate the filenames they need to parse
  // so that the files can be opened just once and read, and used for regex search
  // for each metric repeatedly (avoid file open-read cycle)
  // get filename, and insert into map: "filename" -> "filecontent"
  // then, while processing each metric, regex can use map["filename"] to get content to be searched.
  std::unordered_map<std::string, std::string> metric_file_content_map;

  for (const AuroraMetrics& metric: metrics_list) {

    // if we have already read in the same file, skip:
    if(!!metric_file_content_map.count(metric.filename)) {
      continue;
    }

    std::filesystem::path filepath =
      std::filesystem::path(GlobalSession->GetCompiler()->ProjManager()->projectPath()) / metric.filename;
    
    // if log file does not exist, skip:
    if(!FileUtils::FileExists(filepath)) {
      std::cout << "[warning] file not found: " << metric.filename << std::endl;
      continue;
    }

    // get it into a ifstream
    std::ifstream stream(filepath.string());

    // if log file read failed, skip:
    if (!stream.good()) {
      std::cout << "[warning] file could not be read: " << metric.filename << std::endl;
      stream.close();
      continue;
    }

    std::string filecontent = 
      std::string((std::istreambuf_iterator<char>(stream)),
                  std::istreambuf_iterator<char>());
    stream.close();

    // store filecontent into map
    // std::cout << ">> add to map, filename: " << metric.filename << " --> " << filepath.string() << std::endl;
    metric_file_content_map[metric.filename] = filecontent;
  }
  

  // step 2: loop through each metric, use the regex to obtain the value and store it.
  std::regex regex;
  std::smatch smatches;
  bool found;

  // loop by reference, as we want to modify list item in-place
  for (AuroraMetrics& metric: metrics_list) {

    metric.found = false; // init to false before parsing.

    // std::cout << "\n" << std::endl;

    // ignore metrics, whose file was not found:
    if(!metric_file_content_map.count(metric.filename)) {
      // std::cout << "skipping: " << metric.regex << ", as log file not found: " << metric.filename << std::endl;
      continue;
    }

    // std::cout << "name: " << metric.name << std::endl;
    // std::cout << "regex: " << metric.regex << std::endl;
    regex = std::regex(metric.regex, std::regex::ECMAScript);
    found = std::regex_search ( metric_file_content_map[metric.filename], smatches, regex );

    if(found) {

      if(metric.match_type == "first") {

          metric.string_value = smatches.str(1);
          // std::cout << "metric.string_value: " << metric.string_value << std::endl;
          metric.found = true;
      }
      else if(metric.match_type == "last") {

          metric.string_value = smatches.str(smatches.size()-1);
          // std::cout << "metric.string_value: " << metric.string_value << std::endl;
          metric.found = true;
      }
      // TODO: future, we can concat strings, or add all warnings to get total etc.
      // else if(metric.match_type == "add") {
      //
      //     for(uint32_t i = 0; i < smatches.size(); i++) {
      //       metric.string_value += smatches.str(i);
      //     }
      //     std::cout << "metric.string_value: " << metric.string_value << std::endl;
      // }
      else {
        // unsupported match_type: only first, last
        // std::cout << ">> metric match_type unsupported: " << metric.match_type << std::endl;
      }
    }
    else {
      // std::cout << ">> metric not found in logs: " << metric.regex << std::endl;
    }
  }

  // output the parsed metrics into a file:
  std::string metrics_rpt_filename;
  if(action == Compiler::Action::Synthesis)           { metrics_rpt_filename = "synthesis_metrics.rpt";   }
  else if(action == Compiler::Action::Pack)           { metrics_rpt_filename = "packing_metrics.rpt";     }
  else if(action == Compiler::Action::Detailed)       { metrics_rpt_filename = "placement_metrics.rpt";   }
  else if(action == Compiler::Action::Routing)        { metrics_rpt_filename = "routing_metrics.rpt";     }
  else if(action == Compiler::Action::Bitstream)      { metrics_rpt_filename = "bitstream_metrics.rpt";   }

  if(!metrics_rpt_filename.empty()) {
    std::filesystem::path metrics_rpt_filepath = 
      std::filesystem::path(GlobalSession->GetCompiler()->ProjManager()->projectPath()) / metrics_rpt_filename;
    
    std::ofstream metrics_rpt;
    metrics_rpt.open(metrics_rpt_filepath);
  
    // header:
    metrics_rpt << "category" << "," << "subcategory" << "," << "name" << "," << "found" << "," << "value" << std::endl;
    for (const AuroraMetrics& metric: metrics_list) {
      metrics_rpt << metric.category << "," << metric.subcategory << "," << metric.name << "," << metric.found << "," << metric.string_value << std::endl;
    }

    // close the file stream
    metrics_rpt.close();
  }

  // add the processed metrics for the current action to the full metrics list:
  QLMetricsManager::addParsedMetrics(metrics_list);
}


}
