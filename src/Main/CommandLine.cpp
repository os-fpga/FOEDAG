/*
Copyright 2021 The Foedag team

GPL License

Copyright (c) 2021 The Open-Source FPGA Foundation

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

#include "CommandLine.h"
#include "CRFileCryptProc.hpp"
//#include "AUEnums.hpp"

enum class AUDeviceFamilyType : std::uint8_t {
    ARCTIC_PRO = 0,
    ARCTIC_PRO_2,
    ARCTIC_PRO_3,
    POLAR_PRO_3,
    QLF_K4N8,
    QLF_K6N10,
    MAX_COUNT
};

using namespace FOEDAG;

void CommandLine::ErrorAndExit(const std::string& message) {
  std::cerr << "ERROR: " << message << std::endl;
  exit(1);
}

bool CommandLine::FileExists(const std::filesystem::path& name) {
  std::error_code ec;
  return std::filesystem::exists(name, ec);
}

void CommandLine::processArgs() {
  for (int i = 1; i < m_argc; i++) {
    std::string token(m_argv[i]);
    if (token == "--batch") {
      m_withQt = false;
    } else if (token == "--compiler") {
      i++;
      if (i < m_argc)
        m_compilerName = m_argv[i];
      else {
        ErrorAndExit("Specify a valid compiler!");
      }
    } else if (token == "--qml") {
      m_withQml = true;
    } else if (token == "--replay") {
      i++;
      if (i < m_argc) {
        m_runGuiTest = m_argv[i];
        if (!FileExists(m_runGuiTest)) {
          ErrorAndExit("Cannot open replay file: " + m_runGuiTest);
        }
      } else
        ErrorAndExit("Specify a replay file!");
    } else if (token == "--verific") {
      m_useVerific = true;
    } else if (token == "--script") {
      i++;
      if (i < m_argc) {
        m_runScript = m_argv[i];
        if (!FileExists(m_runScript)) {
          ErrorAndExit("Cannot open script file: " + m_runScript);
        }
      } else
        ErrorAndExit("Specify a script file!");
    } else if (token == "--cmd") {
      i++;
      if (i < m_argc) {
        m_runTclCmd = m_argv[i];
      } else
        ErrorAndExit("Specify a Tcl command!");
    } else if (token == "--help") {
      m_help = true;
    } else if (token == "--version") {
      m_version = true;
    } else if (token == "--testcrypt") {
      std::cout << "Testing encryption and decryption: "<< std::endl;

      
      // load the cryption db file
      if (!CRFileCryptProc::getInstance()->loadCryptKeyDB("xmltest/QLF_K6N10_Supp.db")) {
          std::cout << "Failed loading db file "<< std::endl;
          //return false;
      }

      set<string> fileExtn;
      //fileExtn.insert(dbConstIdef::VERI_EXTN);
      //fileExtn.insert(dbConstIdef::XML_EXTN);
      fileExtn.insert(".xml");
      if (!CRFileCryptProc::getInstance()->encryptFiles("xmltest", fileExtn, "QLF_K6N10"))
      {
          std::cout << "encrypt files failed: "<< std::endl;
          //db_1190(primDirName, lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
          //return false;
      }
      else
      {
        std::cout << "encrypt files passed: "<< std::endl;    
      }



      string vprArchXmlFileEn = "xmltest/vpr.xml.en";
      string openfpgaArchXmlFileEn = "xmltest/openfpga.xml.en";
      string vprArchXmlFileDecrypted = "xmltest/vpr_decrypted.xml";
      string openfpgaArchXmlFileDecrypted = "xmltest/openfpga_decrypted.xml";

      stringstream vprstrm;
      if (!CRFileCryptProc::getInstance()->decryptFile(vprArchXmlFileEn, vprstrm)) {
        // erroe while decrypt file
        std::cout << "decrypting vpr xml failed: "<< std::endl;
        //return false;
      }

      std::ofstream vprofs(vprArchXmlFileDecrypted);
      vprofs << vprstrm.str();
      vprofs.close();

      stringstream openfpgastrm;
      if (!CRFileCryptProc::getInstance()->decryptFile(openfpgaArchXmlFileEn, openfpgastrm)) {
        // erroe while decrypt file
        std::cout << "decrypting openfpga xml failed: "<< std::endl;
        //return false;
      }

      std::ofstream openfpgaofs(openfpgaArchXmlFileDecrypted);
      openfpgaofs << openfpgastrm.str();
      openfpgaofs.close();

    } else {
      std::cout << "ERROR Unknown command line option: " << m_argv[i]
                << std::endl;
      exit(1);
    }
  }
}

CommandLine::~CommandLine() {}
