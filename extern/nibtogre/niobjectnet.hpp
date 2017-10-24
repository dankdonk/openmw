/*
  Copyright (C) 2015-2017 cc9cii

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
#ifndef NIBTOGRE_NIOBJECTNET_H
#define NIBTOGRE_NIOBJECTNET_H

#include <string>
#include <vector>
#include <cstdint>

#include "niobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiObjectNET
//     NiSequenceStreamHelper <-- /* NiObjectNET */
//     NiTexture <--------------- /* not implemented */
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
        std::uint32_t               mNameIndex;
        NiExtraDataRef              mExtraDataIndex;     // first in a chain, to 4.2.2.0
        std::vector<NiExtraDataRef> mExtraDataIndexList; // from 10.0.1.0
        NiTimeControllerRef         mControllerIndex;    // first in a chain

    public:
        NiObjectNET(NiStream& stream, const NiModel& model, bool isBSLightingShaderProperty = false);
        virtual ~NiObjectNET() {}

        // Header needed for the NIF version and strings (TES5)
        virtual void build(const RecordBlocks& objects, const Header& header,
                           Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    typedef NiObjectNET NiSequenceStreamHelper;

    struct NiSourceTexture : public NiObjectNET
    {
        char mUseExternal;

        std::uint32_t mFileNameIndex;
        std::uint32_t mATextureRenderDataRef;

        std::uint32_t mPixelLayout;
        std::uint32_t mUseMipmaps;
        std::uint32_t mAlphaFormat;
        char mIsStatic;
        bool mDirectRender;
        bool mPersistRenderData;

        NiSourceTexture(NiStream& stream, const NiModel& model);
    };
}

#endif // NIBTOGRE_NIOBJECTNET_H
