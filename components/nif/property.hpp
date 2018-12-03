/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (property.h) is part of the OpenMW package.

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

#ifndef OPENMW_COMPONENTS_NIF_PROPERTY_HPP
#define OPENMW_COMPONENTS_NIF_PROPERTY_HPP

#include "base.hpp"
#include "controlled.hpp" // NiSouceTexture

namespace Nif
{

// NiProperty
class Property : public Named
{
public:
    void read(NIFStream *nif)
    {
        Named::read(nif);
    }
};

class NiTexturingProperty : public Property
{
public:
    // A sub-texture
    struct Texture
    {
        /* Clamp mode
        0 - clampS clampT
        1 - clampS wrapT
        2 - wrapS clampT
        3 - wrapS wrapT
        */

        /* Filter:
        0 - nearest
        1 - bilinear
        2 - trilinear
        3, 4, 5 - who knows
        */
        bool inUse;
        NiSourceTexturePtr texture;

        int clamp, uvSet, filter;
        short unknown2;

        Ogre::Vector2 translation;
        Ogre::Vector2 tiling;
        float wRotation;
        unsigned int transformType;
        Ogre::Vector2 cenerOffset;

        void read(NIFStream *nif, unsigned int nifVer)
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

        void post(NIFFile *nif)
        {
            if (!inUse)
                return;

            texture.post(nif);
        }
    };

    int flags;

    /* Apply mode:
        0 - replace
        1 - decal
        2 - modulate
        3 - hilight  // These two are for PS2 only?
        4 - hilight2
    */
    int apply;

    /*
     * The textures in this list are as follows:
     *
     * 0 - Base texture
     * 1 - Dark texture
     * 2 - Detail texture
     * 3 - Gloss texture (never used?)
     * 4 - Glow texture
     * 5 - Bump map texture
     * 6 - Decal texture
     */
    enum TextureType
    {
        BaseTexture = 0,
        DarkTexture = 1,
        DetailTexture = 2,
        GlossTexture = 3,
        GlowTexture = 4,
        BumpTexture = 5,
        DecalTexture = 6,
        // 20.2.0.7 onwards
        NormalTexture = 7,
        Unknown2Texture = 8,
        Decal1Texture = 9,
        Decal2Texture = 10,
        Decal3Texture = 11
    };

    struct ShaderTexture
    {
        Texture texture;
        unsigned int mapIndex;

    };

    Texture textures[12]; // usually max 7
    std::vector<ShaderTexture> shaderTextures;

    void read(NIFStream *nif)
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

    void post(NIFFile *nif)
    {
        Property::post(nif);
        for(int i = 0;i < 12;i++)
            textures[i].post(nif);
    }
};

class PropertyAndFlags : public Property
{
public:
    // The meaning of these depends on the actual property type.
    int flags;

    void read(NIFStream *nif);
};

class NiFogProperty : public PropertyAndFlags
{
public:
    float mFogDepth;
    Ogre::Vector3 mColour;


    void read(NIFStream *nif)
    {
        PropertyAndFlags::read(nif);

        mFogDepth = nif->getFloat();
        mColour = nif->getVector3();
    }
};

// These contain no other data than the 'flags' field in Property
class NiShadeProperty : public PropertyAndFlags { };
class NiDitherProperty : public PropertyAndFlags { };
class NiSpecularProperty : public PropertyAndFlags { };
class NiWireframeProperty : public PropertyAndFlags { };

// The rest are all struct-based
template <typename T>
struct StructPropT : Property
{
    // The meaning of these depends on the actual property type.
    int flags;
    T data;

    void read(NIFStream *nif)
    {
        Property::read(nif);
        flags = nif->getUShort();
        data.read(nif);
    }
};

class NiZBufferProperty : public PropertyAndFlags
{
public:
    unsigned int zCompareMode;

    void read(NIFStream *nif);
};

class NiMaterialProperty : public Property
{
public:
    int flags;

    // The vector components are R,G,B
    Ogre::Vector3 ambient, diffuse, specular, emissive;
    float glossiness, alpha;

    void read(NIFStream *nif)
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
};

struct S_VertexColorProperty
{
    /* Vertex mode:
        0 - source ignore
        1 - source emmisive
        2 - source amb diff

        Lighting mode
        0 - lighting emmisive
        1 - lighting emmisive ambient/diffuse
    */
    unsigned int vertmode, lightmode;

    void read(NIFStream *nif)
    {
        vertmode = nif->getUInt();
        lightmode = nif->getUInt();
    }
};

struct S_AlphaProperty
{
    /*
        In NiAlphaProperty, the flags have the following meaning:

        Bit 0 : alpha blending enable
        Bits 1-4 : source blend mode
        Bits 5-8 : destination blend mode
        Bit 9 : alpha test enable
        Bit 10-12 : alpha test mode
        Bit 13 : no sorter flag ( disables triangle sorting )

        blend modes (glBlendFunc):
        0000 GL_ONE
        0001 GL_ZERO
        0010 GL_SRC_COLOR
        0011 GL_ONE_MINUS_SRC_COLOR
        0100 GL_DST_COLOR
        0101 GL_ONE_MINUS_DST_COLOR
        0110 GL_SRC_ALPHA
        0111 GL_ONE_MINUS_SRC_ALPHA
        1000 GL_DST_ALPHA
        1001 GL_ONE_MINUS_DST_ALPHA
        1010 GL_SRC_ALPHA_SATURATE

        test modes (glAlphaFunc):
        000 GL_ALWAYS
        001 GL_LESS
        010 GL_EQUAL
        011 GL_LEQUAL
        100 GL_GREATER
        101 GL_NOTEQUAL
        110 GL_GEQUAL
        111 GL_NEVER

        Taken from:
        http://niftools.sourceforge.net/doc/nif/NiAlphaProperty.html

        Right now we only use standard alpha blending (see the Ogre code
        that sets it up) and it appears that this is the only blending
        used in the original game. Bloodmoon (along with several mods) do
        however use other settings, such as discarding pixel values with
        alpha < 1.0. This is faster because we don't have to mess with the
        depth stuff like we did for blending. And OGRE has settings for
        this too.
    */

    // Tested against when certain flags are set (see above.)
    unsigned char threshold;

    void read(NIFStream *nif)
    {
        threshold = nif->getChar();
    }
};

/*
    Docs taken from:
    http://niftools.sourceforge.net/doc/nif/NiStencilProperty.html
 */
class NiStencilProperty : public Property
{
public:
    int flags;

    // Is stencil test enabled?
    unsigned char enabled;

    /*
        0   TEST_NEVER
        1   TEST_LESS
        2   TEST_EQUAL
        3   TEST_LESS_EQUAL
        4   TEST_GREATER
        5   TEST_NOT_EQUAL
        6   TEST_GREATER_EQUAL
        7   TEST_ALWAYS
     */
    int compareFunc;
    unsigned stencilRef;
    unsigned stencilMask;
    /*
        Stencil test fail action, depth test fail action and depth test pass action:
        0   ACTION_KEEP
        1   ACTION_ZERO
        2   ACTION_REPLACE
        3   ACTION_INCREMENT
        4   ACTION_DECREMENT
        5   ACTION_INVERT
     */
    int failAction;
    int zFailAction;
    int zPassAction;
    /*
        Face draw mode:
        0   DRAW_CCW_OR_BOTH
        1   DRAW_CCW        [default]
        2   DRAW_CW
        3   DRAW_BOTH
     */
    int drawMode;

    void read(NIFStream *nif)
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
};

class NiAlphaProperty : public StructPropT<S_AlphaProperty> { };
class NiVertexColorProperty : public StructPropT<S_VertexColorProperty> { };

// FIXME: use Ogre::Vector2 instead
struct TexCoord
{
    float u;
    float v;
};

class BSLightingShaderProperty : public Property
{
public:
    unsigned int skyrimShaderType;
    unsigned int shaderFlags1;
    unsigned int shaderFlags2;
    TexCoord uvOffset;
    TexCoord uvScale;
    BSShaderTextureSetPtr textureSet;
    Ogre::Vector3 emissiveColor;
    float emissiveMultiple;
    unsigned int textureClampMode;
    float alpha;
    float unknown2;
    float glossiness;
    Ogre::Vector3 specularColor;
    float specularStrength;
    float lightingEffect1;
    float lightingEffect2;

    float envMapScale;
    Ogre::Vector3 skinTintColor;
    Ogre::Vector3 hairTintColor;
    float maxPasses;
    float scale;
    float parallaxInnerLayerThickness;
    float parallaxRefractionScale;
    TexCoord parallaxInnerLayerTextureScale;
    float parallaxEnvmapStrength;
    Ogre::Vector4 sparkleParm;
    float eyeCubemapScale;
    Ogre::Vector3 leftEyeReflectionCenter;
    Ogre::Vector3 rightEyeReflectionCenter;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSEffectShaderProperty : public Property
{
public:
    unsigned int shaderFlags1;
    unsigned int shaderFlags2;
    TexCoord uvOffset;
    TexCoord uvScale;
    std::string sourceTexture;
    unsigned int textureClampMode;
    float falloffStartAngle;
    float falloffStopAngle;
    float falloffStartOpacity;
    float falloffStopOpacity;
    Ogre::Vector4 emissiveColor;
    float emissiveMultiple;
    float softFalloffDepth;
    std::string greyscaleTexture;

    void read(NIFStream *nif);
};

class BSWaterShaderProperty : public Property
{
public:
    unsigned int shaderFlags1;
    unsigned int shaderFlags2;
    TexCoord uvOffset;
    TexCoord uvScale;
    unsigned char waterShaderFlags;
    unsigned char waterDirection;
    unsigned short unknownS3;

    void read(NIFStream *nif);
};

class BSShaderPPLightingProperty : public Property // FO3
{
public:
    unsigned int skyrimShaderType;

    float envmapScale;
    BSShaderTextureSetPtr textureSet;
    Ogre::Vector3 emissiveColor;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSShaderNoLightingProperty : public Property // FO3
{
public:
    unsigned int skyrimShaderType;

    float envmapScale;
    std::string fileName;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

} // Namespace
#endif
