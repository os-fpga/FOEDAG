/*============================================================================
 *                                                                           *
 *-  Copyright Notice  ------------------------------------------------------*
 *                                                                           *
 *    Licensed Materials - Property of QuickLogic Corp.                      *
 *    Copyright (C) 2016 QuickLogic Corporation                              *
 *    All rights reserved                                                    *
 *    Use, duplication, or disclosure restricted                             *
 *                                                                           *
 *    File   : CRFileCrypt.hpp                                               *
 *    Purpose: Main Class to enable file encryption/decryption.              *
 *                                                                           *
 *===========================================================================*/
#ifndef __CRFileCrypt_HPP_
#define __CRFileCrypt_HPP_

/// Include header files
#include <string>
#include <array>
#include <sstream>
#include "config.h"
#include "aes.h"
#include "secblock.h"

/// Forward class declaration
//class <name>;
//

/// declare namespace
using std::string;
using std::array;
using std::ostringstream;
using CryptoPP::AES;
using CryptoPP::SecByteBlock;
//

class CRFileCrypt
{
public:
	CRFileCrypt(const SecByteBlock& key, const SecByteBlock& iv);
	~CRFileCrypt();

public:
    /// public member functions 
	bool encryptFile(const string& fileToEncrypt, const string& encryptedFile);
	bool decryptFile(const string& fileToDecrypt, ostringstream& decryptedStrm);

private:
    /// private member functions 
	
private:
    /// private member variables
	const SecByteBlock& myKey;
	const SecByteBlock& myIV;
    
private:
	CRFileCrypt();
	CRFileCrypt(const CRFileCrypt&);
	CRFileCrypt& operator= (const CRFileCrypt&);
};

/// inline functions
//


#endif // __CRFileCrypt_HPP_

