/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (record.h) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_RECORD_HPP
#define OPENMW_COMPONENTS_NIF_RECORD_HPP

#include <string>
#include <vector>

namespace Nif
{

class NIFFile;
class NIFStream;

enum RecordType
{
  RC_MISSING = 0,
  RC_NiNode,
  RC_NiBillboardNode,
  RC_AvoidNode,
  RC_NiTriShape,
  RC_NiRotatingParticles,
  RC_NiAutoNormalParticles,
  RC_NiBSParticleNode,
  RC_NiCamera,
  RC_NiTexturingProperty,
  RC_NiFogProperty,
  RC_NiMaterialProperty,
  RC_NiZBufferProperty,
  RC_NiAlphaProperty,
  RC_NiVertexColorProperty,
  RC_NiShadeProperty,
  RC_NiDitherProperty,
  RC_NiWireframeProperty,
  RC_NiSpecularProperty,
  RC_NiStencilProperty,
  RC_NiVisController,
  RC_NiGeomMorpherController,
  RC_NiKeyframeController,
  RC_NiAlphaController,
  RC_NiUVController,
  RC_NiPathController,
  RC_NiMaterialColorController,
  RC_NiBSPArrayController,
  RC_NiParticleSystemController,
  RC_NiFlipController,
  RC_NiBSAnimationNode,
  RC_NiLight,
  RC_NiTextureEffect,
  RC_NiVertWeightsExtraData,
  RC_NiTextKeyExtraData,
  RC_NiStringExtraData,
  RC_NiGravity,
  RC_NiPlanarCollider,
  RC_NiParticleGrowFade,
  RC_NiParticleColorModifier,
  RC_NiParticleRotation,
  RC_NiFloatData,
  RC_NiTriShapeData,
  RC_NiVisData,
  RC_NiColorData,
  RC_NiPixelData,
  RC_NiMorphData,
  RC_NiKeyframeData,
  RC_NiSkinData,
  RC_NiUVData,
  RC_NiPosData,
  RC_NiRotatingParticlesData,
  RC_NiAutoNormalParticlesData,
  RC_NiSequenceStreamHelper,
  RC_NiSourceTexture,
  RC_NiSkinInstance,
  RC_RootCollisionNode,
  //
  RC_bhkBlendCollisionObject,
  RC_bhkBlendController,
  RC_bhkBoxShape,
  RC_bhkCapsuleShape,
  RC_bhkCollisionObject,
  RC_bhkConvexTransformShape,
  RC_bhkConvexVerticesShape,
  RC_bhkLimitedHingeConstraint,
  RC_bhkListShape,
  RC_bhkMalleableConstraint,
  RC_bhkMoppBvTreeShape,
  RC_bhkMultiSphereShape,
  RC_bhkNiTriStripsShape,
  RC_bhkPackedNiTriStripsShape,
  RC_bhkPrismaticConstraint,
  RC_bhkRagdollConstraint,
  RC_bhkRigidBody,
  RC_bhkRigidBodyT,
  RC_bhkSimpleShapePhantom,
  RC_bhkSPCollisionObject,
  RC_bhkSphereShape,
  RC_bhkStiffSpringConstraint,
  RC_bhkTransformShape,
  RC_BSBound,
  RC_BSFurnitureMarker,
  RC_BSParentVelocityModifier,
  RC_BSXFlags,
  RC_hkPackedNiTriStripsData,
  RC_NiBinaryExtraData,
  RC_NiBlendBoolInterpolator,
  RC_NiBlendFloatInterpolator,
  RC_NiBlendPoint3Interpolator,
  RC_NiBlendTransformInterpolator,
  RC_NiBoolData,
  RC_NiBoolInterpolator,
  RC_NiBoolTimelineInterpolator,
  RC_NiBSBoneLODController,
  RC_NiCollisionData,
  RC_NiCollisionObject,
  RC_NiControllerManager,
  RC_NiControllerSequence,
  RC_NiDefaultAVObjectPalette,
  RC_NiFloatInterpolator,
  RC_NiGeometry,
  RC_NiMultiTargetTransformController,
  RC_NiParticleSystem,
  RC_NiPathInterpolator,
  RC_NiPoint3Interpolator,
  RC_NiPSysAgeDeathModifier,
  RC_NiPSysBoundUpdateModifier,
  RC_NiPSysBoxEmitter,
  RC_NiPSysCollider,
  RC_NiPSysColliderManager,
  RC_NiPSysColorModifier,
  RC_NiPSysCylinderEmitter,
  RC_NiPSysData,
  RC_NiPSysDragModifier,
  RC_NiPSysEmitter,
  RC_NiPSysEmitterCtlr,
  RC_NiPSysEmitterInitialRadiusCtlr,
  RC_NiPSysEmitterLifeSpanCtlr,
  RC_NiPSysEmitterSpeedCtlr,
  RC_NiPSysGravityModifier,
  RC_NiPSysGravityStrengthCtlr,
  RC_NiPSysGrowFadeModifier,
  RC_NiPSysMeshEmitter,
  RC_NiPSysModifier,
  RC_NiPSysModifierActiveCtlr,
  RC_NiPSysPlanarCollider,
  RC_NiPSysPositionModifier,
  RC_NiPSysRotationModifier,
  RC_NiPSysSpawnModifier,
  RC_NiPSysSphereEmitter,
  RC_NiPSysUpdateCtlr,
  RC_NiSequence,
  RC_NiSkinPartition,
  RC_NiStringPalette,
  RC_NiTextureTransformController,
  RC_NiTransformController,
  RC_NiTransformData,
  RC_NiTransformInterpolator,
  RC_NiTriStrips,
  RC_NiTriStripsData,
  //
  RC_BSFadeNode,
  RC_bhkCompressedMeshShape,
  RC_bhkCompressedMeshShapeData,
  RC_BSLightingShaderProperty,
  RC_BSEffectShaderProperty,
  RC_BSShaderTextureSet,
  RC_BSFurnitureMarkerNode,
  RC_BSLODTriShape,
  RC_NiExtraData,
  RC_NiBooleanExtraData,
  RC_NiSwitchNode,
  RC_BSValueNode,
  RC_BSInvMarker,
  RC_BSBehaviorGraphExtraData,
  RC_BSPSysLODModifier,
  RC_BSPSysScaleModifier,
  RC_NiPSysSphericalCollider,
  RC_BSLightingShaderPropertyColorController,
  RC_BSLightingShaderPropertyFloatController,
  RC_BSPSysSimpleColorModifier,
  RC_BSOrderedNode,
  RC_BSEffectShaderPropertyColorController,
  RC_BSEffectShaderPropertyFloatController,
  RC_BSBlastNode,
  RC_BSPSysInheritVelocityModifier,
  RC_NiPSysBombModifier,
  RC_BSPSysSubTexModifier,
  RC_BSMultiBoundNode,
  RC_BSMultiBound,
  RC_BSWaterShaderProperty,
  RC_BSLeafAnimNode,
  RC_BSTreeNode,
  RC_BSMultiBoundOBB,
  RC_BSDecalPlacementVectorExtraData,
  RC_NiFloatExtraData,
  RC_BSLagBoneController
};
/// Base class for all records
struct Record
{
    // Record type and type name
    int recType;
    std::string recName;
    size_t recIndex;

    unsigned int nifVer;
    unsigned int userVer;
    unsigned int userVer2;
    std::vector<std::string> *strings; // strings defined in the header

    Record() : recType(RC_MISSING), recIndex(~(size_t)0), nifVer(0x04000002) {}

    /// Parses the record from file
    virtual void read(NIFStream *nif) = 0;

    /// Does post-processing, after the entire tree is loaded
    virtual void post(NIFFile *nif) {}

    virtual ~Record() {}

    /*
       Use these later if you want custom allocation of all NIF objects
    static void* operator new(size_t size);
    static void operator delete(void *p);
    */
};

} // Namespace
#endif
