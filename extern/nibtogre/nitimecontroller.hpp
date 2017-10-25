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
#ifndef NIBTOGRE_NITIMECONTROLLER_H
#define NIBTOGRE_NITIMECONTROLLER_H

#include <string>
#include <vector>
#include <cstdint>

#include <OgreVector3.h>

#include "niobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiTimeController
//     BSFrustumFOVController
//     BSLagBoneController
//     NiBSBoneLODController
//     NiControllerManager
//     NiInterpController <------------------------------- /* NiTimeController */
//         NiGeomMorpherController
//         NiMultiTargetTransformController
//         NiSingleInterpController
//             NiBoolInterpController <------------------- /* not implemented */
//                 NiVisController
//             NiExtraDataController <-------------------- /* not implemented */
//                 NiFloatExtraDataController
//             NiFloatInterpController <------------------ /* not implemented */
//                 BSEffectShaderPropertyColorController
//                 BSEffectShaderPropertyFloatController
//                 BSLightingShaderPropertyColorController
//                 BSLightingShaderPropertyFloatController
//                 NiAlphaController
//                     BSNiAlphaPropertyTestRefController  /* NiAlphaController */
//                 NiFlipController
//                 NiTextureTransformController
//             NiKeyframeController
//                 NiTransformController <---------------- /* NiKeyframeController */
//             NiPSysModifierCtlr
//                 NiPSysEmitterCtlr
//                 NiPSysModifierBoolCtlr <--------------- /* not implemented */
//                     NiPSysModifierActiveCtlr <--------- /* NiPSysModfierCtlr */
//                 NiPSysModifierFloatCtlr
//                     NiPSysEmitterInitialRadiusCtlr <--- /* NiPSysModifierFloatCtlr */
//                     NiPSysEmitterLifeSpanCtlr <-------- /* NiPSysModifierFloatCtlr */
//                     NiPSysEmitterSpeedCtlr <----------- /* NiPSysModifierFloatCtlr */
//                     NiPSysGravityStrengthCtlr <-------- /* NiPSysModifierFloatCtlr */
//             NiPoint3InterpController
//                 NiMaterialColorController <------------ /* NiPoint3InterpController */
//     NiPSysUpdateCtlr <--------------------------------- /* NiTimeController */
//     NiParticleSystemController
//         NiBSPArrayController <------------------------- /* NiParticleSystemController */
//     NiPathController
//     NiUVController
//     bhkBlendController
namespace NiBtOgre
{
    class NiObjectNET;

    class NiTimeController : public NiObject
    {
    public:
        NiTimeControllerRef mNextControllerIndex;
        std::uint16_t mFlags;
        float mFrequency;
        float mPhase;
        float mStartTime;
        float mStopTime;
        // lights/candlefat01.nif (TES4) shows that some of the Ptr refer to objects not yet
        // loaded.  Change to Ref instead.
        //NiObjectNET *mTarget; // Ptr
        NiObjectNETRef mTargetIndex;

        NiTimeController(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class BSFrustumFOVController : public NiTimeController
    {
    public:
        NiFloatInterpolatorRef mInterpolatorIndex;

        BSFrustumFOVController(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class BSLagBoneController : public NiTimeController
    {
    public:
        float mLinearVelocity;
        float mLinearRotation;
        float mMaximumDistance;

        BSLagBoneController(NiStream& stream, const NiModel& model);
    };

    class NiNode;
    struct NiGeometry;

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiBSBoneLODController : public NiTimeController
    {
        struct NodeGroup
        {
            std::uint32_t        numNodes;
            // _male/skeleton.nif (TES4) shows that some of the Ptr refer to objects not yet
            // loaded.  Change to Ref instead.
            //std::vector<NiNode*> nodes; // Ptr
            std::vector<NiNodeRef> nodes;

            void read(NiStream& stream, const NiModel& model);
        };

        struct SkinShapeGroup
        {
            struct SkinShape
            {
                NiGeometry       *shape; // Ptr
                NiSkinInstanceRef skinInstanceIndex;
            };

            std::uint32_t          numLinkPairs;
            std::vector<SkinShape> linkPairs;

            void read(NiStream& stream, const NiModel& model);
        };

        std::vector<NodeGroup>         mNodeGroups;
        std::vector<SkinShapeGroup>    mShapeGroups;
        std::vector<NiTriBasedGeomRef> mShapeGroups2;

    public:
        NiBSBoneLODController(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiControllerManager : public NiTimeController
    {
    public:
        bool mCumulative;
        std::vector<NiControllerSequenceRef> mControllerSequences;
        NiDefaultAvObjectPaletteRef mObjectPaletteIndex;

        NiControllerManager(NiStream& stream, const NiModel& model);
    };

    typedef NiTimeController NiInterpController;

    class NiGeomMorpherController : public NiInterpController
    {
        struct MorphWeight
        {
            NiInterpolatorRef interpolatorIndex;
            float weight;
        };

    public:
        std::uint16_t mExtraFlags;
        NiMorphDataRef mDataIndex;
        unsigned char mAlwaysUpdate;
        std::vector<NiInterpolatorRef> mInterpolators;
        std::vector<MorphWeight> mInterpolatorWeights;
        std::vector<std::uint32_t> mUnknownInts;

        NiGeomMorpherController(NiStream& stream, const NiModel& model);
    };

    class NiAVObject;

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiMultiTargetTransformController : public NiInterpController
    {
    public:
        std::uint16_t mNumExtraTargets;
        // clutter/minotaurhead01.nif (TES4) shows that some of the Ptr refer to objects not yet
        // loaded.  Change to Ref instead.
        //std::vector<NiAVObject*> mExtraTargets; // Ptr
        std::vector<NiAVObjectRef> mExtraTargets;

        NiMultiTargetTransformController(NiStream& stream, const NiModel& model);
    };

    class NiSingleInterpController : public NiInterpController
    {
    public:
        NiInterpolatorRef mInterpolatorIndex;

        NiSingleInterpController(NiStream& stream, const NiModel& model);
    };

    class NiVisController : public NiSingleInterpController
    {
    public:
        NiVisDataRef mDataIndex;

        NiVisController(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class NiFloatExtraDataController : public NiSingleInterpController
    {
    public:
        std::uint32_t mControllerDataIndex;

        NiFloatExtraDataController(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class BSEffectShaderPropertyColorController : public NiSingleInterpController
    {
    public:
        std::uint32_t mUnknownInt1;

        BSEffectShaderPropertyColorController(NiStream& stream, const NiModel& model);
    };

    // FIXME looks identical to BSEffectShaderPropertyColorController
    // Seen in NIF version 20.2.0.7
    class BSEffectShaderPropertyFloatController : public NiSingleInterpController
    {
    public:
        std::uint32_t mTargetVariable;

        BSEffectShaderPropertyFloatController(NiStream& stream, const NiModel& model);
    };

    // FIXME looks identical to BSEffectShaderPropertyColorController
    // Seen in NIF version 20.2.0.7
    class BSLightingShaderPropertyColorController : public NiSingleInterpController
    {
        std::uint32_t mTargetVariable;

    public:
        BSLightingShaderPropertyColorController(NiStream& stream, const NiModel& model);
    };

    // FIXME looks identical to BSEffectShaderPropertyColorController
    // Seen in NIF version 20.2.0.7
    class BSLightingShaderPropertyFloatController : public NiSingleInterpController
    {
        std::uint32_t mTargetVariable;

    public:
        BSLightingShaderPropertyFloatController(NiStream& stream, const NiModel& model);
    };

    class NiAlphaController : public NiSingleInterpController
    {
    public:
        NiFloatDataRef mDataIndex;

        NiAlphaController(NiStream& stream, const NiModel& model);
    };

    typedef NiAlphaController BSNiAlphaPropertyTestRefController; // Seen in NIF version 20.2.0.7

    class NiFlipController : public NiSingleInterpController
    {
    public:
        std::int32_t mTexureSlot;
        float mDelta;
        std::vector<NiSourceTextureRef> mSources;

        NiFlipController(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiTextureTransformController : public NiSingleInterpController
    {
    public:
        std::uint32_t mTextureSlot;
        std::uint32_t mOperation;
#if 0
        NiFloatDataRef mDataIndex;
#endif

        NiTextureTransformController(NiStream& stream, const NiModel& model);
    };

    class NiKeyframeController : public NiSingleInterpController
    {
    public:
        NiKeyframeDataRef mDataIndex;

        NiKeyframeController(NiStream& stream, const NiModel& model);
    };

    typedef NiKeyframeController NiTransformController; // Seen in NIF ver 20.0.0.4, 20.0.0.5

    class NiPSysModifierCtlr : public NiSingleInterpController
    {
    public:
        std::uint32_t mModifierNameIndex;

        NiPSysModifierCtlr(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysEmitterCtlr : public NiPSysModifierCtlr
    {
    public:
#if 0
        NiPSysEmitterCtlrDataRef mDataIndex;
#endif
        NiInterpolatorRef mVisibilityInterpolatorIndex;

        NiPSysEmitterCtlr(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysModifierActiveCtlr : public NiPSysModifierCtlr
    {
    public:
#if 0
        NiVisDataRef mDataIndex;
#endif

        NiPSysModifierActiveCtlr(NiStream& stream, const NiModel& model);
    };

    class NiPSysModifierFloatCtlr : public NiPSysModifierCtlr
    {
    public:
        NiFloatDataRef mDataIndex;

        NiPSysModifierFloatCtlr(NiStream& stream, const NiModel& model);
    };

    typedef NiPSysModifierFloatCtlr NiPSysEmitterInitialRadiusCtlr; // Seen in NIF ver 20.0.0.4, 20.0.0.5

    typedef NiPSysModifierFloatCtlr NiPSysEmitterLifeSpanCtlr; // Seen in NIF ver 20.0.0.4, 20.0.0.5

    typedef NiPSysModifierFloatCtlr NiPSysEmitterSpeedCtlr; // Seen in NIF ver 20.0.0.4, 20.0.0.5

    typedef NiPSysModifierFloatCtlr NiPSysGravityStrengthCtlr; // Seen in NIF ver 20.0.0.4, 20.0.0.5

    class NiPoint3InterpController : public NiSingleInterpController
    {
    public:
        std::uint16_t mTargetColor;
        NiPosDataRef  mDataIndex;

        NiPoint3InterpController(NiStream& stream, const NiModel& model);
    };

    typedef NiPoint3InterpController NiMaterialColorController;

    typedef NiTimeController NiPSysUpdateCtlr; // Seen in NIF ver 20.0.0.4, 20.0.0.5

    class NiParticleSystemController : public NiTimeController
    {
    public:
        struct Particle
        {
            Ogre::Vector3 velocity;
            float lifetime;
            float lifespan;
            float timestamp;
            std::uint16_t vertexID;
        };

        float mSpeed;
        float mSpeedRandom;

        float mVerticalDirection;
        float mVerticalAngle;
        float mHorizontalDirection;
        float mHorizontalAngle;

        float mSize;
        float mEmitStartTime;
        float mEmitStopTime;

        float mEmitRate;
        float mLifetime;
        float mLifetimeRandom;

        std::uint16_t mEmitFlags;

        Ogre::Vector3 mStartRandom;

        NiObject *mEmitter; // Ptr

        std::uint16_t mNumParticles;
        std::uint16_t mNumValid;
        std::vector<Particle> mParticles;

        NiParticleModifierRef mParticleExtraIndex;

        NiParticleSystemController(NiStream& stream, const NiModel& model);
    };

    typedef NiParticleSystemController NiBSPArrayController;

    class NiPathController : public NiTimeController
    {
    public:
        NiPosDataRef   mPosDataIndex;
        NiFloatDataRef mFloatDataIndex;

        NiPathController(NiStream& stream, const NiModel& model);
    };

    class NiUVController : public NiTimeController
    {
    public:
        NiUVDataRef mDataIndex;

        NiUVController(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class bhkBlendController : public NiTimeController
    {
    public:
        std::uint32_t mUnknown;

        bhkBlendController(NiStream& stream, const NiModel& model);
    };
}

#endif // NIBTOGRE_NITIMECONTROLLER_H
