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
    clamp = nif->getInt();
    filter = nif->getInt();
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
    texture.post(nif);
}

void Nif::NiTexturingProperty::read(NIFStream *nif)
{
    Property::read(nif);
    if (nifVer <= 0x0a000102) // 10.0.1.2
        flags = nif->getUShort();

    apply = nif->getInt();

    // Unknown, always 7. Probably the number of textures to read
    // below
    int count = nif->getInt();
    if (count != 7)
        std::cout << "texture count: " << count << std::endl;

    textures[0].read(nif, nifVer); // Base
    textures[1].read(nif, nifVer); // Dark
    textures[2].read(nif, nifVer); // Detail
    textures[3].read(nif, nifVer); // Gloss (never present)
    textures[4].read(nif, nifVer); // Glow
    textures[5].read(nif, nifVer); // Bump map
    if(textures[5].inUse)
    {
        // Ignore these at the moment
        /*float lumaScale =*/ nif->getFloat();
        /*float lumaOffset =*/ nif->getFloat();
        /*const Vector4 *lumaMatrix =*/ nif->getVector4();
    }
    textures[6].read(nif, nifVer); // Decal

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
    for(int i = 0;i < 7;i++)
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

    ambient = nif->getVector3();
    diffuse = nif->getVector3();
    specular = nif->getVector3();
    emissive = nif->getVector3();
    glossiness = nif->getFloat();
    alpha = nif->getFloat();
    // FIXME: emit multi?
}

void Nif::NiStencilProperty::read(NIFStream *nif)
{
    Property::read(nif);
    if (nifVer <= 0x0a000102) // 10.0.1.2
        flags = nif->getUShort();

    enabled = nif->getChar();
    compareFunc = nif->getInt();
    stencilRef = nif->getUInt();
    stencilMask = nif->getUInt();
    failAction = nif->getInt();
    zFailAction = nif->getInt();
    zPassAction = nif->getInt();
    drawMode = nif->getInt();
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
