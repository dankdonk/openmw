#include "property.hpp"

#include <iostream> // FIXME

#include "controlled.hpp" // NiSouceTexture
#include "data.hpp" // BSShaderTextureSet

void Nif::Property::read(NIFStream *nif)
{
    Named::read(nif);
}

void Nif::NiTexturingProperty::Texture::read(NIFStream *nif, unsigned int nifVer)
{
    inUse = nif->getBool(nifVer);
    if(!inUse) return;

    texture.read(nif);
    if (nifVer <= 0x14000005)
    {
        clamp = nif->getInt();
        filter = nif->getInt();
    }

    if (nifVer >= 0x14010003)
        nif->getShort();
    if (nifVer >= 0x14060000)
        nif->getShort();
    if (nifVer <= 0x14000005)
        uvSet = nif->getInt();

    // I have no idea, but I think these are actually two
    // PS2-specific shorts (ps2L and ps2K), followed by an unknown
    // short.
    if (nifVer <= 0x0a040001) // 10.4.0.1
        nif->skip(4);
    if (nifVer <= 0x04010002) // 4.1.0.2
        nif->skip(2);

    if (nifVer >= 0x0a010000) // 10.1.0.0
    {
        bool hasTextureTransform = false;
        if (nifVer >= 0x04000002) // from 4.0.0.2
        {
            hasTextureTransform = nif->getBool(nifVer);
            if (hasTextureTransform)
            {
                translation = nif->getVector2();
                tiling = nif->getVector2();
                wRotation = nif->getFloat();
                transformType = nif->getUInt();
                cenerOffset = nif->getVector2();
            }
        }
    }
}

void Nif::NiTexturingProperty::Texture::post(NIFFile *nif)
{
    if (!inUse)
        return;

    texture.post(nif);
}

void Nif::NiTexturingProperty::read(NIFStream *nif)
{
    Property::read(nif);
    if (nifVer <= 0x0a000102 || nifVer >= 0x14010003)
        flags = nif->getUShort();

    if (nifVer <= 0x14010003)
        apply = nif->getInt();

    // Unknown, always 7. Probably the number of textures to read
    // below
    int count = nif->getInt();
    if (count > 12)
        std::cout << "texture count: " << count << std::endl;

    textures[BaseTexture].read(nif, nifVer);   // Base
    textures[DarkTexture].read(nif, nifVer);   // Dark
    textures[DetailTexture].read(nif, nifVer); // Detail
    textures[GlossTexture].read(nif, nifVer);  // Gloss (never present)
    textures[GlowTexture].read(nif, nifVer);   // Glow
    textures[BumpTexture].read(nif, nifVer);   // Bump map
    if(textures[BumpTexture].inUse)
    {
        // Ignore these at the moment
        /*float lumaScale =*/ nif->getFloat();
        /*float lumaOffset =*/ nif->getFloat();
        /*const Vector4 *lumaMatrix =*/ nif->getVector4();
    }

    if (nifVer >= 0x14020007) // FO3 onwards?
    {
        textures[NormalTexture].read(nif, nifVer);   // Normal
        textures[Unknown2Texture].read(nif, nifVer); // Unknown2
        if (textures[8].inUse)
            nif->getFloat();           // Unknown2 Float
    }

    textures[DecalTexture].read(nif, nifVer);     // Decal 0

    if ((nifVer <= 0x14010003 && count >= 8) || (nifVer >= 0x14020007 && count >= 10))
        textures[Decal1Texture].read(nif, nifVer); // Decal1

    if ((nifVer <= 0x14010003 && count >= 9) || (nifVer >= 0x14020007 && count >= 11))
        textures[Decal2Texture].read(nif, nifVer); // Decal2

    if ((nifVer <= 0x14010003 && count >= 10) || (nifVer >= 0x14020007 && count >= 12))
        textures[Decal3Texture].read(nif, nifVer); // Decal3

    if (nifVer >= 0x0a000100) // 10.0.1.0
    {
        unsigned int numShaderTex = nif->getUInt();
        shaderTextures.resize(numShaderTex);
        for (unsigned int i = 0; i < numShaderTex; ++i)
        {
            shaderTextures[i].texture.read(nif, nifVer);
            shaderTextures[i].mapIndex = nif->getUInt();
        }
    }
}

void Nif::NiTexturingProperty::post(NIFFile *nif)
{
    Property::post(nif);
    for(int i = 0;i < 12;i++)
        textures[i].post(nif);
}

void Nif::PropertyAndFlags::read(NIFStream *nif)
{
    Property::read(nif);
    flags = nif->getUShort();
}

void Nif::NiFogProperty::read(NIFStream *nif)
{
    PropertyAndFlags::read(nif);

    mFogDepth = nif->getFloat();
    mColour = nif->getVector3();
}

void Nif::NiZBufferProperty::read(NIFStream *nif)
{
    PropertyAndFlags::read(nif);
    if (nifVer >= 0x0401000c && nifVer <= 0x14000005) // 4.1.0.12 to 20.0.0.5
        zCompareMode = nif->getUInt();
}

void Nif::NiMaterialProperty::read(NIFStream *nif)
{
    Property::read(nif);
    if (nifVer <= 0x0a000102) // 10.0.1.2
        flags = nif->getUShort();

    if (!(nifVer == 0x14020007 && userVer >= 11 && userVer2 >21)) // FIXME: FO3
    {
        ambient = nif->getVector3();
        diffuse = nif->getVector3();
    }
    specular = nif->getVector3();
    emissive = nif->getVector3();
    glossiness = nif->getFloat();
    alpha = nif->getFloat();

    if (nifVer == 0x14020007 && userVer >= 11 && userVer2 >21) // FIXME: FO3
        float emitMulti = nif->getFloat();
}

void Nif::NiStencilProperty::read(NIFStream *nif)
{
    Property::read(nif);
    if (nifVer <= 0x0a000102) // 10.0.1.2
        flags = nif->getUShort();

    if (nifVer <= 0x14000005) // not in FO3
    {
        enabled = nif->getChar();
        compareFunc = nif->getInt();
        stencilRef = nif->getUInt();
        stencilMask = nif->getUInt();
        failAction = nif->getInt();
        zFailAction = nif->getInt();
        zPassAction = nif->getInt();
        drawMode = nif->getInt();
    }

    if (nifVer >= 0x14010003) // FO3
    {
        flags = nif->getUShort();
        unsigned int StencilRef = nif->getUInt();
        unsigned int StencilMask = nif->getUInt();
    }
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
