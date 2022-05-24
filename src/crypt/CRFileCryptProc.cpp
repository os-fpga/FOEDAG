/*============================================================================
 *                                                                           *
 *-  Copyright Notice  ------------------------------------------------------*
 *                                                                           *
 *    Licensed Materials - Property of QuickLogic Corp.                      *
 *    Copyright (C) 2018 QuickLogic Corporation                              *
 *    All rights reserved                                                    *
 *    Use, duplication, or disclosure restricted                             *
 *                                                                           *
 *    File   : CRFileCryptProc.cpp                                           *
 *    Purpose: Singleton class to invoke encyption/decryption.               *
 *                                                                           *
 *===========================================================================*/

/// Include header files
//
//#include <boost/filesystem.hpp>
#include <fstream>
#include <QDir>
#include <QFile>
#include <QMessageLogger>
#include <QFileInfo>
#include <QDirIterator>
#include <cereal/archives/portable_binary.hpp>
#include "CRFileCryptProc.hpp"
#include "auConstIdefs.hpp"
#include "CRFileCrypt.hpp"
//#include "LGMsgs.hpp"
//#include "EXRunTimeException.hpp"


using namespace std;
//using namespace logging;
//using namespace boost::filesystem;

/// static member variable initialization
CRFileCryptProc* CRFileCryptProc::ourInstance = 0; // initialize pointer

CRFileCryptProc::CRFileCryptProc(/* <arguments> */) : myCryptKeyGenDB()
{
}

CRFileCryptProc::~CRFileCryptProc()
{
}

// Saves cryption key in DB
bool CRFileCryptProc::saveCryptKeyDB(const string& dirName,
                                     const string& familyOrPartName,
                                     string& cryptFileName)
{
    QString qDirName = QString::fromStdString(dirName);
    QDir pathDir(qDirName);
    if (pathDir.exists())
    {
        if (cryptFileName == "") {
            cryptFileName = getCryptDBFileName(dirName, familyOrPartName);
        }
        save(myCryptKeyGenDB, cryptFileName);
    }
    else
    {
        QDir().mkdir(qDirName);
    }
    
    save(myCryptKeyGenDB, cryptFileName);

    #if 0
    // first check if given dir exists
    path p(absolute(path(dirName)));
    if(!exists(p))
    {
        try
        {
            if (!boost::filesystem::create_directory(p)) {
                db_1123(dirName,
                        lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
                return false;
            }
        }
        catch (const boost::filesystem::filesystem_error& e)
        {
            logging::lgExceptionMsg(e.what());
            return false;
        }
    }

    if (cryptFileName == "") {
        cryptFileName = getCryptDBFileName(dirName, familyOrPartName);
    }
    
    logging::db_1049("Supplementary device", cryptFileName);
    
    save(myCryptKeyGenDB, cryptFileName);
    #endif
    return true;
}

// Loads cryption key in DB
bool CRFileCryptProc::loadCryptKeyDB(const string& cryptFileName)
{
    if (cryptFileName.empty()) {
        //logging::db_1188("Supplementary device", "loading",
        //                 logging::lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
        return false;
    }
    QString qFileName = QString::fromStdString(cryptFileName);
        
    if(QFileInfo(qFileName).exists()){
        //The file exists
    }
    else{
        //The file doesn't exist
        return false;
    }
    #if 0
    if(!boost::filesystem::exists(cryptFileName.c_str())) {
        db_1106("Supplementary device", cryptFileName,
                lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
        return false;
    }
    #endif
    return load(myCryptKeyGenDB, cryptFileName);
    
}

// Forms the cyrption DB file name using the base directory for the given
// family name and returns the file name
string CRFileCryptProc::getCryptDBFileName(const string& baseDirName,
                                           const string& familyOrPartName) const
{
    QString qBaseDirName = QString::fromStdString(baseDirName);
    QDir pathDir(qBaseDirName);
    if (pathDir.exists())
    {

    }
    else
    {
        return "";
    }

    // first check if given dir exists
    #if 0    
    if(!boost::filesystem::exists(baseDirName.c_str())) {
        db_1110(baseDirName, familyOrPartName,
                lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
        return "";
    }
    #endif
    ostringstream cryptFileNameStrm;
    cryptFileNameStrm << baseDirName << auConstIdef::PATH_SEP << familyOrPartName
    << auConstIdef::UNDER_SEP << auConstIdef::CRYPT_KEY
    << auConstIdef::DB_EXTN;
    return cryptFileNameStrm.str();
}

// Generates the key if not yet generated or returns the generated
// key
bool CRFileCryptProc::getKey(SecByteBlock& keyObj, SecByteBlock& ivObj)
{
    return myCryptKeyGenDB.getKey(keyObj, ivObj);;
}

// Encrypt all required files and use the encrypted files in installation folder.
bool CRFileCryptProc::encryptFiles(const string& dirName, const set<string>& fileExtnSet,
                                   const string& familyOrPartName)
{

    QString qDirName = QString::fromStdString(dirName);
    QDir pathDir(qDirName);
    if (pathDir.exists())
    {
        //std::cout << "Directory found "<< std::endl;
        SecByteBlock keyObj(AES::DEFAULT_KEYLENGTH);
        SecByteBlock ivObj(AES::BLOCKSIZE);
        getKey(keyObj, ivObj);
        
        CRFileCrypt cryptObj(keyObj, ivObj);
        foreach(QFileInfo info, pathDir.entryInfoList(QStringList( "*.xml" ), QDir::NoDotAndDotDot | QDir::Files ))
        {
            if (info.isFile())
            {                
                //std::cout << "Files "<< info.fileName().toStdString() << " " << info.filePath().toStdString()<< std::endl;
                ostringstream encryptedFileStrm;
                encryptedFileStrm << info.filePath().toStdString() << auConstIdef::ENCRYPT_EXTN; 
                //std::cout << "encryptedFileStrm "<< encryptedFileStrm.str() << std::endl;               
                if(!cryptObj.encryptFile(info.filePath().toStdString(), encryptedFileStrm.str())) {
                    //db_1189(currFile.string(), "encryption",
                    //        lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
                    std::cout << "encryptFile failed "<< std::endl;
                    //return false;
                }
            }
        }        
    }
    else
    {
        std::cout << "Directory does not exist "<< std::endl;
        return false;
    }
    return true;
}

// Decrypt the given file and use the decrypted files.
bool CRFileCryptProc::decryptFile(const string& fileToDecrypt,
                                  stringstream& decryptedStrm)
{
    #if 0
    namespace fs = boost::filesystem;
    
    fs::path dirP(fileToDecrypt);
    
    if(!exists(dirP)) {
        ws_1029("Device", "device", fileToDecrypt,
                lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
        return false;
    }
    #endif
    QString qFileToDecrypt = QString::fromStdString(fileToDecrypt);
    QFileInfo dirP(qFileToDecrypt);
    if (dirP.exists() && dirP.isFile()) 
    {
        SecByteBlock keyObj(AES::DEFAULT_KEYLENGTH);
        SecByteBlock ivObj(AES::BLOCKSIZE);
        //array<byte, AES::DEFAULT_KEYLENGTH> keyArr;
        //array<byte, AES::BLOCKSIZE> ivArr;
        getKey(keyObj, ivObj);
    
        CRFileCrypt cryptObj(keyObj, ivObj);
        ostringstream decrStrm;
        if(!cryptObj.decryptFile(dirP.filePath().toStdString(), decrStrm)) 
        {
            //db_1189(dirP.string(), "decryption",
            //        lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
            return false;
        }

        decryptedStrm << decrStrm.str();    
        return true;
    } 
    else 
    {
        std::cout << "Directory does not exist "<< std::endl;
        return false;
    }    
}

template< typename T >
void CRFileCryptProc::save(T& data, const string& cryptFileName)
{
    std::ofstream ofs(cryptFileName, std::ios::binary);
    {
        cereal::PortableBinaryOutputArchive ar(ofs);
        ar(data);
    }
    #if 0
    std::ofstream ofs(cryptFileName, std::ios::binary);
    {
        cereal::PortableBinaryOutputArchive ar(ofs);
        try {
            ar(data);
        } catch (EXRunTimeException& e) { // our own runtime_exception is caught
            db_1107("saving", cryptFileName, e.what(),
                    lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
        }
    }
    #endif
}

template<typename T>
bool CRFileCryptProc::load(T& data, const string& cryptFileName)
{
    std::ifstream ifs(cryptFileName, std::ios::binary);    
    {
        cereal::PortableBinaryInputArchive ar(ifs);
        try {
            ar(data);
        } catch (std::exception& e) {
            //db_1107("loading", cryptFileName, e.what(),
            //        lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
            return false;
        }
    }
    return true;

    #if 0
    std::ifstream ifs(cryptFileName, std::ios::binary);
    
    {
        cereal::PortableBinaryInputArchive ar(ifs);
        try {
            ar(data);
        } catch (EXRunTimeException& e) { // our own runtime_exception is caught
            db_1107("loading", cryptFileName, e.what(),
                    lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
            return false;
        } catch (std::exception& e) {
            db_1107("loading", cryptFileName, e.what(),
                    lgGetOccurInfo(__FILE__, __FUNCTION__, __LINE__));
            return false;
        }
    }
    #endif   
}

