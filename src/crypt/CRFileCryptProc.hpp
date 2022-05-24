/*============================================================================
 *                                                                           *
 *-  Copyright Notice  ------------------------------------------------------*
 *                                                                           *
 *    Licensed Materials - Property of QuickLogic Corp.                      *
 *    Copyright (C) 2018 QuickLogic Corporation                              *
 *    All rights reserved                                                    *
 *    Use, duplication, or disclosure restricted                             *
 *                                                                           *
 *    File   : CRFileCryptProc.hpp                                           *
 *    Purpose: Singleton class to invoke encyption/decryption.               *
 *                                                                           *
 *===========================================================================*/
#ifndef __CRFileCryptProc_HPP_
#define __CRFileCryptProc_HPP_

/// Include header files
#include <string>
#include <set>
#include <sstream>

#include "CRKeyGen.hpp"
#include "aes.h"
#include "secblock.h"

/// Forward class declaration
class CRKeyGen;
//enum class AUDeviceFamilyType : std::uint8_t;
//enum class AUPartType : std::uint8_t;
//

/// declare namespace
using std::string;
using std::set;
using std::stringstream;

using CryptoPP::AES;
using CryptoPP::SecByteBlock;
//

class CRFileCryptProc
{
public:
	~CRFileCryptProc();

	static CRFileCryptProc* getInstance(/* <arguments> */);

	void destroyInstance();

public:
	/// public member functions

    /************ Encryption related APIs starts here ************/
    
    // Saves cryption key in DB   
    bool saveCryptKeyDB(const string& dirName, const string& familyOrPartName,
                        string& cryptFileName);
    
    // Loads cryption key in DB
    bool loadCryptKeyDB(const string& cryptFileName);
    
    // Forms the cyrption DB file name using the base directory for the given
    // family name and returns the file name
    string getCryptDBFileName(const string& baseDirName, const string& familyOrPartName) const;
    
    // Generates the key if not yet generated or returns the generated
    // key
    bool getKey(SecByteBlock& keyObj, SecByteBlock& ivObj);
    
    // Encrypt all required files and use the encrypted files in installation folder.  
    bool encryptFiles(const string& dirName, const set<string>& fileExtnSet, const string& familyOrPartName);
    
    // Decrypt the given file and use the decrypted files.
    bool decryptFile(const string& fileToDecrypt, stringstream& decryptedStrm);
    
    /************ Encryption related APIs ends here ************/
    
private:
    /// private member functions
    
    template< typename T >
    void save(T& data, const string& cryptFileName);
    
    template< typename T >
    bool load(T& data, const string& cryptFileName);
    
private:
	/// private data member variables
    CRKeyGen  myCryptKeyGenDB;
    
	/// static member variables
	static CRFileCryptProc* ourInstance;

private:
	CRFileCryptProc(/* <arguments> */);
	CRFileCryptProc(const CRFileCryptProc&);
	CRFileCryptProc& operator= (const CRFileCryptProc&);
};

/// inline functions
//
//
inline CRFileCryptProc* CRFileCryptProc::getInstance(/* <arguments> */)
{
	if(!ourInstance)
		ourInstance = new CRFileCryptProc(/* <arguments> */);

	return ourInstance;
}

inline void CRFileCryptProc::destroyInstance()
{
	delete ourInstance;
	ourInstance = 0;
}

#endif // __CRFileCryptProc_HPP_

