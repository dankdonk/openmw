/*
  Copyright (C) 2015-2017 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

*/
#ifndef NIBTOGRE_NIPROPERTY_H
#define NIBTOGRE_NIPROPERTY_H

#include <string>
#include <vector>

#include <OgreVector2.h>
#include <OgreVector3.h>
#include <OgreVector4.h>

#include "niobjectnet.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiObjectNET
//     NiProperty
//         BSEffectShaderProperty
//         BSLightingShaderProperty
//         BSShaderProperty <----------------------- /* TODO */
//             BSShaderLightingProperty <----------- /* TODO */
//                 BSShaderPPLightingProperty <----- /* TODO */
//         BSSkyShaderProperty <-------------------- /* TODO */
//         BSWaterShaderProperty
//         NiAlphaProperty
//         NiDitherProperty
//         NiFogProperty
//         NiMaterialProperty
//         NiShadeProperty
//         NiSpecularProperty
//         NiStencilProperty
//         NiTexturingProperty
//         NiVertexColorProperty
//         NiWireframeProperty
//         NiZBufferProperty
namespace NiBtOgre
{
#if 0
    // Seen in NIF version 20.2.0.7
    class NiProperty : public NiObjectNET
    {
    protected:
        bool mIsBSLightingShaderProperty;

    public:
        NiProperty(NiStream& stream, const NiModel& model);
    };
#endif
    typedef NiObjectNET NiProperty; // Seen in NIF version 20.2.0.7

    // Seen in NIF version 20.2.0.7
    class BSEffectShaderProperty : public NiProperty
    {
    public:
        std::uint32_t mShaderFlags1;
        std::uint32_t mShaderFlags2;
        TexCoord mUVOffset;
        TexCoord mUVScale;
        std::string mSourceTexture;
        std::uint32_t mTextureClampMode;
        float mFalloffStartAngle;
        float mFalloffStopAngle;
        float mFalloffStartOpacity;
        float mFalloffStopOpacity;
        Ogre::Vector4 mEmissiveColor;
        float mEmissiveMultiple;
        float mSoftFalloffDepth;
        std::string mGreyscaleTexture;

        BSEffectShaderProperty(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class BSLightingShaderProperty : public NiProperty
    {
    public:
        std::uint32_t mShaderFlags1;
        std::uint32_t mShaderFlags2;
        TexCoord mUVOffset;
        TexCoord mUVScale;
        BSShaderTextureSetRef mTextureSetIndex;
        Ogre::Vector3 mEmissiveColor;
        float mEmissiveMultiple;
        std::uint32_t mTextureClampMode;
        float mAlpha;
        float mUnknown2;
        float mGlossiness;
        Ogre::Vector3 mSpecularColor;
        float mSpecularStrength;
        float mLightingEffect1;
        float mLightingEffect2;

        float mEnvironmentMapScale;
        Ogre::Vector3 mSkinTintColor;
        Ogre::Vector3 mHairTintColor;
        float mMaxPasses;
        float mScale;
        float mParallaxInnerLayerThickness;
        float mParallaxRefractionScale;
        TexCoord mParallaxInnerLayerTextureScale;
        float mParallaxEnvmapStrength;
        Ogre::Vector4 mSparkleParameters;
        float mEyeCubemapScale;
        Ogre::Vector3 mLeftEyeReflectionCenter;
        Ogre::Vector3 mRightEyeReflectionCenter;

        BSLightingShaderProperty(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class BSWaterShaderProperty : public NiProperty
    {
    public:
        std::uint32_t mShaderFlags1;
        std::uint32_t mShaderFlags2;
        TexCoord mUVOffset;
        TexCoord mUVScale;
        unsigned char mWaterShaderFlags;
        unsigned char mWaterDirection;
        std::uint16_t mUnknownS3;

        BSWaterShaderProperty(NiStream& stream, const NiModel& model);
    };

    struct NiAlphaProperty : public NiProperty
    {
        std::uint16_t mFlags;
        unsigned char mThreshold;

        NiAlphaProperty(NiStream& stream, const NiModel& model);
    };

    struct NiDitherProperty : public NiProperty
    {
        std::uint16_t mFlags;

        NiDitherProperty(NiStream& stream, const NiModel& model);
    };

    struct NiFogProperty : public NiProperty
    {
        std::uint16_t mFlags;
        float         mFogDepth;
        Ogre::Vector3 mFogColor;

        NiFogProperty(NiStream& stream, const NiModel& model);
    };

    struct NiMaterialProperty : public NiProperty
    {
        std::uint16_t mFlags;

        Ogre::Vector3 mAmbientColor;
        Ogre::Vector3 mDiffuseColor;
        Ogre::Vector3 mSpecularColor;
        Ogre::Vector3 mEmissiveColor;
        float mGlossiness;
        float mAlpha;

        NiMaterialProperty(NiStream& stream, const NiModel& model);
    };

    struct NiShadeProperty : public NiProperty
    {
        std::uint16_t mFlags;

        NiShadeProperty(NiStream& stream, const NiModel& model);
    };

    struct NiSpecularProperty : public NiProperty
    {
        std::uint16_t mFlags;

        NiSpecularProperty(NiStream& stream, const NiModel& model);
    };

    struct NiStencilProperty : public NiProperty
    {
        std::uint16_t mFlags;

        unsigned char mStencilEnabled;

        std::uint32_t mStencilFunction;
        std::uint32_t mStencilRef;
        std::uint32_t mStencilMask;
        std::uint32_t mFailAction;
        std::uint32_t mZFailAction;
        std::uint32_t mZPassAction;
        std::uint32_t mDrawMode;

        NiStencilProperty(NiStream& stream, const NiModel& model);
    };

    struct NiTexturingProperty : public NiProperty
    {
        struct TexDesc
        {
            typedef std::uint32_t TexClampMode;
            typedef std::uint32_t TexFilterMode;

            NiSourceTextureRef mSourceIndex;

            TexClampMode clampMode;
            TexFilterMode filterMode;
            std::uint32_t uvSet;
            std::uint16_t flags;
            std::int16_t unknownShort;

            bool hasTextureTransform;
            TexCoord translation;
            TexCoord tiling;
            float wRotation;
            std::uint32_t transformType;
            TexCoord centerOffset;

            void read(NiStream& stream);
        };

        std::uint16_t mFlags;
        std::uint32_t mApplyMode;
        std::uint32_t mTextureCount;

        struct ShaderTexDesc
        {
            bool isused;
            TexDesc textureData;
            std::uint32_t mapIndex;
        };

        bool mHasBaseTexture;
        TexDesc mBaseTexture;

        bool mHasDarkTexture;
        TexDesc mDarkTexture;

        bool mHasDetailTexture;
        TexDesc mDetailTexture;

        bool mHasGlossTexture;
        TexDesc mGlossTexture;

        bool mHasGlowTexture;
        TexDesc mGlowTexture;

        bool mHasBumpMapTexture;
        TexDesc mBumpMapTexture;

        bool mHasNormalTexture;
        TexDesc mNormalTexture;

        bool mHasUnknown2Texture;
        TexDesc mUnknown2Texture;

        bool mHasDecal0Texture;
        TexDesc mDecal0Texture;

        bool mHasDecal1Texture;
        TexDesc mDecal1Texture;

        bool mHasDecal2Texture;
        TexDesc mDecal2Texture;

        bool mHasDecal3Texture;
        TexDesc mDecal3Texture;

        std::vector<ShaderTexDesc> mShaderTextures;

        NiTexturingProperty(NiStream& stream, const NiModel& model);
    };

    struct NiVertexColorProperty : public NiProperty
    {
        std::uint16_t mFlags;
        std::uint32_t mVertexMode;
        std::uint32_t mLightingMode;

        NiVertexColorProperty(NiStream& stream, const NiModel& model);
    };

    struct NiWireframeProperty : public NiProperty
    {
        std::uint16_t mFlags;

        NiWireframeProperty(NiStream& stream, const NiModel& model);
    };

    struct NiZBufferProperty : public NiProperty
    {
        std::uint16_t mFlags;
        std::uint32_t mFunction;

        NiZBufferProperty(NiStream& stream, const NiModel& model);
    };
}

#endif // NIBTOGRE_NIPROPERTY_H
