#include "niffile.hpp"
#include "effect.hpp"
#include "collision.hpp"
#include "property.hpp"
#include "controller.hpp"
#include "controlled.hpp"
#include "extra.hpp"

#include <map>
#include <cassert>
#include <iostream> // FIXME
#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include <OgreResourceGroupManager.h>

namespace Nif
{

/// Open a NIF stream. The name is used for error messages.
NIFFile::NIFFile(const std::string &name)
    : mVer(0), mUserVer(0), mUserVer2(0)
    , filename(name)
{
    parse();
}

NIFFile::~NIFFile()
{
    for (std::vector<Record*>::iterator it = records.begin() ; it != records.end(); ++it)
    {
        delete *it;
    }
}

template <typename NodeType> static Record* construct() { return new NodeType; }

struct RecordFactoryEntry {

    typedef Record* (*create_t) ();

    create_t        mCreate;
    RecordType      mType;

};

///Helper function for adding records to the factory map
static std::pair<std::string,RecordFactoryEntry> makeEntry(std::string recName, Record* (*create_t) (), RecordType type)
{
    RecordFactoryEntry anEntry = {create_t,type};
    return std::make_pair(recName, anEntry);
}

///These are all the record types we know how to read.
static std::map<std::string,RecordFactoryEntry> makeFactory()
{
    std::map<std::string,RecordFactoryEntry> newFactory;
    newFactory.insert(makeEntry("NiNode",                     &construct <NiNode>                      , RC_NiNode                        ));
    newFactory.insert(makeEntry("AvoidNode",                  &construct <NiNode>                      , RC_AvoidNode                     ));
    newFactory.insert(makeEntry("NiBSParticleNode",           &construct <NiNode>                      , RC_NiBSParticleNode              ));
    newFactory.insert(makeEntry("NiBSAnimationNode",          &construct <NiNode>                      , RC_NiBSAnimationNode             ));
    newFactory.insert(makeEntry("NiBillboardNode",            &construct <NiBillboardNode>             , RC_NiBillboardNode               ));
    newFactory.insert(makeEntry("NiTriShape",                 &construct <NiTriShape>                  , RC_NiTriShape                    ));
    newFactory.insert(makeEntry("NiRotatingParticles",        &construct <NiRotatingParticles>         , RC_NiRotatingParticles           ));
    newFactory.insert(makeEntry("NiAutoNormalParticles",      &construct <NiAutoNormalParticles>       , RC_NiAutoNormalParticles         ));
    newFactory.insert(makeEntry("NiCamera",                   &construct <NiCamera>                    , RC_NiCamera                      ));
    newFactory.insert(makeEntry("RootCollisionNode",          &construct <NiNode>                      , RC_RootCollisionNode             ));
    newFactory.insert(makeEntry("NiTexturingProperty",        &construct <NiTexturingProperty>         , RC_NiTexturingProperty           ));
    newFactory.insert(makeEntry("NiFogProperty",              &construct <NiFogProperty>               , RC_NiFogProperty                 ));
    newFactory.insert(makeEntry("NiMaterialProperty",         &construct <NiMaterialProperty>          , RC_NiMaterialProperty            ));
    newFactory.insert(makeEntry("NiZBufferProperty",          &construct <NiZBufferProperty>           , RC_NiZBufferProperty             ));
    newFactory.insert(makeEntry("NiAlphaProperty",            &construct <NiAlphaProperty>             , RC_NiAlphaProperty               ));
    newFactory.insert(makeEntry("NiVertexColorProperty",      &construct <NiVertexColorProperty>       , RC_NiVertexColorProperty         ));
    newFactory.insert(makeEntry("NiShadeProperty",            &construct <NiShadeProperty>             , RC_NiShadeProperty               ));
    newFactory.insert(makeEntry("NiDitherProperty",           &construct <NiDitherProperty>            , RC_NiDitherProperty              ));
    newFactory.insert(makeEntry("NiWireframeProperty",        &construct <NiWireframeProperty>         , RC_NiWireframeProperty           ));
    newFactory.insert(makeEntry("NiSpecularProperty",         &construct <NiSpecularProperty>          , RC_NiSpecularProperty            ));
    newFactory.insert(makeEntry("NiStencilProperty",          &construct <NiStencilProperty>           , RC_NiStencilProperty             ));
    newFactory.insert(makeEntry("NiVisController",            &construct <NiVisController>             , RC_NiVisController               ));
    newFactory.insert(makeEntry("NiGeomMorpherController",    &construct <NiGeomMorpherController>     , RC_NiGeomMorpherController       ));
    newFactory.insert(makeEntry("NiKeyframeController",       &construct <NiKeyframeController>        , RC_NiKeyframeController          ));
    newFactory.insert(makeEntry("NiAlphaController",          &construct <NiAlphaController>           , RC_NiAlphaController             ));
    newFactory.insert(makeEntry("NiUVController",             &construct <NiUVController>              , RC_NiUVController                ));
    newFactory.insert(makeEntry("NiPathController",           &construct <NiPathController>            , RC_NiPathController              ));
    newFactory.insert(makeEntry("NiMaterialColorController",  &construct <NiMaterialColorController>   , RC_NiMaterialColorController     ));
    newFactory.insert(makeEntry("NiBSPArrayController",       &construct <NiBSPArrayController>        , RC_NiBSPArrayController          ));
    newFactory.insert(makeEntry("NiParticleSystemController", &construct <NiParticleSystemController>  , RC_NiParticleSystemController    ));
    newFactory.insert(makeEntry("NiFlipController",           &construct <NiFlipController>            , RC_NiFlipController              ));
    newFactory.insert(makeEntry("NiAmbientLight",             &construct <NiLight>                     , RC_NiLight                       ));
    newFactory.insert(makeEntry("NiDirectionalLight",         &construct <NiLight>                     , RC_NiLight                       ));
    newFactory.insert(makeEntry("NiTextureEffect",            &construct <NiTextureEffect>             , RC_NiTextureEffect               ));
    newFactory.insert(makeEntry("NiVertWeightsExtraData",     &construct <NiVertWeightsExtraData>      , RC_NiVertWeightsExtraData        ));
    newFactory.insert(makeEntry("NiTextKeyExtraData",         &construct <NiTextKeyExtraData>          , RC_NiTextKeyExtraData            ));
    newFactory.insert(makeEntry("NiStringExtraData",          &construct <NiStringExtraData>           , RC_NiStringExtraData             ));
    newFactory.insert(makeEntry("NiGravity",                  &construct <NiGravity>                   , RC_NiGravity                     ));
    newFactory.insert(makeEntry("NiPlanarCollider",           &construct <NiPlanarCollider>            , RC_NiPlanarCollider              ));
    newFactory.insert(makeEntry("NiParticleGrowFade",         &construct <NiParticleGrowFade>          , RC_NiParticleGrowFade            ));
    newFactory.insert(makeEntry("NiParticleColorModifier",    &construct <NiParticleColorModifier>     , RC_NiParticleColorModifier       ));
    newFactory.insert(makeEntry("NiParticleRotation",         &construct <NiParticleRotation>          , RC_NiParticleRotation            ));
    newFactory.insert(makeEntry("NiFloatData",                &construct <NiFloatData>                 , RC_NiFloatData                   ));
    newFactory.insert(makeEntry("NiTriShapeData",             &construct <NiTriShapeData>              , RC_NiTriShapeData                ));
    newFactory.insert(makeEntry("NiVisData",                  &construct <NiVisData>                   , RC_NiVisData                     ));
    newFactory.insert(makeEntry("NiColorData",                &construct <NiColorData>                 , RC_NiColorData                   ));
    newFactory.insert(makeEntry("NiPixelData",                &construct <NiPixelData>                 , RC_NiPixelData                   ));
    newFactory.insert(makeEntry("NiMorphData",                &construct <NiMorphData>                 , RC_NiMorphData                   ));
    newFactory.insert(makeEntry("NiKeyframeData",             &construct <NiKeyframeData>              , RC_NiKeyframeData                ));
    newFactory.insert(makeEntry("NiSkinData",                 &construct <NiSkinData>                  , RC_NiSkinData                    ));
    newFactory.insert(makeEntry("NiUVData",                   &construct <NiUVData>                    , RC_NiUVData                      ));
    newFactory.insert(makeEntry("NiPosData",                  &construct <NiPosData>                   , RC_NiPosData                     ));
    newFactory.insert(makeEntry("NiRotatingParticlesData",    &construct <NiRotatingParticlesData>     , RC_NiRotatingParticlesData       ));
    newFactory.insert(makeEntry("NiAutoNormalParticlesData",  &construct <NiAutoNormalParticlesData>   , RC_NiAutoNormalParticlesData     ));
    newFactory.insert(makeEntry("NiSequenceStreamHelper",     &construct <NiSequenceStreamHelper>      , RC_NiSequenceStreamHelper        ));
    newFactory.insert(makeEntry("NiSourceTexture",            &construct <NiSourceTexture>             , RC_NiSourceTexture               ));
    newFactory.insert(makeEntry("NiSkinInstance",             &construct <NiSkinInstance>              , RC_NiSkinInstance                ));
    //Seen in NIF ver 20.0.0.5
    newFactory.insert(makeEntry("BSXFlags",                   &construct <BSXFlags>                    , RC_BSXFlags                      ));
    newFactory.insert(makeEntry("hkPackedNiTriStripsData",    &construct <hkPackedNiTriStripsData>     , RC_hkPackedNiTriStripsData       ));
    newFactory.insert(makeEntry("bhkPackedNiTriStripsShape",  &construct <bhkPackedNiTriStripsShape>   , RC_bhkPackedNiTriStripsShape     ));
    newFactory.insert(makeEntry("bhkNiTriStripsShape",        &construct <bhkNiTriStripsShape>         , RC_bhkNiTriStripsShape           ));
    newFactory.insert(makeEntry("bhkMoppBvTreeShape",         &construct <bhkMoppBvTreeShape>          , RC_bhkMoppBvTreeShape            ));
    newFactory.insert(makeEntry("bhkRigidBody",               &construct <bhkRigidBody>                , RC_bhkRigidBody                  ));
    newFactory.insert(makeEntry("bhkRigidBodyT",              &construct <bhkRigidBodyT>               , RC_bhkRigidBodyT                 ));
    newFactory.insert(makeEntry("NiCollisionObject",          &construct <NiCollisionObject>           , RC_NiCollisionObject             ));
    newFactory.insert(makeEntry("bhkNiCollisionObject",       &construct <bhkNiCollisionObject>        , RC_bhkNiCollisionObject          ));
    newFactory.insert(makeEntry("bhkCollisionObject",         &construct <bhkCollisionObject>          , RC_bhkCollisionObject            ));
    newFactory.insert(makeEntry("bhkSPCollisionObject",       &construct <bhkSPCollisionObject>        , RC_bhkSPCollisionObject          ));
    newFactory.insert(makeEntry("NiTriStrips",                &construct <NiTriStrips>                 , RC_NiTriStrips                   ));
    newFactory.insert(makeEntry("NiBinaryExtraData",          &construct <NiBinaryExtraData>           , RC_NiBinaryExtraData             ));
    newFactory.insert(makeEntry("NiTriStripsData",            &construct <NiTriStripsData>             , RC_NiTriStripsData               ));
    newFactory.insert(makeEntry("bhkListShape",               &construct <bhkListShape>                , RC_bhkListShape                  ));
    newFactory.insert(makeEntry("bhkBoxShape",                &construct <bhkBoxShape>                 , RC_bhkBoxShape                   ));
    newFactory.insert(makeEntry("bhkSphereShape",             &construct <bhkSphereShape>              , RC_bhkSphereShape                ));
    newFactory.insert(makeEntry("bhkConvexTransformShape",    &construct <bhkConvexTransformShape>     , RC_bhkConvexTransformShape       ));
    newFactory.insert(makeEntry("bhkConvexVerticesShape",     &construct <bhkConvexVerticesShape>      , RC_bhkConvexVerticesShape        ));
    newFactory.insert(makeEntry("NiTransformController",      &construct <NiTransformController>       , RC_NiTransformController         ));
    newFactory.insert(makeEntry("NiPathInterpolator",         &construct <NiPathInterpolator>          , RC_NiPathInterpolator            ));
    newFactory.insert(makeEntry("NiTransformInterpolator",    &construct <NiTransformInterpolator>     , RC_NiTransformInterpolator       ));
    newFactory.insert(makeEntry("NiTransformData",            &construct <NiTransformData>             , RC_NiTransformData               ));
    newFactory.insert(makeEntry("NiGeometry",                 &construct <NiGeometry>                  , RC_NiGeometry                    ));
    newFactory.insert(makeEntry("NiParticleSystem",           &construct <NiParticleSystem>            , RC_NiParticleSystem              ));
    newFactory.insert(makeEntry("NiPSysModifier",             &construct <NiPSysModifier>              , RC_NiPSysModifier                ));
    newFactory.insert(makeEntry("NiControllerManager",        &construct <NiControllerManager>         , RC_NiControllerManager           ));
    newFactory.insert(makeEntry("NiSequence",                 &construct <NiSequence>                  , RC_NiSequence                    ));
    newFactory.insert(makeEntry("NiControllerSequence",       &construct <NiControllerSequence>        , RC_NiControllerSequence          ));
    newFactory.insert(makeEntry("NiStringPalette",            &construct <NiStringPalette>             , RC_NiStringPalette               ));
    newFactory.insert(makeEntry("NiDefaultAVObjectPalette",   &construct <NiDefaultAVObjectPalette>    , RC_NiDefaultAVObjectPalette      ));
    newFactory.insert(makeEntry("NiFloatInterpolator",        &construct <NiFloatInterpolator>         , RC_NiFloatInterpolator           ));
    newFactory.insert(makeEntry("NiBlendFloatInterpolator",   &construct <NiBlendFloatInterpolator>    , RC_NiBlendFloatInterpolator      ));
    newFactory.insert(makeEntry("NiPSysEmitterCtlr",          &construct <NiPSysEmitterCtlr>           , RC_NiPSysEmitterCtlr             ));
    newFactory.insert(makeEntry("NiPSysUpdateCtlr",           &construct <NiPSysUpdateCtlr>            , RC_NiPSysUpdateCtlr              ));
    newFactory.insert(makeEntry("NiBoolInterpolator",         &construct <NiBoolInterpolator>          , RC_NiBoolInterpolator            ));
    newFactory.insert(makeEntry("NiBoolData",                 &construct <NiBoolData>                  , RC_NiBoolData                    ));
    newFactory.insert(makeEntry("NiPSysBoxEmitter",           &construct <NiPSysBoxEmitter>            , RC_NiPSysBoxEmitter              ));
    newFactory.insert(makeEntry("NiPSysAgeDeathModifier",     &construct <NiPSysAgeDeathModifier>      , RC_NiPSysAgeDeathModifier        ));
    newFactory.insert(makeEntry("NiPSysSpawnModifier",        &construct <NiPSysSpawnModifier>         , RC_NiPSysSpawnModifier           ));
    newFactory.insert(makeEntry("NiPSysGrowFadeModifier",     &construct <NiPSysGrowFadeModifier>      , RC_NiPSysGrowFadeModifier        ));
    newFactory.insert(makeEntry("NiPSysColorModifier",        &construct <NiPSysColorModifier>         , RC_NiPSysColorModifier           ));
    newFactory.insert(makeEntry("NiPSysGravityModifier",      &construct <NiPSysGravityModifier>       , RC_NiPSysGravityModifier         ));
    newFactory.insert(makeEntry("NiPSysPositionModifier",     &construct <NiPSysPositionModifier>      , RC_NiPSysPositionModifier        ));
    newFactory.insert(makeEntry("NiPSysBoundUpdateModifier",  &construct <NiPSysBoundUpdateModifier>   , RC_NiPSysBoundUpdateModifier     ));
    newFactory.insert(makeEntry("NiPSysData",                 &construct <NiPSysData>                  , RC_NiPSysData                    ));
    newFactory.insert(makeEntry("NiPoint3Interpolator",       &construct <NiPoint3Interpolator>        , RC_NiPoint3Interpolator          ));
    newFactory.insert(makeEntry("NiBlendPoint3Interpolator",  &construct <NiBlendPoint3Interpolator>   , RC_NiBlendPoint3Interpolator     ));
    newFactory.insert(makeEntry("NiSkinPartition",            &construct <NiSkinPartition>             , RC_NiSkinPartition               ));
    newFactory.insert(makeEntry("bhkCapsuleShape",            &construct <bhkCapsuleShape>             , RC_bhkCapsuleShape               ));
    newFactory.insert(makeEntry("bhkSimpleShapePhantom",      &construct <bhkSimpleShapePhantom>       , RC_bhkSimpleShapePhantom         ));
    newFactory.insert(makeEntry("NiPSysRotationModifier",     &construct <NiPSysRotationModifier>      , RC_NiPSysRotationModifier        ));
    newFactory.insert(makeEntry("NiPSysCylinderEmitter",      &construct <NiPSysCylinderEmitter>       , RC_NiPSysCylinderEmitter         ));
    newFactory.insert(makeEntry("bhkLimitedHingeConstraint",  &construct <bhkLimitedHingeConstraint>   , RC_bhkLimitedHingeConstraint     ));
    newFactory.insert(makeEntry("NiBlendBoolInterpolator",    &construct <NiBlendBoolInterpolator>     , RC_NiBlendBoolInterpolator       ));
    newFactory.insert(makeEntry("NiPSysModifierActiveCtlr",   &construct <NiPSysModifierActiveCtlr>    , RC_NiPSysModifierActiveCtlr      ));
    newFactory.insert(makeEntry("NiPSysDragModifier",         &construct <NiPSysDragModifier>          , RC_NiPSysDragModifier            ));
    newFactory.insert(makeEntry("NiPSysGravityStrengthCtlr",  &construct <NiPSysGravityStrengthCtlr>   , RC_NiPSysGravityStrengthCtlr     ));
    newFactory.insert(makeEntry("NiBoolTimelineInterpolator", &construct <NiBoolTimelineInterpolator>  , RC_NiBoolTimelineInterpolator    ));
    newFactory.insert(makeEntry("BSParentVelocityModifier",   &construct <BSParentVelocityModifier>    , RC_BSParentVelocityModifier      ));
    newFactory.insert(makeEntry("NiPSysDragModifier",         &construct <NiPSysDragModifier>          , RC_NiPSysDragModifier            ));
    newFactory.insert(makeEntry("NiPSysEmitter",              &construct <NiPSysEmitter>               , RC_NiPSysEmitter                 ));
    newFactory.insert(makeEntry("NiPSysMeshEmitter",          &construct <NiPSysMeshEmitter>           , RC_NiPSysMeshEmitter             ));
    newFactory.insert(makeEntry("NiPSysEmitterSpeedCtlr",     &construct <NiPSysEmitterSpeedCtlr>      , RC_NiPSysEmitterSpeedCtlr        ));
    newFactory.insert(makeEntry("bhkRagdollConstraint",       &construct <bhkRagdollConstraint>        , RC_bhkRagdollConstraint          ));
    newFactory.insert(makeEntry("NiTextureTransformController", &construct <NiTextureTransformController> , RC_NiTextureTransformController));
    newFactory.insert(makeEntry("bhkTransformShape",          &construct <bhkTransformShape>           , RC_bhkTransformShape             ));
    newFactory.insert(makeEntry("BSFurnitureMarker",          &construct <BSFurnitureMarker>           , RC_BSFurnitureMarker             ));
    newFactory.insert(makeEntry("bhkPrismaticConstraint",     &construct <bhkPrismaticConstraint>      , RC_bhkPrismaticConstraint        ));
    newFactory.insert(makeEntry("NiPSysSphereEmitter",        &construct <NiPSysSphereEmitter>         , RC_NiPSysSphereEmitter           ));
    newFactory.insert(makeEntry("NiPSysEmitterLifeSpanCtlr",  &construct <NiPSysEmitterLifeSpanCtlr>   , RC_NiPSysEmitterLifeSpanCtlr     ));
    newFactory.insert(makeEntry("BSBound",                    &construct <BSBound>                     , RC_BSBound                       ));
    newFactory.insert(makeEntry("bhkStiffSpringConstraint",   &construct <bhkStiffSpringConstraint>    , RC_bhkStiffSpringConstraint      ));
    newFactory.insert(makeEntry("NiBSBoneLODController",      &construct <NiBSBoneLODController>       , RC_NiBSBoneLODController         ));
    newFactory.insert(makeEntry("bhkBlendController",         &construct <bhkBlendController>          , RC_bhkBlendController            ));
    newFactory.insert(makeEntry("bhkBlendCollisionObject",    &construct <bhkBlendCollisionObject>     , RC_bhkBlendCollisionObject       ));
    newFactory.insert(makeEntry("bhkMalleableConstraint",     &construct <bhkMalleableConstraint>      , RC_bhkMalleableConstraint        ));
    newFactory.insert(makeEntry("NiPSysColliderManager",      &construct <NiPSysColliderManager>       , RC_NiPSysColliderManager         ));
    newFactory.insert(makeEntry("NiPSysPlanarCollider",       &construct <NiPSysPlanarCollider>        , RC_NiPSysPlanarCollider          ));
    newFactory.insert(makeEntry("NiPSysCollider",             &construct <NiPSysCollider>              , RC_NiPSysCollider                ));
    newFactory.insert(makeEntry("bhkMultiSphereShape",        &construct <bhkMultiSphereShape>         , RC_bhkMultiSphereShape           ));
    newFactory.insert(makeEntry("NiPSysEmitterInitialRadiusCtlr", &construct <NiPSysEmitterInitialRadiusCtlr>, RC_NiPSysEmitterInitialRadiusCtlr));
    newFactory.insert(makeEntry("NiBlendTransformInterpolator", &construct <NiBlendTransformInterpolator>, RC_NiBlendTransformInterpolator));
    newFactory.insert(makeEntry("NiMultiTargetTransformController", &construct <NiMultiTargetTransformController>, RC_NiMultiTargetTransformController));
    newFactory.insert(makeEntry("BSWindModifier",             &construct <BSWindModifier>              , RC_BSWindModifier                ));
    newFactory.insert(makeEntry("NiInterpolator",             &construct <NiInterpolator>              , RC_NiInterpolator                ));
    //
    newFactory.insert(makeEntry("BSFadeNode",                 &construct <BSFadeNode>                  , RC_BSFadeNode                    ));
    newFactory.insert(makeEntry("bhkCompressedMeshShape",     &construct <bhkCompressedMeshShape>      , RC_bhkCompressedMeshShape        ));
    newFactory.insert(makeEntry("bhkCompressedMeshShapeData", &construct <bhkCompressedMeshShapeData>  , RC_bhkCompressedMeshShapeData    ));
    newFactory.insert(makeEntry("BSLightingShaderProperty",   &construct <BSLightingShaderProperty>    , RC_BSLightingShaderProperty      ));
    newFactory.insert(makeEntry("BSEffectShaderProperty",     &construct <BSEffectShaderProperty>      , RC_BSEffectShaderProperty        ));
    newFactory.insert(makeEntry("BSShaderTextureSet",         &construct <BSShaderTextureSet>          , RC_BSShaderTextureSet            ));
    newFactory.insert(makeEntry("BSFurnitureMarkerNode",      &construct <BSFurnitureMarkerNode>       , RC_BSFurnitureMarkerNode         ));
    newFactory.insert(makeEntry("BSLODTriShape",              &construct <BSLODTriShape>               , RC_BSLODTriShape                 ));
    newFactory.insert(makeEntry("NiExtraData",                &construct <NiExtraData>                 , RC_NiExtraData                   ));
    newFactory.insert(makeEntry("NiBooleanExtraData",         &construct <NiBooleanExtraData>          , RC_NiBooleanExtraData            ));
    newFactory.insert(makeEntry("NiSwitchNode",               &construct <NiSwitchNode>                , RC_NiSwitchNode                  ));
    newFactory.insert(makeEntry("BSValueNode",                &construct <BSValueNode>                 , RC_BSValueNode                   ));
    newFactory.insert(makeEntry("BSInvMarker",                &construct <BSInvMarker>                 , RC_BSInvMarker                   ));
    newFactory.insert(makeEntry("BSBehaviorGraphExtraData",   &construct <BSBehaviorGraphExtraData>    , RC_BSBehaviorGraphExtraData      ));
    newFactory.insert(makeEntry("BSPSysLODModifier",          &construct <BSPSysLODModifier>           , RC_BSPSysLODModifier             ));
    newFactory.insert(makeEntry("BSPSysScaleModifier",        &construct <BSPSysScaleModifier>         , RC_BSPSysScaleModifier           ));
    newFactory.insert(makeEntry("NiPSysSphericalCollider",    &construct <NiPSysSphericalCollider>     , RC_NiPSysSphericalCollider       ));
    newFactory.insert(makeEntry("BSLightingShaderPropertyColorController", &construct <BSLightingShaderPropertyColorController>, RC_BSLightingShaderPropertyColorController));
    newFactory.insert(makeEntry("BSLightingShaderPropertyFloatController", &construct <BSLightingShaderPropertyFloatController>, RC_BSLightingShaderPropertyFloatController));
    newFactory.insert(makeEntry("BSPSysSimpleColorModifier",  &construct <BSPSysSimpleColorModifier>   , RC_BSPSysSimpleColorModifier     ));
    newFactory.insert(makeEntry("BSOrderedNode",              &construct <BSOrderedNode>               , RC_BSOrderedNode                 ));
    newFactory.insert(makeEntry("BSEffectShaderPropertyColorController", &construct <BSLightingShaderPropertyColorController>, RC_BSLightingShaderPropertyColorController));
    newFactory.insert(makeEntry("BSEffectShaderPropertyFloatController", &construct <BSLightingShaderPropertyFloatController>, RC_BSLightingShaderPropertyFloatController));
    newFactory.insert(makeEntry("BSBlastNode",                &construct <BSBlastNode>                 , RC_BSBlastNode                   ));
    newFactory.insert(makeEntry("BSPSysInheritVelocityModifier", &construct <BSPSysInheritVelocityModifier>, RC_BSPSysInheritVelocityModifier));
    newFactory.insert(makeEntry("NiPSysBombModifier",         &construct <NiPSysBombModifier>          , RC_NiPSysBombModifier            ));
    newFactory.insert(makeEntry("BSPSysSubTexModifier",       &construct <BSPSysSubTexModifier>        , RC_BSPSysSubTexModifier          ));
    newFactory.insert(makeEntry("BSMultiBoundNode",           &construct <BSMultiBoundNode>            , RC_BSMultiBoundNode              ));
    newFactory.insert(makeEntry("BSMultiBound",               &construct <BSMultiBound>                , RC_BSMultiBound                  ));
    newFactory.insert(makeEntry("BSWaterShaderProperty",      &construct <BSWaterShaderProperty>       , RC_BSWaterShaderProperty         ));
    newFactory.insert(makeEntry("BSLeafAnimNode",             &construct <BSLeafAnimNode>              , RC_BSLeafAnimNode                ));
    newFactory.insert(makeEntry("BSTreeNode",                 &construct <BSTreeNode>                  , RC_BSTreeNode                    ));
    newFactory.insert(makeEntry("BSMultiBoundData",           &construct <BSMultiBoundData>            , RC_BSMultiBoundData              ));
    newFactory.insert(makeEntry("BSMultiBoundOBB",            &construct <BSMultiBoundOBB>             , RC_BSMultiBoundOBB               ));
    newFactory.insert(makeEntry("BSDecalPlacementVectorExtraData", &construct <BSDecalPlacementVectorExtraData>, RC_BSDecalPlacementVectorExtraData));
    newFactory.insert(makeEntry("NiFloatExtraData",           &construct <NiFloatExtraData>            , RC_NiFloatExtraData              ));
    newFactory.insert(makeEntry("BSLagBoneController",        &construct <BSLagBoneController>         , RC_BSLagBoneController           ));
    newFactory.insert(makeEntry("BSStripParticleSystem",      &construct <BSStripParticleSystem>       , RC_BSStripParticleSystem         ));
    newFactory.insert(makeEntry("NiFloatExtraDataController", &construct <NiFloatExtraDataController>  , RC_NiFloatExtraDataController    ));
    newFactory.insert(makeEntry("bhkBreakableConstraint",     &construct <bhkBreakableConstraint>      , RC_bhkBreakableConstraint        ));
    newFactory.insert(makeEntry("Property",                   &construct <Property>                    , RC_Property                      ));
    newFactory.insert(makeEntry("bhkBallSocketConstraintChain", &construct <bhkBallSocketConstraintChain>, RC_bhkBallSocketConstraintChain));
    newFactory.insert(makeEntry("BSDamageStage",              &construct <BSDamageStage>               , RC_BSDamageStage                 ));
    newFactory.insert(makeEntry("BSNiAlphaPropertyTestRefController", &construct <NiAlphaController>   , RC_BSNiAlphaPropertyTestRefController));
    newFactory.insert(makeEntry("NiLookAtInterpolator",       &construct <NiLookAtInterpolator>        , RC_NiLookAtInterpolator          ));
    newFactory.insert(makeEntry("NiIntegerExtraData",         &construct <NiIntegerExtraData>          , RC_NiIntegerExtraData            ));
    newFactory.insert(makeEntry("BSFrustumFOVController",     &construct <BSFrustumFOVController>      , RC_BSFrustumFOVController        ));
    newFactory.insert(makeEntry("bhkHingeConstraint",         &construct <bhkHingeConstraint>          , RC_bhkHingeConstraint            ));
    newFactory.insert(makeEntry("BSStripPSysData",            &construct <BSStripPSysData>             , RC_BSStripPSysData               ));
    newFactory.insert(makeEntry("BSPSysStripUpdateModifier",  &construct <BSPSysStripUpdateModifier>   , RC_BSPSysStripUpdateModifier     ));
    newFactory.insert(makeEntry("NiBSplineInterpolator",      &construct <NiBSplineInterpolator>       , RC_NiBSplineInterpolator         ));
    newFactory.insert(makeEntry("NiBSplineFloatInterpolator", &construct <NiBSplineFloatInterpolator>  , RC_NiBSplineFloatInterpolator    ));
    newFactory.insert(makeEntry("NiBSplinePoint3Interpolator", &construct <NiBSplinePoint3Interpolator> , RC_NiBSplinePoint3Interpolator  ));
    newFactory.insert(makeEntry("NiBSplineTransformInterpolator", &construct <NiBSplineTransformInterpolator> , RC_NiBSplineTransformInterpolator));
    newFactory.insert(makeEntry("NiBSplineCompTransformInterpolator", &construct <NiBSplineCompTransformInterpolator> , RC_NiBSplineCompTransformInterpolator));
    newFactory.insert(makeEntry("NiBSplineData",              &construct <NiBSplineData>               , RC_NiBSplineData                 ));
    newFactory.insert(makeEntry("NiBSplineBasisData",         &construct <NiBSplineBasisData>          , RC_NiBSplineBasisData            ));
    return newFactory;
}

///Make the factory map used for parsing the file
static const std::map<std::string,RecordFactoryEntry> factories = makeFactory();

///Good nif file headers
static std::vector<std::string> makeGoodHeaders()
{
    std::vector<std::string> header;
    header.push_back("NetImmerse File Format");
    header.push_back("Gamebryo File Format");
    return header;
}
static const std::vector<std::string> goodHeaders = makeGoodHeaders();

///Good nif file versions
static std::set<unsigned int> makeGoodVersions()
{
    std::set<unsigned int> versions;
    versions.insert(0x04000002); // Morrowind NIF version
    versions.insert(0x14000004);
    versions.insert(0x14000005); // Oblivion
    versions.insert(0x14020007); // Skyrim
    return versions;
}
static const std::set<unsigned int> GoodVersions = makeGoodVersions();

/// Get the file's version in a human readable form
std::string NIFFile::printVersion(unsigned int version)
{
    union ver_quad
    {
        uint32_t full;
        uint8_t quad[4];
    } version_out;

    version_out.full = version;

    return Ogre::StringConverter::toString(version_out.quad[3])
    +"." + Ogre::StringConverter::toString(version_out.quad[2])
    +"." + Ogre::StringConverter::toString(version_out.quad[1])
    +"." + Ogre::StringConverter::toString(version_out.quad[0]);
}

size_t NIFFile::parseHeader(NIFStream nif,
        std::vector<std::string>& blockTypes, std::vector<unsigned short>& blockTypeIndex,
        std::vector<unsigned int>& blockSize, std::vector<std::string>& strings)
{
    //The number of records/blocks to get from the file
    size_t numBlocks = 0;

    // Check the header string
    std::string head = nif.getVersionString();
    bool isGood = false;
    for (std::vector<std::string>::const_iterator it = goodHeaders.begin() ; it != goodHeaders.end(); it++)
    {
        if (head.find(*it) != std::string::npos)
        isGood = true;
    }
    if (!isGood)
		fail("Invalid NIF header:  " + head);

    // Get BCD version
    mVer = nif.getUInt();
    if (GoodVersions.find(mVer) == GoodVersions.end())
        fail("Unsupported NIF version: " + printVersion(mVer));

    if (mVer >= 0x14000004) // 20.0.0.4
    {
        bool isLittleEndian = !!nif.getChar();
        if (!isLittleEndian)
            fail("Is not Little Endian");
    }

    if (mVer >= 0x0a010000) // 10.1.0.0
        mUserVer = nif.getUInt();

    numBlocks = nif.getInt();

    if (mVer >= 0x0a010000) // 10.1.0.0
    {
        mUserVer2 = nif.getUInt();

        //\FIXME This only works if Export Info is empty
        nif.getShortString(mVer); //string creator
        nif.getShortString(mVer); //string exportInfo1
        nif.getShortString(mVer); //string exportInfo1

        unsigned short numBlockTypes = nif.getUShort();
        for (unsigned int i = 0; i < numBlockTypes; ++i)
            blockTypes.push_back(nif.getString());

        blockTypeIndex.resize(numBlocks);
        for (unsigned int i = 0; i < numBlocks; ++i)
            blockTypeIndex[i] = nif.getUShort();
    }

    if (mVer >= 0x14020007) // 20.2.0.7
    {
        blockSize.resize(numBlocks);
        for (unsigned int i = 0; i < numBlocks; ++i)
            blockSize[i] = nif.getUInt();
    }

    if (mVer >= 0x14010003) // 20.1.0.3
    {
        unsigned int numStrings = nif.getUInt();
        unsigned int maxStringLength = nif.getUInt();
        unsigned int size = 0;
        strings.resize(numStrings);
        for (unsigned int i = 0; i < numStrings; ++i)
        {
            size = nif.getUInt();
            strings[i] = nif.getString(size);
        }
    }

    if (mVer >= 0x0a010000) // 10.1.0.0
        nif.getUInt(); //unsigned int unknown

    return numBlocks;
}

void NIFFile::parse()
{
    NIFStream nif (this, Ogre::ResourceGroupManager::getSingleton().openResource(filename));

    std::vector<std::string> blockTypes;
    std::vector<unsigned short> blockTypeIndex;
    std::vector<unsigned int> blockSize;
    std::vector<std::string> strings;

    // Parse the header, and get the Number of records
    size_t recNum = parseHeader(nif, blockTypes, blockTypeIndex, blockSize, strings);

    records.resize(recNum);
    for(size_t i = 0; i < recNum; ++i)
    {
        Record *r = NULL;

        std::string rec;
        if (mVer >= 0x0a010000) // 10.1.0.0
            rec = blockTypes[blockTypeIndex[i]];
        else                   // 4.0.0.2 (for example)
            rec = nif.getString();

        if (rec.empty())
            fail("Record number " + Ogre::StringConverter::toString(i)
                     + " out of " + Ogre::StringConverter::toString(recNum) + " is blank.");

        std::map<std::string,RecordFactoryEntry>::const_iterator entry = factories.find(rec);

        if (entry != factories.end())
        {
            r = entry->second.mCreate ();
            r->recType = entry->second.mType;
        }
        else
            fail("Unknown record type " + rec);

        assert(r != NULL);
        assert(r->recType != RC_MISSING);
        r->recName = rec;
        r->recIndex = i;
        r->nifVer = mVer;
        r->userVer = mUserVer;
        r->userVer2 = mUserVer2;
        r->strings = &strings;
        records[i] = r;
        //std::cout << "Start of " << rec << ", block " << i << ": " << nif.tell() << std::endl; // FIXME
        //assert(nif.tell() < nif.size() && "Nif: EOF but record not read ");
        if (nif.tell() >= nif.size())
            fail ("Nif:: EOF but record not read");

        r->read(&nif);
    }

	//NiFooter
    assert(nif.tell() < nif.size() && "Nif: EOF but footer not read ");
    //if (nif.tell() >= nif.size())
        //std::cerr << "Nif: " << filename << " EOF but footer not read " << std::endl;
    size_t rootNum = nif.getUInt();
    roots.resize(rootNum);

    //Determine which records are roots
    for(size_t i = 0; i < rootNum; i++)
    {
        int idx = nif.getInt();
        if (idx >= 0)
        {
            roots[i] = records.at(idx);
        }
        else
        {
            roots[i] = NULL;
            warn("Null Root found");
        }
    }
    assert(nif.tell() == nif.size() && "Nif: still has data after reading footer");
    //if (nif.tell() != nif.size())
        //std::cerr << "Nif: " << filename << " still has data after reading footer" << std::endl;

    // Once parsing is done, do post-processing.
    for(size_t i = 0; i < recNum; i++)
        records[i]->post(this);
}

}
