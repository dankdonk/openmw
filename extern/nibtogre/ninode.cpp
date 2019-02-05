/*
  Copyright (C) 2015-2019 cc9cii

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
#include "ninode.hpp"

#include <cassert>
#include <stdexcept>
#include <memory>
#include <iostream> // FIXME: debugging only

#include <OgreSkeleton.h>
#include <OgreBone.h>
#include <OgreMesh.h>

#include "nistream.hpp"
#include "nimodel.hpp"
#include "nitimecontroller.hpp"
#include "nidata.hpp"
#include "btogreinst.hpp"
#include "nicollisionobject.hpp"
#include "nigeometry.hpp"
#include "boundsfinder.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

// TES3: if one of the children is a RootCollisionNode, generate collision shape differently?
//       e.g. ./x/ex_hlaalu_win_01.nif (what about if it has a bounding box?)
//       RootCollisionNode seems to be the last of the children
//
// TES4/5: - use bhk* objects for collision
//
// The node name is also used as bone names for those meshes with skins.
// Bipeds seems to have a predefined list of bones. See: meshes/armor/legion/m/cuirass.nif
NiBtOgre::NiNode::NiNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiAVObject(index, stream, model, data)
    , mNodeName(model.indexToString(NiObjectNET::mNameIndex)), mData(data)
{
    //stream.readVector<NiAVObjectRef>(mChildren);
    std::uint32_t numChildren = 0;
    stream.read(numChildren);

    mChildren.resize(numChildren);
    for (std::uint32_t i = 0; i < numChildren; ++i)
    {
        stream.read(mChildren.at(i));

        // store node hierarchy in mModel to help find & build skeletons and meshes
        if (mChildren[i] > 0) // ignore if -1 and a child can't have an index of 0
            data.setNiNodeParent(mChildren[i], this);
    }

    stream.readVector<NiDynamicEffectRef>(mEffects);
    // HACK: oar01.nif suggests that 10.0.1.0 reads one entry even if there is none?
    if (stream.nifVer() == 0x0a000100)
        stream.skip(sizeof(NiDynamicEffectRef));

    /* ---------------------------------------------------------------------- */
    // HACK: should check for root node?
    mParent = (NiObject::mSelfIndex == 0) ? nullptr : &data.getNiNodeParent((NiAVObjectRef)NiObject::mSelfIndex);

    if (mCollisionObjectIndex != -1 || mChildren.size() > 0) // build only if it will be used
    {
//      if (!mParent)
//          mLocalTransform.makeTransform(mTranslation, Ogre::Vector3(mScale), Ogre::Quaternion::IDENTITY);
//      else
            mLocalTransform.makeTransform(mTranslation, Ogre::Vector3(mScale), Ogre::Quaternion(mRotation));

        if (mParent)
            mWorldTransform = mParent->getWorldTransform() * mLocalTransform;
        else
            mWorldTransform = mLocalTransform;
    }
    //else
        //mWorldTransform = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
}

void NiBtOgre::NiNode::registerSubMesh(NiTriBasedGeom* geom)
{
    if (!mModel.showEditorMarkers() && mData.mEditorMarkerPresent && (mNodeName == "EditorMarker"))
        return;

    mData.mMeshBuildList[NiObject::mSelfIndex] = this;
    mSubMeshChildren.push_back(geom);
}


//  Some of the Ogre code in this method is based on v0.36 of OpenMW.
void NiBtOgre::NiNode::buildMesh(Ogre::Mesh *mesh)
{
    BoundsFinder bounds;
    bool needTangents = false;

    // create and update (i.e. apply materials, properties and controllers)
    // an Ogre::SubMesh for each in mSubMeshGeometry
    for (size_t i = 0; i < mSubMeshChildren.size(); ++i)
    {
        needTangents |= mSubMeshChildren.at(i)->createSubMesh(mesh, bounds);
    }

    // build tangents if at least one of the sub-mesh's material needs them
    // TODO: is it possible to use the ones in the NIF files?
    if (needTangents)
    {
        unsigned short src, dest;
        if (!mesh->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
            mesh->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
    }

    // for skeleton.nif an empty mesh is created
    if (mSubMeshChildren.size())
    {
        mesh->_setBounds(Ogre::AxisAlignedBox(bounds.minX()-0.5f, bounds.minY()-0.5f, bounds.minZ()-0.5f,
                                              bounds.maxX()+0.5f, bounds.maxY()+0.5f, bounds.maxZ()+0.5f));
        mesh->_setBoundingSphereRadius(bounds.getRadius());
    }
}

// build a hierarchy of bones (i.e. mChildBoneNodes) so that a skeleton can be built, hopefully
// a much smaller subset of the NiNode hierarchy
//
// recursively traverses the NiNode tree until the specified skeletonRoot is found
// childNode is the index of the caller of this method (which should be a child of this node)
void NiBtOgre::NiNode::findBones(const NiNodeRef skeletonRoot, const NiNodeRef childNode)
{
    if (mChildBoneNodes.size() == 0) // implies skeleton root is not yet found
    {
        if (NiObject::index() == skeletonRoot) // am I the one?
        {
            mChildBoneNodes.push_back(childNode);
            return;
        }

        if (mParent == nullptr) // should not happen!
            throw std::runtime_error("NiNode without parent and Skeleton Root not yet found");

        // not skeleton root, keep searching recursively
        mParent->findBones(skeletonRoot, NiObject::index());
        mChildBoneNodes.push_back(childNode);
    }
    else
    {
        if (std::find(mChildBoneNodes.begin(), mChildBoneNodes.end(), childNode) == mChildBoneNodes.end())
            mChildBoneNodes.push_back(childNode); // only if childNode doesn't exist
    }
}

void NiBtOgre::NiNode::findBones(std::int32_t rootIndex)
{
    // TODO: do we need a bone if the NiTransformController's target is the root?
    if (rootIndex != NiObject::index())
        mParent->findBones(rootIndex, NiObject::index());
}

void NiBtOgre::NiNode::addBones(Ogre::Skeleton *skeleton,
        Ogre::Bone *parentBone, std::map<std::uint32_t, std::uint16_t>& indexToHandle)
{
    // FIXME: check if mNodeName can be empty
    if (mNodeName == "")
        throw std::runtime_error("NiNode has empty name");

    // maybe skeleton was built already? but if so why is this method called at all?
    if (/*!parentBone && */skeleton->hasBone(mNodeName))
        return;

    Ogre::Bone *bone = skeleton->createBone(mNodeName);
    indexToHandle[NiObject::index()] = bone->getHandle();

    if (parentBone)
        parentBone->addChild(bone);

    bone->setPosition(NiAVObject::mTranslation);
    bone->setOrientation(NiAVObject::mRotation);
    bone->setScale(Ogre::Vector3(NiAVObject::mScale));
    bone->setBindingPose();

    for (unsigned int i = 0; i < mChildBoneNodes.size(); ++i)
    {
        NiNode* childNode = mModel.getRef<NiNode>(mChildBoneNodes[i]);
        childNode->addBones(skeleton, bone, indexToHandle);
    }
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
void NiBtOgre::NiNode::build(BtOgreInst *inst, ModelData *data, NiObject* parent)
{
    // There doesn't seem to be a flag to indicate an editor marker.  To filter them out, look
    // out for strings starting with:
    //   EditorLandPlane
    //   FurnitureMarker
    //   marker_ (marker_arrow, marker_divine, marker_horse, marker_north, marker_prison,
    //            marker_sound, marker_temple, marker_travel)
    //   Creature_Marker (marker_creature)
    //   GeoSphere01 (marker_radius)
    //   Marker (MarkerTravel, MarkerX, MarkerXHeading)
    //   Target (this one needs to match the full string to distinguish TargetHeavy01)
    //   TargetSafeZone
    //
    // Still leaves marker_map without a suitable string match.
    //
    // Therefore it might be better to match on the mesh filename instead.
    //   editorlandplane
    //   furnituremarker
    //   marker
    //   target
    //   targetsafezone
    //
    // Maybe none of the files in the root Mesh directory should be rendered? Use mModel.getModelName()
    //
    // inst->mFlags should indicate whether editor markers should be ignored (default to
    // ignore)

    //if (mNodeName == "CathedralCryptLight02") // FIXME: testing only
        //std::cout << "light" << std::endl;

    int flags = inst->mFlags;

    // Should not have TES3 NIF versions since NifOgre::NIFObjectLoader::load() checks before
    // calling this method, at least while testing
    if (mModel.nifVer() <= 0x04000002) // up to 4.0.0.2
        return;// buildTES3(inst->mBaseSceneNode, inst);

    // FIXME: should create a child Ogre::SceneNode here using the NiNode's translation, etc?
    // FIXME: apply any properties (? do this first and pass on for building children?)
    bool enableCollision = (flags & Flag_EnableCollision) != 0;
    // temp debugging note: mExtraDataIndexList is from NiObjectNET
    // TODO: consider removing mExtraDataIndex and just use a vector of size 1 for older NIF versions
    for (unsigned int i = 0; i < mExtraDataIndexList.size(); ++i)
    {
        if (mExtraDataIndexList[i] == -1) // TODO: check if this ever happens (i.e. ref -1)
            continue;

        const std::string& name
            = mModel.indexToString(mModel.getRef<NiExtraData>((int32_t)mExtraDataIndexList[i])->mName);

        if (name == "BSX") // TODO: only for root objects?
        {
            std::uint32_t bsx
                = mModel.getRef<NiIntegerExtraData>((int32_t)mExtraDataIndexList[i])->mIntegerData;

            if ((bsx & 0x01) != 0)               // FIXME: different for TES5
                flags |= Flag_EnableHavok;

            enableCollision = (bsx & 0x02) != 0; // FIXME: different for TES5
            if (enableCollision)
                flags |= Flag_EnableCollision;

            if ((bsx & 0x08) != 0)               // FIXME: different for TES5
                flags |= Flag_EnableAnimation;

            // Ragdoll (both Animation and Havok enabled):
            //     TargetHeavy01, CathedralCryptLight02
            //
            // Not animated despite the flag:
            //     ICDoor04, UpperChest02 - maybe also need to check if a time controller is present?
            //     (or just assign bones but don't animate?)
            //
            // FIXME: for testing only
            //if ((flags & Flag_EnableAnimation) != 0)
                //std::cout << "anim" << std::endl;
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
            const std::string& upb = NiObject::mModel.indexToString(stringIndex);
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
            const std::string& prn = NiObject::mModel.indexToString(stringIndex);
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

    // don't build collision for an EditorMarker
    if (!mModel.showEditorMarkers() && data->mEditorMarkerPresent && (mNodeName == "EditorMarker"))
        return;

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
    enableCollision = true; // FIXME: temp testing
    if (enableCollision && mCollisionObjectIndex != -1)
    {
        //mModel.getRef<NiObject>((int32_t)mCollisionObjectIndex)->build(inst, data, this);
//      data->mBhkRigidBodyMap[NiObject::index()]
//          = std::make_pair(mModel.getModelName()+"@"+mNodeName,
//                           mModel.getRef<bhkCollisionObject>(mCollisionObjectIndex)->getBodyIndex());
    }

    // NiTransformController (e.g. fire/FireTorchLargeSmoke)
    // NiVisController (e.g. oblivion/seige/siegecrawlerdeathsigil, oblivion/gate/oblivionmagicgate01)
    //
    // NiControllerManager (e.g. architecture/ships/MainMast02)
    //   NiMultiTargetTransformController
    //   NiControllerSequence
    //
    // NiBSBoneLODController (e.g. creature/skeleton) (NOTE: LOD means Level of Detail)
    // bhkBlendController  (for Bip01 NonAccum only?)
    //
    //
    //
    //
    //
    // NiGeomMorpherController
    //
    // NiPSysEmitterCtlr,
    // NiMaterialColorController
    //
    //  e.g. StoneWallGateDoor01
    //       FireTorchLargeSmoke    (cow "tamriel" 5 11)
    //       MainMast01, MainMast02 (cow "tamriel" 5 11)
    //
    // Some of the animations are applied before the entity is built (i.e. to the mesh), e.g.
    // NiGeomMorpherController and for the newer versions of the NIF the keyframe details are
    // in the interpolators indicated in the controlled blocks.
    //
    // The controllers are built before NiGeometry in case there are animations and hence some
    // mappings need to be prepared in advance.
    //
    // But, the target nodes (children) have not been built yet - should the controllers be
    // built after the children?  To build the controllers later, the controller index should
    // be stored in 'inst' so that duing a build of NiGeomety the required data can be found.
    //
    // How are targets mentioned in NiMultiTargetTransformController meant to be used?  They
    // seem to correspond to the controlled blocks in NiControllerSquence, anyway. These are
    // also listed in the object palette.
    NiTimeControllerRef controllerIndex = NiObjectNET::mControllerIndex;
// FIXME: testing only
#if 0
    if (controllerIndex != -1)
    {
        std::string name = mModel.blockType(mModel.getRef<NiTimeController>(controllerIndex)->index());
        if (name != "NiControllerManager" && name != "NiTransformController" && name != "NiVisController"
                && name != "NiBSBoneLODController" && name != "bhkBlendController")
            std::cout << name << std::endl;
    }
#endif
    // NiVisController (idleobjects/GenericBook.NIF)
    // NiTransformController (fire/FireTorchLargeSmoke.NIF)
    while (controllerIndex != -1)
    {
        controllerIndex
            = NiObject::mModel.getRef<NiTimeController>(controllerIndex)->build(NiObjectNET::mControllers);
    }

    // NiNode, NiGeometry, NiCamera, NiLight or NiTextureEffect
    for (unsigned int i = 0; i < mChildren.size(); ++i)
    {
        if (mChildren[i] == -1) // no object
            continue;

        // NiGeometry blocks are only registered here and built later.  One possible side
        // benefit is that at least at this NiNode level we know if there are any skins.
        NiObject::mModel.getRef<NiObject>((int32_t)mChildren[i])->build(inst, data, this);
    }

    // FIXME: too many bones filename = "meshes\\oblivion\\gate\\oblivionarchgate01.nif"

    // FIXME: testing only: Doesn't look like NiNodes have properties?
    // (which means NiGeometry doesn't need to check its parent for properties)
//    if (NiAVObject::mProperty.size())
//        throw std::runtime_error("NiNode has properties");
    //+		mModelName	"meshes\\oblivion\\environment\\oblivionsmokeemitter01.nif"	std::basic_string<char,std::char_traits<char>,std::allocator<char> >
    // NiZBufferProperty


    for (unsigned int i = 0; i < mEffects.size(); ++i)
    {
        if (mEffects[i] == -1) // no object
            continue;

        NiObject::mModel.getRef<NiObject>((int32_t)mEffects[i])->build(inst, data, this);
    }

}

// 1. check extra data for MRK, NCO, etc
// 2. check if the last of the children is RootCollisionNode to build collision object
// 3. build each of the children
// 3.1  apply translation, rotation and scale to a child sceneNode?
// 4. build each of the dynamic effects
// 5. apply any properties (? do this first and pass on for building children?)
// ?. what do do with any bounding boxes?
//void NiBtOgre::NiNode::buildTES3(Ogre::SceneNode* sceneNode, BtOgreInst *inst, NiObject *parent)
//{
//}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSBlastNode::BSBlastNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiNode(index, stream, model, data)
{
    stream.read(mUnknown1);
    stream.read(mUnknown2);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSDamageStage::BSDamageStage(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiNode(index, stream, model, data)
{
    stream.read(mUnknown1);
    stream.read(mUnknown2);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSMultiBoundNode::BSMultiBoundNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiNode(index, stream, model, data)
{
    stream.read(mMultiBoundIndex);
    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7
        stream.read(mUnknown);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSOrderedNode::BSOrderedNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiNode(index, stream, model, data)
{
    stream.read(mAlphaSortBound);
    stream.read(mIsStaticBound);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSTreeNode::BSTreeNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiNode(index, stream, model, data)
{
    stream.readVector<NiNodeRef>(mBones1);
    stream.readVector<NiNodeRef>(mBones2);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSValueNode::BSValueNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiNode(index, stream, model, data)
{
    stream.read(mValue);

    stream.skip(sizeof(char)); // unknown byte
}

NiBtOgre::NiBillboardNode::NiBillboardNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiNode(index, stream, model, data)
{
    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        stream.read(mBillboardMode);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiSwitchNode::NiSwitchNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiNode(index, stream, model, data)
{
    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        stream.read(mUnknownFlags);

    stream.read(mUnknownInt);
}
