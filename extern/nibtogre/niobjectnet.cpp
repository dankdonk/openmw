/*
  Copyright (C) 2015-2020 cc9cii

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
#include "niobjectnet.hpp"

#include "nistream.hpp"
#include "nimodel.hpp"
#include "nidata.hpp"

NiBtOgre::NiObjectNET::NiObjectNET(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data,
                                   bool isBSLightingShaderProperty)
    : NiObject(index, stream, model, data), mIsBSLightingShaderProperty(isBSLightingShaderProperty)
  //, mControllerRef(-1)
{
    if (!stream) // must be a dummy block being inserted
    {
        mNameIndex = -1;
        mExtraDataRef = -1;
        mExtraDataRefList.clear();
        mControllerRef = -1;

        return;
    }

    if (mIsBSLightingShaderProperty)
    {
        if (stream->userVer() >= 12)
            stream->read(mSkyrimShaderType);
        else
            mSkyrimShaderType = 0;
    }

    if (stream->nifVer() == 0x0a000100)     // HACK not sure about this one
        stream->skip(sizeof(std::int32_t)); // e.g. clutter/farm/oar01.nif version 10.0.1.0
    else if (stream->nifVer() == 0x0a01006a)
        stream->skip(sizeof(std::int32_t)); // e.g. creatures/horse/Bridle.NIF version 10.1.0.106

    stream->readLongString(mNameIndex);

    if (stream->nifVer() <= 0x04020200) // up to 4.2.2.0
        stream->read(mExtraDataRef);

    if (stream->nifVer() >= 0x0a000100) // from 10.0.1.0
        stream->readVector<NiExtraDataRef>(mExtraDataRefList);

    stream->read(mControllerRef);
}

void NiBtOgre::NiObjectNET::build(Ogre::SceneNode *sceneNode, NiObject *parent)
{
}

// WARN: this only works from version 10.0.1.0
std::string NiBtOgre::NiObjectNET::getStringExtraData(const std::string& name) const
{
    if (mExtraDataRefList.empty())
        return "";

    for (std::size_t i = 0; i < mExtraDataRefList.size(); ++i)
    {
        if (mExtraDataRefList[i] == -1 || mModel.blockType(mExtraDataRefList[i]) != "NiStringExtraData")
            continue;

        NiStringExtraData *extra = mModel.getRef<NiStringExtraData>(mExtraDataRefList[i]);
        if (mModel.indexToString(extra->mName) != name)
            continue;

        return mModel.indexToString(extra->mStringData);
    }

    return ""; // none found
}

// FIXME: what to do with normal textures? (*_fn.dds)
NiBtOgre::NiSourceTexture::NiSourceTexture(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiTexture(index, stream, model, data), mDirectRender(false), mPersistRenderData(false)
{
    if (!stream) // must be a dummy block being inserted
    {
        mUseExternal = true;

        std::string nif = model.getName();
        std::size_t pos = nif.find_last_of("\\");
        if (pos != std::string::npos)
        {
            std::string tex = "textures\\landscapelod\\generated"+nif.substr(pos, nif.size()-3-pos)+"dds";
            mFileName = const_cast<NiModel&>(model).addString(tex); // const hack
        }
        else
            mFileName = -1;

        // FIXME: find suitable default values
        mPixelLayout = 6;
        mUseMipmaps = 1;
        mAlphaFormat = 3;
        mIsStatic = true;
        mDirectRender = true;

        return;
    }

    stream->read(mUseExternal);

    if (mUseExternal != 0)
    {
        stream->readLongString(mFileName); // external filename

        if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
            stream->skip(sizeof(NiObjectRef));
    }
    else
    {
        if (stream->nifVer() <= 0x0a000100) // up to 10.0.1.0
            stream->skip(sizeof(char));

        if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
            stream->readLongString(mFileName); // original filename

        stream->read(mATextureRenderDataRef);
    }

    stream->read(mPixelLayout);
    stream->read(mUseMipmaps);
    stream->read(mAlphaFormat);
    stream->read(mIsStatic);

    if (stream->nifVer() >= 0x0a01006a) // from 10.1.0.106
        mDirectRender = stream->getBool();

    if (stream->nifVer() >= 0x14020007) // from 20.2.0.7
        mPersistRenderData = stream->getBool();
}
