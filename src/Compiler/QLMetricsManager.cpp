
#include "Compiler/Compiler.h"
#include "Utils/FileUtils.h"
#include "Utils/LogUtils.h"
#include "Utils/StringUtils.h"
#include "MainWindow/Session.h"
#include "NewProject/ProjectManager/project_manager.h"
#include "QLMetricsManager.h"

#include <algorithm>


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

std::string QLMetricsManager::getStringValue(std::string stage, std::string name) {

  std::string string_value;

  // std::cout << "getStringValue(): " << ", " << stage << ", " << name << std::endl;

  for (AuroraMetrics metric: QLMetricsManager::getInstance()->aurora_metrics_list) {

    // std::cout << "\n\nmetric:" << std::endl;
    // std::cout << "    " << ", " << metric.stage << ", " << metric.name << ", " << metric.found << ", " << metric.string_value << std::endl;

    if(metric.stage == stage &&
       metric.name == name &&
       metric.found == true) {

        string_value = metric.string_value;
        // std::cout << "metric.string_value: " << metric.string_value << std::endl;
        break;
    }
  }

  return string_value;
}


int QLMetricsManager::getIntValue(std::string stage, std::string name) {

  int int_value = 0;

  std::string string_value = QLMetricsManager::getStringValue(stage, name);

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


double QLMetricsManager::getDoubleValue(std::string stage, std::string name) {

  double double_value = 0;

  std::string string_value = QLMetricsManager::getStringValue(stage, name);

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
    // std::cout << "[warning] expected metrics json file does not exist: " << metrics_json_filepath.string() << std::endl;
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

    for (auto [stage_name, stage_json] : metrics_json.items()) {

      for (auto [parameter_name, parameter_json] : stage_json.items()) {

        // filter the metrics according to the stage and metrics classification:
        if(action == Compiler::Action::Synthesis) {
          if(stage_name != "synthesis") {
            continue;
          }
        }
        else if(action == Compiler::Action::Pack) {
          if( (stage_name != "packing") && (stage_name != "general") ) {
            continue;
          }
        }
        else if(action == Compiler::Action::Detailed) {
          if( (stage_name != "placement") && (stage_name != "general") ) {
            continue;
          }
        }
        else if(action == Compiler::Action::Routing) {
          if( (stage_name != "routing") && (stage_name != "general") ) {
            continue;
          }
        }

        // create a metric object
        AuroraMetrics metric;
        metric.stage = stage_name;  // synthesis, packing, placement, routing
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

  // std::cout << "buildMetricsListForAction : " << metrics_list.size() << " metrics to parse" << std::endl;

  return metrics_list;
}


void QLMetricsManager::parseMetricsForAction(Compiler::Action action) {

  // parse the logs according to the step that was completed, assuming the logs are available.
  // obtain the metrics that we need using the logs and regex
  // produce a metrics report log
  // save the parsed metrics list into the QLMetricsManager for later API access by other
  // components.

  // std::string stage_str;
  // if(action == Compiler::Action::Synthesis)         { stage_str = "synthesis";   }
  // else if(action == Compiler::Action::Pack)         { stage_str = "packing";     }
  // else if(action == Compiler::Action::Detailed)     { stage_str = "placement";   }
  // else if(action == Compiler::Action::Routing)      { stage_str = "routing";     }
  // else if(action == Compiler::Action::Bitstream)    { stage_str = "bitstream";   }
  // else                                              { stage_str = "!!unknown!!"; }
  // std::cout << "\n\n\n>>> ParseLogs() for: " << stage_str << std::endl;

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
      // std::cout << "[warning] file not found: " << metric.filename << std::endl;
      continue;
    }

    // get it into a ifstream
    std::ifstream stream(filepath.string());

    // if log file read failed, skip:
    if (!stream.good()) {
      // std::cout << "[warning] file could not be read: " << metric.filename << std::endl;
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
          if(regex.mark_count() == 2) {
            // we expect to also have a "units" capture group at 2:
            metric.value_units = smatches.str(2);
          }
          metric.found = true;
      }
      else if(metric.match_type == "last") {

          // this is not the right way for last, we need to do repeated search till end.
          // FIX IT.
          metric.string_value = smatches.str(1);
          // std::cout << "metric.string_value: " << metric.string_value << std::endl;
          if(regex.mark_count() == 2) {
            // we expect to also have a "units" capture group at 2:
            metric.value_units = smatches.str(2);
          }
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
      // if not found:
      //    if string -> keep it empty?
      //    if int -> make it 0
      //    if double -> make it 0
      if(metric.type == "string") {
        metric.string_value = "not-available";
      }
      else if(metric.type == "int") {
        metric.string_value = "0";
      }
      else if (metric.type == "double") {
        metric.string_value = "0";
      }
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
    metrics_rpt << "stage" << rpt_delimiter
                << "name" << rpt_delimiter
                << "found" << rpt_delimiter
                << "value" << rpt_delimiter
                << "units" << std::endl;
    
    for (const AuroraMetrics& metric: metrics_list) {
      metrics_rpt << metric.stage << rpt_delimiter
                  << metric.name << rpt_delimiter
                  << metric.found << rpt_delimiter
                  << metric.string_value << rpt_delimiter
                  << metric.value_units << std::endl;
    }

    // close the file stream
    metrics_rpt.close();
  }

  // add the processed metrics for the current action to the full metrics list:
  QLMetricsManager::addParsedMetrics(metrics_list);
}

void QLMetricsManager::parseRoutingReportForDetailedUtilization() {

  // approach:
  // 1. extract the vpr report section, from routing log, beginning from:
  //    "Pb types usage..."
  //    upto:
  //    blank line
  //
  // 2. split the usage report into multiple sections
  // we use the indentation in the log file to detect hierarchy:
  // hierarchy level 1 == min_num_of_spaces (generally 2 spaces) - this is detected.
  // this should yield: io/clb/bram/dsp
  //
  // 3. io/bram/dsp only have one level of hierarchy under them, so parse the required
  //    fields using regex and store the values.
  //    io : io_input, io_output
  //    bram: nonsplit, split (and list all subtypes with A and B widths) : mem_36K_BRAM_xxxx or mem_36K_FIFO_xxxx
  //    dsp: list all subtypes with QL_DSP2_xxxx
  //
  // 4. clb
  // for clb, we have the next hierarchy as fle section: min_num_spaces + 2
  // which should yield ble6/lut5inter
  // further parse ble 6 section to get: lut6, ff
  // further parse lut5inter section to get: flut5, lut5, ff, adder, lut4
  //
  // 5. derive other required values from these, and output into "utilization.rpt"


  // std::cout << "parseRoutingReportForDetailedUtilization()" << std::endl;

  // read the routing log file:
  std::filesystem::path filepath =
  std::filesystem::path(GlobalSession->GetCompiler()->ProjManager()->projectPath()) / "routing.rpt";
  
  // if log file does not exist, skip:
  if(!FileUtils::FileExists(filepath)) {
     std::cout << "[warning] file not found: " << filepath << std::endl;
    return;
  }

  // get it into a ifstream
  std::ifstream stream(filepath.string());

  // if log file read failed, skip:
  if (!stream.good()) {
     std::cout << "[warning] file could not be read: " << filepath << std::endl;
    stream.close();
    return;
  }

  // obtain the vpr usage section:
  std::vector<std::string> vpr_pb_types_usage_section;
  std::string file_line;
  bool section_save = false;
  int lines = 0;
  while(std::getline(stream, file_line)) {
    lines++;
    (void)lines;
    
    if(section_save && file_line.empty()) {
      section_save = false;
      break;
    }

    if(section_save) {
      vpr_pb_types_usage_section.push_back(file_line);
    }

    // note: the if condition here is *after* the save, because we don't want to save this 'marker' line!
    if(std::string("Pb types usage...") == file_line) {
      section_save = true;
    }
  }
  
  stream.close();

  // debug
  // int index = 0;
  // for (std::string each_line: vpr_pb_types_usage_section) {
  //   std::cout << index++ << " : " << each_line << std::endl;
  // }


  // find the min_num_indent_spaces for the level 1 hierarchy (usually 2 spaces indented, but we don't assume)
  int min_num_indent_spaces = -1;
  for (std::string each_line: vpr_pb_types_usage_section) {

    int line_length = each_line.size();
    int num_spaces = 0;

    for (int index=0; index < line_length; index++) {

      if(!std::isspace(each_line[index])) {
        num_spaces = index;
        break;
      }
      
    }

    if(min_num_indent_spaces == -1) {
      min_num_indent_spaces = num_spaces;
    }
    else {
      min_num_indent_spaces = std::min(min_num_indent_spaces, num_spaces);
    }

    // debug
    // std::cout << num_spaces << "  " << min_num_indent_spaces << std::endl;
  }


  // split into sections
  std::vector<std::string> io_section;
  std::vector<std::string> clb_section;
  std::vector<std::string> bram_section;
  std::vector<std::string> dsp_section;

  bool io_section_save = false;
  bool clb_section_save = false;
  bool bram_section_save = false;
  bool dsp_section_save = false;
  for (std::string each_line: vpr_pb_types_usage_section) {

    int line_length = each_line.size();
    int num_spaces = 0;

    for (int index=0; index < line_length; index++) {
      if(!std::isspace(each_line[index])) {
        num_spaces = index;
        break;
      }
    }

    if(num_spaces == min_num_indent_spaces) {
      // level 1 section begins, figure out which section it is.
      io_section_save = false;
      clb_section_save = false;
      bram_section_save = false;
      dsp_section_save = false;

      if (each_line.rfind("io", min_num_indent_spaces) == (unsigned)min_num_indent_spaces) io_section_save = true;
      else if (each_line.rfind("clb", min_num_indent_spaces) ==  (unsigned)min_num_indent_spaces) clb_section_save = true;
      else if (each_line.rfind("bram", min_num_indent_spaces) ==  (unsigned)min_num_indent_spaces) bram_section_save = true;
      else if (each_line.rfind("dsp", min_num_indent_spaces) ==  (unsigned)min_num_indent_spaces) dsp_section_save = true;
    }

    if(io_section_save) {
      io_section.push_back(each_line);
    }
    else if(clb_section_save) {
      clb_section.push_back(each_line);
    }
    else if(bram_section_save) {
      bram_section.push_back(each_line);
    }
    else if(dsp_section_save) {
      dsp_section.push_back(each_line);
    }
  }

  // debug
  // int index = 0;
  // std::cout << "\n io_section" << std::endl;
  // for (std::string each_line: io_section) {
  //   std::cout << index++ << " : " << each_line << std::endl;
  // }
  // index = 0;
  // std::cout << "\n clb_section" <<std::endl;
  // for (std::string each_line: clb_section) {
  //   std::cout << index++ << " : " << each_line << std::endl;
  // }
  // index = 0;
  // std::cout << "\n bram_section" <<std::endl;
  // for (std::string each_line: bram_section) {
  //   std::cout << index++ << " : " << each_line << std::endl;
  // }
  // index = 0;
  // std::cout << "\n dsp_section" << std::endl;
  // for (std::string each_line: dsp_section) {
  //   std::cout << index++ << " : " << each_line << std::endl;
  // }

  // extract information, use regex for robust matching
  std::regex regex;
  std::smatch smatches;
  bool found;

  // io section
  // debug
  // std:: cout << "\n report_io_section: " << std::endl;
  unsigned int io_output = 0;
  unsigned int io_input = 0;
  try {
    if(io_section.size() > 0) {

      for (std::string each_line: io_section) {

        regex = std::regex("\\s+io_output\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          io_output = std::stoul(smatches.str(1));
        }

        regex = std::regex("\\s+io_input\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          io_input = std::stoul(smatches.str(1));
        }
      }
    }
  }
  catch (std::invalid_argument const &e) {
    std::cout << "[utilization] Bad input: std::invalid_argument thrown: " << "io" << std::endl;
  }
  catch (std::out_of_range const &e) {
    std::cout << "[utilization] Integer overflow: std::out_of_range thrown: " << "io" << std::endl;
  }
  // debug
  // std:: cout << "io_output: " << io_output << std::endl;
  // std:: cout << "io_input: " << io_input << std::endl;



  // clb section parse
  // debug
  // std:: cout << "\n report_clb_section: " << std::endl;
  unsigned int clb = 0;
  unsigned int fle = 0;
  std::string report_clb_clb = "0";
  std::string report_clb_fle = "0";
  try {
    if(clb_section.size() > 0) {

      for (std::string each_line: clb_section) {

        regex = std::regex("\\s+clb\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          clb = std::stoul(smatches.str(1));
        }

        regex = std::regex("\\s+fle\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          fle = std::stoul(smatches.str(1));
        }
      }
    }
  }
  catch (std::invalid_argument const &e) {
    std::cout << "[utilization] Bad input: std::invalid_argument thrown: " << "clb" << std::endl;
  }
  catch (std::out_of_range const &e) {
    std::cout << "[utilization] Integer overflow: std::out_of_range thrown: " << "clb" << std::endl;
  }
  // debug
  // std:: cout << "clb: " << clb << std::endl;
  // std:: cout << "fle: " << fle << std::endl;



  // clb subsections parse [ble6 and lut5inter and shift_reg]
  // clb -> next indent is fle
  // fle -> next indent is ble6 and lut5inter and shift_reg
  // so we take indent = min_indent + 2 to find these 2 sections.
  std::vector<std::string> ble6_section;
  std::vector<std::string> lut5inter_section;
  std::vector<std::string> shift_reg_section;
  bool ble6_section_save = false;
  bool lut5inter_section_save = false;
  bool shift_reg_section_save = false;
  int fle_section_num_indent_spaces = min_num_indent_spaces + 2;
  for (std::string each_line: clb_section) {

    int line_length = each_line.size();
    int num_spaces = 0;

    for (int index=0; index < line_length; index++) {
      if(!std::isspace(each_line[index])) {
        num_spaces = index;
        break;
      }
    }

    if(num_spaces == fle_section_num_indent_spaces) {
      // level 3 section begins, figure out which section it is.
      ble6_section_save = false;
      lut5inter_section_save = false;
      shift_reg_section_save = false;

      if (each_line.rfind("ble6", fle_section_num_indent_spaces) == (unsigned)fle_section_num_indent_spaces) ble6_section_save = true;
      else if (each_line.rfind("lut5inter", fle_section_num_indent_spaces) ==  (unsigned)fle_section_num_indent_spaces) lut5inter_section_save = true;
      else if (each_line.rfind("shift_reg", fle_section_num_indent_spaces) ==  (unsigned)fle_section_num_indent_spaces) shift_reg_section_save = true;
    }

    if(ble6_section_save) {
      ble6_section.push_back(each_line);
    }
    else if(lut5inter_section_save) {
      lut5inter_section.push_back(each_line);
    }
    else if(shift_reg_section_save) {
      shift_reg_section.push_back(each_line);
    }
  }
  // debug
  // int index = 0;
  // std::cout << "\n ble6_section" << std::endl;
  // for (std::string each_line: ble6_section) {
  //   std::cout << index++ << " : " << each_line << std::endl;
  // }
  // index = 0;
  // std::cout << "\n lut5inter_section" <<std::endl;
  // for (std::string each_line: lut5inter_section) {
  //   std::cout << index++ << " : " << each_line << std::endl;
  // }
  // index = 0;
  // std::cout << "\n shift_reg_section" <<std::endl;
  // for (std::string each_line: shift_reg_section) {
  //   std::cout << index++ << " : " << each_line << std::endl;
  // }



  // split the lut5inter_ble section into 2 parts:
  // flut5 section
  // soft_adder section
  // clb subsections parse [ble6 and lut5inter and shift_reg]
  // clb -> next indent is fle
  // fle -> next indent is ble6 and lut5inter and shift_reg
  // so we take indent = min_indent + 2 to find these 2 sections.
  std::vector<std::string> lut5inter_ble_flut5_section;
  std::vector<std::string> lut5inter_ble_soft_adder_section;
  bool lut5inter_ble_flut5_section_save = false;
  bool lut5inter_ble_soft_adder_section_save = false;
  int lut5inter_ble_num_indent_spaces = min_num_indent_spaces + 4 ;
  for (std::string each_line: lut5inter_section) {

    int line_length = each_line.size();
    int num_spaces = 0;

    for (int index=0; index < line_length; index++) {
      if(!std::isspace(each_line[index])) {
        num_spaces = index;
        break;
      }
    }

    if(num_spaces == lut5inter_ble_num_indent_spaces) {
      // level 3 section begins, figure out which section it is.
      lut5inter_ble_flut5_section_save = false;
      lut5inter_ble_soft_adder_section_save = false;

      if (each_line.rfind("flut5", lut5inter_ble_num_indent_spaces) == (unsigned)lut5inter_ble_num_indent_spaces) lut5inter_ble_flut5_section_save = true;
      else if (each_line.rfind("soft_adder", lut5inter_ble_num_indent_spaces) ==  (unsigned)lut5inter_ble_num_indent_spaces) lut5inter_ble_soft_adder_section_save = true;
    }

    if(lut5inter_ble_flut5_section_save) {
      lut5inter_ble_flut5_section.push_back(each_line);
    }
    else if(lut5inter_ble_soft_adder_section_save) {
      lut5inter_ble_soft_adder_section.push_back(each_line);
    }
  }
  // debug
  // int index = 0;
  // std::cout << "\n lut5inter_ble_flut5_section" << std::endl;
  // for (std::string each_line: lut5inter_ble_flut5_section) {
  //   std::cout << index++ << " : " << each_line << std::endl;
  // }
  // index = 0;
  // std::cout << "\n lut5inter_ble_soft_adder_section" <<std::endl;
  // for (std::string each_line: lut5inter_ble_soft_adder_section) {
  //   std::cout << index++ << " : " << each_line << std::endl;
  // }


  // debug
  // std:: cout << "\n report_ble6_section: " << std::endl;
  unsigned int ble6 = 0;
  unsigned int ble6_lut6 = 0;
  unsigned int ble6_ff = 0;
  try {
    if(ble6_section.size() > 0) {

      for (std::string each_line: ble6_section) {

        regex = std::regex("\\s+ble6\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          ble6 = std::stoul(smatches.str(1));
        }

        regex = std::regex("\\s+lut6\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          ble6_lut6 = std::stoul(smatches.str(1));
        }

        regex = std::regex("\\s+ff\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          ble6_ff = std::stoul(smatches.str(1));
        }
      }
    }
  }
  catch (std::invalid_argument const &e) {
    std::cout << "[utilization] Bad input: std::invalid_argument thrown: " << "ble6" << std::endl;
  }
  catch (std::out_of_range const &e) {
    std::cout << "[utilization] Integer overflow: std::out_of_range thrown: " << "ble6" << std::endl;
  }
  // debug
  // std:: cout << "ble6: " << ble6 << std::endl;
  // std:: cout << "ble6_lut6: " << ble6_lut6 << std::endl;
  // std:: cout << "ble6_ff: " << ble6_ff << std::endl;



  // debug
  // std:: cout << "\n report_lut5inter_section: " << std::endl;
  unsigned int lut5inter = 0;
  unsigned int lut5inter_ble5 = 0;
  try {
    if(lut5inter_section.size() > 0) {

      for (std::string each_line: lut5inter_section) {

        regex = std::regex("\\s+lut5inter\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          lut5inter = std::stoul((smatches.str(1)));
        }

        regex = std::regex("\\s+ble5\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          lut5inter_ble5 = std::stoul((smatches.str(1)));
        }
      }
    }
  }
  catch (std::invalid_argument const &e) {
    std::cout << "[utilization] Bad input: std::invalid_argument thrown: " << "lut5inter" << std::endl;
  }
  catch (std::out_of_range const &e) {
    std::cout << "[utilization] Integer overflow: std::out_of_range thrown: " << "lut5inter" << std::endl;
  }
  // debug
  // std:: cout << "lut5inter: " << lut5inter << std::endl;
  // std:: cout << "lut5inter_ble5: " << lut5inter_ble5 << std::endl;

  // debug
  // std:: cout << "\n report_lut5inter_ble_flut5_section: " << std::endl;
  unsigned int lut5inter_flut5_flut5 = 0;
  unsigned int lut5inter_flut5_lut5 = 0;
  unsigned int lut5inter_flut5_ff = 0;
  try {
    if(lut5inter_ble_flut5_section.size() > 0) {

      for (std::string each_line: lut5inter_ble_flut5_section) {

        regex = std::regex("\\s+flut5\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          lut5inter_flut5_flut5 = std::stoul((smatches.str(1)));
        }

        regex = std::regex("\\s+lut5\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          lut5inter_flut5_lut5 = std::stoul((smatches.str(1)));
        }

        regex = std::regex("\\s+ff\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          lut5inter_flut5_ff = std::stoul((smatches.str(1)));
        }
      }
    }
  }
  catch (std::invalid_argument const &e) {
    std::cout << "[utilization] Bad input: std::invalid_argument thrown: " << "lut5inter" << std::endl;
  }
  catch (std::out_of_range const &e) {
    std::cout << "[utilization] Integer overflow: std::out_of_range thrown: " << "lut5inter" << std::endl;
  }
  // debug
  // std:: cout << "lut5inter_flut5_flut5: " << lut5inter_flut5_flut5 << std::endl;
  // std:: cout << "lut5inter_flut5_lut5: " << lut5inter_flut5_lut5 << std::endl;
  // std:: cout << "lut5inter_flut5_ff: " << lut5inter_flut5_ff << std::endl;


  // debug
  // std:: cout << "\n report_lut5inter_ble_soft_adder_section: " << std::endl;
  unsigned int lut5inter_adder_adder = 0;
  unsigned int lut5inter_adder_lut4 = 0;
  unsigned int lut5inter_adder_ff = 0;
  try {
    if(lut5inter_ble_soft_adder_section.size() > 0) {

      for (std::string each_line: lut5inter_ble_soft_adder_section) {

        regex = std::regex("\\s+adder\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          lut5inter_adder_adder = std::stoul((smatches.str(1)));
        }

        regex = std::regex("\\s+lut4\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          lut5inter_adder_lut4 = std::stoul((smatches.str(1)));
        }

        regex = std::regex("\\s+ff\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          lut5inter_adder_ff = std::stoul((smatches.str(1)));
        }
      }
    }
  }
  catch (std::invalid_argument const &e) {
    std::cout << "[utilization] Bad input: std::invalid_argument thrown: " << "lut5inter" << std::endl;
  }
  catch (std::out_of_range const &e) {
    std::cout << "[utilization] Integer overflow: std::out_of_range thrown: " << "lut5inter" << std::endl;
  }
  // debug
  // std:: cout << "lut5inter_adder_adder: " << lut5inter_adder_adder << std::endl;
  // std:: cout << "lut5inter_adder_ff: " << lut5inter_adder_ff << std::endl;
  // std:: cout << "lut5inter_adder_lut4: " << lut5inter_adder_lut4 << std::endl;



  // debug
  // std:: cout << "\n report_shift_reg_section: " << std::endl;
  unsigned int shift_reg = 0;
  unsigned int shift_reg_ff = 0;
  try {
  if(shift_reg_section.size() > 0) {

    for (std::string each_line: shift_reg_section) {

      regex = std::regex("\\s+shift_reg\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
      found = std::regex_match ( each_line, smatches, regex );
      if(found) {
        shift_reg = std::stoul(smatches.str(1));
      }

      regex = std::regex("\\s+ff\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
      found = std::regex_match ( each_line, smatches, regex );
      if(found) {
        shift_reg_ff = std::stoul(smatches.str(1));
      }
    }
  }
  }
  catch (std::invalid_argument const &e) {
    std::cout << "[utilization] Bad input: std::invalid_argument thrown: " << "shift_reg" << std::endl;
  }
  catch (std::out_of_range const &e) {
    std::cout << "[utilization] Integer overflow: std::out_of_range thrown: " << "shift_reg" << std::endl;
  }
  // debug
  // std:: cout << "shift_reg: " << shift_reg << std::endl;
  // std:: cout << "shift_reg_ff: " << shift_reg_ff << std::endl;



  // bram section
  // debug
  // std:: cout << "\n report_bram_section: " << std::endl;
  unsigned int bram_nonsplit = 0;
  std::unordered_map<std::string, unsigned int> bram_nonsplit_numbers_map;
  unsigned int bram_split = 0;
  std::unordered_map<std::string, unsigned int> bram_split_numbers_map;
  unsigned int fifo_nonsplit = 0;
  std::unordered_map<std::string, unsigned int> fifo_nonsplit_numbers_map;
  unsigned int fifo_split = 0;
  std::unordered_map<std::string, unsigned int> fifo_split_numbers_map;
  try {
    if(bram_section.size() > 0) {

      unsigned int value;

      for (std::string each_line: bram_section) {

        regex = std::regex("\\s+(mem_36K_BRAM_.+_nonsplit)\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          value = std::stoul(smatches.str(2));
          bram_nonsplit_numbers_map[smatches.str(1)] = value;
          bram_nonsplit += value;
        }

        regex = std::regex("\\s+(mem_36K_BRAM_.+_split)\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          value = std::stoul(smatches.str(2));
          bram_split_numbers_map[smatches.str(1)] = value;
          bram_split += value;
        }

        regex = std::regex("\\s+(mem_36K_FIFO_.+_nonsplit)\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          value = std::stoul(smatches.str(2));
          fifo_nonsplit_numbers_map[smatches.str(1)] = value;
          fifo_nonsplit += value;
        }

        regex = std::regex("\\s+(mem_36K_FIFO_.+_split)\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          value = std::stoul(smatches.str(2));
          fifo_split_numbers_map[smatches.str(1)] = value;
          fifo_split += value;
        }
      }
    }
  }
  catch (std::invalid_argument const &e) {
    std::cout << "[utilization] Bad input: std::invalid_argument thrown: " << "bram" << std::endl;
  }
  catch (std::out_of_range const &e) {
    std::cout << "[utilization] Integer overflow: std::out_of_range thrown: " << "bram" << std::endl;
  }
  // debug
  // std:: cout << "bram_nonsplit: " << bram_nonsplit << std::endl;
  // for (const auto & [ key, value ] : bram_nonsplit_numbers_map) {
  //   std::cout << key << ": " << value << std::endl;
  // }
  // std:: cout << "bram_split: " << bram_split << std::endl;
  // for (const auto & [ key, value ] : bram_split_numbers_map) {
  //   std::cout << key << ": " << value << std::endl;
  // }
  // std:: cout << "fifo_nonsplit: " << fifo_nonsplit << std::endl;
  // for (const auto & [ key, value ] : fifo_nonsplit_numbers_map) {
  //   std::cout << key << ": " << value << std::endl;
  // }
  // std:: cout << "fifo_split: " << fifo_split << std::endl;
  // for (const auto & [ key, value ] : fifo_split_numbers_map) {
  //   std::cout << key << ": " << value << std::endl;
  // }


  // dsp section
  // debug
  // std:: cout << "\n report_dsp_section: " << std::endl;
  unsigned int dsp = 0;
  std::unordered_map<std::string, unsigned int> dsp_numbers_map;
  try {
    if(dsp_section.size() > 0) {

      unsigned int value;

      for (std::string each_line: dsp_section) {

        regex = std::regex("\\s+(QL_DSP2.*)\\s+:\\s+(\\d+)\\s*", std::regex::ECMAScript);
        found = std::regex_match ( each_line, smatches, regex );
        if(found) {
          value = std::stoul(smatches.str(2));
          dsp_numbers_map[smatches.str(1)] = value;
          dsp += value;
        }

      }
    }
  }
  catch (std::invalid_argument const &e) {
    std::cout << "[utilization] Bad input: std::invalid_argument thrown: " << "dsp" << std::endl;
  }
  catch (std::out_of_range const &e) {
    std::cout << "[utilization] Integer overflow: std::out_of_range thrown: " << "dsp" << std::endl;
  }
  // debug
  // std:: cout << "dsp: " << dsp << std::endl;
  // for (const auto & [ key, value ] : dsp_numbers_map) {
  //   std::cout << key << ": " << value << std::endl;
  // }


  // final calculations for the detailed utilization

  // [1] CLB/FLE
  // total CLBs = clb
  // total FLEs available in CLBs = clb*10
  // FLEs used = fle


  // [2] LUT
  // BLE6 can be used as LUT6 only or LUT6+FF or FF only ( lut6 mode ff only never happens practically!)
  unsigned int ble6_lut6_only = 0;
  unsigned int ble6_ff_only = 0;
  unsigned int ble6_lut6_ff = 0;
  // VPR report interpreted as equations:
  // ble6 == lut6only + lut6ff + ffonly --> EQ1
  // ble6_lut6 == lut6only + lut6ff --> EQ2
  // ble6_ff == lut6ff + ffonly --> EQ3
  //
  // using EQ2, EQ1 can be rewritten:
  // ble6 = ble6_lut6 + ffonly, so:   ffonly = ble6 - ble6_lut6           --> EQ4 (obtain ffonly)
  // from EQ3, we can write:          lut6ff = ble6_ff - ffonly           --> EQ5 (obtain lut6ff)
  // from EQ1,                        lut6only = ble6 - lut6ff - ffonly   --> EQ6 (obtain lut6only)
  //
  ble6_ff_only = ble6 - ble6_lut6;
  ble6_lut6_ff = ble6_ff - ble6_ff_only;
  ble6_lut6_only = ble6 - ble6_lut6_ff - ble6_ff_only;


  // FLUT5 can be used as LUT5 only or LUT5+FF or FF only
  unsigned int flut5_lut5_only = 0;
  unsigned int flut5_ff_only = 0;
  unsigned int flut5_lut5_ff = 0;
  // VPR report interpreted as equations:
  // flut5 == lut5only + lut5ff + ffonly --> EQ1
  // lut5 == lut5only + lut5ff --> EQ2
  // ff == lut5ff + ffonly --> EQ3
  //
  // using EQ2, EQ1 can be rewritten:
  // flut5 = lut5 + ffonly, so:   ffonly = flut5 - lut5                 --> EQ4 (obtain ffonly)
  // from EQ3, we can write:      lut5ff = ff - ffonly                  --> EQ5 (obtain lut5ff)
  // from EQ1,                    lut5only = flut5 - lut5ff - ffonly    --> EQ6 (obtain lut5only)
  //
  flut5_ff_only = lut5inter_flut5_flut5 - lut5inter_flut5_lut5;
  flut5_lut5_ff = lut5inter_flut5_ff - flut5_ff_only;
  flut5_lut5_only = lut5inter_flut5_flut5 - flut5_lut5_ff - flut5_ff_only;

  // LUT4 in soft_adder = lut5inter_adder_lut4
  // FF in soft_adder = lut5inter_adder_ff

  // total LUTs utilized
  unsigned int total_LUTs = ble6_lut6 + lut5inter_flut5_lut5 + lut5inter_adder_lut4;


  // [3] FF
  // total FFs utilized
  unsigned int total_FFs = ble6_ff + lut5inter_flut5_ff + lut5inter_adder_ff + shift_reg_ff;
  // from BLE6 mode FLEs = ble6_ff
  // from BLE5 mode FLEs = lut5inter_flut5_ff and lut5inter_adder_ff
  // from shift_reg mode FLEs = shift_reg_ff



  // [4] BRAM
  // total BRAMs utilized
  unsigned int total_brams = bram_split + bram_nonsplit + fifo_split + fifo_nonsplit;
  //  as non split 36k RAM blocks = bram_nonsplit
  //    breakdown the list using: bram_nonsplit_numbers_map
  //  as split 2x18k RAM blocks = bram_split
  //    breakdown the list using: bram_split_numbers_map
  //  as non split 36k FIFO blocks = fifo_nonsplit
  //    breakdown the list using: fifo_nonsplit_numbers_map
  //  as split 2x18k FIFO blocks = fifo_split
  //    breakdown the list using: fifo_split_numbers_map


  // [5] DSP
  // total DSPs utilized = dsp
  // breakdown the list using: dsp_numbers_map


  // [6] IO
  // total IOs utilized
  unsigned int total_ios = io_input + io_output;
  //   as output = io_output
  //   as inputs = io_input
  // Q: any buffer flipflops? DFF? DFFN


  // double-check calculations:
  // [1] fle = ble6 + lut5inter + shift_reg
  // [2] lut5inter in terms of ble5: ble5 <= lut5inter*2 and ble5 >= lut5inter (ble5 is one-half unit of a lut5inter, so ...)
  // [3] ble5 = flut5 + adder
  if( !(fle == ble6 + lut5inter + shift_reg) ) {
    // std::cout << "[error]: !(fle == ble6 + lut5inter + shift_reg)" << std::endl;
  }
  if( !(lut5inter_ble5 >= lut5inter) ) {
    // std::cout << "[error]: !(ble5 >= lut5inter)" << std::endl;
  }
  if( !(lut5inter_ble5 <= lut5inter*2) ) {
    // std::cout << "[error]: !(ble5 <= lut5inter*2)" << std::endl;
  }
  if( !(lut5inter_ble5 = lut5inter_flut5_flut5 + lut5inter_adder_adder) ) {
    // std::cout << "[error]: !(lut5inter_ble5 = lut5inter_flut5_flut5 + lut5inter_adder_adder)" << std::endl;
  }

  // store the final numbers into the utilization object:
  aurora_routing_utilization.clb = clb;
  aurora_routing_utilization.fle = clb*10;
  aurora_routing_utilization.clb_fle = fle;
  
  aurora_routing_utilization.clb_lut = total_LUTs;
  aurora_routing_utilization.clb_lut_lut4 = lut5inter_adder_lut4;
  aurora_routing_utilization.clb_lut_lut5 = flut5_lut5_only;
  aurora_routing_utilization.clb_lut_lut5ff = flut5_lut5_ff;
  aurora_routing_utilization.clb_lut_lut6 = ble6_lut6_only;
  aurora_routing_utilization.clb_lut_lut6ff = ble6_lut6_ff;

  aurora_routing_utilization.clb_ff = total_FFs;
  aurora_routing_utilization.clb_ff_lut5ff = flut5_lut5_ff;
  aurora_routing_utilization.clb_ff_lut6ff = ble6_lut6_ff;
  aurora_routing_utilization.clb_ff_ff = ble6_ff_only + flut5_ff_only + lut5inter_adder_ff;
  aurora_routing_utilization.clb_ff_shiftreg = shift_reg_ff;

  aurora_routing_utilization.bram = total_brams;
  aurora_routing_utilization.bram_bram_split = bram_split;
  aurora_routing_utilization.bram_bram_split_map = bram_split_numbers_map;
  aurora_routing_utilization.bram_bram_nonsplit = bram_nonsplit;
  aurora_routing_utilization.bram_bram_nonsplit_map = bram_nonsplit_numbers_map;
  aurora_routing_utilization.bram_fifo_split = fifo_split;
  aurora_routing_utilization.bram_fifo_split_map = fifo_split_numbers_map;
  aurora_routing_utilization.bram_fifo_nonsplit = fifo_nonsplit;
  aurora_routing_utilization.bram_fifo_nonsplit_map = fifo_nonsplit_numbers_map;

  aurora_routing_utilization.dsp = dsp;
  aurora_routing_utilization.dsp_map = dsp_numbers_map;

  aurora_routing_utilization.io = total_ios;
  aurora_routing_utilization.io_input = io_input;
  aurora_routing_utilization.io_output = io_output;

  bool debug_extended = false;

  std::filesystem::path utilization_rpt_filepath = 
      std::filesystem::path(GlobalSession->GetCompiler()->ProjManager()->projectPath()) / "routing_utilization.rpt";

  std::string level_1_indent = "  ";
  std::string level_2_indent = "    ";
  std::string level_3_indent = "      ";
    
  std::ofstream utilization_rpt;
  utilization_rpt.open(utilization_rpt_filepath);

  utilization_rpt << "## Resource Utilization Report ##\n\n" << std::endl;
  
  utilization_rpt << "top module: " << getStringValue("synthesis", "top_module") << "\n\n" << std::endl;

  utilization_rpt << "Resource Usage:" << std::endl;

  utilization_rpt << level_1_indent << aurora_routing_utilization.clb << " CLB used" << std::endl;
  utilization_rpt << level_1_indent << "which contains " <<aurora_routing_utilization.fle << " FLE" << std::endl;
  utilization_rpt << level_1_indent << "of which, " << fle << " FLE used" << std::endl;
  utilization_rpt << "" << std::endl;

  utilization_rpt << level_2_indent << aurora_routing_utilization.clb_lut << " LUT used" << std::endl;
  if(aurora_routing_utilization.clb_lut > 0) {
    if(aurora_routing_utilization.clb_lut_lut6 > 0) {
      utilization_rpt << level_3_indent << aurora_routing_utilization.clb_lut_lut6 << " as LUT6" << std::endl;
    }
    if(aurora_routing_utilization.clb_lut_lut6ff > 0) {
      utilization_rpt << level_3_indent << aurora_routing_utilization.clb_lut_lut6ff << " as LUT6+FF" << std::endl;
    }
    if(aurora_routing_utilization.clb_lut_lut5 > 0) {
      utilization_rpt << level_3_indent << aurora_routing_utilization.clb_lut_lut5 << " as LUT5" << std::endl;
    }
    if(aurora_routing_utilization.clb_lut_lut5ff > 0) {
      utilization_rpt << level_3_indent << aurora_routing_utilization.clb_lut_lut5ff << " as LUT5+FF" << std::endl;
    }
    if(aurora_routing_utilization.clb_lut_lut4 > 0) {
      utilization_rpt << level_3_indent << aurora_routing_utilization.clb_lut_lut4 << " as LUT4 (adder carry chain)" << std::endl;
    }
  }
  utilization_rpt << "" << std::endl;

  utilization_rpt << level_2_indent << aurora_routing_utilization.clb_ff << " FF used" << std::endl;
  if(aurora_routing_utilization.clb_ff > 0) {
    if(aurora_routing_utilization.clb_ff_lut6ff > 0) {
      utilization_rpt << level_3_indent << aurora_routing_utilization.clb_ff_lut6ff << " as LUT6+FF combination" << std::endl;
    }
    if(aurora_routing_utilization.clb_ff_lut5ff > 0) {
      utilization_rpt << level_3_indent << aurora_routing_utilization.clb_ff_lut5ff << " as LUT5+FF combination" << std::endl;
    }
    if(aurora_routing_utilization.clb_ff_ff > 0) {
      utilization_rpt << level_3_indent << aurora_routing_utilization.clb_ff_ff << " as FF only" << std::endl;
    }
    if(aurora_routing_utilization.clb_ff_shiftreg > 0) {
      utilization_rpt << level_3_indent << aurora_routing_utilization.clb_ff_shiftreg << " as FF in Shift Register mode" << std::endl;
    }
  }
  utilization_rpt << "" << std::endl;

  utilization_rpt << level_1_indent << aurora_routing_utilization.bram << " BRAM used" << std::endl;
  if(aurora_routing_utilization.bram > 0) {
    if((aurora_routing_utilization.bram_bram_nonsplit + aurora_routing_utilization.bram_fifo_nonsplit) > 0) {
      utilization_rpt << level_2_indent << (aurora_routing_utilization.bram_bram_nonsplit + aurora_routing_utilization.bram_fifo_nonsplit) << " as 36k BRAM blocks" << std::endl;
      for (const auto & [ key, value ] : aurora_routing_utilization.bram_bram_nonsplit_map) {
        utilization_rpt << level_3_indent << value << " of " << key << std::endl;
      }
      for (const auto & [ key, value ] : aurora_routing_utilization.bram_fifo_nonsplit_map) {
        utilization_rpt << level_3_indent << value << " of " << key << std::endl;
      }
    }
    if((aurora_routing_utilization.bram_bram_split + aurora_routing_utilization.bram_fifo_split) > 0) {
      utilization_rpt << level_2_indent << (aurora_routing_utilization.bram_bram_split + aurora_routing_utilization.bram_fifo_split) << " as 2x18k BRAM blocks" << std::endl;
      for (const auto & [ key, value ] : aurora_routing_utilization.bram_bram_split_map) {
        utilization_rpt << level_3_indent << value << " of " << key << std::endl;
      }
      for (const auto & [ key, value ] : aurora_routing_utilization.bram_fifo_split_map) {
        utilization_rpt << level_3_indent << value << " of " << key << std::endl;
      }
    }
  }
  utilization_rpt << "" << std::endl;

  utilization_rpt << level_1_indent << aurora_routing_utilization.dsp << " DSP used" << std::endl;
  if(aurora_routing_utilization.dsp > 0) {
    // utilization_rpt << level_2_indent << "list of DSP block types:" << std::endl;
    for (const auto & [ key, value ] : aurora_routing_utilization.dsp_map) {
      utilization_rpt << level_2_indent << value << " of " << key << std::endl;
    }
  }
  utilization_rpt << "" << std::endl;

  utilization_rpt << level_1_indent << aurora_routing_utilization.io << " IO used" << std::endl;
  if(aurora_routing_utilization.io > 0) {
    if(aurora_routing_utilization.io_input > 0) {
      utilization_rpt << level_2_indent << aurora_routing_utilization.io_input << " as input" << std::endl;
    }
    if(aurora_routing_utilization.io_output > 0) {
      utilization_rpt << level_2_indent << aurora_routing_utilization.io_output << " as output" << std::endl;
    }
  }
  utilization_rpt << "" << std::endl;

  if(debug_extended) {
    utilization_rpt << "\n----------------------------------------" << std::endl;
    utilization_rpt << "ble6:                       " << ble6 << std::endl;
    utilization_rpt << " ble6_lut6:                 " << ble6_lut6 << std::endl;
    utilization_rpt << " ble6_ff:                   " << ble6_ff << std::endl;
    utilization_rpt << "lut5inter:                  " << lut5inter << std::endl;
    utilization_rpt << " lut5inter_ble5:            " << lut5inter_ble5 << std::endl;
    utilization_rpt << "  lut5inter_flut5_flut5:    " << lut5inter_flut5_flut5 << std::endl;
    utilization_rpt << "   lut5inter_flut5_lut5:    " << lut5inter_flut5_lut5 << std::endl;
    utilization_rpt << "   lut5inter_flut5_ff:      " << lut5inter_flut5_ff << std::endl;
    utilization_rpt << " lut5inter_adder_adder:     " << lut5inter_adder_adder << std::endl;
    utilization_rpt << "  lut5inter_adder_lut4:     " << lut5inter_adder_lut4 << std::endl;
    utilization_rpt << "  lut5inter_adder_ff:       " << lut5inter_adder_ff << std::endl;
    utilization_rpt << "shift_reg:                  " << shift_reg << std::endl;
    utilization_rpt << " shift_reg_ff:              " << shift_reg_ff << std::endl;
    utilization_rpt << "----------------------------------------" << std::endl;
    utilization_rpt << "" << std::endl;
  }

  utilization_rpt << "" << std::endl;
  utilization_rpt << "## Resource Utilization Complete ##" << std::endl;

  utilization_rpt.close();
}


}
