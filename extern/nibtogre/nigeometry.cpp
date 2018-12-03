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

  Some of the Ogre code in this file is based on v0.36 of OpenMW.

*/
#include "nigeometry.hpp"

#include <cassert>
#include <stdexcept>

#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreHardwareBufferManager.h>
#include <OgreRoot.h>
#include <OgreRenderSystem.h>

#include "nistream.hpp"
#include "nimodel.hpp"
#include "ninode.hpp"
#include "btogreinst.hpp"
#include "nidata.hpp" // NiGeometryData
#include "niproperty.hpp" // NiProperty
#include "ogrematerial.hpp"
#include "nitimecontroller.hpp"

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
        std::cout << "Shadow : " << model.getModelName() << " : " << model.indexToString(mName) << std::endl;
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
    // FIXME: seems rather hacky, but needed to access parent and/or world transforms
    mParent = static_cast<NiNode*>(parent);

    if (mSkinInstanceIndex != -1)
        inst->mFlags |= Flag_HasSkin;

    // The model name and parent node name are concatenated for use with Ogre::MeshManager
    // without triggering exeptions due to duplicates.
    // e.g. meshes\\architecture\\imperialcity\\icwalltower01.nif:ICWallTower01
    //
    // FIXME: probably should normalise the names to lowercase
    inst->registerNiGeometry(parent->index(), mModel.getModelName()+":"+mParent->getNodeName(), this);
}

// TODO: new classes OgreMaterial, SubEntityController
//
// Can't remember why I wanted SubEntityController (a base class maybe?)
//
// OgreMaterial would be an object that has all the properties the engine (in this case
// Ogre) can support.  It would be passed to each of the NiProperty objects for this sub-mesh
// and updated as required.
//
// Kind of works like Nif::Node::getProperties and NIFMaterialLoader::getMaterial
// (both called from NIFMeshLoader::createSubMesh).

// the parameter 'controllers' are keyed to the index of this sub-mesh
void NiBtOgre::NiGeometry::applyProperties(std::vector<Ogre::Controller<float> >& controllers)
{
    OgreMaterial ogreMaterial;

    // NiGeometry is derived from NiAVObject, so it has its own transform and properties (like NiNode)
    for (unsigned int i = 0; i < NiAVObject::mProperty.size(); ++i)
    {
        NiProperty* property = mModel.getRef<NiProperty>(NiAVObject::mProperty[i]);

        // FIXME: for testing only; note some properties have a blank name
        std::cout << "property " << mModel.indexToString(property->getNameIndex()) << std::endl;

        property->applyMaterialProperty(ogreMaterial);

        // Each property can have a chain of NiTimeControllers.
        //property->setup(properties, controllers); // FIXME

#if 0
        while (property->mControllerIndex != -1)
        {
            NiTimeController* controller = mModel.getRef<NiTimeController*>(property->mControllerIndex);

            // current place in controller map
            //
            //       BtOgreInst
            //          o
            //          | 0..N
            //      *NiMeshLoader       map key: parent NiNode block index
            //          o
            //          | 1..N
            //       *NiGeometry        vector index: sub mesh index
            //          o
            //          | 0..N
            //       *NiProperty
            //          o
            //          | 0..N
            //     *NiTimeController
            //
            int n = inst->mEntityCIMap[mParent->index()].size(); // n-1 is the current sub mesh index

            // under this sub mesh index there are 0..N time controllers
        }
#endif
    }

#if 0
    // Ogre material for the sub-mesh needs to know about *all* the properties
    std::string materialName = NIFMaterialLoader::getMaterialName(ogreMaterial);
    if(materialName.length() > 0)
        sub->setMaterialName(materialName);
#endif
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
void NiBtOgre::NiGeometry::createSubMesh(BtOgreInst *inst, Ogre::Mesh *mesh)
{
    // If inst->mFlags says no animation, no havok then most likely static.  Also check if
    // there is a skin instance (and maybe also see if Oblivion layer is OL_STATIC even though
    // that flag is just for the editor)

    // If ((inst->mFlags & Flag_EnableHavok) != 0)
    //
    // This means physics will determine the position of the rendering mesh. To allow that the
    // Ogre::Entity for the corresponding Ogre::Mesh needs to be controlled via its Ogre::SceneNode.
    //
    // Each NiGeometry probably should be a SubMesh. e.g. clutter/apple01.nif has 4 sub meshes
    // defined by 4 NiTriStrips, Apple01:0 - Apple01:3.  The ChildSceneNode associated with the
    // target of bhkRigidBody, Apple01, is then controlled by Bullet. Apple01 is the parent of
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
    //Ogre::SubMesh *sub = mesh->createSubMesh();

    NiGeometryData* data = mModel.getRef<NiGeometryData>(mDataIndex);
    std::string type = mModel.blockType(data->index());

    // ICDoor04, UpperChest02 - these have animation flag but are static. Maybe ignore this flag?
    // NOTE: Flag_HasSkin may not be set if only some of the NiGeometry blocks have skin.
    bool isStatic = (inst->mFlags & Flag_HasSkin) == 0 &&
                    (inst->mFlags & Flag_EnableHavok) == 0 &&
                    (inst->mFlags & Flag_EnableAnimation) == 0;

    Ogre::Matrix4 transform;
    if (!isStatic)
        transform = mParent->getLocalTransform();
    else
        transform = mParent->getWorldTransform();

    // NOTE: below code copied from components/nifogre/mesh.cpp (OpenMW)

    std::vector<Ogre::Vector3> srcVerts = data->mVertices;
    std::vector<Ogre::Vector3> srcNorms = data->mNormals;
    Ogre::HardwareBuffer::Usage vertUsage = Ogre::HardwareBuffer::HBU_STATIC;
    bool vertShadowBuffer = false;

    for (size_t i = 0;i < srcVerts.size();i++)
    {
        Ogre::Vector4 vec4(srcVerts[i].x, srcVerts[i].y, srcVerts[i].z, 1.0f);
        vec4 = transform*vec4;
        srcVerts[i] = Ogre::Vector3(&vec4[0]);
    }

    for (size_t i = 0;i < srcNorms.size();i++)
    {
        Ogre::Vector4 vec4(srcNorms[i].x, srcNorms[i].y, srcNorms[i].z, 0.0f);
        vec4 = transform*vec4;
        srcNorms[i] = Ogre::Vector3(&vec4[0]);
    }

    // This function is just one long stream of Ogre-barf, but it works great.
    Ogre::HardwareBufferManager *hwBufMgr = Ogre::HardwareBufferManager::getSingletonPtr();
    Ogre::HardwareVertexBufferSharedPtr vbuf;
    Ogre::HardwareIndexBufferSharedPtr ibuf;
    Ogre::VertexBufferBinding *bind;
    Ogre::VertexDeclaration *decl;
    int nextBuf = 0;

    Ogre::SubMesh *sub = mesh->createSubMesh();

    // Add vertices
    sub->useSharedVertices = false;
    sub->vertexData = new Ogre::VertexData();
    sub->vertexData->vertexStart = 0;
    sub->vertexData->vertexCount = srcVerts.size();

    decl = sub->vertexData->vertexDeclaration;
    bind = sub->vertexData->vertexBufferBinding;
    if (srcVerts.size())
    {
        vbuf = hwBufMgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                            srcVerts.size(), vertUsage, vertShadowBuffer);
        vbuf->writeData(0, vbuf->getSizeInBytes(), &srcVerts[0][0], true);

        decl->addElement(nextBuf, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
        bind->setBinding(nextBuf++, vbuf);
    }

    // Vertex normals
    if (srcNorms.size())
    {
        vbuf = hwBufMgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                            srcNorms.size(), vertUsage, vertShadowBuffer);
        vbuf->writeData(0, vbuf->getSizeInBytes(), &srcNorms[0][0], true);

        decl->addElement(nextBuf, 0, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
        bind->setBinding(nextBuf++, vbuf);
    }

    // Vertex colors
    const std::vector<Ogre::Vector4> &colors = data->mVertexColors;
    if (colors.size())
    {
        Ogre::RenderSystem *rs = Ogre::Root::getSingleton().getRenderSystem();
        std::vector<Ogre::RGBA> colorsRGB(colors.size());
        for (size_t i = 0; i < colorsRGB.size(); ++i)
        {
            Ogre::ColourValue clr(colors[i][0], colors[i][1], colors[i][2], colors[i][3]);
            rs->convertColourValue(clr, &colorsRGB[i]);
        }
        vbuf = hwBufMgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR),
                                            colorsRGB.size(), Ogre::HardwareBuffer::HBU_STATIC);
        vbuf->writeData(0, vbuf->getSizeInBytes(), &colorsRGB[0], true);
        decl->addElement(nextBuf, 0, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
        bind->setBinding(nextBuf++, vbuf);
    }

    // Texture UV coordinates
    size_t numUVs = data->mUVSets.size();
    if (numUVs)
    {
        size_t elemSize = Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);

        for(unsigned short i = 0; i < numUVs; ++i)
            decl->addElement(nextBuf, (unsigned short)elemSize*i, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, i);

        vbuf = hwBufMgr->createVertexBuffer(decl->getVertexSize(nextBuf), srcVerts.size(),
                                            Ogre::HardwareBuffer::HBU_STATIC);

        std::vector<Ogre::Vector2> allUVs;
        allUVs.reserve(srcVerts.size()*numUVs);
        for (size_t vert = 0; vert < srcVerts.size(); ++vert)
            for (size_t i = 0; i < numUVs; i++)
                allUVs.push_back(data->mUVSets[i][vert]);

        vbuf->writeData(0, elemSize*srcVerts.size()*numUVs, &allUVs[0], true);

        bind->setBinding(nextBuf++, vbuf);
    }

    // Triangle faces
    const std::vector<uint16_t> *srcIdx;
    srcIdx = &static_cast<NiTriBasedGeomData*>(data)->getTriangles();

    if (srcIdx->size())
    {
        ibuf = hwBufMgr->createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT, srcIdx->size(),
                                           Ogre::HardwareBuffer::HBU_STATIC);
        ibuf->writeData(0, ibuf->getSizeInBytes(), &(*srcIdx)[0], true);
        sub->indexData->indexBuffer = ibuf;
        sub->indexData->indexCount = srcIdx->size();
        sub->indexData->indexStart = 0;
    }
    // Assign bone weights for this TriShape

    //$ find . -type f -print0 | xargs -0 strings -f | grep -E 'NiSkinData'
    //./architecture/arena/arenaspectatorf01.nif: NiSkinData    <-- ControllerSequence
    //./architecture/arena/arenaspectatorm01.nif: NiSkinData    <-- ControllerSequence
    //./architecture/ships/shipflag01.nif: NiSkinData
    //./architecture/ships/shipwrecksail01.nif: NiSkinData      <-- havok animation
    //./architecture/ships/shipwrecksail02.nif: NiSkinData      <-- havok animation
    //./architecture/ships/shipwrecksail03.nif: NiSkinData      <-- havok animation
    //./armor/amber/f/boots.nif: NiSkinData
    //./armor/amber/f/cuirass.nif: NiSkinData
    //...
    //./creatures/zombie/righttorso02.nif: NiSkinData
    //./dungeons/caves/clutter01/roperock01.nif: NiSkinData     <-- havok + ControllerSequence
    //./dungeons/caves/clutter01/rythorspendant01.nif: NiSkinData
    //./dungeons/caves/roperock01.nif: NiSkinData               <-- havok + ControllerSequence
    //./dungeons/chargen/ropebucket01.nif: NiSkinData           <-- havok animation
    //./dungeons/misc/mdtapestryskinned01.nif: NiSkinData       <-- havok animation
    //./dungeons/misc/necrotapestryskinned01.nif: NiSkinData    <-- havok animation
    //./dungeons/misc/roothavok01.nif: NiSkinData
    //./dungeons/misc/roothavok02.nif: NiSkinData
    //./dungeons/misc/roothavok04.nif: NiSkinData
    //./dungeons/misc/roothavok05.nif: NiSkinData
    //./dungeons/misc/roothavok06.nif: NiSkinData
    //./dungeons/misc/roothavok07.nif: NiSkinData
    //./dungeons/misc/ropeskull01.nif: NiSkinData
    //./oblivion/clutter/containers/clawstandcontainer.nif: NiSkinData
    //./oblivion/clutter/traps/oblivionclawtrap01.nif: NiSkinData
    //./oblivion/gate/oblivionarchgate01.nif: NiSkinData
    //./oblivion/gate/obliviongate_forming.nif: NiSkinData
    //./oblivion/gate/obliviongate_simple.nif: NiSkinData
    //
    // Skin seems to be present in things like headhuman, hand, boots, cuirass, gauntlets, greaves,
    // which are used for animation of characters with skeletons
    //
    // If there is a skin, a skeleton is needed (skeleton is at the NIF level, but skin is at a
    // sub mesh level?)
    if (mSkinInstanceIndex != -1)
    {
        NiSkinInstance * skinInst = mModel.getRef<NiSkinInstance>(mSkinInstanceIndex);

        // build skeleton on demand; need to check for each mSkinInstanceIndex
        //
        // start at Skeleton Root and build a tree? but that can result
        // in way too many bones (e.g. oblivion/gate/oblivionarchgate01.nif)
        //
        // best to reverse search from each of the affected bones
        skinInst->mSkeletonRoot->clearSkeleton(); // i.e. only one search at a time
        for (unsigned int i = 0; i < skinInst->mBones.size(); ++i)
        {
            NiNode *node = mModel.getRef<NiNode>(skinInst->mBones[i]);
            // FIXME: check for getParentNode() returning nullptr?
            node->getParentNode()->findBones(skinInst->mSkeletonRoot->index(), skinInst->mBones[i]);
        }

        // FIXME: store the created skeleton in 'inst'? there may be more than one skeleton at
        // the same skeleton root node
        skinInst->mSkeletonRoot->buildSkeleton(inst, mSkinInstanceIndex);

        // bone assignments here?
#if 0
        Ogre::SkeletonPtr skel = Ogre::SkeletonManager::getSingleton().getByName(mName);

        const Nif::NiSkinData *data = skin->data.getPtr();
        const Nif::NodeList &bones = skin->bones;
        for(size_t i = 0;i < bones.length();i++)
        {
            Ogre::VertexBoneAssignment boneInf;
            boneInf.boneIndex = skel->getBone(bones[i]->name)->getHandle();

            const std::vector<Nif::NiSkinData::VertWeight> &weights = data->bones[i].weights;
            for(size_t j = 0;j < weights.size();j++)
            {
                boneInf.vertexIndex = weights[j].vertex;
                boneInf.weight = weights[j].weight;
                sub->addBoneAssignment(boneInf);
            }
        }
#endif
    }

    // End of code copied from components/nifogre/mesh.cpp

    // find and apply the material

    // apply controllers e.g. NiGeomMorpherController
    //
    // NOTE: NiUVController needs to be associated with a sub-entity, so the build/setup is
    //       compelted at a later point.  In comparison, NiGeomMorpherController adds animation
    //       to the sub-mesh so no further setup is required.
    NiTimeControllerRef controllerIndex = NiObjectNET::mControllerIndex;
    while (controllerIndex != -1)
    {
        controllerIndex = mModel.getRef<NiTimeController>(controllerIndex)->build(inst, mesh);
    }
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
