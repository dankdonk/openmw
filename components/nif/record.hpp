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
  RC_BSXFlags,
  RC_hkPackedNiTriStripsData,
  RC_bhkPackedNiTriStripsShape,
  RC_bhkNiTriStripsShape,
  RC_bhkMoppBvTreeShape,
  RC_bhkRigidBody,
  RC_bhkRigidBodyT,
  RC_NiCollisionObject,
  RC_NiCollisionData,
  RC_bhkCollisionObject,
  RC_NiTriStrips,
  RC_NiBinaryExtraData,
  RC_NiTriStripsData,
  RC_bhkListShape,
  RC_bhkBoxShape,
  RC_bhkSphereShape,
  RC_bhkConvexTransformShape,
  RC_bhkConvexVerticesShape,
  RC_NiTransformController,
  RC_NiPathInterpolator,
  RC_NiTransformInterpolator,
  RC_NiTransformData,
  RC_NiGeometry,
  RC_NiParticleSystem,
  RC_NiPSysModifier,
  RC_NiControllerManager,
  RC_NiSequence,
  RC_NiControllerSequence,
  RC_NiMultiTargetTransformController,
  RC_NiStringPalette,
  RC_NiDefaultAVObjectPalette,
  RC_NiFloatInterpolator,
  RC_NiBlendFloatInterpolator,
  RC_NiPSysEmitterCtlr,
  RC_NiPSysUpdateCtlr,
  RC_NiBoolInterpolator,
  RC_NiBoolData,
  RC_NiPSysBoxEmitter,
  RC_NiPSysAgeDeathModifier,
  RC_NiPSysSpawnModifier,
  RC_NiPSysGrowFadeModifier,
  RC_NiPSysColorModifier,
  RC_NiPSysGravityModifier,
  RC_NiPSysPositionModifier,
  RC_NiPSysBoundUpdateModifier,
  RC_RootCollisionNode
};

/// Base class for all records
struct Record
{
    // Record type and type name
    int recType;
    std::string recName;
    size_t recIndex;
    unsigned int nifVer;

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
