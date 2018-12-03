#include "property.hpp"

#include <iostream> // FIXME

#include "data.hpp" // BSShaderTextureSet

void Nif::PropertyAndFlags::read(NIFStream *nif)
{
    Property::read(nif);
    flags = nif->getUShort();
}

void Nif::NiZBufferProperty::read(NIFStream *nif)
{
    PropertyAndFlags::read(nif);
    if (nifVer >= 0x0401000c && nifVer <= 0x14000005) // 4.1.0.12 to 20.0.0.5
        zCompareMode = nif->getUInt();
}

void Nif::BSLightingShaderProperty::read(NIFStream *nif)
{
    if (userVer >= 12)
        skyrimShaderType = nif->getUInt();
    else
        skyrimShaderType = 0;

    Property::read(nif);

    if (userVer == 12)
    {
        shaderFlags1 = nif->getUInt();
        shaderFlags2 = nif->getUInt();
    }
    uvOffset.u = nif->getFloat();
    uvOffset.v = nif->getFloat();
    uvScale.u = nif->getFloat();
    uvScale.v = nif->getFloat();
    textureSet.read(nif);
    emissiveColor = nif->getVector3();
    emissiveMultiple = nif->getFloat();
    textureClampMode = nif->getUInt();
    alpha = nif->getFloat();
    unknown2 = nif->getFloat();
    glossiness = nif->getFloat();
    specularColor = nif->getVector3();
    specularStrength = nif->getFloat();
    lightingEffect1 = nif->getFloat();
    lightingEffect2 = nif->getFloat();

    if (skyrimShaderType == 1)
        envMapScale = nif->getFloat();
    else if (skyrimShaderType == 5)
        skinTintColor = nif->getVector3();
    else if (skyrimShaderType == 6)
        hairTintColor = nif->getVector3();
    else if (skyrimShaderType == 7)
    {
        maxPasses = nif->getFloat();
        scale = nif->getFloat();
    }
    else if (skyrimShaderType == 11)
    {
        parallaxInnerLayerThickness = nif->getFloat();
        parallaxRefractionScale = nif->getFloat();
        parallaxInnerLayerTextureScale.u = nif->getFloat();
        parallaxInnerLayerTextureScale.v = nif->getFloat();
        parallaxEnvmapStrength = nif->getFloat();
    }
    else if (skyrimShaderType == 14)
        sparkleParm = nif->getVector4();
    else if (skyrimShaderType == 16)
    {
        eyeCubemapScale = nif->getFloat();
        leftEyeReflectionCenter = nif->getVector3();
        rightEyeReflectionCenter = nif->getVector3();
    }
}

void Nif::BSLightingShaderProperty::post(NIFFile *nif)
{
    textureSet.post(nif);
}

void Nif::BSEffectShaderProperty::read(NIFStream *nif)
{
    Property::read(nif);

    shaderFlags1 = nif->getUInt();
    shaderFlags2 = nif->getUInt();
    uvOffset.u = nif->getFloat();
    uvOffset.v = nif->getFloat();
    uvScale.u = nif->getFloat();
    uvScale.v = nif->getFloat();
    unsigned int size = nif->getUInt();
    sourceTexture = nif->getString(size);
    textureClampMode = nif->getUInt();
    falloffStartAngle = nif->getFloat();
    falloffStopAngle = nif->getFloat();
    falloffStartOpacity = nif->getFloat();
    falloffStopOpacity = nif->getFloat();
    emissiveColor = nif->getVector4();
    emissiveMultiple = nif->getFloat();
    softFalloffDepth = nif->getFloat();
    size = nif->getUInt();
    greyscaleTexture = nif->getString(size);
}

void Nif::BSWaterShaderProperty::read(NIFStream *nif)
{
    Property::read(nif);

    shaderFlags1 = nif->getUInt();
    shaderFlags2 = nif->getUInt();
    uvOffset.u = nif->getFloat();
    uvOffset.v = nif->getFloat();
    uvScale.u = nif->getFloat();
    uvScale.v = nif->getFloat();
    waterShaderFlags = nif->getChar();
    waterDirection = nif->getChar();
    unknownS3 = nif->getUShort();
}

void Nif::BSShaderPPLightingProperty::read(NIFStream *nif) // FO3
{
    if (userVer >= 12)
        skyrimShaderType = nif->getUInt();
    else
        skyrimShaderType = 0;

    Property::read(nif);

    short flags = nif->getShort();
    unsigned int shaderType = nif->getUInt();
    unsigned int shaderFlags = nif->getUInt();
    int unknownInt2 = nif->getInt();
    envmapScale = nif->getFloat();
    int unknownInt3 = nif->getInt();

    textureSet.read(nif);

    float unknownFloat2 = nif->getFloat();
    int refractionPeriod = nif->getInt();
    float unknownFloat4 = nif->getFloat();
    float unknownFloat5 = nif->getFloat();

    if (userVer >= 12)
        emissiveColor = nif->getVector3();
}

void Nif::BSShaderPPLightingProperty::post(NIFFile *nif) // FO3
{
    Property::post(nif);
    textureSet.post(nif);
}

void Nif::BSShaderNoLightingProperty::read(NIFStream *nif) // FO3
{
    if (userVer >= 12)
        skyrimShaderType = nif->getUInt();
    else
        skyrimShaderType = 0;

    Property::read(nif);

    short flags = nif->getShort();
    unsigned int shaderType = nif->getUInt();
    unsigned int shaderFlags = nif->getUInt();
    int unknownInt2 = nif->getInt();
    if (userVer == 11)
        envmapScale = nif->getFloat();
    if (userVer <= 11)
        int unknownInt3 = nif->getInt();

    unsigned int size = nif->getUInt();
    if (size)
        fileName = nif->getString(size);

    if (userVer >= 11 && userVer2 > 26)
    {
        float unknownFloat2 = nif->getFloat();
        float unknownFloat3 = nif->getFloat();
        float unknownFloat4 = nif->getFloat();
        float unknownFloat5 = nif->getFloat();
    }
}

void Nif::BSShaderNoLightingProperty::post(NIFFile *nif) // FO3
{
    Property::post(nif);
}
