/*============================================================================
 *                                                                           *
 *-  Copyright Notice  ------------------------------------------------------*
 *                                                                           *
 *    Licensed Materials - Property of QuickLogic Corp.                      *
 *    Copyright (C) 2016 QuickLogic Corporation                              *
 *    All rights reserved                                                    *
 *    Use, duplication, or disclosure restricted                             *
 *                                                                           *
 *    File   : CRKeyGen.cpp                                                  *
 *    Purpose: Key generator and serializer.                                 *
 *                                                                           *
 *===========================================================================*/

/// Include header files
//
#include "CRKeyGen.hpp"
#include "osrng.h"

/// declare namespace
using CryptoPP::AutoSeededRandomPool;
using namespace std;

CRKeyGen::CRKeyGen() : myIsEmpty(true)
{
}

CRKeyGen::~CRKeyGen()
{
}
 
// Generates the key if not yet generated or returns the generated
// key
bool CRKeyGen::getKey(SecByteBlock& keyObj, SecByteBlock& ivObj)
{
    using CryptoPP::byte;
    if(myIsEmpty) {
        // generate key
        AutoSeededRandomPool rng;
        
		SecByteBlock currKey(AES::DEFAULT_KEYLENGTH);
		SecByteBlock currIV(AES::BLOCKSIZE);

        rng.GenerateBlock(currKey, currKey.size());
        rng.GenerateBlock(currIV, currIV.size());

		myKey = std::string(reinterpret_cast<const char*>(currKey.data()), currKey.size());
		myIV = std::string(reinterpret_cast<const char*>(currIV.data()), currIV.size());

        myIsEmpty = false;
    }
    
    // returns the existing key
	keyObj = SecByteBlock(reinterpret_cast<const byte*>(myKey.data()), myKey.size());
	ivObj = SecByteBlock(reinterpret_cast<const byte*>(myIV.data()), myIV.size());;
    
    return true;
}
