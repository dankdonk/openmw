#include "foreign.hpp"

#include "data.hpp"
#include "node.hpp" // NiParticleSystem
#include "controlled.hpp" // NiSourceTexture
#include "extra.hpp"

void Nif::NiPSysModifier::read(NIFStream *nif)
{
    name = nif->getSkyrimString(nifVer, Record::strings);
    order = nif->getUInt();
    target.read(nif);
    active = nif->getBool(nifVer);
}

void Nif::NiPSysModifier::post(NIFFile *nif)
{
    target.post(nif);
}

void Nif::BSWindModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    strength = nif->getFloat();
}

void Nif::BSPSysSubTexModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    startFrame = nif->getUInt();
    startFrameFudge = nif->getFloat();
    endFrame = nif->getFloat();
    loopStartFrame = nif->getFloat();
    loopStartFrameFudge = nif->getFloat();
    frameCount = nif->getFloat();
    frameCountFudge = nif->getFloat();
}

void Nif::NiPSysBombModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    bombObject.read(nif);
    bombAxis = nif->getVector3();
    decay = nif->getFloat();
    deltaV = nif->getFloat();
    decayType = nif->getUInt();
    symmetryType = nif->getUInt();
}

void Nif::BSPSysInheritVelocityModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    unknownI1 = nif->getUInt();
    unknownF1 = nif->getFloat();
    unknownF2 = nif->getFloat();
    unknownF3 = nif->getFloat();
}

void Nif::BSPSysLODModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    unknown1 = nif->getFloat();
    unknown2 = nif->getFloat();
    unknown3 = nif->getFloat();
    unknown4 = nif->getFloat();
}

void Nif::BSPSysScaleModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    unsigned int numFloats = nif->getUInt();
    nif->getFloats(floats, numFloats);
}

void Nif::BSPSysSimpleColorModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    fadeInPercent = nif->getFloat();
    fadeOutPercent = nif->getFloat();
    color1EndPerCent = nif->getFloat();
    color1StartPerCent = nif->getFloat();
    color2EndPerCent = nif->getFloat();
    color2StartPerCent = nif->getFloat();

    nif->getVector4s(colors, 3);
}

void Nif::BSPSysStripUpdateModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    updateDeltaTime = nif->getFloat();
}

void Nif::NiPSysEmitter::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    speed = nif->getFloat();
    speedVar = nif->getFloat();
    declination = nif->getFloat();
    declinationVar = nif->getFloat();
    planarAngle = nif->getFloat();
    planarAngleVar = nif->getFloat();
    initialColor = nif->getVector4();
    initialRadius = nif->getFloat();
    radiusVar = nif->getFloat();
    lifeSpan = nif->getFloat();
    lifeSpanVar = nif->getFloat();
}

void Nif::NiPSysBoxEmitter::read(NIFStream *nif)
{
    NiPSysEmitter::read(nif);

    emitteObj.read(nif);
    width = nif->getFloat();
    height = nif->getFloat();
    depth = nif->getFloat();
}

void Nif::NiPSysCylinderEmitter::read(NIFStream *nif)
{
    NiPSysEmitter::read(nif);

    emitteObj.read(nif);
    radius = nif->getFloat();
    height = nif->getFloat();
}

void Nif::NiPSysMeshEmitter::read(NIFStream *nif)
{
    NiPSysEmitter::read(nif);

    unsigned int numMeshes = nif->getUInt();
    emitterMeshes.resize(numMeshes);
    for (unsigned int i = 0; i < numMeshes; ++i)
        emitterMeshes[i].read(nif);

    velocityType = nif->getUInt();
    emissionType = nif->getUInt();
    emissionAxis = nif->getVector3();
}

void Nif::NiPSysMeshEmitter::post(NIFFile *nif)
{
    for (unsigned int i = 0; i < emitterMeshes.size(); ++i)
        emitterMeshes[i].post(nif);
}

void Nif::NiPSysSphereEmitter::read(NIFStream *nif)
{
    NiPSysEmitter::read(nif);

    emitteObj.read(nif);
    radius = nif->getFloat();
}

void Nif::NiPSysAgeDeathModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    spawnOnDeath = nif->getBool(nifVer);
    spawnModifier.read(nif);
}

void Nif::NiPSysAgeDeathModifier::post(NIFFile *nif)
{
    spawnModifier.post(nif);
}

void Nif::NiPSysDragModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    parent.read(nif);
    dragAxis = nif->getVector3();
    percent = nif->getFloat();
    range = nif->getFloat();
    rangeFalloff = nif->getFloat();
}

void Nif::NiPSysSpawnModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    numSpawnGen = nif->getUShort();
    percentSpawn = nif->getFloat();
    minSpawn = nif->getUShort();
    maxSpawn = nif->getUShort();
    spawnSpeedChaos = nif->getFloat();
    spawnDirChaos = nif->getFloat();
    lifeSpan = nif->getFloat();
    lifeSpanVar = nif->getFloat();
}

void Nif::NiPSysGrowFadeModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    growTime = nif->getFloat();
    growGen = nif->getUShort();
    fadeTime = nif->getFloat();
    fadeGen = nif->getUShort();
    if (nifVer >= 0x14020007) // from 20.2.0.7
        baseScale = nif->getFloat();
}

void Nif::NiPSysColorModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    colorData.read(nif);
}

void Nif::NiPSysColorModifier::post(NIFFile *nif)
{
    colorData.post(nif);
}

void Nif::NiPSysGravityModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    gravityObj.read(nif);
    gravityAxis = nif->getVector3();
    decay = nif->getFloat();
    strength = nif->getFloat();
    forceType = nif->getUInt();
    turbulence = nif->getFloat();
    turbulenceScale = nif->getFloat();
    if (nifVer >= 0x14020007) // from 20.2.0.7
        nif->getChar();
}

void Nif::NiPSysBoundUpdateModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    nif->getUShort();
}

void Nif::NiPSysRotationModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    initialRotSpeed = nif->getFloat();

    if (nifVer >= 0x14000004) // from 20.0.0.4
    {
        initialRotSpeedVar = nif->getFloat();
        initialRotAngle = nif->getFloat();
        initialRotAngleVar = nif->getFloat();
        randomRotSpeedSign = nif->getBool(nifVer);
    }
    nif->getBool(nifVer);
    nif->getVector3();
}

void Nif::BSParentVelocityModifier::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

    damping = nif->getFloat();
}

void Nif::NiPSysColliderManager::read(NIFStream *nif)
{
    NiPSysModifier::read(nif);

   collider.read(nif);
}

void Nif::NiPSysColliderManager::post(NIFFile *nif)
{
    collider.post(nif);
}

void Nif::NiPSysCollider::read(NIFStream *nif)
{
    bounce = nif->getFloat();
    spawnOnCollide = nif->getBool(nifVer);
    dieOnCollide = nif->getBool(nifVer);
    nif->getInt(); // spawnModifier.read(nif);
    parent.read(nif);
    nextCollider.read(nif);
    colliderObj.read(nif);
}

void Nif::NiPSysCollider::post(NIFFile *nif)
{
    //spawnModifier.post(nif);
}

void Nif::NiPSysPlanarCollider::read(NIFStream *nif)
{
    NiPSysCollider::read(nif);

    width = nif->getFloat();
    height = nif->getFloat();
    xAxis = nif->getVector3();
    yAxis = nif->getVector3();
}

void Nif::NiPSysSphericalCollider::read(NIFStream *nif)
{
    NiPSysCollider::read(nif);

    radius = nif->getFloat();
}

void Nif::BSLightingShaderPropertyColorController::read(NIFStream *nif)
{
    Controller::read(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);

    targetVariable = nif->getUInt();
}

void Nif::BSLightingShaderPropertyColorController::post(NIFFile *nif)
{
    Controller::post(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
}

void Nif::BSLightingShaderPropertyFloatController::read(NIFStream *nif)
{
    Controller::read(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);

    targetVariable = nif->getUInt();
}

void Nif::BSLightingShaderPropertyFloatController::post(NIFFile *nif)
{
    Controller::post(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
}

void Nif::BSEffectShaderPropertyColorController::read(NIFStream *nif)
{
    Controller::read(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);

    targetVariable = nif->getUInt();
}

void Nif::BSEffectShaderPropertyColorController::post(NIFFile *nif)
{
    Controller::post(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
}

void Nif::BSEffectShaderPropertyFloatController::read(NIFStream *nif)
{
    Controller::read(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);

    targetVariable = nif->getUInt();
}

void Nif::BSEffectShaderPropertyFloatController::post(NIFFile *nif)
{
    Controller::post(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
}

void Nif::BSLagBoneController::read(NIFStream *nif)
{
    Controller::read(nif);

    linearVelocity = nif->getFloat();
    linearRotation = nif->getFloat();
    maxDistance = nif->getFloat();
}

void Nif::BSLagBoneController::post(NIFFile *nif)
{
    Controller::post(nif);
}

void Nif::NiPSysModifierActiveCtlr::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);
    modifierName = nif->getSkyrimString(nifVer, Record::strings);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        nif->getInt(); // NiVisDataPtr
}

void Nif::NiPSysModifierActiveCtlr::post(NIFFile *nif)
{
    Controller::post(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
    //if (nifVer <= 0x0a010000) // up to 10.1.0.0
        // NiVisDataPtr
}

void Nif::NiPSysEmitterInitialRadiusCtlr::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);
    modifierName = nif->getSkyrimString(nifVer, Record::strings);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        nif->getFloat(); // NiFloatDataPtr
}

void Nif::NiPSysEmitterInitialRadiusCtlr::post(NIFFile *nif)
{
    Controller::post(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
    //if (nifVer <= 0x0a010000) // up to 10.1.0.0
        // NiFloatDataPtr
}

void Nif::NiPSysEmitterLifeSpanCtlr::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);
    modifierName = nif->getSkyrimString(nifVer, Record::strings);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        nif->getFloat(); // NiFloatDataPtr
}

void Nif::NiPSysEmitterLifeSpanCtlr::post(NIFFile *nif)
{
    Controller::post(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
    //if (nifVer <= 0x0a010000) // up to 10.1.0.0
        // NiFloatDataPtr
}

void Nif::NiPSysEmitterSpeedCtlr::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);
    modifierName = nif->getSkyrimString(nifVer, Record::strings);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        nif->getFloat(); // NiFloatDataPtr
}

void Nif::NiPSysEmitterSpeedCtlr::post(NIFFile *nif)
{
    Controller::post(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
    //if (nifVer <= 0x0a010000) // up to 10.1.0.0
        // NiFloatDataPtr
}

void Nif::NiTextureTransformController::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);
    nif->getChar();
    textureSlot = nif->getUInt();
    operation = nif->getUInt();
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        nif->getFloat(); // NiFloatDataPtr
}

void Nif::NiTextureTransformController::post(NIFFile *nif)
{
    Controller::post(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
    //if (nifVer <= 0x0a010000) // up to 10.1.0.0
        // NiFloatDataPtr
}

void Nif::NiPSysGravityStrengthCtlr::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);
    modifierName = nif->getSkyrimString(nifVer, Record::strings);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        nif->getFloat(); // NiFloatDataPtr
}

void Nif::NiPSysGravityStrengthCtlr::post(NIFFile *nif)
{
    Controller::post(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
    //if (nifVer <= 0x0a010000) // up to 10.1.0.0
        // NiFloatDataPtr
}

void Nif::SkinShape::read(NIFStream *nif, unsigned int nifVer)
{
    shape.read(nif);
    skin.read(nif);
}

void Nif::SkinShape::post(NIFFile *nif)
{
    shape.post(nif);
    skin.post(nif);
}

void Nif::SkinShapeGroup::read(NIFStream *nif, unsigned int nifVer)
{
    unsigned int numLinkPairs = nif->getUInt();
    linkPairs.resize(numLinkPairs);
    for (unsigned int i = 0; i < numLinkPairs; ++i)
        linkPairs[i].read(nif, nifVer);
}

void Nif::SkinShapeGroup::post(NIFFile *nif)
{
    for (unsigned int i = 0; i < linkPairs.size(); ++i)
        linkPairs[i].post(nif);
}

void Nif::NiBSBoneLODController::read(NIFStream *nif)
{
    Controller::read(nif);

    nif->getUInt();
    unsigned int numNodeGrp = nif->getUInt();
    unsigned int numNodeGrp2 = nif->getUInt();

    nodeGroups.resize(numNodeGrp);
    for (unsigned int i = 0; i < numNodeGrp; ++i)
        nodeGroups[i].read(nif, nifVer);

#if 0 // seems to be user version controlled
    if (nifVer >= 0x04020200) // from 4.2.2.0
    {
        unsigned int numShapeGrp = nif->getUInt();
        shapeGroups.resize(numShapeGrp);
        for (unsigned int i = 0; i < numShapeGrp; ++i)
            shapeGroups[i].read(nif, nifVer);

        unsigned int numShapeGrp2 = nif->getUInt();
        shapeGroups2.resize(numShapeGrp2);
        for (unsigned int i = 0; i < numShapeGrp2; ++i)
            shapeGroups2[i].read(nif);
    }
#endif
}

void Nif::NiBSBoneLODController::post(NIFFile *nif)
{
    Controller::post(nif);

    // FIXME: the rest
}

void Nif::bhkBlendController::read(NIFStream *nif)
{
    Controller::read(nif);
    unknown = nif->getUInt();
}

void Nif::bhkBlendController::post(NIFFile *nif)
{
    Controller::post(nif);
}

void Nif::NiFloatExtraDataController::read(NIFStream *nif)
{
    Controller::read(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
    {
        interpolator.read(nif);
        controllerData = nif->getSkyrimString(nifVer, Record::strings);
    }

    if (nifVer <= 0x0a010000) // up to 10.1.0.0
    {
        unsigned char numExtra = nif->getChar();
        for (unsigned int i = 0; i < 7; ++i)
            nif->getChar();
        for (unsigned int i = 0; i < numExtra; ++i)
            nif->getChar();
    }
}

void Nif::NiFloatExtraDataController::post(NIFFile *nif)
{
    Controller::post(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
}

void Nif::NiBoolInterpolator::read(NIFStream *nif)
{
    //NiInterpolator::read(nif);

    value = nif->getBool(nifVer);
    boolData.read(nif);
}

void Nif::NiBoolInterpolator::post(NIFFile *nif)
{
    boolData.post(nif);
}

void Nif::NiBlendBoolInterpolator::read(NIFStream *nif)
{
    //NiInterpolator::read(nif);

    nif->getUShort();
    nif->getUInt();

    value = nif->getChar();
}

void Nif::NiFloatInterpolator::read(NIFStream *nif)
{
    //NiInterpolator::read(nif);

    value = nif->getFloat();
    floatData.read(nif);
}

void Nif::NiFloatInterpolator::post(NIFFile *nif)
{
    floatData.post(nif);
}

void Nif::NiBlendFloatInterpolator::read(NIFStream *nif)
{
    //NiInterpolator::read(nif);

    nif->getUShort();
    nif->getUInt();

    value = nif->getFloat();
}

void Nif::NiBlendTransformInterpolator::read(NIFStream *nif)
{
    //NiInterpolator::read(nif);

    unknown1 = nif->getUShort();
    unknown2 = nif->getUInt();
}

void Nif::NiPathInterpolator::read(NIFStream *nif)
{
    //NiInterpolator::read(nif);

    nif->getUShort();
    nif->getUInt();
    nif->getFloat();
    nif->getFloat();
    nif->getUShort();

    posData.read(nif);
    floatData.read(nif);
}

void Nif::NiPathInterpolator::post(NIFFile *nif)
{
    posData.post(nif);
    floatData.post(nif);
}

void Nif::NiPoint3Interpolator::read(NIFStream *nif)
{
    //NiInterpolator::read(nif);

    value = nif->getVector3();

    posData.read(nif);
}

void Nif::NiPoint3Interpolator::post(NIFFile *nif)
{
    posData.post(nif);
}

void Nif::NiBlendPoint3Interpolator::read(NIFStream *nif)
{
    //NiInterpolator::read(nif);

    nif->getUShort();
    nif->getInt();

    value = nif->getVector3();
}

void Nif::NiTransformInterpolator::read(NIFStream *nif)
{
    //NiInterpolator::read(nif);

    translation = nif->getVector3();
    rotation = nif->getQuaternion();
    scale = nif->getFloat();

    transformData.read(nif);
}

void Nif::NiTransformInterpolator::post(NIFFile *nif)
{
    transformData.post(nif);
}

void Nif::NiLookAtInterpolator::read(NIFStream *nif)
{
    //NiInterpolator::read(nif);

    unknown = nif->getUShort();
    lookAt.read(nif);
    target = nif->getSkyrimString(nifVer, Record::strings);
    if (nifVer <= 0x14050000) // up to 20.5.0.0
    {
        translation = nif->getVector3();
        rotation = nif->getQuaternion();
        scale = nif->getFloat();
    }
    nif->getInt();
    nif->getInt();
    nif->getInt();
}

void Nif::NiLookAtInterpolator::post(NIFFile *nif)
{
    lookAt.post(nif);
}

void Nif::NiBSplineInterpolator::read(NIFStream *nif)
{
    //NiInterpolator::read(nif);

    startTime = nif->getFloat();
    stopTime = nif->getFloat();

    splineData.read(nif);
    basisData.read(nif);
}

void Nif::NiBSplineInterpolator::post(NIFFile *nif)
{
    splineData.post(nif);
    basisData.post(nif);
}

void Nif::NiBSplinePoint3Interpolator::read(NIFStream *nif)
{
    NiBSplineInterpolator::read(nif);

    unknown1 = nif->getFloat();
    unknown2 = nif->getFloat();
    unknown3 = nif->getFloat();
    unknown4 = nif->getFloat();
    unknown5 = nif->getFloat();
    unknown6 = nif->getFloat();
}

void Nif::NiBSplinePoint3Interpolator::post(NIFFile *nif)
{
    NiBSplineInterpolator::post(nif);
}

void Nif::NiBSplineTransformInterpolator::read(NIFStream *nif)
{
    NiBSplineInterpolator::read(nif);

    translation = nif->getVector3();
    rotation = nif->getQuaternion();
    scale = nif->getFloat();
    translationOffset = nif->getUInt();
    rotationOffset = nif->getUInt();
    scaleOffset = nif->getUInt();
}

void Nif::NiBSplineTransformInterpolator::post(NIFFile *nif)
{
    NiBSplineInterpolator::post(nif);
}

void Nif::NiBSplineCompTransformInterpolator::read(NIFStream *nif)
{
    NiBSplineTransformInterpolator::read(nif);

    translationBias = nif->getFloat();
    translationMultiplier = nif->getFloat();
    rotationBias = nif->getFloat();
    rotationMultiplier = nif->getFloat();
    scaleBias = nif->getFloat();
    scaleMultiplier = nif->getFloat();
}

void Nif::NiBSplineCompTransformInterpolator::post(NIFFile *nif)
{
    NiBSplineTransformInterpolator::post(nif);
}

void Nif::BSFrustumFOVController::read(NIFStream *nif)
{
    Controller::read(nif);

    interpolator.read(nif);
}

void Nif::BSFrustumFOVController::post(NIFFile *nif)
{
    Controller::post(nif);

    interpolator.post(nif);
}

void Nif::NiTransformController::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);

    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        data.read(nif);
}

void Nif::NiTransformController::post(NIFFile *nif)
{
    Controller::post(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);

    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        data.post(nif);
}

void Nif::NiMultiTargetTransformController::read(NIFStream *nif)
{
    Controller::read(nif);

    numExtraTargets = nif->getUShort();
    extraTargets.resize(numExtraTargets);
    for (unsigned int i = 0; i < numExtraTargets; ++i)
        extraTargets[i].read(nif);
}

void Nif::NiMultiTargetTransformController::post(NIFFile *nif)
{
    Controller::post(nif);

    for (unsigned int i = 0; i < extraTargets.size(); ++i)
        extraTargets[i].post(nif);
}

void Nif::NiControllerManager::read(NIFStream *nif)
{
    Controller::read(nif);

    cumulative = nif->getBool(nifVer);
    unsigned int numControllerSequences = nif->getUInt();
    controllerSequences.resize(numControllerSequences);
    for (unsigned int i = 0; i < numControllerSequences; ++i)
        controllerSequences[i].read(nif);
    nif->getUInt(); // FIXME
}

void Nif::NiControllerManager::post(NIFFile *nif)
{
    Controller::post(nif);

    for (unsigned int i = 0; i < controllerSequences.size(); ++i)
        controllerSequences[i].post(nif);
    //objectPalette.post(nif);
}

void Nif::ControllerLink::read(NIFStream *nif, unsigned int nifVer, std::vector<std::string> *strings)
{
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
    {
        targetName = nif->getString();
        controller.read(nif);
    }

    if (nifVer >= 0x0a01006a) // from 10.1.0.106
    {
        interpolator.read(nif);
        controller2.read(nif);
        if (nifVer == 0x0a01006a) // 10.1.0.106
        {
            nif->getUInt(); // FIXME
            nif->getUShort();
        }
        priority = nif->getChar(); // TODO userVer >= 10
    }

    if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
        stringPalette.read(nif);

    if (nifVer >= 0x14010003) // 20.1.0.3
        nodeName = nif->getSkyrimString(nifVer, strings);
    else if (nifVer == 0x0a01006a) // 10.1.0.106
        nodeName = nif->getString();

    if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
        nodeNameOffset = nif->getInt();

    if (nifVer >= 0x14010003) // 20.1.0.3
        propertyType = nif->getSkyrimString(nifVer, strings);
    else if (nifVer == 0x0a01006a) // 10.1.0.106
        propertyType = nif->getString();

    if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
        propertyTypeOffset = nif->getInt();

    if (nifVer >= 0x14010003) // 20.1.0.3
        controllerType = nif->getSkyrimString(nifVer, strings);
    else if (nifVer == 0x0a01006a) // 10.1.0.106
        controllerType = nif->getString();

    if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
        controllerTypeOffset = nif->getInt();

    if (nifVer >= 0x14010003) // 20.1.0.3
        variable1 = nif->getSkyrimString(nifVer, strings);
    else if (nifVer == 0x0a01006a) // 10.1.0.106
        variable1 = nif->getString();

    if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
        variable1Offset = nif->getInt();

    if (nifVer >= 0x14010003) // 20.1.0.3
        variable2 = nif->getSkyrimString(nifVer, strings);
    else if (nifVer == 0x0a01006a) // 10.1.0.106
        variable2 = nif->getString();

    if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
        variable2Offset = nif->getInt();
}

void Nif::ControllerLink::post(NIFFile *nif, unsigned int nifVer)
{
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        controller.post(nif);

    if (nifVer >= 0x0a01006a) // from 10.1.0.106
    {
        interpolator.post(nif);
        controller2.post(nif);
    }

    if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
        stringPalette.post(nif);
}

void Nif::NiSequence::read(NIFStream *nif)
{
    name = nif->getSkyrimString(nifVer, Record::strings);

    if (nifVer <= 0x0a010000) // up to 10.1.0.0
    {
        textKeysName = nif->getString();
        textKeys.read(nif);
    }
    unsigned int numControlledBlocks = nif->getUInt();
    if (nifVer >= 0x0a01006a) // from 10.1.0.106
        unsigned int unknown = nif->getUInt();
    controlledBlocks.resize(numControlledBlocks);
    for (unsigned int i = 0; i < numControlledBlocks; ++i)
        controlledBlocks[i].read(nif, nifVer, Record::strings);
}

void Nif::NiSequence::post(NIFFile *nif)
{
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        textKeys.post(nif);

    for (unsigned int i = 0; i < controlledBlocks.size(); ++i)
        controlledBlocks[i].post(nif, nifVer);
}

void Nif::NiControllerSequence::read(NIFStream *nif)
{
    NiSequence::read(nif);

    if (nifVer >= 0x0a01006a) // from 10.1.0.106
    {
        weight = nif->getFloat();
        NiControllerSequence::textKeys.read(nif);
        cycleType = nif->getUInt();

        if (nifVer == 0x0a01006a) // 10.1.0.106
            unknown0 = nif->getUInt();

        frequency = nif->getFloat();
        startTime = nif->getFloat();

        if (nifVer >= 0x0a020000 && nifVer <= 0x01040001)
            unknown2 = nif->getFloat();

        stopTime = nif->getFloat();

        if (nifVer == 0x0a01006a) // 10.1.0.106
            unknownByte = nif->getChar();

        manager.read(nif);

        if (nifVer >= 0x14020007 && !Record::strings->empty()) // from 20.2.0.7 (Skyrim)
        {
            unsigned int index = nif->getUInt();
            if (index == -1)
                targetName = "";
            else
                targetName = (*Record::strings)[index]; // FIXME: validate index size
        }
        else
            targetName = nif->getString(); // FIXME just a guess

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            stringPalette.read(nif);

        if (nifVer >= 0x14020007 && userVer >= 11 && (userVer2 >= 24 && userVer2 <= 28))
            nif->getInt(); // FIXME BSAnimNotesPtr

        if (nifVer >= 0x14020007 && userVer2 > 28)
            nif->getUShort(); // Unknown Short 1
    }
}

void Nif::NiControllerSequence::post(NIFFile *nif)
{
    NiSequence::post(nif);

    if (nifVer >= 0x0a01006a) // from 10.1.0.106
    {
        NiControllerSequence::textKeys.post(nif);
        manager.post(nif);
        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            stringPalette.post(nif);
    }
}

void Nif::NiTriStripsData::read(NIFStream *nif)
{
    if (nifVer == 0x0a000100) // HACK: not sure why this is needed
        nif->getInt();

    ShapeData::read(nif);            // NiGeometryData

    /*int tris =*/ nif->getUShort(); // NiTriBasedGeomData

    unsigned short numStrips = nif->getUShort();
    stripLengths.resize(numStrips);
    for (unsigned int i = 0; i < numStrips; ++i)
    {
        stripLengths[i] = nif->getUShort();
    }

    bool hasPoints = false;
    if (nifVer <= 0x0a000102) // up to 10.0.1.2
        hasPoints = true;
    else
        hasPoints = nif->getBool(nifVer);

    if (hasPoints)
    {
        points.resize(numStrips);
        for (unsigned int i = 0; i < numStrips; ++i)
        {
            points[i].resize(stripLengths[i]);
            for (unsigned int j = 0; j < stripLengths[i]; ++j)
                points[i][j] = nif->getUShort();
        }
    }

    // there are (N-2)*3 vertex indicies for triangles
    // where N = stripLengths[stripIndex]
    //
    // e.g. strip length = 150
    //      (150-2)*3 = 148*3 = 444
    unsigned int base = 0;
    for (unsigned int i = 0; i < numStrips; ++i)
    {
        base = static_cast<unsigned int>(triangles.size());
        triangles.resize(base + (stripLengths[i]-2)*3);
        for (unsigned int j = 0; j < (unsigned int)(stripLengths[i]-2); ++j)
        {
            if (j & 1)
            {
                triangles[base+j*3]   = points[i][j];
                triangles[base+j*3+1] = points[i][j+2];
                triangles[base+j*3+2] = points[i][j+1];
            }
            else
            {
                triangles[base+j*3]   = points[i][j];
                triangles[base+j*3+1] = points[i][j+1];
                triangles[base+j*3+2] = points[i][j+2];
            }
        }
    }
}

void Nif::NiBSplineData::read(NIFStream *nif)
{
    unsigned int numPoints = nif->getUInt();
    floatControlPoints.resize(numPoints);
    for (unsigned int i = 0; i < numPoints; ++i)
        floatControlPoints[i] = nif->getFloat();

    numPoints = nif->getUInt();
    shortControlPoints.resize(numPoints);
    for (unsigned int i = 0; i < numPoints; ++i)
        shortControlPoints[i] = nif->getShort();
};

void Nif::NiBSplineBasisData::read(NIFStream *nif)
{
    numControlPoints = nif->getUInt();
};

void Nif::NiStringPalette::read(NIFStream *nif)
{
    unsigned int lth = nif->getUInt();

    buffer.resize(lth+1, 0);
    nif->getBuffer(lth, &buffer[0]);
#if 0
    char* str = &buffer[0];
    while (str - &buffer[0] < lth)
    {
        palette.push_back(std::string(str));
        str += palette.back().size() + 1;
    }
#endif
    unsigned int check = nif->getUInt();
    if (lth != check)
        std::cerr << "Error parsing string palette" << std::endl; // FIXME: throw?
}

void Nif::NiDefaultAVObjectPalette::read(NIFStream *nif)
{
    nif->getUInt();
    unsigned int numObjs = nif->getUInt();
    objs.resize(numObjs);
    for(unsigned int i = 0; i < numObjs; i++)
    {
        objs[i].name = nif->getString(); // TODO: sized string?
        objs[i].avObject.read(nif);
    }
}

void Nif::AVObject::post(NIFFile *nif)
{
    avObject.post(nif);
}

void Nif::NiDefaultAVObjectPalette::post(NIFFile *nif)
{
    for (unsigned int i = 0; i < objs.size(); ++i)
        objs[i].post(nif);
}

void Nif::BSShaderTextureSet::read(NIFStream *nif)
{
    unsigned int numTextures = nif->getUInt();
    textures.resize(numTextures);
    for (unsigned int i = 0; i < numTextures; ++i)
    {
        unsigned int size = nif->getUInt();
        textures[i] = nif->getString(size);
    }
}

void Nif::Particle::read(NIFStream *nif, unsigned int nifVer)
    {
        translation = nif->getVector3();
        if (nifVer <= 0x0a040001) // up to 10.4.0.1
            nif->getFloats(unknownFloats, 3);
        unknown1 = nif->getFloat();
        unknown2 = nif->getFloat();
        unknown3 = nif->getFloat();
        unknown = nif->getInt();
    }

void Nif::NiPSysData::read(NIFStream *nif)
{
    mIsNiPSysData = true;

    NiRotatingParticlesData::read(nif);

    if (!(nifVer >= 0x14020007 && userVer >= 11))
    {
        particleDesc.resize(vertices.size());
        for (unsigned int i = 0; i < vertices.size(); ++i)
            particleDesc[i].read(nif, nifVer);
    }

    if (nifVer >= 0x14000004 && !(nifVer >= 0x14020007 && userVer >= 11))
    {
        if (nif->getBool(nifVer))
            nif->getFloats(unknownFloats3, vertices.size());
    }

    if (!(nifVer >= 0x14020007 && userVer == 11))
    {
        nif->getUShort(); // Unknown short 1
        nif->getUShort(); // Unknown short 2
    }

    if (nifVer >= 0x14020007 && userVer >= 12)
    {
        std::vector<Ogre::Vector4> subTexOffsetUVs;

        bool hasSubTexOffsetUVs = nif->getBool(nifVer);
        unsigned int numSubTexOffsetUVs = nif->getUInt();
        float aspectRatiro = nif->getFloat();
        if (hasSubTexOffsetUVs)
            nif->getVector4s(subTexOffsetUVs, numSubTexOffsetUVs);
        nif->getUInt(); // Unknown Int 4
        nif->getUInt(); // Unknown Int 5
        nif->getUInt(); // Unknown Int 6
        nif->getUShort(); // Unknown short 3
        nif->getChar(); // Unknown byte 4
    }
}

void Nif::BSStripPSysData::read(NIFStream *nif)
{
    NiPSysData::read(nif);

    unknown5 = nif->getShort();
    unknown6 = nif->getChar();
    unknown7 = nif->getInt();
    unknown8 = nif->getFloat();
}

void Nif::NiSkinPartition::SkinPartitionBlock::read(NIFStream *nif,
        unsigned int nifVer, unsigned int userVer)
{
    numVerts = nif->getUShort();
    numTriangles = nif->getUShort();
    numBones = nif->getUShort();
    numStrips = nif->getUShort();
    numWeightsPerVert = nif->getUShort();

    bones.resize(numBones);
    for (unsigned int i = 0; i < numBones; ++i)
        bones[i] = nif->getUShort();

    hasVertMap = nif->getBool(nifVer);
    if (hasVertMap)
    {
        vertMap.resize(numVerts);
        for (unsigned int i = 0; i < numVerts; ++i)
        {
            vertMap[i] = nif->getUShort();
        }
    }

    hasVertWeights = nif->getBool(nifVer);
    if (hasVertWeights)
    {
        vertWeights.resize(numVerts);
        for (unsigned int i = 0; i < numVerts; ++i)
        {
            vertWeights[i].resize(numWeightsPerVert);
            for (unsigned int j = 0; j < numWeightsPerVert; ++j)
            {
                vertWeights[i][j] = nif->getFloat();
            }
        }
    }

    stripLengths.resize(numStrips);
    for (unsigned int i = 0; i < numStrips; ++i)
        stripLengths[i] = nif->getUShort();

    hasFaces = nif->getBool(nifVer);
    if (hasFaces && numStrips != 0)
    {
        strips.resize(numStrips);
        for (unsigned int i = 0; i < numStrips; ++i)
        {
            strips[i].resize(stripLengths[i]);
            for (unsigned int j = 0; j < stripLengths[i]; ++j)
            {
                strips[i][j] = nif->getUShort();
            }
        }
    }
    else if (hasFaces && numStrips == 0)
    {
        triangles.resize(numTriangles);
        for (unsigned int i = 0; i < numTriangles; ++i)
        {
            triangles[i].v1 = nif->getUShort();
            triangles[i].v2 = nif->getUShort();
            triangles[i].v3 = nif->getUShort();
        }
    }

    hasBoneIndicies = nif->getBool(nifVer);
    if (hasBoneIndicies)
    {
        boneIndicies.resize(numVerts);
        for (unsigned int i = 0; i < numVerts; ++i)
        {
            boneIndicies[i].resize(numWeightsPerVert);
            for (unsigned int j = 0; j < numWeightsPerVert; ++j)
            {
                boneIndicies[i][j] = nif->getChar();
            }
        }
    }

    if (userVer >= 12)
        nif->getUShort();
}

void Nif::NiSkinPartition::read(NIFStream *nif)
{
    numSkinPartitionBlocks = nif->getUInt();
    skinPartitionBlocks.resize(numSkinPartitionBlocks);
    for (unsigned int i = 0; i < numSkinPartitionBlocks; ++i)
        skinPartitionBlocks[i].read(nif, nifVer, userVer);
}

void Nif::BSDismemberSkinInstance::read(NIFStream *nif)
{
    NiSkinInstance::read(nif);
    unsigned int numPartitions = nif->getUInt();
    for (unsigned int i = 0; i < numPartitions; ++i)
    {
        nif->getShort();
        nif->getShort();
    }
}

void Nif::BSDismemberSkinInstance::post(NIFFile *nif)
{
    NiSkinInstance::post(nif);
}

