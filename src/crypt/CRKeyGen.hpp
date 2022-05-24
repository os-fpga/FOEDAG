/*============================================================================
 *                                                                           *
 *-  Copyright Notice  ------------------------------------------------------*
 *                                                                           *
 *    Licensed Materials - Property of QuickLogic Corp.                      *
 *    Copyright (C) 2016 QuickLogic Corporation                              *
 *    All rights reserved                                                    *
 *    Use, duplication, or disclosure restricted                             *
 *                                                                           *
 *    File   : CRKeyGen.hpp                                                  *
 *    Purpose: Key generator and serializer.                                 *
 *                                                                           *
 *===========================================================================*/
#ifndef __CRKeyGen_HPP_
#define __CRKeyGen_HPP_

/// Include header files
#include <cereal/types/string.hpp>
#include "aes.h"
#include "secblock.h"

/// Forward class declaration
//class <name>;
//

/// declare namespace
using std::string;
using CryptoPP::AES;
using CryptoPP::SecByteBlock;

class CRKeyGen
{
    friend class cereal::access;
    friend class CRFileCryptProc;
    
public:
	CRKeyGen(void);
	~CRKeyGen();

public:
    /// public member functions
    
    // Generates the key if not yet generated or returns the generated
    // key
	bool getKey(SecByteBlock& keyObj, SecByteBlock& ivObj);
    
    // Returns true if key is already generated
    bool getIsKeyGenerated() const;
    
private:
    /// private member functions 

    template <class Archive>
    void serialize(Archive& ar, const unsigned version) {
        // NOTE: If you add/delete any new member variable that should be
        // serialized, then increment the version count and update the below
        // condition for archiving.
        if (1 == version) {
            ar(myIsEmpty, myKey, myIV);
        }
    }
    
private:
    /// private member variables
    bool    myIsEmpty; // key not yet generated
	
	string	myKey;
	string  myIV;

private:
	CRKeyGen(const CRKeyGen&);
	CRKeyGen& operator= (const CRKeyGen&);
};

// Increment version number if you are adding any variable for serialization.
CEREAL_CLASS_VERSION(CRKeyGen, 1);

/// inline functions
//
inline bool CRKeyGen::getIsKeyGenerated() const
{
    return !myIsEmpty;
}

#endif // __CRKeyGen_HPP_

