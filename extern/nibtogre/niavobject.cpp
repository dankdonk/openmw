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
#include "niavobject.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "nistream.hpp"

NiBtOgre::NiAVObject::NiAVObject(NiStream& stream, const NiModel& model)
    : NiObjectNET(stream, model), mHasBoundingBox(false)
{
    stream.read(mFlags);

    if (stream.nifVer() >= 0x14020007 && (stream.userVer() >= 11 || stream.userVer2() > 26)) // from 20.2.0.7
        stream.skip(sizeof(std::uint16_t));

    stream.read(mTranslation);
    stream.read(mRotation);
    stream.read(mScale);

    if (stream.nifVer() <= 0x04020200) // up to 4.2.2.0
        stream.read(mVelocity);

    if (stream.nifVer() < 0x14020007 || stream.userVer() <= 11) // less than 20.2.0.7 (or user version <= 11)
        stream.readVector<NiPropertyRef>(mProperty);

    if (stream.nifVer() <= 0x04020200) // up to 4.2.2.0
    {
        if(mHasBoundingBox = stream.getBool())
        {
            stream.read(mBoundingBox.unknownInt);
            stream.read(mBoundingBox.translation);
            stream.read(mBoundingBox.rotation);
            stream.read(mBoundingBox.radius);
        }
    }

    if (stream.nifVer() >= 0x0a000100) // from 10.0.1.0
        stream.read(mCollisionObjectIndex);
}

NiBtOgre::NiCamera::NiCamera(NiStream& stream, const NiModel& model)
    : NiAVObject(stream, model), mUseOrthographicProjection(false)
{
    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        stream.skip(sizeof(std::uint16_t));

    stream.read(mFrustumLeft);
    stream.read(mFrustumRight);
    stream.read(mFrustumTop);
    stream.read(mFrustumBottom);
    stream.read(mFrustumNear);
    stream.read(mFrustumFar);

    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        mUseOrthographicProjection = stream.getBool();

    stream.read(mViewportLeft);
    stream.read(mViewportRight);
    stream.read(mViewportTop);
    stream.read(mViewportBottom);

    stream.read(mLODAdjust);

    stream.skip(sizeof(std::uint32_t)); // Unknown Ref
    stream.skip(sizeof(std::uint32_t)); // Unknown Int
    if (stream.nifVer() >= 0x04020100) // from 4.2.1.0
        stream.skip(sizeof(std::uint32_t)); // Unknown Int2
}

NiBtOgre::NiDynamicEffect::NiDynamicEffect(NiStream& stream, const NiModel& model)
    : NiAVObject(stream, model), mSwitchState(false)
{
    mSwitchState = stream.getBool();
    stream.readVector<NiAVObjectRef>(mAffectedNodes);
}

NiBtOgre::NiLight::NiLight(NiStream& stream, const NiModel& model)
    : NiDynamicEffect(stream, model)
{
    stream.read(mDimmer);
    stream.read(mAmbientColor);
    stream.read(mDiffuseColor);
    stream.read(mSpecularColor);
}

NiBtOgre::NiTextureEffect::NiTextureEffect(NiStream& stream, const NiModel& model)
    : NiDynamicEffect(stream, model)
{
    stream.read(mModelProjectionMatrix);
    stream.read(mModelProjectionTransform);

    stream.read(mTextureFiltering);
    stream.read(mTextureClamping);
    stream.read(mTextureType);
    stream.read(mCoordGenType);

    stream.read(mSourceTexture); // from 4.0.0.0

    stream.read(mClippingPlane);
    stream.skip(4*sizeof(float)); // Unknown Vector3 and float

    if (stream.nifVer() <= 0x0a020000) // up to 10.2.0.0
    {
        stream.skip(sizeof(std::int16_t)); // 0?
        stream.skip(sizeof(std::int16_t)); // -75?
    }

    if (stream.nifVer() <= 0x0401000c) // up to 4.1.0.12
        stream.skip(sizeof(std::uint16_t));
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiGeometry::NiGeometry(NiStream& stream, const NiModel& model)
    : NiAVObject(stream, model), mHasShader(false), mDirtyFlag(false)
{
    stream.read(mDataIndex);
    stream.read(mSkinInstanceIndex);

    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7 (Skyrim)
    {
        std::uint32_t numMaterials;
        stream.read(numMaterials);

        mMaterialName.resize(numMaterials);
        for (unsigned int i = 0; i < numMaterials; ++i)
            stream.readLongString(mMaterialName.at(i));

        mMaterialExtraData.resize(numMaterials);
        for (unsigned int i = 0; i < numMaterials; ++i)
            stream.read(mMaterialExtraData.at(i));

        //mMaterials.resize(numMaterials);
        stream.skip(sizeof(std::uint32_t)); // active material?
    }

    if (stream.nifVer() >= 0x0a000100 && stream.nifVer() <= 0x14010003)
    {
        if (mHasShader = stream.getBool())
        {
            stream.readLongString(mShaderName);
            stream.skip(sizeof(std::int32_t)); // unknown integer
        }
    }

    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7 (Skyrim)
    {
        mDirtyFlag = stream.getBool();
        mBSProperties.resize(2);
        stream.read(mBSProperties.at(0));
        stream.read(mBSProperties.at(1));
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiParticleSystem::NiParticleSystem(NiStream& stream, const NiModel& model)
    : NiParticles(stream, model), mWorldSpace(false)
{
    if (stream.userVer() >= 12)
    {
        stream.read(mUnknownS2);
        stream.read(mUnknownS3);
        stream.read(mUnknownI1);
    }

    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
    {
        mWorldSpace = stream.getBool();
        std::uint32_t numModifiers;
        stream.read(numModifiers);
        mModifiers.resize(numModifiers);
        for (unsigned int i = 0; i < numModifiers; ++i)
            stream.read(mModifiers.at(i));
    }
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLODTriShape::BSLODTriShape(NiStream& stream, const NiModel& model)
    : NiGeometry(stream, model)
{
    stream.read(mLevel0Size);
    stream.read(mLevel1Size);
    stream.read(mLevel2Size);
}

NiBtOgre::NiNode::NiNode(NiStream& stream, const NiModel& model)
    : NiAVObject(stream, model)
{
    stream.readVector<NiAVObjectRef>(mChildren);
    stream.readVector<NiDynamicEffectRef>(mEffects);
#if 0
    // Discard tranformations for the root node, otherwise some meshes
    // occasionally get wrong orientation. Only for NiNode-s for now, but
    // can be expanded if needed.
    if (0 == recIndex && stream.nifVer <= 0x04010000) // FIXME experiment
    {
        if (static_cast<Nif::Node*>(this)->trafo.rotation != Nif::Transformation::getIdentity().rotation)
            std::cout << "Non-identity rotation: " << this->name << ", ver " << std::hex << nifVer << std::endl;
        static_cast<Nif::Node*>(this)->trafo = Nif::Transformation::getIdentity();
    }
#endif
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSBlastNode::BSBlastNode(NiStream& stream, const NiModel& model)
    : NiNode(stream, model)
{
    stream.read(mUnknown1);
    stream.read(mUnknown2);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSDamageStage::BSDamageStage(NiStream& stream, const NiModel& model)
    : NiNode(stream, model)
{
    stream.read(mUnknown1);
    stream.read(mUnknown2);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSMultiBoundNode::BSMultiBoundNode(NiStream& stream, const NiModel& model)
    : NiNode(stream, model)
{
    stream.read(mMultiBoundIndex);
    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7
        stream.read(mUnknown);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSOrderedNode::BSOrderedNode(NiStream& stream, const NiModel& model)
    : NiNode(stream, model)
{
    stream.read(mAlphaSortBound);
    stream.read(mIsStaticBound);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSTreeNode::BSTreeNode(NiStream& stream, const NiModel& model)
    : NiNode(stream, model)
{
    stream.readVector<NiNodeRef>(mBones1);
    stream.readVector<NiNodeRef>(mBones2);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSValueNode::BSValueNode(NiStream& stream, const NiModel& model)
    : NiNode(stream, model)
{
    stream.read(mValue);

    stream.skip(sizeof(char)); // unknown byte
}

NiBtOgre::NiBillboardNode::NiBillboardNode(NiStream& stream, const NiModel& model)
    : NiNode(stream, model)
{
    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        stream.read(mBillboardMode);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiSwitchNode::NiSwitchNode(NiStream& stream, const NiModel& model)
    : NiNode(stream, model)
{
    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        stream.read(mUnknownFlags);

    stream.read(mUnknownInt);
}
