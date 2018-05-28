/*
  Copyright (C) 2015-2018 cc9cii

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

  Much of the information on the NIF file structures are based on the NifSkope
  documenation.  See http://niftools.sourceforge.net/wiki/NifSkope for details.

*/
#include "niproperty.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "nistream.hpp"

#if 0
// Seen in NIF version 20.2.0.7
NiBtOgre::NiProperty::NiProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiObjectNET(index, stream, model), mIsBSLightingShaderProperty(false)
{
}
#endif

// Seen in NIF version 20.2.0.7
NiBtOgre::BSEffectShaderProperty::BSEffectShaderProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    stream.read(mShaderFlags1);
    stream.read(mShaderFlags2);
    stream.read(mUVOffset.u);
    stream.read(mUVOffset.v);
    stream.read(mUVScale.u);
    stream.read(mUVScale.v);
    stream.readSizedString(mSourceTexture);
    stream.read(mTextureClampMode);
    stream.read(mFalloffStartAngle);
    stream.read(mFalloffStopAngle);
    stream.read(mFalloffStartOpacity);
    stream.read(mFalloffStopOpacity);
    stream.read(mEmissiveColor);
    stream.read(mEmissiveMultiple);
    stream.read(mSoftFalloffDepth);
    stream.readSizedString(mGreyscaleTexture);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLightingShaderProperty::BSLightingShaderProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model, true)
{
    if (stream.userVer() == 12)
    {
        stream.read(mShaderFlags1);
        stream.read(mShaderFlags2);
    }
    stream.read(mUVOffset.u);
    stream.read(mUVOffset.v);
    stream.read(mUVScale.u);
    stream.read(mUVScale.v);
    stream.read(mTextureSetIndex);
    stream.read(mEmissiveColor);
    stream.read(mEmissiveMultiple);
    stream.read(mTextureClampMode);
    stream.read(mAlpha);
    stream.read(mUnknown2);
    stream.read(mGlossiness);
    stream.read(mSpecularColor);
    stream.read(mSpecularStrength);
    stream.read(mLightingEffect1);
    stream.read(mLightingEffect2);

    switch (mSkyrimShaderType)
    {
        case 1: // Environment Map
            stream.read(mEnvironmentMapScale);
            break;
        case 5: // Skin Tint
            stream.read(mSkinTintColor);
            break;
        case 6: // Hair Tint
            stream.read(mHairTintColor);
            break;
        case 7: // Parallax Occ Material
            stream.read(mMaxPasses);
            stream.read(mScale);
            break;
        case 11: // MultiLayer Parallax
            stream.read(mParallaxInnerLayerThickness);
            stream.read(mParallaxRefractionScale);
            stream.read(mParallaxInnerLayerTextureScale.u);
            stream.read(mParallaxInnerLayerTextureScale.v);
            stream.read(mParallaxEnvmapStrength);
            break;
        case 14: // Sparkle Snow
            stream.read(mSparkleParameters);
            break;
        case 16: // Eye Envmap
            stream.read(mEyeCubemapScale);
            stream.read(mLeftEyeReflectionCenter);
            stream.read(mRightEyeReflectionCenter);
            break;
        default:
            break;
    }
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSWaterShaderProperty::BSWaterShaderProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    stream.read(mShaderFlags1);
    stream.read(mShaderFlags2);
    stream.read(mUVOffset.u);
    stream.read(mUVOffset.v);
    stream.read(mUVScale.u);
    stream.read(mUVScale.v);
    stream.read(mWaterShaderFlags);
    stream.read(mWaterDirection);
    stream.read(mUnknownS3);
}

NiBtOgre::NiAlphaProperty::NiAlphaProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    stream.read(mFlags);
    stream.read(mThreshold);
}

NiBtOgre::NiDitherProperty::NiDitherProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    stream.read(mFlags);
}

NiBtOgre::NiFogProperty::NiFogProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    stream.read(mFlags);
    stream.read(mFogDepth);
    stream.read(mFogColor);
}

NiBtOgre::NiMaterialProperty::NiMaterialProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    if (stream.nifVer() <= 0x0a000102) // 10.0.1.2
        stream.read(mFlags);

    stream.read(mAmbientColor);
    stream.read(mDiffuseColor);
    stream.read(mSpecularColor);
    stream.read(mEmissiveColor);
    stream.read(mGlossiness);
    stream.read(mAlpha);
    // FIXME: mEmitMulti
}

NiBtOgre::NiShadeProperty::NiShadeProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    stream.read(mFlags);
}

NiBtOgre::NiSpecularProperty::NiSpecularProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    stream.read(mFlags);
}

NiBtOgre::NiStencilProperty::NiStencilProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    if (stream.nifVer() <= 0x0a000102) // 10.0.1.2
        stream.read(mFlags);

    stream.read(mStencilEnabled);
    stream.read(mStencilFunction);
    stream.read(mStencilRef);
    stream.read(mStencilMask);
    stream.read(mFailAction);
    stream.read(mZFailAction);
    stream.read(mZPassAction);
    stream.read(mDrawMode);
}

void NiBtOgre::NiTexturingProperty::TexDesc::read(NiStream& stream)
{
    stream.read(mSourceIndex);

    if (stream.nifVer() <= 0x14000005) // up to 20.0.0.5
    {
        stream.read(clampMode);
        stream.read(filterMode);
    }

    if (stream.nifVer() >= 0x14010003) // from 20.1.0.3
        stream.read(flags);

    //if (stream.nifVer() >= 0x14060000) // from 20.6.0.0 (unsupported version, commented out)
        //stream.read(unknownShort);

    if (stream.nifVer() <= 0x14000005) // up to 20.0.0.5
        stream.read(uvSet);

    if (stream.nifVer() <= 0x0a040001) // up to 10.4.0.1
    {
        stream.skip(sizeof(std::int16_t)*2);
    }

    if (stream.nifVer() <= 0x0401000c) // up to 4.1.0.12
        stream.skip(sizeof(std::uint16_t)); // Unknown1

    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
    {
        if (hasTextureTransform = stream.getBool())
        {
            stream.read(translation.u);
            stream.read(translation.v);
            stream.read(tiling.u);
            stream.read(tiling.v);
            stream.read(wRotation);
            stream.read(transformType);
            stream.read(centerOffset.u);
            stream.read(centerOffset.v);
        }
    }
}

NiBtOgre::NiTexturingProperty::NiTexturingProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    if (stream.nifVer() <= 0x0a000102 || stream.nifVer() >= 0x14010003) // up to 10.0.1.2 or from 20.1.0.3
        stream.read(mFlags);

    if (stream.nifVer() <= 0x14000005) // up to 20.0.0.5
        stream.read(mApplyMode);

    stream.read(mTextureCount);
    if (mTextureCount != 7) // FIXME
        std::cout << "NiTexturingProperty::texture count: " << mTextureCount << std::endl;

    if (mHasBaseTexture = stream.getBool())
        mBaseTexture.read(stream);

    if (mHasDarkTexture = stream.getBool())
        mDarkTexture.read(stream);

    if (mHasDetailTexture = stream.getBool())
        mDetailTexture.read(stream);

    if (mHasGlossTexture = stream.getBool())
        mGlossTexture.read(stream);

    if (mHasGlowTexture = stream.getBool())
        mGlowTexture.read(stream);

    if (mHasBumpMapTexture = stream.getBool())
    {
        mBumpMapTexture.read(stream);

        stream.skip(sizeof(float));   // Bump Map Luma Scale
        stream.skip(sizeof(float));   // Bump Map Luma Offset
        stream.skip(sizeof(float)*4); // Bump Map Matrix
    }

    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7
    {
        if (mHasNormalTexture = stream.getBool())
            mNormalTexture.read(stream);

        if (mHasUnknown2Texture = stream.getBool())
        {
            mUnknown2Texture.read(stream);
            stream.skip(sizeof(float));
        }
    }

    if (mHasDecal0Texture = stream.getBool())
        mDecal0Texture.read(stream);

    unsigned int offset = 0;
    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7
        offset = 2;

    if (mTextureCount >= 8+offset)
        if (mHasDecal1Texture = stream.getBool())
            mDecal1Texture.read(stream);

    if (mTextureCount >= 9+offset)
        if (mHasDecal2Texture = stream.getBool())
            mDecal2Texture.read(stream);

    if (mTextureCount >= 10+offset)
        if (mHasDecal3Texture = stream.getBool())
            mDecal3Texture.read(stream);

    if (stream.nifVer() >= 0x0a000100) // 10.0.1.0
    {
        std::uint32_t numShaderTextures;
        stream.read(numShaderTextures);
        mShaderTextures.resize(numShaderTextures);
        for (unsigned int i = 0; i < numShaderTextures; ++i)
        {
            mShaderTextures[i].textureData.read(stream);
            stream.read(mShaderTextures[i].mapIndex);
        }
    }
}

NiBtOgre::NiVertexColorProperty::NiVertexColorProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    stream.read(mFlags);
    stream.read(mVertexMode);
    stream.read(mLightingMode);
}

NiBtOgre::NiWireframeProperty::NiWireframeProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    stream.read(mFlags);
}

NiBtOgre::NiZBufferProperty::NiZBufferProperty(uint32_t index, NiStream& stream, const NiModel& model)
    : NiProperty(index, stream, model)
{
    stream.read(mFlags);

    if (stream.nifVer() >= 0x0401000c && stream.nifVer() <= 0x14000005) // 4.1.0.12 to 20.0.0.5
        stream.read(mFunction);
}
