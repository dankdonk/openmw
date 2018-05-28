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

  Much of the information on the NIF file structures are based on the NifSkope
  documenation.  See http://niftools.sourceforge.net/wiki/NifSkope for details.

*/
#ifndef NIBTOGRE_NIOBJECTNET_H
#define NIBTOGRE_NIOBJECTNET_H

#include <string>
#include <vector>
#include <cstdint>

#include "niobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiObjectNET
//     NiSequenceStreamHelper <-- /* typedef NiObjectNET */
//     NiTexture <--------------- /* typedef NiObjectNET */
//          NiSourceTexture
namespace NiBtOgre
{
    class NiStream;
    class Header;

    class NiObjectNET : public NiObject
    {
        NiObjectNET();
        //NiObjectNET(const NiObjectNET& other);// copy constructor
        //NiObjectNET(NiObjectNET&& other);     // move constructor
        //NiObjectNET& operator=(const NiObjectNET& other); // copy assignment
        //NiObjectNET& operator=(NiObjectNET&& other);      // move assignment

    protected:
        bool                        mIsBSLightingShaderProperty;
        std::uint32_t               mSkyrimShaderType;
        StringIndex                 mName;
        NiExtraDataRef              mExtraDataIndex;     // first in a chain, up to 4.2.2.0
        std::vector<NiExtraDataRef> mExtraDataIndexList; // from 10.0.1.0
        NiTimeControllerRef         mControllerIndex;    // first in a chain

    public:
        NiObjectNET(uint32_t index, NiStream& stream, const NiModel& model, bool isBSLightingShaderProperty = false);
        virtual ~NiObjectNET() {}

        inline StringIndex getName() const { return mName; }

        virtual void build(Ogre::SceneNode *sceneNode, BtOgreInst *inst, NiObject *parent = nullptr);
    };

    typedef NiObjectNET NiSequenceStreamHelper;
    typedef NiObjectNET NiTexture;

    struct NiSourceTexture : public NiTexture
    {
        typedef std::uint32_t PixelLayout;
        typedef std::uint32_t MipMapFormat;
        typedef std::uint32_t AlphaFormat;

        char mUseExternal;

        StringIndex   mFileName;
        std::uint32_t mATextureRenderDataRef;

        PixelLayout  mPixelLayout;
        MipMapFormat mUseMipmaps;
        AlphaFormat  mAlphaFormat;
        char mIsStatic;
        bool mDirectRender;
        bool mPersistRenderData;

        NiSourceTexture(uint32_t index, NiStream& stream, const NiModel& model);
    };
}

#endif // NIBTOGRE_NIOBJECTNET_H
