/*============================================================================
 *                                                                           *
 *-  Copyright Notice  ------------------------------------------------------*
 *                                                                           *
 *    Licensed Materials - Property of QuickLogic Corp.                      *
 *    Copyright (C) 2020 QuickLogic Corporation                              *
 *    All rights reserved                                                    *
 *    Use, duplication, or disclosure restricted                             *
 *                                                                           *
 *    File   : auVersion.hpp                                                 *
 *    Purpose: To store the Version Information of Aurora                    *
 *                                                                           *
 *===========================================================================*/

#ifndef __auVERSION_HPP_
#define __auVERSION_HPP_

#include <string>
#include <sstream>
#include "aurora_vars.h"
using std::string;
using std::ostringstream;

namespace auVersion {
    // member variables
#ifdef APPLICATION_PREFIX
    const string RELEASE_PREFIX = APPLICATION_PREFIX;
#else
    const string RELEASE_PREFIX = "";
#endif

#ifdef APPLICATION_VERSION_MAJOR
    const string RELEASE_YEAR = APPLICATION_VERSION_MAJOR; //This is no more Year, it is major version
#else
    const string RELEASE_YEAR = ""; //This is no more Year, it is major version
#endif

#ifdef APPLICATION_VERSION_MINOR
    const string MAJOR_RELEASE = APPLICATION_VERSION_MINOR; // This is not for Major, it is now minor version
#else
    const string MAJOR_RELEASE = ""; // This is not for Major, it is now minor version
#endif

#ifdef APPLICATION_VERSION_PATCH
    const string MINOR_RELEASE = APPLICATION_VERSION_PATCH; // This is not for Minor Release, it is now for Patch no.
#else
    const string MINOR_RELEASE = ""; // This is not for Minor Release, it is now for Patch no.
#endif

#ifdef SW_RELEASE_SUFFIX
    const string RELEASE_SUFFIX = SW_RELEASE_SUFFIX;// For production release, should be empty; for daily builds, should
                                                    // be "PreRelease".
#else
    const string   RELEASE_SUFFIX = ""; // For production release, should be empty; for daily builds, should
                                                  // be "PreRelease".
#endif

#ifdef EDA_SW_PATH
    const string EDA_SW_ROOT = EDA_SW_PATH;
#else
    const string EDA_SW_ROOT = "";
#endif

#ifdef EDA_DEV_PATH
    const string EDA_DEV_ROOT = EDA_DEV_PATH;
#else
    const string EDA_DEV_ROOT = "";
#endif

#ifdef EDA_INSTALL_PATH
    const string EDA_INSTALL_ROOT = EDA_INSTALL_PATH;
#else
    const string EDA_INSTALL_ROOT = "";
#endif

#ifdef APPLICATION_NAME
    const string EDA_APP_NAME = APPLICATION_NAME;
#else
    const string EDA_APP_NAME = "Aurora";
#endif

#ifdef APPLICATION_PROGRAM_NAME
    const string EDA_PROGRAM_NAME = APPLICATION_PROGRAM_NAME;
#else
    const string EDA_PROGRAM_NAME = "aurora";
#endif

#ifdef APPLICATION_ORG_NAME
    const string EDA_ORG_NAME = APPLICATION_ORG_NAME;
#else
    const string EDA_ORG_NAME = "QuickLogic"
#endif

    const string RELEASE_TYPE = "DailyBuild"; //DailyBuild DO NOT USE THIS

    const string TOOL_ABOUT_MESSAGE =  "WARNING: This program is protected by copyright law and international treaties." \
                                        "Unauthorised reproduction or distribution of this program, or any portion of it, " \
                                        "may result in severe civil and criminal penalties, " \
                                        "and will be prosecuted to the maximum extent possible under the law.";

    // Member Functions
 
    // Returns the Release String that must be used in cmdline, tcl shell or GUI
    string getReleaseString();
 
    string getReleaseDate();

    string getCopyrightMessage();

    // Returns the current year
    string getCurrentYear();
} 


#endif // __auVERSION_HPP
