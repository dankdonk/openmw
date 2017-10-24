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
#include "nisequence.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "nistream.hpp"
#include "nitimecontroller.hpp" // static_cast NiControllerManager
#include "nimodel.hpp"

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

    if (stream.nifVer() == 0x0a01006a || stream.nifVer() >= 0x14010003) // from header string
    {
        stream.readLongString(nodeNameIndex);
        stream.readLongString(propertyTypeIndex);
        stream.readLongString(controllerTypeIndex);
        stream.readLongString(variable1Index);
        stream.readLongString(variable2Index);
    }
    else if (stream.nifVer() >= 0x0a020000 && stream.nifVer() <= 0x14000005) // from string palette
    {
        stream.read(stringPaletteIndex);

        stream.read(nodeNameOffset);
        stream.read(propertyTypeOffset);
        stream.read(controllerTypeOffset);
        stream.read(variable1Offset);
        stream.read(variable2Offset);
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiSequence::NiSequence(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    stream.readLongString(mNameIndex);

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
NiBtOgre::NiControllerSequence::NiControllerSequence(NiStream& stream, const NiModel& model)
    : NiSequence(stream, model)
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
        std::int32_t index = -1;
        stream.read(index);
        mManager = model.getRef<NiControllerManager>(index);

        stream.readLongString(mTargetNameIndex);

        if (stream.nifVer() >= 0x0a020000 && stream.nifVer() <= 0x14000005)
            stream.read(mStringPaletteIndex);

        if (stream.nifVer() >= 0x14020007 && stream.userVer() >= 11 && (stream.userVer2() >= 24 && stream.userVer2() <= 28))
            stream.read(mAnimNotesIndex);

        if (stream.nifVer() >= 0x14020007 && stream.userVer2() > 28)
            stream.read(mUnknownShort1);
    //}
}
