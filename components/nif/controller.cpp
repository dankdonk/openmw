#include "controller.hpp"

#include "controlled.hpp" // NiSouceTexture
#include "extra.hpp" // NiTextKeyExtraData

void Nif::NiParticleSystemController::read(NIFStream *nif)
{
    Controller::read(nif);

    velocity = nif->getFloat();
    velocityRandom = nif->getFloat();
    verticalDir = nif->getFloat();
    verticalAngle = nif->getFloat();
    horizontalDir = nif->getFloat();
    horizontalAngle = nif->getFloat();
    /*normal?*/ nif->getVector3();
    /*color?*/ nif->getVector4();
    size = nif->getFloat();
    startTime = nif->getFloat();
    stopTime = nif->getFloat();
    nif->getChar();
    emitRate = nif->getFloat();
    lifetime = nif->getFloat();
    lifetimeRandom = nif->getFloat();

    emitFlags = nif->getUShort();
    offsetRandom = nif->getVector3();

    emitter.read(nif);

    /* Unknown Short, 0?
     * Unknown Float, 1.0?
     * Unknown Int, 1?
     * Unknown Int, 0?
     * Unknown Short, 0?
     */
    nif->skip(16);

    numParticles = nif->getUShort();
    activeCount = nif->getUShort();

    particles.resize(numParticles);
    for(size_t i = 0;i < particles.size();i++)
    {
        particles[i].velocity = nif->getVector3();
        nif->getVector3(); /* unknown */
        particles[i].lifetime = nif->getFloat();
        particles[i].lifespan = nif->getFloat();
        particles[i].timestamp = nif->getFloat();
        nif->getUShort(); /* unknown */
        particles[i].vertex = nif->getUShort();
    }

    nif->getUInt(); /* -1? */
    extra.read(nif); // Ref<NiParticleModifier>
    nif->getUInt(); /* -1? */
    nif->getChar();
}

void Nif::NiParticleSystemController::post(NIFFile *nif)
{
    Controller::post(nif);
    emitter.post(nif);
    extra.post(nif);
}

void Nif::NiMaterialColorController::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);
    if (nifVer >= 0x0a010000) // from 10.1.0.0
        targetColor = nif->getUShort();
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        data.read(nif);
}

void Nif::NiMaterialColorController::post(NIFFile *nif)
{
    Controller::post(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        data.post(nif);
}

void Nif::NiPathController::read(NIFStream *nif)
{
    Controller::read(nif);

    if (nifVer >= 0x0a010000) // from 10.1.0.0
        nif->getUShort();

    /*
       int = 1
       2xfloat
       short = 0 or 1
    */
    nif->skip(14);
    posData.read(nif);
    floatData.read(nif);
}

void Nif::NiPathController::post(NIFFile *nif)
{
    Controller::post(nif);

    posData.post(nif);
    floatData.post(nif);
}

void Nif::NiUVController::read(NIFStream *nif)
{
    Controller::read(nif);

    nif->getUShort(); // always 0
    data.read(nif);
}

void Nif::NiUVController::post(NIFFile *nif)
{
    Controller::post(nif);
    data.post(nif);
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

void Nif::NiKeyframeController::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        data.read(nif);
}

void Nif::NiKeyframeController::post(NIFFile *nif)
{
    Controller::post(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        data.post(nif);
}

void Nif::NiAlphaController::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        data.read(nif);
}

void Nif::NiAlphaController::post(NIFFile *nif)
{
    Controller::post(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        data.post(nif);
}

void Nif::NiGeomMorpherController::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a000102) // from 10.0.1.2
        extraFlags = nif->getUShort();
    if (nifVer == 0x0a01006a) // 10.1.0.106
        nif->getChar();
    data.read(nif);
    nif->getChar(); // always 0
    if (nifVer >= 0x0a01006a) // from 10.1.0.106
    {
        unsigned int numInterpolators = nif->getUInt();
        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
        {
            interpolators.resize(numInterpolators);
            for (unsigned int i = 0; i < numInterpolators; ++i)
                interpolators[i].read(nif);
        }
        if (nifVer >= 0x14010003) // from 20.1.0.3
        {
            interpolatorWeights.resize(numInterpolators);
            for (unsigned int i = 0; i < numInterpolators; ++i)
            {
                interpolatorWeights[i].interpolator.read(nif);
                interpolatorWeights[i].weight = nif->getFloat();
            }
        }
        unsigned int count = nif->getUInt();
        unknownInts.resize(count);
        for (unsigned int i = 0; i < count; ++i)
            unknownInts[i] = nif->getUInt();
    }
}

void Nif::NiGeomMorpherController::post(NIFFile *nif)
{
    Controller::post(nif);
    data.post(nif);

    if (nifVer >= 0x0a01006a) // from 10.1.0.106
    {
        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
        {
            for (unsigned int i = 0; i < interpolators.size(); ++i)
                interpolators[i].post(nif);
        }
    }
}

void Nif::NiVisController::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        data.read(nif);
}

void Nif::NiVisController::post(NIFFile *nif)
{
    Controller::post(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        data.post(nif);
}

void Nif::NiFlipController::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);
    mTexSlot = nif->getUInt();
    if (nifVer >= 0x04000000  && nifVer <= 0x0a010000)
        /*unknown=*/nif->getUInt();/*0?*/
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        mDelta = nif->getFloat();
    mSources.read(nif);
}

void Nif::NiFlipController::post(NIFFile *nif)
{
    Controller::post(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
    mSources.post(nif);
}

void Nif::NiPSysEmitterCtlr::read(NIFStream *nif)
{
    Controller::read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.read(nif);
    modifierName = nif->getSkyrimString(nifVer, Record::strings);
    if (nifVer <= 0x0a010000) // up to 10.1.0.0
        nif->getInt(); // NiPSysEmtterCtlrDataPtr
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        visibilityInterpolator.read(nif);
}

void Nif::NiPSysEmitterCtlr::post(NIFFile *nif)
{
    Controller::post(nif);

    if (nifVer >= 0x0a020000) // from 10.2.0.0
        interpolator.post(nif);
    //if (nifVer <= 0x0a010000) // up to 10.1.0.0
        // NiPSysEmtterCtlrDataPtr
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        visibilityInterpolator.post(nif);
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
        textKeys2.read(nif);
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
        textKeys2.post(nif);
        manager.post(nif);
        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            stringPalette.post(nif);
    }
}
