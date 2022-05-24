/*============================================================================
 *                                                                           *
 *-  Copyright Notice  ------------------------------------------------------*
 *                                                                           *
 *    Licensed Materials - Property of QuickLogic Corp.                      *
 *    Copyright (C) 2016 QuickLogic Corporation                              *
 *    All rights reserved                                                    *
 *    Use, duplication, or disclosure restricted                             *
 *                                                                           *
 *    File   : CRFileCrypt.cpp                                               *
 *    Purpose: Main Class to enable file encryption/decryption.              *
 *                                                                           *
 *===========================================================================*/

/// Include header files
//
#include <sstream>
#include <fstream>
#include "CRFileCrypt.hpp"
//#include "EXGenException.hpp"
//#include "LGMsgs.hpp"
#include "osrng.h"
#include "aes.h"
#include "gcm.h"
#include "filters.h"
#include "files.h"

/// declare namespace
using CryptoPP::GCM;
using CryptoPP::AutoSeededRandomPool;
using CryptoPP::FileSink;
using CryptoPP::FileSource;
using CryptoPP::StringSink;
using CryptoPP::StringSource;
using CryptoPP::AuthenticatedEncryptionFilter;
using CryptoPP::AuthenticatedDecryptionFilter;
using CryptoPP::AES;

using namespace std;
//using namespace logging;


CRFileCrypt::CRFileCrypt(const SecByteBlock& key, const SecByteBlock& iv): myKey(key),
    myIV(iv)
{

}

CRFileCrypt::~CRFileCrypt()
{
}

bool CRFileCrypt::encryptFile(const string& fileToEncrypt, const string& encryptedFile)
{
    try {
        ifstream infile(fileToEncrypt);
        //std::cout << "fileToEncrypt "<< fileToEncrypt << std::endl;
        //std::cout<<infile.good()<<std::endl;
        //std::cout<<infile.eof()<<std::endl;
        //std::cout<<infile.fail()<<std::endl;
        //std::cout<<infile.bad()<<std::endl;
        
        if (!infile.good()) {
            //cr_1000(fileToEncrypt);
            std::cout << "infile not good "<< std::endl;
            return false;
        }
		infile.close();

        GCM< AES >::Encryption e;
		e.SetKeyWithIV(myKey, myKey.size(), myIV, myIV.size());

		string cipherBuf;
		FileSource fs1(fileToEncrypt.c_str(), true,
			new AuthenticatedEncryptionFilter(e,
				new StringSink(cipherBuf)));

		ofstream outfile(encryptedFile, ios::binary);
        outfile << cipherBuf;
        outfile.close();

    } catch (std::exception& ex) {
        //cr_1001("encryption", fileToEncrypt, ex.what(),
                //lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
        std::cout << "Exception"<< std::endl;
    }
    return true;
}

bool CRFileCrypt::decryptFile(const string& fileToDecrypt, ostringstream& decryptedStrm)
{
    try {
        ifstream readFile(fileToDecrypt, ios::binary);
        std::cout << "fileToDecrypt "<< fileToDecrypt << std::endl;
        std::cout<<readFile.good()<<std::endl;
        std::cout<<readFile.eof()<<std::endl;
        std::cout<<readFile.fail()<<std::endl;
        std::cout<<readFile.bad()<<std::endl;
        std::cout << fileToDecrypt.c_str()<<std::endl;
        if(!readFile.good()) {
            //cr_1000(fileToDecrypt);
            std::cout << "read File not good "<< std::endl;
            return false;
        }

        GCM< AES >::Decryption decryptor;
		decryptor.SetKeyWithIV(myKey, myKey.size(), myIV, myIV.size());

		string recovered;
		FileSource fs2(fileToDecrypt.c_str(), true,
			new AuthenticatedDecryptionFilter(decryptor,
				new StringSink(recovered)));

        decryptedStrm << recovered;
    } catch (std::exception& ex) {
        //cr_1001("decryption", fileToDecrypt, ex.what(),
                //lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
        std::cout << "decryptFile Exception "<< ex.what()<<std::endl;        
    }
    return true;
}
