/*============================================================================
 *                                                                           *
 *-  Copyright Notice  ------------------------------------------------------*
 *                                                                           *
 *    Licensed Materials - Property of QuickLogic Corp.                      *
 *    Copyright (C) 2014 QuickLogic Corporation                              *
 *    All rights reserved                                                    *
 *    Use, duplication, or disclosure restricted                             *
 *                                                                           *
 *    File   : AUEnums.hpp                                                 *
 *    Purpose: To store enum class type which are needed by end users.       *
 *                                                                           *
 *===========================================================================*/
#ifndef __AUEnums_HPP_
#define __AUEnums_HPP_
#ifdef WIN32
#include <cstdint>
#endif

enum class DumpFolderType : std::uint8_t {
    FOLDER_SYNTHESIS,
    FOLDER_LIBMAP,
    FOLDER_LOGIC_OPTIMIZER,
    FOLDER_PLACER,
    FOLDER_ROUTER,
    FOLDER_STA,
    FOLDER_ADMODEL,
    FOLDER_POWER_ESTIMATION,
    FOLDER_CONFIG_BIT_GENERATION,
    FOLDER_BACKEND_FILE_GENERATION,
    INVALID
};

// Define all components which are part of the tool flow
enum class AUComponentType : std::uint8_t {
    DEVICE_ESTIMATE,
    SYNTHESIS,
    LOADER,
    SAVE_PROJECT,
    OPTIMIZER, 
    PLACER,  
    ROUTER,
    CFG_BIT_GEN,
    STA,
    POWER_CALCULATOR,
    INVALID_STATE // add any element before this. Do not add after this.
};

enum class AUDeviceFamilyType : std::uint8_t {
    ARCTIC_PRO = 0,
    ARCTIC_PRO_2,
    ARCTIC_PRO_3,
    POLAR_PRO_3,
    QLF_K4N8,
    QLF_K6N10,
    MAX_COUNT
};

enum class AUPartType : std::uint8_t {
    QLAL3S2,
    QLAL4S3B,
    QL3P1K,
    QL734A,
    QL736A,
    QL737A,
    QL737B,
    QL745A,
    MAX_COUNT
};
        
enum class AUFoundryType : std::uint8_t {
    SMIC = 0,
    GF,
    TSMC,
    SAMSUNG,
    UMC,
    SKY,
    MAX_COUNT
};

enum class AUTechNode : std::uint8_t {
    NM22 = 0,
    NM28,
    NM40,
    NM40_EFLASH,
    NM65,
    NM90,
    NM130,
    MAX_COUNT
};
    
enum class AUSupportedFamilyFoundryNodeList : std::uint8_t {
    ARCTIC_PRO_SMIC_40 = 0,
    ARCTIC_PRO_GF_40,
    ARCTIC_PRO_TSMC_40,
    ARCTIC_PRO_2_GF_22,
    ARCTIC_PRO_2_TSMC_28,
    ARCTIC_PRO_3_GF_22,
    ARCTIC_PRO_3_SAMSUNG_28,
    ARCTIC_PRO_TSMC_40_EFLASH,
    QLF_K4N8_UMC_22,
    QLF_K6N10_UMC_22,
    POLAR_PRO_3_TSMC_65NM,
    QLF_K6N10_TSMC_22,
    QLF_K6N10_SKY_65,
    QLF_K6N10_SKY_90,
    QLF_K6N10_SKY_130,
    MAX_COUNT
};
    

enum class AUSupportedFeatures : std::uint8_t {
    ARCTIC_PRO_SMIC_40 = 0,
    ARCTIC_PRO_GF_40,
    ARCTIC_PRO_TSMC_40,
    ARCTIC_PRO_2_GF_22, // AP2 GF 22nm default flow
    ARCTIC_PRO_2_TSMC_28,
    QL736A,
    QL737A,
    QL745A,
    CONFIG_BIT_GEN,
    BACK_ANNOTATION,
    ARCTIC_PRO_TSMC_40_EFLASH,
    QLF_K4N8_UMC_22,
    QLF_K6N10_UMC_22,
    QLAL4S3B,
    QLAL3S2,
    QL3P1K,
    ARCTIC_PRO_3_GF_22,
    ARCTIC_PRO_3_SAMSUNG_28,
    QLF_K6N10_TSMC_22,
    QLF_K6N10_SKY_65,
    QLF_K6N10_SKY_90,
    QLF_K6N10_SKY_130,
    MAX_COUNT
};

#endif // __AUEnums_HPP_
