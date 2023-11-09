/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "FileLoaderOldStructure.h"

#include <filesystem>

#include "Compiler/CompilerDefines.h"
#include "MainWindow/CompressProject.h"
#include "Utils/FileUtils.h"

namespace FOEDAG {

FileLoaderOldStructure::FileLoaderOldStructure(const QString &projectFileName)
    : m_project(projectFileName) {}

std::pair<bool, QString> FileLoaderOldStructure::Migrate() const {
  if (m_project.isEmpty())
    return {false, "Project path not specified"};  // internal
  auto path = fs::path{m_project.toStdString()};

  auto result = CompressProject::CompressZip(path.parent_path(),
                                             path.stem().filename().string());
  if (!result.first) return {false, "Failed to create backup"};

  // remove old files
  auto projectName = path.stem().filename().string();
  const auto folders = FoldersToRemove(projectName);
  for (auto &folder : folders) {
    std::error_code errorCode;
    std::filesystem::remove_all(path.parent_path() / folder, errorCode);
  }
  auto filesToRemove = FilesToRemove(projectName);
  filesToRemove += FilesByRegex(
      path.parent_path(),
      {std::regex{".+_post_synthesis\\..+"}, std::regex{".+_stars\\..+"}});
  filesToRemove +=
      FilesByExtencion(path.parent_path(), {".cf", ".out", ".fst"});
  for (const auto &file : filesToRemove) {
    FileUtils::removeFile(path.parent_path() / file);
  }
  return {true, QString{}};
}

StringVector FileLoaderOldStructure::FilesToRemove(
    const std::string &projectName) {
  return {/*analysis*/ ANALYSIS_LOG,
          "port_info.json",
          "hier_info.json",
          projectName + "_analyzer.cmd", /*synth*/
          std::string{projectName + "_post_synth.blif"},
          std::string{projectName + "_post_synth.eblif"},
          std::string{projectName + "_post_synth.edif"},
          std::string{projectName + "_post_synth.v"},
          std::string{projectName + "_post_synth.vhd"},
          std::string{projectName + ".ys"},
          std::string{projectName + "_synth.log"},
          SYNTHESIS_LOG /*pack*/,
          std::string{projectName + "_post_synth.net"},
          std::string{projectName + "_pack.cmd"},
          "check_rr_node_warnings.log",
          "packing_pin_util.rpt",
          "pre_pack.report_timing.setup.rpt",
          std::string{projectName + "_openfpga.sdc"},
          std::string{projectName + "_post_synth_ports.json"},
          "vpr_stdout.log",
          PACKING_LOG /*place*/,
          "packing_pin_util.rpt",
          std::string{projectName + "_post_place_timing.rpt"},
          std::string{projectName + "_post_synth_ports.json"},
          std::string{projectName + "_place.cmd"},
          std::string{projectName + "_openfpga.pcf"},
          "check_rr_node_warnings.log",
          std::string{projectName + "_post_synth.place"},
          std::string{projectName + "_pin_loc.cmd"},
          std::string{projectName + "_pin_loc.place"},
          "post_place_timing.rpt",
          PLACEMENT_LOG /*routing*/,
          "check_rr_node_warnings.log",
          std::string{projectName + "_post_synth_ports.json"},
          std::string{projectName + "_route.cmd"},
          std::string{projectName + "_post_synth.route"},
          "packing_pin_util.rpt",
          "post_place_timing.rpt",
          "post_route_timing.rpt",
          "report_timing.hold.rpt",
          "report_timing.setup.rpt",
          "report_unconstrained_timing.hold.rpt",
          "report_unconstrained_timing.setup.rpt",
          ROUTING_LOG /*timing*/,
          "check_rr_node_warnings.log",
          std::string{projectName + "_sta.cmd"},
          std::string{projectName + "_post_synth_ports.json"},
          "packing_pin_util.rpt",
          "post_place_timing.rpt",
          "post_route_timing.rpt",
          "post_ta_timing.rpt",
          "report_timing.hold.rpt",
          "report_timing.setup.rpt",
          "report_unconstrained_timing.hold.rpt",
          "report_unconstrained_timing.setup.rpt",
          TIMING_ANALYSIS_LOG,
          std::string{projectName + "_opensta.tcl"} /*power*/,
          "post_place_timing.rpt",
          "post_route_timing.rpt",
          "post_ta_timing.rpt",
          POWER_ANALYSIS_LOG /*bitstream*/,
          std::string{projectName + ".openfpga"},
          std::string{projectName + "_bitstream.cmd"},
          std::string{projectName + "_post_synth_ports.json"},
          "fabric_bitstream.bit",
          "fabric_independent_bitstream.xml",
          "packing_pin_util.rpt",
          "PinMapping.xml",
          "post_place_timing.rpt",
          "post_route_timing.rpt",
          "post_ta_timing.rpt",
          "report_timing.hold.rpt",
          "report_timing.setup.rpt",
          "report_unconstrained_timing.hold.rpt",
          "report_unconstrained_timing.setup.rpt",
          BITSTREAM_LOG /*simulation*/,
          "simulation_rtl.rpt",
          "simulation_gate.rpt",
          "simulation_pnr.rpt",
          "simulation_bitstream_front.rpt",
          "simulation_bitstream_back.rpt"};
}

StringVector FileLoaderOldStructure::FilesByRegex(
    const fs::path &path, const std::vector<std::regex> &regexes) {
  StringVector files;
  for (const auto &reg : regexes) {
    const auto found = FileUtils::FindFilesByName(path, reg);
    for (const auto &file : found) files.push_back(file.string());
  }
  return files;
}

StringVector FileLoaderOldStructure::FoldersToRemove(
    const std::string &projectName) {
  return {"obj_dir", "reports", projectName + ".runs"};
}

StringVector FileLoaderOldStructure::FilesByExtencion(
    const fs::path &path, const StringVector &extensions) {
  StringVector files;
  for (const auto &exe : extensions) {
    auto found = FileUtils::FindFilesByExtension(path, exe);
    for (const auto &file : found) files.push_back(file.string());
  }
  return files;
}

}  // namespace FOEDAG
