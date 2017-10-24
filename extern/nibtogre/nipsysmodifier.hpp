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
#ifndef NIBTOGRE_NIPSYSMODIFIER_H
#define NIBTOGRE_NIPSYSMODIFIER_H

#include <string>
#include <vector>
#include <cstdint>

#include <OgreVector3.h>
#include <OgreVector4.h>

#include "niobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiPSysModifier
//     BSPSysInheritVelocityModifier
//     BSPSysLODModifier
//     BSPSysScaleModifier
//     BSPSysSimpleColorModifier
//     BSPSysStripUpdateModifier
//     BSPSysSubTexModifier
//     BSParentVelocityModifier
//     BSWindModifier
//     NiPSysAgeDeathModifier
//     NiPSysBombModifier
//     NiPSysBoundUpdateModifier
//     NiPSysColliderManager
//     NiPSysColorModifier
//     NiPSysDragModifier
//     NiPSysEmitter
//         NiPSysMeshEmitter
//         NiPSysVolumeEmitter
//             NiPSysBoxEmitter
//             NiPSysCylinderEmitter
//             NiPSysSphereEmitter
//     NiPSysGravityModifier
//     NiPSysGrowFadeModifier
//     NiPSysPositionModifier
//     NiPSysRotationModifier
//     NiPSysSpawnModifier
namespace NiBtOgre
{
    class NiStream;
    class Header;

    struct NiParticleSystem;

    class NiPSysModifier : public NiObject
    {
    public:
        std::uint32_t     mNameIndex;
        std::uint32_t     mOrder;
        NiParticleSystem *mTarget; // Ptr
        bool              mActive;

        NiPSysModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class BSPSysInheritVelocityModifier : public NiPSysModifier
    {
    public:
        std::uint32_t mUnknownI1;
        float         mUnknownF1;
        float         mUnknownF2;
        float         mUnknownF3;

        BSPSysInheritVelocityModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class BSPSysLODModifier : public NiPSysModifier
    {
    public:
        float mUnknown1;
        float mUnknown2;
        float mUnknown3;
        float mUnknown4;

        BSPSysLODModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class BSPSysScaleModifier : public NiPSysModifier
    {
    public:
        std::vector<float> mFloats;

        BSPSysScaleModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class BSPSysSimpleColorModifier : public NiPSysModifier
    {
    public:
        float mFadeInPercent;
        float mFadeOutPercent;
        float mColor1EndPerCent;
        float mColor1StartPerCent;
        float mColor2EndPerCent;
        float mColor2StartPerCent;
        std::vector<Ogre::Vector4> mColors;

        BSPSysSimpleColorModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class BSPSysStripUpdateModifier : public NiPSysModifier
    {
    public:
        float mUpdateDeltaTime;

        BSPSysStripUpdateModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class BSPSysSubTexModifier : public NiPSysModifier
    {
    public:
        std::uint32_t mStartFrame;
        float mStartFrameFudge;
        float mEndFrame;
        float mLoopStartFrame;
        float mLoopStartFrameFudge;
        float mFrameCount;
        float mFrameCountFudge;

        BSPSysSubTexModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class BSParentVelocityModifier : public NiPSysModifier
    {
    public:
        float mDamping;

        BSParentVelocityModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class BSWindModifier : public NiPSysModifier
    {
    public:
        float mStrength;

        BSWindModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysAgeDeathModifier : public NiPSysModifier
    {
    public:
        bool mSpawnOnDeath;
        NiPSysSpawnModifierRef mSpawnModifierIndex;

        NiPSysAgeDeathModifier(NiStream& stream, const NiModel& model);
    };

    class NiNode;

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysBombModifier : public NiPSysModifier
    {
    public:
        NiNode *mBombObject; // Ptr
        Ogre::Vector3 mBombAxis;
        float mDecay;
        float mDeltaV;
        std::uint32_t mDecayType;
        std::uint32_t mSymmetryType;

        NiPSysBombModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysBoundUpdateModifier : public NiPSysModifier
    {
    public:
        std::uint16_t mUpdateSkip;

        NiPSysBoundUpdateModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysColliderManager : public NiPSysModifier
    {
    public:
        NiPSysColliderRef mColliderIndex;

        NiPSysColliderManager(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysColorModifier : public NiPSysModifier
    {
    public:
        NiColorDataRef mDataIndex;

        NiPSysColorModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysDragModifier : public NiPSysModifier
    {
    public:
        NiObject *mParent; // Ptr
        Ogre::Vector3 mDragAxis;
        float mPercentage;
        float mRange;
        float mRangeFalloff;

        NiPSysDragModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysEmitter : public NiPSysModifier
    {
    public:
        float mSpeed;
        float mSpeedVariation;
        float mDeclination;
        float mDeclinationVariation;
        float mPlanarAngle;
        float mPlanarAngleVariation;
        Ogre::Vector4 mInitialColor;
        float mInitialRadius;
        float mRadiusVariation;
        float mLifeSpan;
        float mLifeSpanVariation;

        NiPSysEmitter(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysMeshEmitter : public NiPSysEmitter
    {
    public:
        std::vector<NiTriBasedGeomRef> mEmitterMeshes;
        std::uint32_t mInitialVelocityType;
        std::uint32_t mEmissionType;
        Ogre::Vector3 mEmissionAxis;

        NiPSysMeshEmitter(NiStream& stream, const NiModel& model);
    };

    class NiPSysVolumeEmitter : public NiPSysEmitter
    {
        NiNode *mEmitterObject; // Ptr

    public:
        NiPSysVolumeEmitter(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysBoxEmitter : public NiPSysVolumeEmitter
    {
    public:
        float mWidth;
        float mHeight;
        float mDepth;

        NiPSysBoxEmitter(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysCylinderEmitter : public NiPSysVolumeEmitter
    {
    public:
        float mRadius;
        float mHeight;

        NiPSysCylinderEmitter(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysSphereEmitter : public NiPSysVolumeEmitter
    {
    public:
        float mRadius;

        NiPSysSphereEmitter(NiStream& stream, const NiModel& model);
    };

    class NiNode;

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysGravityModifier : public NiPSysModifier
    {
    public:
        NiNode *mGravityObject; // Ptr
        Ogre::Vector3 mGravityAxis;
        float mDecay;
        float mStrength;
        std::uint32_t mForceType;
        float mTurbulence;
        float mTurbulenceScale;

        NiPSysGravityModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysGrowFadeModifier : public NiPSysModifier
    {
    public:
        float mGrowTime;
        std::uint16_t mGrowGeneration;
        float mFadeTime;
        std::uint16_t mFadeGeneration;
        float mBaseScale;

        NiPSysGrowFadeModifier(NiStream& stream, const NiModel& model);
    };

    typedef NiPSysModifier NiPSysPositionModifier; // Seen in NIF ver 20.0.0.4, 20.0.0.5

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysRotationModifier : public NiPSysModifier
    {
    public:
        float mInitialRotationSpeed;
        float mInitialRotationSpeedVariation;
        float mInitialRotationAngle;
        float mInitialRotationAngleVariation;
        bool  mRandomRotSpeedSign;
        bool  mRandomInitialAxis;
        Ogre::Vector3 mInitialAxis;

        NiPSysRotationModifier(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysSpawnModifier : public NiPSysModifier
    {
    public:
        std::uint16_t mNumSpawnGenerations;
        float mPercentSpawned;
        std::uint16_t mMinNumToSpawn;
        std::uint16_t mMaxNumToSpawn;
        float mSpawnSpeedChaos;
        float mSpawnDirChaos;
        float mLifeSpan;
        float mLifeSpanVariation;

        NiPSysSpawnModifier(NiStream& stream, const NiModel& model);
    };
}

#endif // NIBTOGRE_NIPSYSMODIFIER_H
