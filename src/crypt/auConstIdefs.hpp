/*============================================================================
 *                                                                           *
 *-  Copyright Notice  ------------------------------------------------------*
 *                                                                           *
 *    Licensed Materials - Property of QuickLogic Corp.                      *
 *    Copyright (C) 2014 QuickLogic Corporation                              *
 *    All rights reserved                                                    *
 *    Use, duplication, or disclosure restricted                             *
 *                                                                           *
 *    File   : auConstIdefs.hpp                                              *
 *    Purpose: Stores all constant identifiers and variables.                *
 *                                                                           *
 *===========================================================================*/

#ifndef AUCONSTIDEFS_HPP
#define	AUCONSTIDEFS_HPP

#include <string>
#ifdef WIN32
#include <cstdint>
#endif
#include "auVersion.hpp"

using std::string;

namespace auConstIdef {
    // String constants
	const string SW_STR = "SOFTWARE";
    const string QL_STR = auVersion::EDA_ORG_NAME;

    const string INSTALL_ROOT_ENV = auVersion::RELEASE_PREFIX + "_INSTALL_PATH";
    
    const string ENABLE_FIXED_DEVICE = "ENABLE_FIXED_DEVICE";
    const string ENABLE_SDC_FLOW = "ENABLE_SDC_FLOW";
	
    const string FIELD_SEP = ",";
    const string UNDER_SEP = "_";
    const string IDENT_SEP = ":";
    const string COND_SEP = "&";
    const string EXP_SEP = "=";
    const string EMPTY_STR = "";

    const string SPACE = " ";
    const string CRYPT_KEY = "Supp";
    const string ENCRYPT_EXTN = ".en";
    const string DB_EXTN = ".db";
    const string HEX_EXTN = ".hex";
#ifdef WIN32
    const string PATH_SEP = "\\";
#else
    const string PATH_SEP = "/";
#endif
    const string EOL_SEP = "\r";
    
    const string NA_STR  = "NA";
    const string QLAL4S3B = "QLAL4S3B";
    const string QLAL3S2 = "QLAL3S2";
    const string QL3P1K   = "QL3P1K";
    const string QL734A   = "QL734A";
    const string QL736A   = "QL736A";
    const string QL737A   = "QL737A";
    const string QL737B   = "QL737B";
    const string QL745A   = "QL745A";
    const string SMIC     = "SMIC";
    const string GF       = "GF";
    const string TSMC     = "TSMC";
    const string UMC      = "UMC";
    const string SAMSUNG  = "SAMSUNG";
    const string NODE_130  = "130";
    const string NODE_90  = "90";
    const string NODE_65  = "65";
    const string NODE_40  = "40";
    const string NODE_40_EFLASH  = "40_eFLASH";
    const string NODE_22  = "22";
    const string NODE_28  = "28";
    const string PROCESS_NODE_130 = "130nm";
    const string PROCESS_NODE_90 = "90nm";
    const string PROCESS_NODE_65 = "65nm";
    const string PROCESS_NODE_40 = "40nm";
    const string PROCESS_NODE_40_EFLASH = "40_eFLASHnm";
    const string PROCESS_NODE_22 = "22nm";
    const string PROCESS_NODE_28 = "28nm";
    const string NODE_UNIT_NM = "nm";
    
    const string PU90      = "PU90";
    const string PD64      = "PD64";
    const string WD30      = "WD30";
    const string PU225     = "PU225";
    const string AL4S3B    = "AL4S3B";
    const string PQFP144   = "PQFP144";
    const string WR42      = "WR42";
    const string PU64      = "PU64";
    
    const string CALI     = "CALI";
    const string ARCTICLINK4 = "ArcticLink 4";
    const string POLARPRO4 = "PolarPro 4";
    const string POLARPRO3 = "PP3";
    const string LIBERTY_STR = "liberty";
    const string ROUTING_STR = "routing";
    const string ROUTING_DELAY = "Routing_Delay";
    const string ARCTIC_PRO = "ArcticPro";
    const string ARCTIC_PRO_2 = "ArcticPro2";
    const string ARCTIC_PRO_3 = "ArcticPro3";
    const string POLAR_PRO_3 = "PolarPro3";
    const string QLF_K4N8 = "QLF_K4N8";
    const string QLF_K6N10 = "QLF_K6N10";
    const string QLF_K4N8_UMC_22 = "QLF_K4N8_UMC_22";
    const string QLF_K6N10_UMC_22 = "QLF_K6N10_UMC_22";
    const string QLF_K6N10_TSMC_22 = "QLF_K6N10_TSMC_22";
    const string QLF_K6N10_SKY_65 = "QLF_K6N10_SKY_65";
    const string QLF_K6N10_SKY_90 = "QLF_K6N10_SKY_90";
    const string QLF_K6N10_SKY_130 = "QLF_K6N10_SKY_130";
    const string ARCTIC_PRO_SMIC_40 = "ArcticPro_SMIC_40";
    const string ARCTIC_PRO_GF_40 = "ArcticPro_GF_40";
    const string ARCTIC_PRO_TSMC_40 = "ArcticPro_TSMC_40";
    const string ARCTIC_PRO_TSMC_40_EFLASH = "ArcticPro_TSMC_40_EFLASH";
    const string ARCTIC_PRO_2_GF_22 = "ArcticPro2_GF_22";
    const string ARCTIC_PRO_3_GF_22 = "ArcticPro3_GF_22";
    const string ARCTIC_PRO_3_SAMSUNG_28 = "ArcticPro3_Samsung_28";
    const string ARCTIC_PRO_2_TSMC_28 = "ArcticPro2_TSMC_28";
    const string ARCTIC_PRO_SMIC_40NM = "ArcticPro_SMIC_40NM";
    const string ARCTIC_PRO_GF_40NM = "ArcticPro_GF_40NM";
    const string ARCTIC_PRO_TSMC_40NM = "ArcticPro_TSMC_40NM";
    const string ARCTIC_PRO_TSMC_40NM_EFLASH = "ArcticPro_TSMC_40_EFLASHNM";
    const string ARCTIC_PRO_2_GF_22NM = "ArcticPro2_GF_22NM";
    const string ARCTIC_PRO_2_TSMC_28NM = "ArcticPro2_TSMC_28NM";
    const string POLAR_PRO_3_TSMC_65NM = "PolarPro3_TSMC_65NM";
    const string CONFIG_BIT_GEN = "Config bit generation";
    const string BACK_ANNOTATION = "Back annotation";
    const string BACKEND_DIR = "backendData";
    const string VPR_STR = "vpr";
    const string OPENFPGA_STR = "openfpga";
    const string FOUNDRY_SMIC = "SMIC";
    const string FOUNDRY_TSMC = "TSMC";
    const string FOUNDRY_GF = "GF";
    const string FOUNDRY_UMC = "UMC";    
    const string FOUNDRY_SAMSUNG = "Samsung";
    const string FOUNDRY_SKY = "SKY";
    const string OPENFPGA_PLUGIN = "aurora-openfpga-plugins";
    const string ARCH_STR = "arch";
    const string FABRIC_KEY = "fabric_key";
    
    const string LIB_FILE_COMMENT_STRING = "comment : ";
    const string QUOTE_STR = "\"";
    const string VERSION = "Version : ";
    const string SEMI_COLON_STR = ";";
    const string COMMENT_START_STR = "/*";
    const string COMMENT_END_STR = "*/";
    
    const string xmlFALSEText = "FALSE";
    const string xmlfalseText = "false";
    const string xmlNOText = "NO";
    const string xmlTRUEText = "TRUE";
    const string xmltrueText = "true";
    const string xmlYESText = "YES";

};

#endif	/* AUCONSTIDEFS_HPP */
