/*
  Copyright (C) 2015-2019 cc9cii

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

#include <OgreController.h>

#include "nistream.hpp"
#include "nitimecontroller.hpp" // static_cast NiControllerManager
#include "nigeommorphercontroller.hpp"
#include "nikeyframecontroller.hpp"
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
// mTextKeysIndex holds text keys such as 'start' 'end' or sound triggers: 'sound: TRPGearsClaws' in
// oblivion/architecture/citadel/interior/citadelsmalltower/oblivionsmalltowergatelatch01.nif
// (probably needs to have actions done through 'inst')
//
// FIXME: how to get the sound object EditorID lookup? probably need to implement something
//        similar to getting world using EditorID, i.e. mStore.get<ForeignWorld>().getFormId(world);
//
// Each of the Controlled Blocks refer to the target node name via the string palette which
// in turn needs to be looked up via the object palette in NiControllerManager.  But how to
// make sense of the palette?  It appears as if the string palette is just a list of all the
// unique strings that need to be looked up:
//
//     strings in the palette     value             index  (deduced from)
//     -------------------------- ----------------- ------ -----------------
//     MainMast02:0               nodeName          118    object palette
//     NiGeomMorpherController    controllerType      7    controller2Index
//     Base                       interpolatorId           NiMorphData::mFrameName (i.e. pose)
//     MainMastMorphA             interpolatorId           NiMorphData::mFrameName (i.e. pose)
//     MainMastMorphB             interpolatorId           NiMorphData::mFrameName (i.e. pose)
//     MainMast02:3               nodeName          124    object palette
//     MainMast02:4               nodeName          129    object palette
//     MainMast02:6               nodeName          134    object palette
//     MainMast02:7               nodeName          139    object palette
//     MainMast02:8               nodeName          144    object palette
//     MainMast02:15              nodeName          149    object palette
//     MainMast02:23              nodeName          156    object palette
//     MainMast02:25              nodeName          161    object palette
//     MainMast02:41              nodeName          167    object palette
//     MainMast02                 nodeName            0    object palette
//     NiTransformController      controllerType      4    controller2Index
//     MainMast02 NonAccum        nodeName          111    object palette
//
void NiBtOgre::NiControllerSequence::build(const NiDefaultAVObjectPalette* objects)
{
    // FIXME: process mTextKeysIndex here

    // Each of the controlled block maps to an Ogre::AnimationTrack.  i.e. each controller
    // block is for one sub-mesh. There can be more than one block for the same sub-mesh.
    // (one for each pose)
    //
    // The interpolator for each block provides the data for Ogre::KeyFrame.
    //
    // NiGeomMorpherController   Ogre::VertexAnimationTrack   Ogre::VertexPoseKeyFrame
    // NiTransformController     Ogre::NodeAnimationTrack(?)  Ogre::TranformKeyFrame
    //
    for (unsigned int i = 0; i < mControlledBlocks.size(); ++i)
    {
        // FIXME: for testing only
        if (mModel.nifVer() >= 0x0a020000 && mModel.nifVer() <= 0x14000005)
            assert(mStringPaletteIndex == mControlledBlocks[i].stringPaletteIndex); // should be the same

        if (mModel.nifVer() < 0x0a01006a) // 10.1.0.106
            throw std::logic_error("NiControllerSequence less than version 10.1.0.106");

        std::int32_t interpolatorIndex = mControlledBlocks[i].interpolatorIndex;
        if (interpolatorIndex < 0) // -1
            continue;

        std::string ctlrTypeName = getObjectName(mControlledBlocks[i].controllerTypeIndex);
        if (ctlrTypeName =="")
            continue;

        // targets are usually NiTriBasedGeom or NiNode
        std::string targetName = getObjectName(mControlledBlocks[i].nodeNameIndex);
        NiAVObject *target = mModel.getRef<NiAVObject>(objects->getObjectRef(targetName));

        if (!target)
            continue;

        //target->setHasAnim();

        if (ctlrTypeName == "NiGeomMorpherController")
        {
            if (mModel.blockType(interpolatorIndex) != "NiFloatInterpolator")
                throw std::runtime_error("unsupported interpolator: "+mModel.blockType(interpolatorIndex));

            // NOTE: Some interpolators do not have any data!  Ignore these controlled blocks.
            NiFloatInterpolator *interpolator = mModel.getRef<NiFloatInterpolator>(interpolatorIndex);
            if (interpolator->mDataIndex < 0) // -1
                continue;

            // targetNode can be nullptr if nodeNameIndex == -1
            if (mControlledBlocks[i].nodeNameIndex == -1)
                continue;

            // Each interpolator type probably has its own animation method get the controller to
            // do the building; pass the interpolator data for the keys
            //
            // NOTE: ingore the chain of controllers here since NiControllerSequence is managing
            // them all?
            //
            // Most of the targets wouldn't have been built yet so just place the data to be picked
            // up later.
            //NiObject::mModel.getRef<NiTimeController>(mControlledBlocks[i]controllerIndex)
                //->buildFromNiControllerSequence(controllers, targetIndex, data);
            //
            // For NiGeomMorpherController the target would be a NiTriBasedGeom which corresponds
            // to an Ogre::SubMesh (which can get to its parent Ogre::Mesh).
            //
            // So we need access to a map<NiGeometyRef, Ogre::SubMesh*> probably from ModelData.
            //

            std::string frameName = getObjectName(mControlledBlocks[i].variable2Index);
            if (frameName =="")
                continue;

            NiGeomMorpherController *controller
                = mModel.getRef<NiGeomMorpherController>(mControlledBlocks[i].controller2Index);

            // setup the controller for the build later when the sub-mesh gets built on demand
            controller->setInterpolator(this, frameName, interpolator);
        }
        else if (ctlrTypeName == "NiTransformController")
        {
            if (mModel.blockType(interpolatorIndex) != "NiTransformInterpolator")
                throw std::runtime_error("unsupported interpolator: "+mModel.blockType(interpolatorIndex));

            // NOTE: Some interpolators do not have any data!  Ignore these controlled blocks.
            NiTransformInterpolator *interpolator = mModel.getRef<NiTransformInterpolator>(interpolatorIndex);
            if (interpolator->mDataIndex < 0) // -1
                continue;

            // targetNode can be nullptr if nodeNameIndex == -1
            if (mControlledBlocks[i].nodeNameIndex == -1)
                continue;

            NiMultiTargetTransformController *controller
                = mModel.getRef<NiMultiTargetTransformController>(mControlledBlocks[i].controller2Index);

            // FIXME: build it again
            controller->build(mNameIndex, target, interpolator, mStartTime, mStopTime);
        }
    }
}

std::string NiBtOgre::NiControllerSequence::getObjectName(std::uint32_t stringOffset) const
{
    if (mModel.nifVer() >= 0x0a020000 && mModel.nifVer() <= 0x14000005)
    {
        if (stringOffset == 0xffffffff) // -1, horrible hack
            return "";

        NiStringPalette *palette = mModel.getRef<NiStringPalette>(mStringPaletteIndex);

        size_t len = palette->mPalette.find_first_of('\0', stringOffset) - stringOffset;
        return palette->mPalette.substr(stringOffset, len);
    }
    else //if (mModel.nifVer() == 0x0a01006a || mModel.nifVer() >= 0x14010003)
    {
        return mModel.indexToString(stringOffset); // for these versions stringOffset is objectIndex
    }
}
