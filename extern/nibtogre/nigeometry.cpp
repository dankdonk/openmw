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

  Ogre code in this file is based on v0.36 of OpenMW.

*/
#include "nigeometry.hpp"

#include <cassert>
#include <stdexcept>

#include <OgreMesh.h>

#include "nistream.hpp"
#include "nimodel.hpp"
#include "ninode.hpp"
#include "btogreinst.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

NiBtOgre::NiGeometry::NiGeometry(uint32_t index, NiStream& stream, const NiModel& model)
    : NiAVObject(index, stream, model), mHasShader(false), mDirtyFlag(false)
{
#if 0
    // Some NiTriShapes are "Shadow", possibly simplified mesh for animated (i.e. non-static)
    // objects to cast shadows
    if ((mFlags & 0x40) != 0) // FIXME: testing only, 67 == 0x43, 69 = 0x45
    {
        std::cout << "Shadow : " << model.getModelName() << " : " << model.getLongString(mName) << std::endl;
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

// actual build happens later, after the parent NiNode has processed all its children
void NiBtOgre::NiGeometry::build(BtOgreInst *inst, NiObject *parent)
{
    // The model name and node name are concatenated for use with Ogre::MeshManager
    // without triggering exeptions due to duplicates.
    // e.g. meshes\\architecture\\imperialcity\\icwalltower01.nif:ICWallTower01
    inst->addMeshGeometry(parent->index(),
            mModel.getModelName()+":"+static_cast<NiNode*>(parent)->getNodeName(), this);
}

// build Ogre mesh and apply shader/material using scene
//
// 1. get data and skin objects
// 2. get shader if present
// 3. TES5 has more stuff to do
//
// Should check if animated, has skeleton and/or has skin (static objects do not have these)
//
// Some NiTriStrips have NiBinaryExtraData (tangent space) - not sure what to do with them
void NiBtOgre::NiGeometry::createSubMesh(Ogre::Mesh *mesh)
{
    // Skin seems to be present in things like headhuman, hand, boots, cuirass, gauntlets, greaves,
    // which are used for animation of characters with skeletons
    //
    // If there is a skin, skeleton is needed (skeleton is at mesh level, but skin is at
    // sub mesh level?)
    //
    // FIXME: testing only
    //if (mSkinInstanceIndex != -1)
        //std::cout << "skin" << std::endl;

    // If inst->mFlags says no animation, no havok then most likely static.  Also check if
    // there is a skin instance (and maybe also see if Oblivion layer is OL_STATIC even though
    // that flag is just for the editor)

    // If ((inst->mFlags & Flag_EnableHavok) != 0)
    //
    // This means physics will determine the position of the rendering mesh. To allow that the
    // Ogre::Entity for the corresponding Ogre::Mesh needs to be controlled via its Ogre::SceneNode.
    //
    // Each NiGeometry is probably a SubMesh. e.g. clutter/apple01.nif has 4 sub meshes defined
    // by 4 NiTriStrips, Apple01:0 - Apple01:3.  The ChildSceneNode associated with the target
    // of bhkRigidBody, Apple01, is then controlled by Bullet. Apple01 is the parent of
    // Apple01:0 - Apple01:3.
    //
    // There seems to be a convention with the names - e.g.
    //
    //   NiNode: TargetHeavyTarget <---------- Mesh
    //     bhkCollisionObject
    //     NiTriStrips TargetHeavyTarget:0 <-- SubMesh (front hitting surface)
    //     NiTriStrips TargetHeavyTarget:1 <-- SubMesh (top metal hooks)
    //     NiTriStrips TargetHeavyTarget:2 <-- SubMesh (main body)
    //     NiTriStrips TargetHeavyTarget:3 <-- SubMesh (side surfaces)
    //     NiTriStrips TargetHeavyTarget:4 <-- SubMesh (vertical metal bars)
    //
    // Note ragdoll here is done with Bullet moving the SceneNode associated with the Entity.
    // i.e. no bones are used (or required)

    // FIXME: if the mesh is static, we probably need to apply the full transform of the model
    // else we need the transform only from the parent NiNode
    // This decision can be made by NiGeometry itself
    //
    Ogre::SubMesh *subMesh = mesh->createSubMesh();
}

// FIXME: maybe make this one for TES5 instead?
void NiBtOgre::NiGeometry::buildTES3(Ogre::SceneNode *sceneNode, BtOgreInst *inst, NiObject *parent)
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
    // TODO
}
