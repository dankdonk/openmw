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
#include "nipsysmodifier.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "nistream.hpp"
#include "niavobject.hpp" // static_cast NiParticleSystem, NiNode
#include "nimodel.hpp"

NiBtOgre::NiPSysModifier::NiPSysModifier(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    stream.readLongString(mNameIndex);
    stream.read(mOrder);

    //stream.getPtr<NiParticleSystem>(mTarget, model.objects());
    std::int32_t index = -1;
    stream.read(index);
    mTarget = model.getRef<NiParticleSystem>(index);

    mActive = stream.getBool();
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSPSysInheritVelocityModifier::BSPSysInheritVelocityModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mUnknownI1);
    stream.read(mUnknownF1);
    stream.read(mUnknownF2);
    stream.read(mUnknownF3);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSPSysLODModifier::BSPSysLODModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mUnknown1);
    stream.read(mUnknown2);
    stream.read(mUnknown3);
    stream.read(mUnknown4);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSPSysScaleModifier::BSPSysScaleModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.readVector<float>(mFloats);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSPSysSimpleColorModifier::BSPSysSimpleColorModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mFadeInPercent);
    stream.read(mFadeOutPercent);
    stream.read(mColor1EndPerCent);
    stream.read(mColor1StartPerCent);
    stream.read(mColor2EndPerCent);
    stream.read(mColor2StartPerCent);

    mColors.resize(3);
    stream.read(mColors.at(0));
    stream.read(mColors.at(1));
    stream.read(mColors.at(2));
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSPSysStripUpdateModifier::BSPSysStripUpdateModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mUpdateDeltaTime);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSPSysSubTexModifier::BSPSysSubTexModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mStartFrame);
    stream.read(mStartFrameFudge);
    stream.read(mEndFrame);
    stream.read(mLoopStartFrame);
    stream.read(mLoopStartFrameFudge);
    stream.read(mFrameCount);
    stream.read(mFrameCountFudge);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::BSParentVelocityModifier::BSParentVelocityModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mDamping);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::BSWindModifier::BSWindModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mStrength);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysAgeDeathModifier::NiPSysAgeDeathModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mSpawnOnDeath);
    stream.read(mSpawnModifierIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysBombModifier::NiPSysBombModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    //stream.getPtr<NiNode>(mBombObject, model.objects());
    std::int32_t index = -1;
    stream.read(index);
    mBombObject = model.getRef<NiNode>(index);

    stream.read(mBombAxis);
    stream.read(mDecay);
    stream.read(mDeltaV);
    stream.read(mDecayType);
    stream.read(mSymmetryType);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysBoundUpdateModifier::NiPSysBoundUpdateModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mUpdateSkip);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysColliderManager::NiPSysColliderManager(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mColliderIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysColorModifier::NiPSysColorModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mDataIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysDragModifier::NiPSysDragModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    //stream.getPtr<NiObject>(mParent, model.objects());
    std::int32_t index = -1;
    stream.read(index);
    mParent = model.getRef<NiObject>(index);

    stream.read(mDragAxis);
    stream.read(mPercentage);
    stream.read(mRange);
    stream.read(mRangeFalloff);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysEmitter::NiPSysEmitter(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mSpeed);
    stream.read(mSpeedVariation);
    stream.read(mDeclination);
    stream.read(mDeclinationVariation);
    stream.read(mPlanarAngle);
    stream.read(mPlanarAngleVariation);
    stream.read(mInitialColor);
    stream.read(mInitialRadius);
    stream.read(mRadiusVariation); // from 10.4.0.1
    stream.read(mLifeSpan);
    stream.read(mLifeSpanVariation);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysMeshEmitter::NiPSysMeshEmitter(NiStream& stream, const NiModel& model)
    : NiPSysEmitter(stream, model)
{
    stream.readVector<NiTriBasedGeomRef>(mEmitterMeshes);

    stream.read(mInitialVelocityType);
    stream.read(mEmissionType);
    stream.read(mEmissionAxis);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5 ????
NiBtOgre::NiPSysVolumeEmitter::NiPSysVolumeEmitter(NiStream& stream, const NiModel& model)
    : NiPSysEmitter(stream, model)
{
    //stream.getPtr<NiNode>(mEmitterObject, model.objects());
    std::int32_t index = -1;
    stream.read(index);
    mEmitterObject = model.getRef<NiNode>(index); // from 10.1.0.0
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysBoxEmitter::NiPSysBoxEmitter(NiStream& stream, const NiModel& model)
    : NiPSysVolumeEmitter(stream, model)
{
    stream.read(mWidth);
    stream.read(mHeight);
    stream.read(mDepth);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysCylinderEmitter::NiPSysCylinderEmitter(NiStream& stream, const NiModel& model)
    : NiPSysVolumeEmitter(stream, model)
{
    stream.read(mRadius);
    stream.read(mHeight);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysSphereEmitter::NiPSysSphereEmitter(NiStream& stream, const NiModel& model)
    : NiPSysVolumeEmitter(stream, model)
{
    stream.read(mRadius);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysGravityModifier::NiPSysGravityModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mGravityObjectIndex);

    stream.read(mGravityAxis);
    stream.read(mDecay);
    stream.read(mStrength);
    stream.read(mForceType);
    stream.read(mTurbulence);
    stream.read(mTurbulenceScale);
    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7
        stream.skip(sizeof(char));
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysGrowFadeModifier::NiPSysGrowFadeModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mGrowTime);
    stream.read(mGrowGeneration);
    stream.read(mFadeTime);
    stream.read(mFadeGeneration);
    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7
        stream.read(mBaseScale);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysRotationModifier::NiPSysRotationModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mInitialRotationSpeed);

    if (stream.nifVer() >= 0x14000004) // from 20.0.0.4
    {
        stream.read(mInitialRotationSpeedVariation);
        stream.read(mInitialRotationAngle);
        stream.read(mInitialRotationAngleVariation);
        stream.read(mRandomRotSpeedSign);
    }
    mRandomInitialAxis = stream.getBool();
    stream.read(mInitialAxis);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysSpawnModifier::NiPSysSpawnModifier(NiStream& stream, const NiModel& model)
    : NiPSysModifier(stream, model)
{
    stream.read(mNumSpawnGenerations);
    stream.read(mPercentSpawned);
    stream.read(mMinNumToSpawn);
    stream.read(mMaxNumToSpawn);
    stream.read(mSpawnSpeedChaos);
    stream.read(mSpawnDirChaos);
    stream.read(mLifeSpan);
    stream.read(mLifeSpanVariation);
    // 10.4.0.1 has an unknown int here
}
