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
#include "nisequence.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "nitimecontroller.hpp" // static_cast NiControllerManager
#include "nimodel.hpp"
#include "nidata.hpp" // NiStringPalette
#include "niavobject.hpp" // getTargetObject (see: http://www.cplusplus.com/forum/beginner/134129/#msg717505)
#include "niinterpolator.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

// NOTE: assumed that this is not used in TES3 (seems to use NiSequenceStreamHelper),
//       10.2.0.0 (TES4) seems to be the earliest example
void NiBtOgre::NiSequence::ControllerLink::read(NiStream& stream)
{
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
    {
        stream.readSizedString(targetName);
        stream.read(controllerIndex);
    }

    if (stream.nifVer() >= 0x0a01006a) // from 10.1.0.106
    {
        stream.read(interpolatorIndex);
        stream.read(controller2Index);
        if (stream.nifVer() == 0x0a01006a) // 10.1.0.106
        {
            stream.skip(sizeof(std::uint32_t)); // Unknown Link 2
            stream.skip(sizeof(std::uint16_t)); // Unknown Short 0
        }
        stream.read(priority); // TODO userVer >= 10
    }

    if (stream.nifVer() == 0x0a01006a || stream.nifVer() >= 0x14010003) // from header
    {
        stringPaletteIndex = -1;

        stream.readLongString(nodeNameIndex);
        stream.readLongString(propertyTypeIndex);
        stream.readLongString(controllerTypeIndex);
        stream.readLongString(variable1Index);
        stream.readLongString(variable2Index);
    }
    else if (stream.nifVer() >= 0x0a020000 && stream.nifVer() <= 0x14000005) // from string palette
    {
        stream.read(stringPaletteIndex); // block index

        stream.read(nodeNameIndex);
        stream.read(propertyTypeIndex);
        stream.read(controllerTypeIndex);
        stream.read(variable1Index);
        stream.read(variable2Index);
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiSequence::NiSequence(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiObject(index, stream, model, data)
{
    stream.readLongString(mNameIndex);

    // probably unused, skip instead?
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
    {
        stream.readLongString(mTextKeysNameIndex);
        stream.read(mTextKeysIndex);
    }

    std::uint32_t numControlledBlocks;
    stream.read(numControlledBlocks);
    if (stream.nifVer() >= 0x0a01006a) // from 10.1.0.106
        stream.skip(sizeof(std::uint32_t));

    mControlledBlocks.resize(numControlledBlocks);
    for (unsigned int i = 0; i < numControlledBlocks; ++i)
        mControlledBlocks[i].read(stream);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiControllerSequence::NiControllerSequence(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiSequence(index, stream, model, data)
{
    //if (stream.nifVer() >= 0x0a01006a) // from 10.1.0.106
    //{
        stream.read(mWeight);
        stream.read(mTextKeysIndex);
        stream.read(mCycleType);

        if (stream.nifVer() == 0x0a01006a) // 10.1.0.106
            stream.read(mUnknown0);

        stream.read(mFrequency);
        stream.read(mStartTime);

        if (stream.nifVer() >= 0x0a020000 && stream.nifVer() <= 0x0a040001)
            stream.read(mUnknownFloat2);

        stream.read(mStopTime);

        if (stream.nifVer() == 0x0a01006a) // 10.1.0.106
            stream.read(mUnknownByte);

        //stream.getPtr<NiControllerManager>(mManager, model.objects());
        std::int32_t rIndex = -1;
        stream.read(rIndex);
        mManager = model.getRef<NiControllerManager>(rIndex);

        stream.readLongString(mTargetNameIndex);

        if (stream.nifVer() >= 0x0a020000 && stream.nifVer() <= 0x14000005)
            stream.read(mStringPaletteIndex);

        if (stream.nifVer() >= 0x14020007 && stream.userVer() >= 11 && (stream.userVer2() >= 24 && stream.userVer2() <= 28))
            stream.read(mAnimNotesIndex);

        if (stream.nifVer() >= 0x14020007 && stream.userVer2() > 28)
            stream.read(mUnknownShort1);
    //}
}

// Each of the NiControlSequences are "playable" animations.
//
// mTextKeysIndex holds text keys such as sound triggers, e.g. 'sound: TRPGearsClaws' in
// oblivion/architecture/citadel/interior/citadelsmalltower/oblivionsmalltowergatelatch01.nif
// (probably needs to have actions done through 'inst')
//
// FIXME: how to get the sound object EditorID lookup? probably need to implement something
//        similar to getting world using EditorID, i.e. mStore.get<ForeignWorld>().getFormId(world);
//
// Each of the Controlled Blocks refer to the target node name via the string palette which
// in turn needs to be looked up via the object palette in NiControllerManager
void NiBtOgre::NiControllerSequence::build(BtOgreInst *inst, const NiDefaultAVObjectPalette* objects)
{
    for (unsigned int i = 0; i < mControlledBlocks.size(); ++i)
    {
        //if (mModel.nifVer() >= 0x0a020000 && mModel.nifVer() <= 0x14000005)
            //assert(mStringPaletteIndex == mControlledBlocks[i].stringPaletteIndex); // should be the same

        // NOTE: can be nullptr if nodeNameIndex == 0xffffffff
        NiAVObject* targetNode = getTargetObject(mControlledBlocks[i].nodeNameIndex, objects);

        // FIXME: from 10.1.0.106 only
        NiInterpolator *interpolator = mModel.getRef<NiInterpolator>(mControlledBlocks[i].interpolatorIndex);

        // TODO: apply interpolator to the target here?
        // targets are usually NiGeometry or NiNode
        // each interpolator type probably has its own animation method

    }

    // FIXME: process mTextKeysIndex here
}

NiBtOgre::NiAVObject* NiBtOgre::NiControllerSequence::getTargetObject(std::uint32_t objIndex,
        const NiDefaultAVObjectPalette* objects) const
{
    std::string objName;
    if (mModel.nifVer() >= 0x0a020000 && mModel.nifVer() <= 0x14000005)
    {
        if (objIndex == 0xffffffff) // -1, horrible hack
            return nullptr;

        NiStringPalette *palette = mModel.getRef<NiStringPalette>(mStringPaletteIndex);

        size_t len = palette->mPalette.find_first_of('\0', objIndex) - objIndex;
        objName = palette->mPalette.substr(objIndex, len);
    }
    else //if (mModel.nifVer() == 0x0a01006a || mModel.nifVer() >= 0x14010003)
    {
        objName = mModel.indexToString(objIndex);
    }

    return mModel.getRef<NiAVObject>(objects->getObjectRef(objName));
}
