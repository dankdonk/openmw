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
#include "nitimecontroller.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "niavobject.hpp"  // static_cast NiGeometry
#include "nimodel.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

NiBtOgre::NiTimeController::NiTimeController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiObject(index, stream, model)
{
    stream.read(mNextControllerIndex);
    stream.read(mFlags);
    stream.read(mFrequency);
    stream.read(mPhase);
    stream.read(mStartTime);
    stream.read(mStopTime);

    stream.read(mTargetIndex);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSFrustumFOVController::BSFrustumFOVController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model)
{
    stream.read(mInterpolatorIndex);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLagBoneController::BSLagBoneController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model)
{
    stream.read(mLinearVelocity);
    stream.read(mLinearRotation);
    stream.read(mMaximumDistance);
}

void NiBtOgre::NiBSBoneLODController::NodeGroup::read(NiStream& stream, const NiModel& model)
{
    stream.read(numNodes);

    nodes.resize(numNodes);
    for (unsigned int i = 0; i < numNodes; ++i)
        stream.read(nodes.at(i));
}

void NiBtOgre::NiBSBoneLODController::SkinShapeGroup::read(NiStream& stream, const NiModel& model)
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
NiBtOgre::NiBSBoneLODController::NiBSBoneLODController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model)
{
    stream.skip(sizeof(std::uint32_t)); // Unknown Int 1
    std::uint32_t numNodeGroups;
    stream.read(numNodeGroups);
    std::uint32_t numNodeGroups2;
    stream.read(numNodeGroups2);

    mNodeGroups.resize(numNodeGroups);
    for (unsigned int i = 0; i < numNodeGroups; ++i)
        mNodeGroups[i].read(stream, model);

    //if (stream.nifVer() >= 0x04020200) // from 4.2.2.0
    //{
#if 0 // seems to be user version controlled
        std::uint32_t numShapeGroups;
        stream.read(numShapeGroups);
        mShapeGroups.resize(numShapeGroups);
        for (unsigned int i = 0; i < numShapeGroups; ++i)
            mShapeGroups[i].read(index, stream, model);

        std::uint32_t numShapeGroups2;
        stream.read(numShapeGroups2);
        mShapeGroups2.resize(numShapeGroups2);
        for (unsigned int i = 0; i < numShapeGroups2; ++i)
            stream.read(mShapeGroups2.at(i));
#endif
    //}
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiControllerManager::NiControllerManager(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model)
{
    mCumulative = stream.getBool();

    std::uint32_t numControllerSequences;
    stream.read(numControllerSequences);
    mControllerSequences.resize(numControllerSequences);
    for (unsigned int i = 0; i < numControllerSequences; ++i)
        stream.read(mControllerSequences.at(i));

    stream.read(mObjectPaletteIndex);
}

NiBtOgre::NiGeomMorpherController::NiGeomMorpherController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model)
{
    if (stream.nifVer() >= 0x0a000102) // from 10.0.1.2
        stream.read(mExtraFlags);

    if (stream.nifVer() == 0x0a01006a) // 10.1.0.106
        stream.skip(sizeof(char)); // Unknown 2

    stream.read(mDataIndex);
    stream.read(mAlwaysUpdate);

    if (stream.nifVer() >= 0x0a01006a) // from 10.1.0.106
    {
        std::uint32_t numInterpolators;
        stream.read(numInterpolators);
        if (stream.nifVer() >= 0x0a020000 && stream.nifVer() <= 0x14000005)
        {
            mInterpolators.resize(numInterpolators);
            for (unsigned int i = 0; i < numInterpolators; ++i)
                stream.read(mInterpolators.at(i));
        }

        if (stream.nifVer() >= 0x14010003) // from 20.1.0.3
        {
            mInterpolatorWeights.resize(numInterpolators);
            for (unsigned int i = 0; i < numInterpolators; ++i)
            {
                stream.read(mInterpolatorWeights[i].interpolatorIndex);
                stream.read(mInterpolatorWeights[i].weight);
            }
        }

        if (stream.nifVer() >= 0x14000004 && stream.nifVer() <= 0x14000005 && stream.userVer() >= 10)
        {
            std::uint32_t numUnknownInts;
            stream.read(numUnknownInts);
            mUnknownInts.resize(numUnknownInts);
            for (unsigned int i = 0; i < numUnknownInts; ++i)
                stream.read(mUnknownInts.at(i));
        }
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiMultiTargetTransformController::NiMultiTargetTransformController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model)
{
    //std::int32_t rIndex = -1;

    stream.read(mNumExtraTargets);
    mExtraTargets.resize(mNumExtraTargets);
    for (unsigned int i = 0; i < mNumExtraTargets; ++i)
        stream.read(mExtraTargets.at(i));
}

NiBtOgre::NiSingleInterpController::NiSingleInterpController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model)
{
    if (stream.nifVer() >= 0x0a020000) // from 10.2.0.0
        stream.read(mInterpolatorIndex);
}

NiBtOgre::NiVisController::NiVisController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
{
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataIndex);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiFloatExtraDataController::NiFloatExtraDataController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
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
NiBtOgre::BSEffectShaderPropertyColorController::BSEffectShaderPropertyColorController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
{
    stream.read(mUnknownInt1);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSEffectShaderPropertyFloatController::BSEffectShaderPropertyFloatController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
{
    stream.read(mTargetVariable);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLightingShaderPropertyColorController::BSLightingShaderPropertyColorController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
{
    stream.read(mTargetVariable);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLightingShaderPropertyFloatController::BSLightingShaderPropertyFloatController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
{
    stream.read(mTargetVariable);
}

NiBtOgre::NiAlphaController::NiAlphaController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
{
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataIndex);
}

NiBtOgre::NiFlipController::NiFlipController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
{
    stream.read(mTexureSlot);

    if (stream.nifVer() >= 0x04000000  && stream.nifVer() <= 0x0a010000)
        stream.skip(sizeof(std::uint32_t)); // Unknown Int 2

    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDelta);

    stream.readVector<NiSourceTextureRef>(mSources); // from 4.0.0.0
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiTextureTransformController::NiTextureTransformController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
{
    stream.skip(sizeof(char)); // Unknown2
    stream.read(mTextureSlot);
    stream.read(mOperation);

#if 0 // commented out since this object is not seen in TES3
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataIndex);
#endif
}

NiBtOgre::NiKeyframeController::NiKeyframeController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
{
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataIndex);
}

NiBtOgre::NiPSysModifierCtlr::NiPSysModifierCtlr(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
{
    stream.readLongString(mModifierNameIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysEmitterCtlr::NiPSysEmitterCtlr(uint32_t index, NiStream& stream, const NiModel& model)
    : NiPSysModifierCtlr(index, stream, model)
{
#if 0 // commented out since this object is not seen in TES3
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataIndex);
    if (stream.nifVer() >= 0x0a020000) // from 10.2.0.0
#endif
        stream.read(mVisibilityInterpolatorIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysModifierActiveCtlr::NiPSysModifierActiveCtlr(uint32_t index, NiStream& stream, const NiModel& model)
    : NiPSysModifierCtlr(index, stream, model)
{
#if 0 // commented out since this object is not seen in TES3
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataIndex);
#endif
}

NiBtOgre::NiPSysModifierFloatCtlr::NiPSysModifierFloatCtlr(uint32_t index, NiStream& stream, const NiModel& model)
    : NiPSysModifierCtlr(index, stream, model)
{
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataIndex);
}

NiBtOgre::NiPoint3InterpController::NiPoint3InterpController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
{
    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        stream.read(mTargetColor);

    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataIndex);
}

NiBtOgre::NiParticleSystemController::NiParticleSystemController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model)
{
    stream.read(mSpeed);
    stream.read(mSpeedRandom);
    stream.read(mVerticalDirection);
    stream.read(mVerticalAngle);
    stream.read(mHorizontalDirection);
    stream.read(mHorizontalAngle);
    stream.skip(sizeof(float)*3);// normal?
    stream.skip(sizeof(float)*4);// color?
    stream.read(mSize);
    stream.read(mEmitStartTime);
    stream.read(mEmitStopTime);
    stream.skip(sizeof(char)); // Unknown Byte, from 4.0.0.2
    stream.read(mEmitRate);
    stream.read(mLifetime);
    stream.read(mLifetimeRandom);

    stream.read(mEmitFlags);
    stream.read(mStartRandom);

    //stream.getPtr<NiObject>(mEmitter, model.objects());
    std::int32_t rIndex = -1;
    stream.read(rIndex);
    mEmitter = model.getRef<NiObject>(rIndex);

    stream.skip(16); // FIXME: provide more detail on what's being skipped

    stream.read(mNumParticles);
    stream.read(mNumValid);

    mParticles.resize(mNumParticles);
    for (unsigned int i = 0; i < mNumParticles; ++i)
    {
        stream.read(mParticles[i].velocity);
        stream.skip(sizeof(float)*3); // Unknown Vector
        stream.read(mParticles[i].lifetime);
        stream.read(mParticles[i].lifespan);
        stream.read(mParticles[i].timestamp);
        stream.skip(sizeof(std::int16_t)); // Unknown Short
        stream.read(mParticles[i].vertexID);
    }

    stream.skip(sizeof(std::int32_t)); // Unknown Link
    stream.read(mParticleExtraIndex);
    stream.skip(sizeof(std::int32_t)); // Unknown Link 2
    stream.skip(sizeof(char)); // Trailer
}

NiBtOgre::NiPathController::NiPathController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model)
{
    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        stream.skip(sizeof(std::uint16_t)); // Unknown Short 2

    stream.skip(sizeof(std::uint32_t)); // Unknown Int 1
    stream.skip(sizeof(float)*2);       // Unknown Float 2, 3
    stream.skip(sizeof(std::uint16_t)); // Unknown Short

    stream.read(mPosDataIndex);
    stream.read(mFloatDataIndex);
}

NiBtOgre::NiUVController::NiUVController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model)
{
    stream.skip(sizeof(std::uint16_t)); // Unknown Short
    stream.read(mDataIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkBlendController::bhkBlendController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model)
{
    stream.read(mUnknown);
}
