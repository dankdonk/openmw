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
#include "niobjectnet.hpp"

#include "nistream.hpp"

NiBtOgre::NiObjectNET::NiObjectNET(NiStream& stream, const NiModel& model, bool isBSLightingShaderProperty)
    : NiObject(stream, model), mIsBSLightingShaderProperty(isBSLightingShaderProperty)
{
    if (mIsBSLightingShaderProperty)
    {
        if (stream.userVer() >= 12)
            stream.read(mSkyrimShaderType);
        else
            mSkyrimShaderType = 0;
    }

    stream.readLongString(mNameIndex);

    if (stream.nifVer() <= 0x04020200) // up to 4.2.2.0
        stream.read(mExtraDataIndex);

    if (stream.nifVer() >= 0x0a000100) // from 10.0.1.0
        stream.readVector<NiExtraDataRef>(mExtraDataIndexList);

    stream.read(mControllerIndex);
}

void NiBtOgre::NiObjectNET::build(const RecordBlocks& objects, const Header& header,
                             Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene)
{
}


NiBtOgre::NiSourceTexture::NiSourceTexture(NiStream& stream, const NiModel& model)
    : NiObjectNET(stream, model), mDirectRender(false), mPersistRenderData(false)
{
    stream.read(mUseExternal);

    if (mUseExternal != 0)
    {
        stream.readLongString(mFileNameIndex); // external filename

        if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
            stream.skip(sizeof(NiObjectRef));
    }
    else
    {
        if (stream.nifVer() <= 0x0a000100) // up to 10.0.1.0
            stream.skip(sizeof(char));

        if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
            stream.readLongString(mFileNameIndex); // original filename

        stream.read(mATextureRenderDataRef);
    }

    stream.read(mPixelLayout);
    stream.read(mUseMipmaps);
    stream.read(mAlphaFormat);
    stream.read(mIsStatic);

    if (stream.nifVer() >= 0x0a01006a) // from 10.1.0.106
        mDirectRender = stream.getBool();

    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7
        mPersistRenderData = stream.getBool();
}
