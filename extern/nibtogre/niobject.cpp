/*
  Copyright (C) 2017-2019 cc9cii

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
#include "niobject.hpp"

#include "nidata.hpp"
#include "nicollisionobject.hpp"
#include "niinterpolator.hpp"

#include "niavobject.hpp"
#include "nigeometry.hpp"
#include "niparticlesystem.hpp"
#include "ninode.hpp"

#include "niproperty.hpp"
#include "niparticlemodifier.hpp"
#include "nipsyscollider.hpp"
#include "nipsysmodifier.hpp"
#include "nisequence.hpp"

#include "nitimecontroller.hpp"
#include "nialphacontroller.hpp"
#include "niflipcontroller.hpp"
#include "nigeommorphercontroller.hpp"
#include "nikeyframecontroller.hpp"
#include "nimaterialcolorcontroller.hpp"
#include "niparticlesystemcontroller.hpp"
#include "nipsysmodifierctlr.hpp"
#include "niuvcontroller.hpp"
#include "niviscontroller.hpp"

#include "bhkrefobject.hpp"

namespace NiBtOgre
{
    NiObject::Factory NiObject::mFactory;

    static bool defineObjects()
    {
        // Seen in NIF version 4.0.0.2
        NiObject::define<AvoidNode>("AvoidNode");
        NiObject::define<NiAlphaController>("NiAlphaController");
        NiObject::define<NiAlphaProperty>("NiAlphaProperty");
        NiObject::define<NiAmbientLight>("NiAmbientLight");
        NiObject::define<NiAutoNormalParticles>("NiAutoNormalParticles");
        NiObject::define<NiAutoNormalParticlesData>("NiAutoNormalParticlesData");
        NiObject::define<NiBSAnimationNode>("NiBSAnimationNode");
        NiObject::define<NiBSPArrayController>("NiBSPArrayController");
        NiObject::define<NiBSParticleNode>("NiBSParticleNode");
        NiObject::define<NiBillboardNode>("NiBillboardNode");
        NiObject::define<NiCamera>("NiCamera");
        NiObject::define<NiColorData>("NiColorData");
        NiObject::define<NiDirectionalLight>("NiDirectionalLight");
        NiObject::define<NiDitherProperty>("NiDitherProperty");
        NiObject::define<NiFlipController>("NiFlipController");
        NiObject::define<NiFloatData>("NiFloatData");
        NiObject::define<NiFogProperty>("NiFogProperty");
        NiObject::define<NiGeomMorpherController>("NiGeomMorpherController");
        NiObject::define<NiGravity>("NiGravity");
        NiObject::define<NiKeyframeController>("NiKeyframeController");
        NiObject::define<NiKeyframeData>("NiKeyframeData");
        NiObject::define<NiMaterialColorController>("NiMaterialColorController");
        NiObject::define<NiMaterialProperty>("NiMaterialProperty");
        NiObject::define<NiMorphData>("NiMorphData");
        NiObject::define<NiNode>("NiNode");
        NiObject::define<NiParticleColorModifier>("NiParticleColorModifier");
        NiObject::define<NiParticleGrowFade>("NiParticleGrowFade");
        NiObject::define<NiParticleRotation>("NiParticleRotation");
        NiObject::define<NiParticleSystemController>("NiParticleSystemController");
        NiObject::define<NiPathController>("NiPathController");
        NiObject::define<NiPixelData>("NiPixelData");
        NiObject::define<NiPlanarCollider>("NiPlanarCollider");
        NiObject::define<NiPosData>("NiPosData");
        NiObject::define<NiRotatingParticles>("NiRotatingParticles");
        NiObject::define<NiRotatingParticlesData>("NiRotatingParticlesData");
        NiObject::define<NiSequenceStreamHelper>("NiSequenceStreamHelper");
        NiObject::define<NiShadeProperty>("NiShadeProperty");
        NiObject::define<NiSkinData>("NiSkinData");
        NiObject::define<NiSkinInstance>("NiSkinInstance");
        NiObject::define<NiSourceTexture>("NiSourceTexture");
        NiObject::define<NiSpecularProperty>("NiSpecularProperty");
        NiObject::define<NiStencilProperty>("NiStencilProperty");
        NiObject::define<NiStringExtraData>("NiStringExtraData");
        NiObject::define<NiTextKeyExtraData>("NiTextKeyExtraData");
        NiObject::define<NiTextureEffect>("NiTextureEffect");
        NiObject::define<NiTexturingProperty>("NiTexturingProperty");
        NiObject::define<NiTriShape>("NiTriShape");
        NiObject::define<NiTriShapeData>("NiTriShapeData");
        NiObject::define<NiUVController>("NiUVController");
        NiObject::define<NiUVData>("NiUVData");
        NiObject::define<NiVertWeightsExtraData>("NiVertWeightsExtraData");
        NiObject::define<NiVertexColorProperty>("NiVertexColorProperty");
        NiObject::define<NiVisController>("NiVisController");
        NiObject::define<NiVisData>("NiVisData");
        NiObject::define<NiWireframeProperty>("NiWireframeProperty");
        NiObject::define<NiZBufferProperty>("NiZBufferProperty");
        NiObject::define<RootCollisionNode>("RootCollisionNode");
        // Seen in NIF version 10.0.1.0
        NiObject::define<bhkConvexSweepShape>("bhkConvexSweepShape");
        // Seen in NIF version 20.0.0.4, 20.0.0.5
        NiObject::define<BSBound>("BSBound");
        NiObject::define<BSFurnitureMarker>("BSFurnitureMarker");
        NiObject::define<BSParentVelocityModifier>("BSParentVelocityModifier");
        NiObject::define<BSWindModifier>("BSWindModifier");
        NiObject::define<BSXFlags>("BSXFlags");
        NiObject::define<NiBSBoneLODController>("NiBSBoneLODController");
        NiObject::define<NiBinaryExtraData>("NiBinaryExtraData");
        NiObject::define<NiBlendBoolInterpolator>("NiBlendBoolInterpolator");
        NiObject::define<NiBlendFloatInterpolator>("NiBlendFloatInterpolator");
        NiObject::define<NiBlendPoint3Interpolator>("NiBlendPoint3Interpolator");
        NiObject::define<NiBlendTransformInterpolator>("NiBlendTransformInterpolator");
        NiObject::define<NiBoolData>("NiBoolData");
        NiObject::define<NiBoolInterpolator>("NiBoolInterpolator");
        NiObject::define<NiBoolTimelineInterpolator>("NiBoolTimelineInterpolator");
        NiObject::define<NiCollisionObject>("NiCollisionObject");
        NiObject::define<NiControllerManager>("NiControllerManager");
        NiObject::define<NiControllerSequence>("NiControllerSequence");
        NiObject::define<NiDefaultAVObjectPalette>("NiDefaultAVObjectPalette");
        NiObject::define<NiFloatInterpolator>("NiFloatInterpolator");
        NiObject::define<NiGeometry>("NiGeometry");
        NiObject::define<NiInterpolator>("NiInterpolator");
        NiObject::define<NiMultiTargetTransformController>("NiMultiTargetTransformController");
        NiObject::define<NiPSysAgeDeathModifier>("NiPSysAgeDeathModifier");
        NiObject::define<NiPSysBoundUpdateModifier>("NiPSysBoundUpdateModifier");
        NiObject::define<NiPSysBoxEmitter>("NiPSysBoxEmitter");
        NiObject::define<NiPSysCollider>("NiPSysCollider");
        NiObject::define<NiPSysColliderManager>("NiPSysColliderManager");
        NiObject::define<NiPSysColorModifier>("NiPSysColorModifier");
        NiObject::define<NiPSysCylinderEmitter>("NiPSysCylinderEmitter");
        NiObject::define<NiPSysData>("NiPSysData");
        NiObject::define<NiPSysDragModifier>("NiPSysDragModifier");
        NiObject::define<NiPSysEmitter>("NiPSysEmitter");
        NiObject::define<NiPSysEmitterCtlr>("NiPSysEmitterCtlr");
        NiObject::define<NiPSysEmitterInitialRadiusCtlr>("NiPSysEmitterInitialRadiusCtlr");
        NiObject::define<NiPSysEmitterDeclinationCtlr>("NiPSysEmitterDeclinationCtlr");
        NiObject::define<NiPSysEmitterLifeSpanCtlr>("NiPSysEmitterLifeSpanCtlr");
        NiObject::define<NiPSysEmitterSpeedCtlr>("NiPSysEmitterSpeedCtlr");
        NiObject::define<NiPSysGravityModifier>("NiPSysGravityModifier");
        NiObject::define<NiPSysGravityStrengthCtlr>("NiPSysGravityStrengthCtlr");
        NiObject::define<NiPSysGrowFadeModifier>("NiPSysGrowFadeModifier");
        NiObject::define<NiPSysMeshEmitter>("NiPSysMeshEmitter");
        NiObject::define<NiPSysModifier>("NiPSysModifier");
        NiObject::define<NiPSysModifierActiveCtlr>("NiPSysModifierActiveCtlr");
        NiObject::define<NiPSysPlanarCollider>("NiPSysPlanarCollider");
        NiObject::define<NiPSysPositionModifier>("NiPSysPositionModifier");
        NiObject::define<NiPSysRotationModifier>("NiPSysRotationModifier");
        NiObject::define<NiPSysSpawnModifier>("NiPSysSpawnModifier");
        NiObject::define<NiPSysSphereEmitter>("NiPSysSphereEmitter");
        NiObject::define<NiPSysUpdateCtlr>("NiPSysUpdateCtlr");
        NiObject::define<NiParticleSystem>("NiParticleSystem");
        NiObject::define<NiPathInterpolator>("NiPathInterpolator");
        NiObject::define<NiPoint3Interpolator>("NiPoint3Interpolator");
        NiObject::define<NiSequence>("NiSequence");
        NiObject::define<NiSkinPartition>("NiSkinPartition");
        NiObject::define<NiStringPalette>("NiStringPalette");
        NiObject::define<NiTextureTransformController>("NiTextureTransformController");
        NiObject::define<NiTransformController>("NiTransformController");
        NiObject::define<NiTransformData>("NiTransformData");
        NiObject::define<NiTransformInterpolator>("NiTransformInterpolator");
        NiObject::define<NiTriStrips>("NiTriStrips");
        NiObject::define<NiTriStripsData>("NiTriStripsData");
        NiObject::define<bhkBlendCollisionObject>("bhkBlendCollisionObject");
        NiObject::define<bhkBlendController>("bhkBlendController");
        NiObject::define<bhkBoxShape>("bhkBoxShape");
        NiObject::define<bhkCapsuleShape>("bhkCapsuleShape");
        NiObject::define<bhkCollisionObject>("bhkCollisionObject");
        NiObject::define<bhkConvexTransformShape>("bhkConvexTransformShape");
        NiObject::define<bhkConvexVerticesShape>("bhkConvexVerticesShape");
        NiObject::define<bhkLimitedHingeConstraint>("bhkLimitedHingeConstraint");
        NiObject::define<bhkListShape>("bhkListShape");
        NiObject::define<bhkMalleableConstraint>("bhkMalleableConstraint");
        NiObject::define<bhkMoppBvTreeShape>("bhkMoppBvTreeShape");
        NiObject::define<bhkMultiSphereShape>("bhkMultiSphereShape");
        NiObject::define<bhkNiCollisionObject>("bhkNiCollisionObject");
        NiObject::define<bhkNiTriStripsShape>("bhkNiTriStripsShape");
        NiObject::define<bhkPackedNiTriStripsShape>("bhkPackedNiTriStripsShape");
        NiObject::define<bhkPrismaticConstraint>("bhkPrismaticConstraint");
        NiObject::define<bhkRagdollConstraint>("bhkRagdollConstraint");
        NiObject::define<bhkRigidBody>("bhkRigidBody");
        NiObject::define<bhkRigidBodyT>("bhkRigidBodyT");
        NiObject::define<bhkSPCollisionObject>("bhkSPCollisionObject");
        NiObject::define<bhkSimpleShapePhantom>("bhkSimpleShapePhantom");
        NiObject::define<bhkSphereShape>("bhkSphereShape");
        NiObject::define<bhkStiffSpringConstraint>("bhkStiffSpringConstraint");
        NiObject::define<bhkTransformShape>("bhkTransformShape");
        NiObject::define<hkPackedNiTriStripsData>("hkPackedNiTriStripsData");
        // Seen in NIF version 20.2.0.7
        NiObject::define<BSBoneLODExtraData>("BSBoneLODExtraData");
        NiObject::define<BSBehaviorGraphExtraData>("BSBehaviorGraphExtraData");
        NiObject::define<BSBlastNode>("BSBlastNode");
        NiObject::define<BSDamageStage>("BSDamageStage");
        NiObject::define<BSDecalPlacementVectorExtraData>("BSDecalPlacementVectorExtraData");
        NiObject::define<BSEffectShaderProperty>("BSEffectShaderProperty");
        NiObject::define<BSEffectShaderPropertyColorController>("BSEffectShaderPropertyColorController");
        NiObject::define<BSEffectShaderPropertyFloatController>("BSEffectShaderPropertyFloatController");
        NiObject::define<BSFadeNode>("BSFadeNode");
        NiObject::define<BSFrustumFOVController>("BSFrustumFOVController");
        NiObject::define<BSFurnitureMarkerNode>("BSFurnitureMarkerNode");
        NiObject::define<BSInvMarker>("BSInvMarker");
        NiObject::define<BSLODTriShape>("BSLODTriShape");
        NiObject::define<BSLagBoneController>("BSLagBoneController");
        NiObject::define<BSLeafAnimNode>("BSLeafAnimNode");
        NiObject::define<BSLightingShaderProperty>("BSLightingShaderProperty");
        NiObject::define<BSLightingShaderPropertyColorController>("BSLightingShaderPropertyColorController");
        NiObject::define<BSLightingShaderPropertyFloatController>("BSLightingShaderPropertyFloatController");
        NiObject::define<BSMultiBound>("BSMultiBound");
        NiObject::define<BSMultiBoundData>("BSMultiBoundData");
        NiObject::define<BSMultiBoundNode>("BSMultiBoundNode");
        NiObject::define<BSMultiBoundOBB>("BSMultiBoundOBB");
        NiObject::define<BSNiAlphaPropertyTestRefController>("BSNiAlphaPropertyTestRefController");
        NiObject::define<BSOrderedNode>("BSOrderedNode");
        NiObject::define<BSPSysInheritVelocityModifier>("BSPSysInheritVelocityModifier");
        NiObject::define<BSPSysLODModifier>("BSPSysLODModifier");
        NiObject::define<BSPSysScaleModifier>("BSPSysScaleModifier");
        NiObject::define<BSPSysSimpleColorModifier>("BSPSysSimpleColorModifier");
        NiObject::define<BSPSysStripUpdateModifier>("BSPSysStripUpdateModifier");
        NiObject::define<BSPSysSubTexModifier>("BSPSysSubTexModifier");
        NiObject::define<BSShaderTextureSet>("BSShaderTextureSet");
        NiObject::define<BSStripPSysData>("BSStripPSysData");
        NiObject::define<BSStripParticleSystem>("BSStripParticleSystem");
        NiObject::define<BSTreeNode>("BSTreeNode");
        NiObject::define<BSValueNode>("BSValueNode");
        NiObject::define<BSWaterShaderProperty>("BSWaterShaderProperty");
        NiObject::define<NiBSplineBasisData>("NiBSplineBasisData");
        NiObject::define<NiBSplineCompTransformInterpolator>("NiBSplineCompTransformInterpolator");
        NiObject::define<NiBSplineCompFloatInterpolator>("NiBSplineCompFloatInterpolator");
        NiObject::define<NiBSplineData>("NiBSplineData");
        NiObject::define<NiBSplineFloatInterpolator>("NiBSplineFloatInterpolator");
        NiObject::define<NiBSplineInterpolator>("NiBSplineInterpolator");
        NiObject::define<NiBSplinePoint3Interpolator>("NiBSplinePoint3Interpolator");
        NiObject::define<NiBSplineTransformInterpolator>("NiBSplineTransformInterpolator");
        NiObject::define<NiBooleanExtraData>("NiBooleanExtraData");
        NiObject::define<NiExtraData>("NiExtraData");
        NiObject::define<NiFloatExtraData>("NiFloatExtraData");
        NiObject::define<NiFloatExtraDataController>("NiFloatExtraDataController");
        NiObject::define<NiIntegerExtraData>("NiIntegerExtraData");
        NiObject::define<NiLookAtInterpolator>("NiLookAtInterpolator");
        NiObject::define<NiPSysBombModifier>("NiPSysBombModifier");
        NiObject::define<NiPSysSphericalCollider>("NiPSysSphericalCollider");
        NiObject::define<NiSwitchNode>("NiSwitchNode");
        NiObject::define<NiProperty>("NiProperty");
        NiObject::define<bhkBallSocketConstraintChain>("bhkBallSocketConstraintChain");
        NiObject::define<bhkBreakableConstraint>("bhkBreakableConstraint");
        NiObject::define<bhkCompressedMeshShape>("bhkCompressedMeshShape");
        NiObject::define<bhkCompressedMeshShapeData>("bhkCompressedMeshShapeData");
        NiObject::define<bhkHingeConstraint>("bhkHingeConstraint");
        // Seen in FO3
        NiObject::define<BSRangeNode>("BSRangeNode");
        NiObject::define<BSDebrisNode>("BSDebrisNode");
        NiObject::define<BSShaderLightingProperty>("BSShaderLightingProperty");
        NiObject::define<BSShaderPPLightingProperty>("BSShaderPPLightingProperty");
        NiObject::define<BSShaderNoLightingProperty>("BSShaderNoLightingProperty");
        NiObject::define<bhkConvexListShape>("bhkConvexListShape");
        NiObject::define<BSDismemberSkinInstance>("BSDismemberSkinInstance");
        NiObject::define<BSMaterialEmittanceMultController>("BSMaterialEmittanceMultController");
      //NiObject::define<BSAnimNotes>("BSAnimNotes");
      //NiObject::define<BSMaterialEmittanceMultController>("BSMaterialEmittanceMultController");
      //NiObject::define<BSMultiBoundAABB>("BSMultiBoundAABB");
      //NiObject::define<BSMultiBoundSphere>("BSMultiBoundSphere");
      //NiObject::define<BSRefractionStrengthController>("BSRefractionStrengthController");
      //NiObject::define<BSSegmentedTriShape>("BSSegmentedTriShape");
      //NiObject::define<BSTreadTransfController>("BSTreadTransfController");
      //NiObject::define<BSTreadTransfInterpolator>("BSTreadTransfInterpolator");
      //NiObject::define<NiAdditionalGeometryData>("NiAdditionalGeometryData");
      //NiObject::define<NiBSplineCompPoint3Interpolator>("NiBSplineCompPoint3Interpolator");
      NiObject::define<NiLightColorController>("NiLightColorController");
      //NiObject::define<NiLightDimmerController>("NiLightDimmerController");
      //NiObject::define<NiOptimizeKeep>("NiOptimizeKeep");
      //NiObject::define<NiPSysAgeDeath>("NiPSysAgeDeath");
      NiObject::define<NiPointLight>("NiPointLight");
      //NiObject::define<bhkLiquidAction>("bhkLiquidAction");
      NiObject::define<bhkOrientHingedBodyAction>("bhkOrientHingedBodyAction");
      //NiObject::define<bhkPoseArray>("bhkPoseArray");

        return true; // it seems compiler can't distinguish the definition
    }
    // https://social.msdn.microsoft.com/Forums/vstudio/en-US/af83854b-3f02-4060-8555-de588842a50d/
    // error-c2761-void-foosetint-member-function-redeclaration-not-allowed?forum=vcgeneral
    bool dummy = defineObjects(); // dummy assignment workaround
}
