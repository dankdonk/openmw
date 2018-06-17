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
#include "niavobject.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "nistream.hpp"
#include "nimodel.hpp"
#include "nidata.hpp"
#include "btogreinst.hpp"

NiBtOgre::NiAVObject::NiAVObject(uint32_t index, NiStream& stream, const NiModel& model)
    : NiObjectNET(index, stream, model), mHasBoundingBox(false)//, mWorldTransform(Ogre::Matrix4::IDENTITY)
{
    stream.read(mFlags);

    if (stream.nifVer() >= 0x14020007 && (stream.userVer() >= 11 && stream.userVer2() > 26)) // from 20.2.0.7
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
        // Looks like only used for animations, examples include:
        //   ./r/xashslave.nif
        //   ./r/xbabelfish.nif
        //   ./r/xcavemudcrab.nif
        //   ./r/xcliffracer.nif
        //   ./r/xcorprus_stalker.nif
        //   ./r/xguar.nif
        //   ./r/xkwama forager.nif
        //   ./r/xminescrib.nif
        //   ./r/xrust rat.nif
        //   ./r/xscamp_fetch.nif
        //   ./r/xslaughterfish.nif
        //   ./xbase_anim.1st.nif
        //   ./xbase_anim.nif
        //   ./xbase_animkna.nif
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

void NiBtOgre::NiAVObject::build(BtOgreInst *inst, NiObject *parent)
{
}

NiBtOgre::NiCamera::NiCamera(uint32_t index, NiStream& stream, const NiModel& model)
    : NiAVObject(index, stream, model), mUseOrthographicProjection(false)
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

void NiBtOgre::NiCamera::build(BtOgreInst *inst, NiObject *parent)
{
}

NiBtOgre::NiDynamicEffect::NiDynamicEffect(uint32_t index, NiStream& stream, const NiModel& model)
    : NiAVObject(index, stream, model), mSwitchState(false)
{
    if (stream.nifVer() >= 0x0a01006a) // from 10.1.0.106
        mSwitchState = stream.getBool();
    // TODO: how to decode the pointers in ver 4.0.0.2
    stream.readVector<NiAVObjectRef>(mAffectedNodes);
}

NiBtOgre::NiLight::NiLight(uint32_t index, NiStream& stream, const NiModel& model)
    : NiDynamicEffect(index, stream, model)
{
    stream.read(mDimmer);
    stream.read(mAmbientColor);
    stream.read(mDiffuseColor);
    stream.read(mSpecularColor);
}

NiBtOgre::NiTextureEffect::NiTextureEffect(uint32_t index, NiStream& stream, const NiModel& model)
    : NiDynamicEffect(index, stream, model)
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

NiBtOgre::NiGeometry::NiGeometry(uint32_t index, NiStream& stream, const NiModel& model)
    : NiAVObject(index, stream, model), mHasShader(false), mDirtyFlag(false)
{
#if 0
    // Some NiTriShapes are "Shadow", possibly simplified mesh for animated (i.e. non-static)
    // objects to cast shadows
    if ((mFlags & 0x40) != 0) // FIXME: testing only, 67 == 0x43, 69 = 0x45
    {
        std::cout << "Shadow : " << model.getName() << " : " << model.getLongString(mName) << std::endl;
    }
#endif
    stream.read(mDataIndex);
    stream.read(mSkinInstanceIndex);

    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7 (TES5)
    {
        std::uint32_t numMaterials;
        stream.read(numMaterials);

        if (numMaterials != 0)
        {
            mMaterialName.resize(numMaterials);
            for (unsigned int i = 0; i < numMaterials; ++i)
                stream.readLongString(mMaterialName.at(i));

            mMaterialExtraData.resize(numMaterials);
            for (unsigned int i = 0; i < numMaterials; ++i)
                stream.read(mMaterialExtraData.at(i));
        }

        stream.skip(sizeof(std::int32_t)); // active material?
    }

    if (stream.nifVer() >= 0x0a000100 && stream.nifVer() <= 0x14010003)
    {
        if (mHasShader = stream.getBool())
        {
            stream.readLongString(mShaderName);
            stream.skip(sizeof(std::int32_t)); // unknown integer
        }
    }

    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7 (TES5)
    {
        mDirtyFlag = stream.getBool();
        if (stream.userVer() == 12) // not present in FO3?
        {
            mBSProperties.resize(2);
            stream.read(mBSProperties.at(0));
            stream.read(mBSProperties.at(1));
        }
    }
}

// build Ogre mesh and apply shader/material using scene
//
// 1. get data and skin objects
// 2. get shader if present
// 3. TES5 has more
//
// Should check if animated, has bones and/or has skin (suspect static objects do not have these)
//
// Some NiTriStrips have NiBinaryExtraData (tangent space) - not sure what to do with them
void NiBtOgre::NiGeometry::build(BtOgreInst *inst, NiObject *parent)
{
}

// FIXME: maybe make this one for TES5 instead?
void NiBtOgre::NiGeometry::buildTES3(Ogre::SceneNode *sceneNode, BtOgreInst *inst, NiObject *parent)
{
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiParticleSystem::NiParticleSystem(uint32_t index, NiStream& stream, const NiModel& model)
    : NiParticles(index, stream, model), mWorldSpace(false)
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

void NiBtOgre::NiParticleSystem::build(BtOgreInst *inst, NiObject *parent)
{
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSLODTriShape::BSLODTriShape(uint32_t index, NiStream& stream, const NiModel& model)
    : NiGeometry(index, stream, model)
{
    stream.read(mLevel0Size);
    stream.read(mLevel1Size);
    stream.read(mLevel2Size);
}

void NiBtOgre::BSLODTriShape::build(BtOgreInst *inst, NiObject *parent)
{
}

// TES3: if one of the children is a RootCollisionNode, generate collision shape differently?
//       e.g. ./x/ex_hlaalu_win_01.nif (what about if it has a bounding box?)
//       RootCollisionNode seems to be the last of the children
//       (see ManualBulletShapeLoader::hasAutoGeneratedCollision and
//       NifOgre::NIFObjectLoader::createObjects)
//
// TES4/5: - use bhk* objects for collision
//         - each root node need to check name for "marker_" or "Creature_Marker"
//         or "Marker" or "Target" (seems inefficient, maybe compare 64bit binary first?)
//         ??? How to handle "Scene Root" for marker_map.nif ???
NiBtOgre::NiNode::NiNode(uint32_t index, NiStream& stream, const NiModel& model)
    : NiAVObject(index, stream, model)
{
    stream.readVector<NiAVObjectRef>(mChildren);
    stream.readVector<NiDynamicEffectRef>(mEffects);
}

//   name string
//   extra data (e.g. BSX Flags)
//   controller(s)
//   flags
//   translation, rotation, scale
//   properties
//   bounding box (sometimes)
//   list of child objects
//   list of objects with dynamic effects
//
// With the above information, need figure out what needs to be built.
//
// Maybe another parameter is needed to provide a hint? i.e. the caller probably knows what
// type of object is being built.
//
//   e.g. animation, static objects, dynamic objects or a combination
//
// BSX Flags for controlling animation and collision (from niflib):
//
//           TES4                  TES5
//           --------------------- ------------
//   Bit 0 : enable havok          Animated
//   Bit 1 : enable collision      Havok
//   Bit 2 : is skeleton nif?      Ragdoll
//   Bit 3 : enable animation      Complex
//   Bit 4 : FlameNodes present    Addon
//   Bit 5 : EditorMarkers present
//   Bit 6 :                       Dynamic
//   Bit 7 :                       Articulated
//   Bit 8 :                       IKTarget
//   Bit 9 :                       Unknown
//
// necromancer/hood_gnd.nif is 0x0b, i.e. 1011 - animation, collision, havok
void NiBtOgre::NiNode::build(BtOgreInst *inst, NiObject* parent)
{
    if (mCollisionObjectIndex != -1 || mChildren.size() > 0) // build only if it will be used
    {
        Ogre::Matrix4 localTransform = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
        localTransform.makeTransform(mTranslation, Ogre::Vector3(mScale), Ogre::Quaternion(mRotation));

        if (parent != nullptr)
            mWorldTransform = static_cast<NiAVObject*>(parent)->getWorldTransform() * localTransform;
        else
            mWorldTransform = localTransform;
    }
    //else
        //mWorldTransform = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);

    int flags = inst->mFlags;

    // Should not have TES3 NIF versions since NifOgre::NIFObjectLoader::load() checks before
    // calling this method, at least while testing
    if (mModel.nifVer() <= 0x04000002) // up to 4.0.0.2
        return buildTES3(inst->mBaseNode, inst);

    // FIXME: should create a child Ogre::SceneNode here using the NiNode's translation, etc?
    // FIXME: apply any properties (? do this first and pass on for building children?)
    bool editorMarkerPresent = (flags & Flag_EditorMarkerPresent) != 0;
    bool enableCollision = (flags & Flag_EnableCollision) != 0;
    // temp debugging note: mExtraDataIndexList is from NiObjectNET
    // TODO: consider removing mExtraDataIndex and just use a vector of size 1 for older NIF versions
    for (unsigned int i = 0; i < mExtraDataIndexList.size(); ++i)
    {
        if (mExtraDataIndexList[i] == -1) // TODO: check if this ever happens (i.e. ref -1)
            continue;

        const std::string& name
            = mModel.getLongString(mModel.getRef<NiExtraData>((int32_t)mExtraDataIndexList[i])->mName);

        if (name == "BSX") // TODO: only for root objects?
        {
            std::uint32_t bsx
                = mModel.getRef<NiIntegerExtraData>((int32_t)mExtraDataIndexList[i])->mIntegerData;

            enableCollision = (bsx & 0x02) != 0; // FIXME: different for TES5
            if (enableCollision)
                flags &= Flag_EnableCollision;

            if ((bsx & 0x01) != 0)               // FIXME: different for TES5
                flags &= Flag_EnableHavok;
        }
        else if (name == "UPB") // usually present for collision nodes, AttachLight, FlameNode2 FlameNode0
        {
            // check for overrides, billboard node stuff, etc (delimited by \r\n)
            //
            // zMode10
            // billboardUp
            // Collision_Groups = 0
            // Mass = 0.000000
            // Ellasticity = 0.300000
            // Friction = 0.300000
            // Unyielding = 0
            // Simulation_Geometry = 2
            // Proxy_Geometry = <None>
            // Use_Display_Proxy = 0
            // Display_Children = 1
            // Disable_Collisions = 0
            // Inactive = 0
            // Display_Proxy = <None>
            StringIndex stringIndex
                = mModel.getRef<NiStringExtraData>((int32_t)mExtraDataIndexList[i])->mStringData;
            const std::string& upb = mModel.getLongString(stringIndex);
            // TODO: split the string into a map
        }
        else if (name == "BBX") // BSBound
        {
            // bounding box seen in skeleton.nif
        }
        else if (name == "FRN") // BSFurnitureMarker
        {
        }
        else if (name == "Prn") // NiStringExtraData, e.g. armor/legionhorsebackguard/helmet.nif
        {
            // Seems to point to a Bone attach point? e.g. "Bip01 Head"
            StringIndex stringIndex
                = mModel.getRef<NiStringExtraData>((int32_t)mExtraDataIndexList[i])->mStringData;
            const std::string& prn = mModel.getLongString(stringIndex);
        }
        else
        {
            // FIXME: need another way of dealing with NiExtraData
            //
            // Some NiStringExtraData have no name, e.g. ./architecture/cathedral/crypt/cathedralcryptlight02.nif
            // object index 130 that has "sgoKeep=1" (should have been UPB)
            //
            // Maybe keep a type info in NiExtraData object (faster and more accurate than
            // string search each time)

            //std::cout << "Unhandled ExtraData: " << name << std::endl;
        }
    }

    inst->mFlags = flags;

    // temp debugging note: woc "icmarketdistrict" 8 16
    //    meshes/architecture/imperialcity/icwalltower01.nif has a collision shape and
    //    calls bhkCollisionObject::build() which is currently WIP
    //
    // Should call build() or another method that returns a collision shape?
    //
    //
    // Current implementation updates NifBullet::ManualBulletShapeLoader::mShape, which is
    // OEngine::Physic::BulletShape, with generated bullet collision objects.
    // For ease of porting things over it might be worth while doing the same for now.
    //
    //
    //
    // the collision object might be attached to one of the children, see necromancer/hood_gnd.nif
    if (enableCollision && mCollisionObjectIndex != -1)
        mModel.getRef<NiObject>((int32_t)mCollisionObjectIndex)->build(inst, this);

    for (unsigned int i = 0; i < mChildren.size(); ++i)
    {
        if (mChildren[i] == -1) // no object
            continue;

        // FIXME: need to be able to support OpenCS, but how the caller instructs to render?
        if (editorMarkerPresent)
        {
            const std::string& nodeName
                = mModel.getLongString(mModel.getRef<NiAVObject>((int32_t)mChildren[i])->getNameIndex());
            if (nodeName == "EditorMarker")
                continue;
        }

        mModel.getRef<NiObject>((int32_t)mChildren[i])->build(inst, this);
    }

    for (unsigned int i = 0; i < mEffects.size(); ++i)
    {
        if (mEffects[i] == -1) // no object
            continue;

        mModel.getRef<NiObject>((int32_t)mEffects[i])->build(inst, this);
    }

}

// 1. check extra data for MRK, NCO, etc
// 2. check if the last of the children is RootCollisionNode to build collision object
// 3. build each of the children
// 3.1  apply translation, rotation and scale to a child sceneNode?
// 4. build each of the dynamic effects
// 5. apply any properties (? do this first and pass on for building children?)
// ?. what do do with any bounding boxes?
void NiBtOgre::NiNode::buildTES3(Ogre::SceneNode* sceneNode, BtOgreInst *inst, NiObject *parent)
{
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSBlastNode::BSBlastNode(uint32_t index, NiStream& stream, const NiModel& model)
    : NiNode(index, stream, model)
{
    stream.read(mUnknown1);
    stream.read(mUnknown2);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSDamageStage::BSDamageStage(uint32_t index, NiStream& stream, const NiModel& model)
    : NiNode(index, stream, model)
{
    stream.read(mUnknown1);
    stream.read(mUnknown2);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSMultiBoundNode::BSMultiBoundNode(uint32_t index, NiStream& stream, const NiModel& model)
    : NiNode(index, stream, model)
{
    stream.read(mMultiBoundIndex);
    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7
        stream.read(mUnknown);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSOrderedNode::BSOrderedNode(uint32_t index, NiStream& stream, const NiModel& model)
    : NiNode(index, stream, model)
{
    stream.read(mAlphaSortBound);
    stream.read(mIsStaticBound);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSTreeNode::BSTreeNode(uint32_t index, NiStream& stream, const NiModel& model)
    : NiNode(index, stream, model)
{
    stream.readVector<NiNodeRef>(mBones1);
    stream.readVector<NiNodeRef>(mBones2);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSValueNode::BSValueNode(uint32_t index, NiStream& stream, const NiModel& model)
    : NiNode(index, stream, model)
{
    stream.read(mValue);

    stream.skip(sizeof(char)); // unknown byte
}

NiBtOgre::NiBillboardNode::NiBillboardNode(uint32_t index, NiStream& stream, const NiModel& model)
    : NiNode(index, stream, model)
{
    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        stream.read(mBillboardMode);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiSwitchNode::NiSwitchNode(uint32_t index, NiStream& stream, const NiModel& model)
    : NiNode(index, stream, model)
{
    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        stream.read(mUnknownFlags);

    stream.read(mUnknownInt);
}
