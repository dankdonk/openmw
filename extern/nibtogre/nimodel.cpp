/*
  Copyright (C) 2015-2018 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

*/
#include "nimodel.hpp"

#include <stdexcept>
#include <iostream> // FIXME: debugging only

#include "niobject.hpp"

// "name" is the full path to the mesh from the resource directory/BSA added to Ogre::ResourceGroupManager.
// The file is opened by mNiStream::mStream.
//
// FIXME: there could be duplicates b/w TES3 and TES4/5
NiBtOgre::NiModel::NiModel(const std::string& name) : mNiStream(name), mHeader(mNiStream)
                                                      , filename(name) // FIXME: testing only
{
    mObjects.resize(mHeader.numBlocks());
    if (mNiStream.nifVer() >= 0x0a000100) // from 10.0.1.0
    {

        for (std::uint32_t i = 0; i < mHeader.numBlocks(); ++i)
        {
            //std::cout << "Block " << mHeader.blockType(i) << std::endl; // FIXME: for testing only
            mCurrIndex = i; // FIXME: debugging only

            // From ver 10.0.1.0 (i.e. TES4) we already know the object types from the header.
            mObjects[i] = NiObject::create(mHeader.blockType(i), i, mNiStream, *this);
        }
    }
    else
    {
        for (std::uint32_t i = 0; i < mHeader.numBlocks(); ++i)
        {
            mCurrIndex = i; // FIXME: debugging only
//#if 0
            std::string blockName = mNiStream.readString();
            //if (blockName == "RootCollisionNode")
                //std::cout << name << " : " << "RootCollisionNode" << std::endl;
            //if (blockName == "AvoidNode")
                //std::cout << name << " : " << "AvoidNode" << std::endl;
            //if (blockName == "NiStringExtraData")
                //std::cout << name << " : " << "NiStringExtraData" << std::endl;
            //std::cout << name << " : " << "BoundingBox" << std::endl;
            mObjects[i] = NiObject::create(blockName, i, mNiStream, *this);
            //std::cout << "Block " << blockName << std::endl; // FIXME: for testing only
//#endif
            // For TES3, the object type string is read first to determine the type.
            //mObjects[i] = NiObject::create(mNiStream.readString(), mNiStream, *this);
        }
    }

    // TODO: should assert that the first object, i.e. mObjects[0], is either a NiNode (TES3/TES4)
    //       or BSFadeNode (TES5)

    // read the footer to check for root nodes
    std::uint32_t numRoots = 0;
    mNiStream.read(numRoots);

    mRoots.resize(numRoots);
    for (std::uint32_t i = 0; i < numRoots; ++i)
        mNiStream.read(mRoots.at(i));

    if (numRoots == 0)
        throw std::runtime_error(name + " has no roots");
    else if (numRoots > 1) // FIXME: debugging only, to find out which NIF has multiple roots
        std::cout << name << " has numRoots: " << numRoots << std::endl;

    // FIXME: testing only
    //if (mNiStream.nifVer() >= 0x0a000100)
        //std::cout << "roots " << mHeader.blockType(mRoots[0]) << std::endl;
}

NiBtOgre::NiModel::~NiModel()
{
}

void NiBtOgre::NiModel::build(BtOgreInst *inst)
{
    // build the first root
    mObjects[mRoots[0]]->build(inst);

    // FIXME: what to do with other roots?
}
