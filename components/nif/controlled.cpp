#include "controlled.hpp"

#include "data.hpp"
#include "node.hpp" // NiParticleSystem

void Nif::NiSourceTexture::read(NIFStream *nif)
{
    Named::read(nif);

    external = !!nif->getChar();
    if(external)
    {
        filename = nif->getString();
        if (nifVer >= 0x0a010000) // 10.1.0.0
            nif->getUInt(); // refNiObject // FIXME
    }
    else
    {
        if (nifVer >= 0x0a010000) // 10.1.0.0
            originalFile = nif->getString();
        else
            nif->getChar(); // always 1 // FIXME: is this presentn on 10.1.0.0?
        data.read(nif);
    }

    pixel = nif->getInt();
    mipmap = nif->getInt();
    alpha = nif->getInt();

    nif->getChar(); // always 1

    if (nifVer >= 0x0a01006a) // 10.1.0.106
        directRenderer = nif->getBool(nifVer);
}

void Nif::NiSourceTexture::post(NIFFile *nif)
{
    Named::post(nif);
    data.post(nif);
}

void Nif::NiParticleGrowFade::read(NIFStream *nif)
{
    NiParticleModifier::read(nif);
    growTime = nif->getFloat();
    fadeTime = nif->getFloat();
}

void Nif::NiParticleColorModifier::read(NIFStream *nif)
{
    NiParticleModifier::read(nif);
    data.read(nif);
}

void Nif::NiParticleColorModifier::post(NIFFile *nif)
{
    NiParticleModifier::post(nif);
    data.post(nif);
}

void Nif::NiGravity::read(NIFStream *nif)
{
    NiParticleModifier::read(nif);

    /*unknown*/nif->getFloat();
    mForce = nif->getFloat();
    mType = nif->getUInt();
    mPosition = nif->getVector3();
    mDirection = nif->getVector3();
}

void Nif::NiPlanarCollider::read(NIFStream *nif)
{
    NiParticleModifier::read(nif);

    // (I think) 4 floats + 4 vectors
    nif->skip(4*16);
}

void Nif::NiParticleRotation::read(NIFStream *nif)
{
    NiParticleModifier::read(nif);

    /*
       byte (0 or 1)
       float (1)
       float*3
    */
    nif->skip(17);
}

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
