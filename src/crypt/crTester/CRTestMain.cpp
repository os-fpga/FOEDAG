/*============================================================================
 *                                                                           *
 *-  Copyright Notice  ------------------------------------------------------*
 *                                                                           *
 *    Licensed Materials - Property of QuickLogic Corp.                      *
 *    Copyright (C) 2014 QuickLogic Corporation                              *
 *    All rights reserved                                                    *
 *    Use, duplication, or disclosure restricted                             *
 *                                                                           *
 *    File   : WSTestMain.cpp                                                *
 *    Purpose: Main test program to validate the functionality of various    *
 *             APIs of Load-Save CLass.                                      *
 *                                                                           *
 *===========================================================================*/
#include <stdafx.h>
#include <iostream>
#include <boost/program_options.hpp>
#include "QLDAException.hpp"
#include "ProcMemRunTime.hpp"
#include "LGMsgs.hpp"
#include "LGTop.hpp"
#include "osrng.h"
#include "aes.h"
#include "CRFileCrypt.hpp"

using namespace std;
using namespace excpt;
using namespace logging;
using CryptoPP::AutoSeededRandomPool;
using CryptoPP::SecByteBlock;
using CryptoPP::AES;

int main(int argc, char *argv[]) 
{
    try {
        ProcMemRunTime memObj;

        // enable logging mechanism
        bool doesExist = false;
        logging::lgCreateFileSink("CRTester.log", doesExist);
        logging::lgCreateConsoleSink("Console", doesExist);

        logging::lgSuppressMsgType(logging::LEVEL::LG_DEBUG);
        logging::lgSetMsgLevel("placer_10", logging::LEVEL::LG_SUPPRESS);

        namespace po = boost::program_options;
        po::options_description desc("Options");
        desc.add_options()
            ("help,h", "Print help messages")
            ("inputFile,i", po::value<string>(), "Decrypt/Encrypt this File.\n")
            ("outputFile,o", po::value<string>(), "Decrypt/Encrypt to this File.\n");

        po::variables_map vm;
        try {
            po::store(po::parse_command_line(argc, argv, desc), vm); // can throw
       
            /** --help option        */
            if (vm.count("help")) {
                std::cout << "CR Tester Options" << endl
                        << desc << endl;
                logging::resetLogs();
                return 0;                                
            }
            
            po::notify(vm);
        } catch (po::error& e) {
            std::cerr << "ERROR: " << e.what() << endl << std::endl;
            std::cerr << desc << std::endl;
            logging::resetLogs();
            return -1;
        }
        
        AutoSeededRandomPool rng;

		SecByteBlock keyObj(AES::DEFAULT_KEYLENGTH);
		SecByteBlock ivObj(AES::BLOCKSIZE);

        rng.GenerateBlock(keyObj, keyObj.size());
        rng.GenerateBlock(ivObj, ivObj.size());

        CRFileCrypt crObj(keyObj, ivObj);

        if(vm.count("inputFile") && vm.count("outputFile")) {
            string inputFile = vm["inputFile"].as<string>();
            string outputFile = vm["outputFile"].as<string>();

            string encryptedFile = "encryptedFile.en";
            if(!crObj.encryptFile(inputFile, encryptedFile)) {
                crt_1003(inputFile, "encryption",
                        lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
                logging::resetLogs();
                return -1;
            }
            
            ostringstream strm;
            
            if(!crObj.decryptFile(encryptedFile, strm)) {
                crt_1003(outputFile, "decryption",
                        lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
                logging::resetLogs();
                return -1;
            }
            ofstream fp(outputFile);
            fp << strm.str();
            
        } else {
            logging::crt_1002("encrypt", "inputFile", "outputFile",
                    lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
            logging::resetLogs();
            return -1;
        }

        logging::db_100(memObj.getElapsedTime(), memObj.getCPUPercentUsage(),
                memObj.getPeakPhysicalMem());
        logging::resetLogs();
    } catch (EXRunTimeException& e) { // rt_except caught
            printExceptionMsg(e.getExLocationInfo()); // location info
            printExceptionMsg(e.what());
            return -1;
    } catch (EXLogicException& e) { // logic_except caught
            printExceptionMsg(e.getExLocationInfo()); // location info
            printExceptionMsg(e.what());
            return -1;
    } catch (EXGenException& e) { // gen_except caught
            printExceptionMsg(e.getExLocationInfo()); // location info
            printExceptionMsg(e.what());
            return -1;
    } catch (std::exception& e) { 
            cerr << e.what() << endl;
            return -1;
    } catch (...) {
            cerr << "Unhandled exception occurred.\n";
            return -1;
    }
    return 0;      
}

