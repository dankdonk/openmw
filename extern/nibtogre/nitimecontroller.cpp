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
#include "nitimecontroller.hpp"

#include <cassert>
#include <stdexcept>
#include <iostream> // FIXME: debugging only

#include <OgreSkeleton.h>
#include <OgreKeyFrame.h>
#include <OgreBone.h>

#include "nistream.hpp"
#include "nigeometry.hpp"  // static_cast NiGeometry
#include "nimodel.hpp"
#include "nisequence.hpp"
#include "nidata.hpp" // NiDefaultAVObjectPalette
#include "niavobject.hpp"
#include "niinterpolator.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

// below code is copied from NifSkope gl/glcontroller.cpp (except for the delta change hack)
namespace
{
bool timeIndex(float time, const std::vector<NiBtOgre::Key<float> >& frames, int & last, int & next, float & x )
{
    int count = static_cast<int>(frames.size());

    if (count > 0)
    {
        if ( time <= frames[0].time) {         // <= first key
            last = next = 0;
            x = 0.0f;

            return true;
        }

        //  hmmm... shouldn't this wrap around instead?
        if ( time >= frames[count - 1].time) { // >= last key
            last = next = count - 1;
            x = 0.0f;

            return true;
        }

        if ( last < 0 || last >= count )
            last = 0;

        float tI = frames[last].time;

        if ( time > tI ) {
            next = last + 1;
            float tJ;

            while ( time >= ( tJ = frames[next].time ) ) {
                last  = next++;
                tI = tJ;
            }

            x = ( time - tI ) / ( tJ - tI );

            return true;
        } else if ( time < tI ) {
            next = last - 1;
            float tJ;

            while ( time <= ( tJ = frames[next].time ) ) {
                last  = next--;
                tI = tJ;
            }

            x = ( time - tI ) / ( tJ - tI );

            // Quadratic Bug Fix
            // Invert x
            //    Previously, this branch was causing x to decrement from 1.0.
            //    (This works fine for linear interpolation apparently)
            x = 1.0f - x;

            // Swap I and J
            //    With x inverted, we must swap I and J or the animation will reverse.
            auto tmpI = last;
            last = next;
            next = tmpI;
            // End Bug Fix

            return true;
        }

        next = last;
        x = 0.0f;

        return true;
    }

    return false;
}

//template <typename T>
bool interpolate( /*T*/float & value, const NiBtOgre::KeyGroup</*T*/float>& keyGroup, float time, int & last )
{
    std::vector<NiBtOgre::Key</*T*/float> > frames = keyGroup.keys;
    int next;
    float x;

    if ( timeIndex( time, frames, last, next, x ) ) {
        /*T*/float v1 = frames[last].value;
        /*T*/float v2 = frames[next].value;

        switch ( keyGroup.interpolation ) {

        case 2:
        {
            // Quadratic
            /*
                In general, for keyframe values v1 = 0, v2 = 1 it appears that
                setting v1's corresponding "Backward" value to 1 and v2's
                corresponding "Forward" to 1 results in a linear interpolation.
            */

            // Tangent 1
            float t1 = frames[last].backward;
            // Tangent 2
            float t2 = frames[next].forward;

            float x2 = x * x;
            float x3 = x2 * x;

            // Cubic Hermite spline
            //    x(t) = (2t^3 - 3t^2 + 1)P1  + (-2t^3 + 3t^2)P2 + (t^3 - 2t^2 + t)T1 + (t^3 - t^2)T2

            value = v1 * (2.0f * x3 - 3.0f * x2 + 1.0f) + v2 * (-2.0f * x3 + 3.0f * x2) + t1 * (x3 - 2.0f * x2 + x) + t2 * (x3 - x2);

            value -= frames[0].value; // HACK: let's consider delta changes only (for testing only!)

        }    return true;

        case 5:
            // Constant
            if ( x < 0.5 )
                value = v1;
            else
                value = v2;

            return true;
        default:
            value = v1 + ( v2 - v1 ) * x;
            return true;
        }
    }

    return false;
}
} // anon namespace

NiBtOgre::NiTimeController::NiTimeController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiObject(index, stream, model, data)
{
    stream.read(mNextControllerIndex);
    stream.read(mFlags);
    stream.read(mFrequency);
    stream.read(mPhase);
    stream.read(mStartTime);
    stream.read(mStopTime);

    stream.read(mTargetIndex);
}

// baseclass does nothing?
NiBtOgre::NiTimeControllerRef NiBtOgre::NiTimeController::build(Ogre::Mesh *mesh)
{
    //std::cerr << "controller not implemented: " << NiObject::mModel.blockType(NiObject::index()) << std::endl;

    return mNextControllerIndex;
}

// baseclass does nothing?
NiBtOgre::NiTimeControllerRef NiBtOgre::NiTimeController::build(std::vector<Ogre::Controller<float> >& controllers)
{
    //std::cerr << "controller not implemented: " << NiObject::mModel.blockType(NiObject::index()) << std::endl;

    return mNextControllerIndex;
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSFrustumFOVController::BSFrustumFOVController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiTimeController(index, stream, model, data)
{
    stream.read(mInterpolatorIndex);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLagBoneController::BSLagBoneController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiTimeController(index, stream, model, data)
{
    stream.read(mLinearVelocity);
    stream.read(mLinearRotation);
    stream.read(mMaximumDistance);
}

void NiBtOgre::NiBSBoneLODController::NodeGroup::read(NiStream& stream, const NiModel& model, ModelData& data)
{
    stream.read(numNodes);

    nodes.resize(numNodes);
    for (unsigned int i = 0; i < numNodes; ++i)
        stream.read(nodes.at(i));
}

void NiBtOgre::NiBSBoneLODController::SkinShapeGroup::read(NiStream& stream, const NiModel& model, ModelData& data)
{
    std::int32_t rIndex = -1;
    stream.read(numLinkPairs);

    linkPairs.resize(numLinkPairs);
    for (unsigned int i = 0; i < numLinkPairs; ++i)
    {
        //stream.getPtr<NiGeometry>(linkPairs.at(i).shape, model.objects());
        rIndex = -1;
        stream.read(rIndex);
        linkPairs.at(i).shape = model.getRef<NiGeometry>(rIndex);
        stream.read(linkPairs.at(i).skinInstanceIndex);
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBSBoneLODController::NiBSBoneLODController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiTimeController(index, stream, model, data)
{
    stream.skip(sizeof(std::uint32_t)); // Unknown Int 1
    std::uint32_t numNodeGroups;
    stream.read(numNodeGroups);
    std::uint32_t numNodeGroups2;
    stream.read(numNodeGroups2);

    mNodeGroups.resize(numNodeGroups);
    for (unsigned int i = 0; i < numNodeGroups; ++i)
        mNodeGroups[i].read(stream, model, data);

    //if (stream.nifVer() >= 0x04020200) // from 4.2.2.0
    //{
#if 0 // seems to be user version controlled
        std::uint32_t numShapeGroups;
        stream.read(numShapeGroups);
        mShapeGroups.resize(numShapeGroups);
        for (unsigned int i = 0; i < numShapeGroups; ++i)
            mShapeGroups[i].read(index, stream, model, data);

        std::uint32_t numShapeGroups2;
        stream.read(numShapeGroups2);
        mShapeGroups2.resize(numShapeGroups2);
        for (unsigned int i = 0; i < numShapeGroups2; ++i)
            stream.read(mShapeGroups2.at(i));
#endif
    //}
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiControllerManager::NiControllerManager(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiTimeController(index, stream, model, data)
{
    mCumulative = stream.getBool();

    std::uint32_t numControllerSequences;
    stream.read(numControllerSequences);
    mControllerSequences.resize(numControllerSequences);
    for (unsigned int i = 0; i < numControllerSequences; ++i)
        stream.read(mControllerSequences.at(i));

    stream.read(mObjectPaletteIndex);
}

// Each NiControllerSequence is a "playable" animation. The animation in Ogre implementation
// maybe skeletal, vertex, SceneNode, etc.  If the name of the animation if "Idle" it is
// automatically played (and most likely to be CYCLE_LOOP).
//
// Each NiControllerSquence specifies the following animation attributes:
//
//   * target node name
//   * cycle type - loop, run once, reverse <---- ignore the ones in NiControllerManager
//   * start/stop time, frequency <-------------- ignore the ones in NiControllerManager
//   * string palette
//   * text keys (e.g. 'start', 'end'), etc
//
// FIXME: how to decide whether to read in the 'kf' files in the directory?
//
NiBtOgre::NiTimeControllerRef NiBtOgre::NiControllerManager::build(std::vector<Ogre::Controller<float> >& controllers)
{
    // object palette appears to be a lookup table to map the target name string to the block number
    // that NiSequenceController can use to get to the target objects
    const NiDefaultAVObjectPalette* objects = mModel.getRef<NiDefaultAVObjectPalette>(mObjectPaletteIndex);

    for (std::uint32_t i = 0; i < mControllerSequences.size(); ++i)
    {
        // FIXME: shoud update a map in 'inst' so that the animation can be played
        // (? how to get the entity for mapping against the animation name?)
        mModel.getRef<NiControllerSequence>(mControllerSequences[i])->build(objects);
    }

    return mNextControllerIndex;
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiMultiTargetTransformController::NiMultiTargetTransformController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiTimeController(index, stream, model, data), mData(data) // for accessing mSkeleton later
{
    //std::int32_t rIndex = -1;

    stream.read(mNumExtraTargets);
    mExtraTargets.resize(mNumExtraTargets);
    for (unsigned int i = 0; i < mNumExtraTargets; ++i)
    {
        stream.read(mExtraTargets.at(i));
        data.addSkelLeafIndex(mExtraTargets.at(i));
    }
}

void NiBtOgre::NiMultiTargetTransformController::build(int32_t nameIndex, NiAVObject* target, NiTransformInterpolator *interpolator, float startTime, float stopTime)
{
    if ((NiTimeController::mFlags & 0x8) == 0) // not active
        return;

    // NOTE: assume that NiControllerManager and NiControllerSequence is built already
    //assert(mNiControllerSequeceBuilt == true); // throw instead?

    //if (mModel.indexToString(nameIndex) == "Close") // FIXME: testing
        //return;

    std::string animationId = /*"NiMTTransform@block_" + std::to_string(interpolator->index()) + */mModel.indexToString(nameIndex);
    float totalAnimationLength = stopTime - startTime; // use the ones from the controller sequence



    // FIXME: animation should be created by NiSequenceController
    Ogre::Animation *animation;
    if (mData.mSkeleton->hasAnimation(animationId))
        animation = mData.mSkeleton->getAnimation(animationId);
    else
    {
        animation = mData.mSkeleton->createAnimation(animationId, totalAnimationLength);
        animation->setInterpolationMode(Ogre::Animation::IM_SPLINE);
    }

    // lookup bones
    std::string boneName = mModel.indexToString(target->getNameIndex());
    Ogre::Bone *bone = mData.mSkeleton->getBone(boneName);

    if (animation->hasNodeTrack(bone->getHandle()))
        return; // HACK: stop creation of the same object multiple times

    // use the interpolator block index and the track handle
    Ogre::NodeAnimationTrack* track = animation->createNodeTrack(bone->getHandle(), bone);

    NiTransformData *data = mModel.getRef<NiTransformData>(interpolator->mDataIndex);

    // setup data for later
    mData.setDoorBoneName(animationId, boneName);



    // FIXME: handle other types
    if (data->mRotationType == 4) // XYZ
    {
        //size_t maxKeys = std::max(data->mXRotations.keys.size(), data->mYRotations.keys.size());
        //maxKeys = std::max(maxKeys, data->mZRotations.keys.size());
        if (mModel.getModelName() == "meshes\\dungeons\\chargen\\idgate01.nif")
            std::cout << time << std::endl;

        std::vector<float> timeKeys;
        // get all the unique time keys
        std::vector<Key<float> > frames = data->mXRotations.keys;
        for (unsigned int i = 0; i < frames.size(); ++i)
            if (std::find(timeKeys.begin(), timeKeys.end(), frames[i].time) == timeKeys.end())
                timeKeys.push_back(frames[i].time);
        frames = data->mYRotations.keys;
        for (unsigned int i = 0; i < frames.size(); ++i) // FIXME: repeated code
            if (std::find(timeKeys.begin(), timeKeys.end(), frames[i].time) == timeKeys.end())
                timeKeys.push_back(frames[i].time);
        frames = data->mZRotations.keys;
        for (unsigned int i = 0; i < frames.size(); ++i) // FIXME: repeated code
            if (std::find(timeKeys.begin(), timeKeys.end(), frames[i].time) == timeKeys.end())
                timeKeys.push_back(frames[i].time);

        // from avakar's answer on https://stackoverflow.com/questions/2758080/how-to-sort-an-stl-vector
        std::sort(timeKeys.begin(), timeKeys.end(), [] (float const& a, float const& b) { return a < b; });

        for (unsigned int i = 0; i < timeKeys.size(); ++i)
        {
            float time = timeKeys[i];
            float value;
            int last;
            if (!interpolate(value, data->mXRotations, time, last)) value = 0.f;
            Ogre::Quaternion xr(Ogre::Radian(value), Ogre::Vector3::UNIT_X);
            if (!interpolate(value, data->mYRotations, time, last)) value = 0.f;
            Ogre::Quaternion yr(Ogre::Radian(value), Ogre::Vector3::UNIT_Y);
            if (!interpolate(value, data->mZRotations, time, last)) value = 0.f;
            Ogre::Quaternion zr(Ogre::Radian(value), Ogre::Vector3::UNIT_Z);

            Ogre::Quaternion q(zr*yr*xr);
            Ogre::TransformKeyFrame *kf = track->createNodeKeyFrame(time);
            // FIXME: have no idea why inverse is needed
            // NOTE: commented out since using the delta change hack
            kf->setRotation(/*interpolator->mRotation.Inverse() * */q);

            // FIXME: dungeons\chargen\idgate01.nif rotation values are wrong (coc "imperialdungeon01")
//#if 0
            //if (mModel.getModelName() == "meshes\\dungeons\\chargen\\idgate01.nif")
                std::cout << mModel.getModelName() << ": " << animationId << ": time " << time << " roll "
                          << Ogre::Quaternion(/*interpolator->mRotation.Inverse() * */q).getRoll().valueDegrees()
                          << std::endl;
//#endif
        }
    }

    // FIXME
        //kf->setTranslate();
        //kf->setScale();

}

NiBtOgre::NiSingleInterpController::NiSingleInterpController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiTimeController(index, stream, model, data)
{
    if (stream.nifVer() >= 0x0a020000) // from 10.2.0.0
        stream.read(mInterpolatorIndex);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiFloatExtraDataController::NiFloatExtraDataController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    if (stream.nifVer() >= 0x0a020000) // from 10.2.0.0
        stream.readLongString(mControllerDataIndex);

    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
    {
        unsigned char numExtraBytes;
        stream.read(numExtraBytes);
        stream.skip(sizeof(char)*7);
        for (unsigned int i = 0; i < numExtraBytes; ++i)
            stream.skip(sizeof(char));
    }
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSEffectShaderPropertyColorController::BSEffectShaderPropertyColorController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream.read(mUnknownInt1);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSEffectShaderPropertyFloatController::BSEffectShaderPropertyFloatController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream.read(mTargetVariable);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLightingShaderPropertyColorController::BSLightingShaderPropertyColorController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream.read(mTargetVariable);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLightingShaderPropertyFloatController::BSLightingShaderPropertyFloatController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream.read(mTargetVariable);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiTextureTransformController::NiTextureTransformController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream.skip(sizeof(char)); // Unknown2
    stream.read(mTextureSlot);
    stream.read(mOperation);

#if 0 // commented out since this object is not seen in TES3
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataIndex);
#endif
}

NiBtOgre::NiPathController::NiPathController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiTimeController(index, stream, model, data)
{
    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        stream.skip(sizeof(std::uint16_t)); // Unknown Short 2

    stream.skip(sizeof(std::uint32_t)); // Unknown Int 1
    stream.skip(sizeof(float)*2);       // Unknown Float 2, 3
    stream.skip(sizeof(std::uint16_t)); // Unknown Short

    stream.read(mPosDataIndex);
    stream.read(mFloatDataIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkBlendController::bhkBlendController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiTimeController(index, stream, model, data)
{
    stream.read(mUnknown);
}
