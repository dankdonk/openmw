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

*/
#include "nimodel.hpp"

#include <stdexcept>
#include <iostream> // FIXME: debugging only

#include <OgreMeshManager.h>
#include <OgreMesh.h>
#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreSkeletonManager.h>
//#include <OgreResourceManager.h> // ResourceCreateOrRetrieveResult

#include "niobject.hpp"
#include "bhkrefobject.hpp" // bhkConstraint
#include "btogreinst.hpp"
#include "meshloader.hpp"
#include "skeletonloader.hpp"

// "name" is the full path to the mesh from the resource directory/BSA added to Ogre::ResourceGroupManager.
// This name is required later for Ogre resource managers such as MeshManager.
// The file is opened by mNiStream::mStream.
//
// FIXME: there could be duplicates b/w TES3 and TES4/5
NiBtOgre::NiModel::NiModel(const std::string& name, const std::string& group, bool showEditorMarkers)
    : mNiStream(name), mHeader(mNiStream), mGroup(group), mModelName(name), mModelData(*this)
    , mSkeletonLoader(nullptr), mShowEditorMarkers(showEditorMarkers)
{
    mObjects.resize(mHeader.numBlocks());
    if (mNiStream.nifVer() >= 0x0a000100) // from 10.0.1.0
    {

        for (std::uint32_t i = 0; i < mHeader.numBlocks(); ++i)
        {
            //std::cout << "Block " << mHeader.blockType(i) << std::endl; // FIXME: for testing only
            mCurrIndex = i; // FIXME: debugging only

            // From ver 10.0.1.0 (i.e. TES4) we already know the object types from the header.
            mObjects[i] = NiObject::create(mHeader.blockType(i), i, mNiStream, *this, mModelData);
        }
    }
    else
    {
        for (std::uint32_t i = 0; i < mHeader.numBlocks(); ++i)
        {
            mCurrIndex = i; // FIXME: debugging only
//#if 0
            std::string blockName = mNiStream.readString();
            //if (blockName == "RootCollisionNode")
                //std::cout << name << " : " << "RootCollisionNode" << std::endl;
            //if (blockName == "AvoidNode")
                //std::cout << name << " : " << "AvoidNode" << std::endl;
            //if (blockName == "NiStringExtraData")
                //std::cout << name << " : " << "NiStringExtraData" << std::endl;
            //std::cout << name << " : " << "BoundingBox" << std::endl;
            mObjects[i] = NiObject::create(blockName, i, mNiStream, *this, mModelData);
            //std::cout << "Block " << blockName << std::endl; // FIXME: for testing only
//#endif
            // For TES3, the object type string is read first to determine the type.
            //mObjects[i] = NiObject::create(mNiStream.readString(), mNiStream, *this);
        }
    }

    // TODO: should assert that the first object, i.e. mObjects[0], is either a NiNode (TES3/TES4)
    //       or BSFadeNode (TES5)

    // read the footer to check for root nodes
    std::uint32_t numRoots = 0;
    mNiStream.read(numRoots);

    mRoots.resize(numRoots);
    for (std::uint32_t i = 0; i < numRoots; ++i)
        mNiStream.read(mRoots.at(i));

    if (numRoots == 0)
        throw std::runtime_error(name + " has no roots");
    else if (numRoots > 1) // FIXME: debugging only, to find out which NIF has multiple roots
        throw std::runtime_error(name + " has too many roots");
        //std::cout << name << " has numRoots: " << numRoots << std::endl;

    // FIXME: testing only
    //if (mNiStream.nifVer() >= 0x0a000100)
        //std::cout << "roots " << mHeader.blockType(mRoots[0]) << std::endl;

    // find the bones, if any
    for (unsigned int i = 0; i < mModelData.mSkelLeafIndicies.size(); ++i)
        mObjects[mModelData.mSkelLeafIndicies[i]]->findBones(mRoots[0]);
}

NiBtOgre::NiModel::~NiModel()
{
    if (mSkeletonLoader)
        delete mSkeletonLoader;
}

void NiBtOgre::NiModel::build(BtOgreInst *inst)
{
    // FIXME: model name can clash with TES3 model names, e.g. characters/_male/skeleton.nif
    if (mModelData.mSkelLeafIndicies.size() > 0)
    {
        mSkeletonLoader = new SkeletonLoader(*this);

        Ogre::ResourceManager::ResourceCreateOrRetrieveResult res // true if newly created
            = Ogre::SkeletonManager::getSingleton().createOrRetrieve(
                    /*std::to_string(nifVer())+":"+*/getModelName(), // use the NIF name, since there is one skeleton per NIF
                    mGroup/*Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME*/,
                    true/*isManual*/,
                    mSkeletonLoader);

        //res.first->load(); // FIXME: for testing loading
    }

    // build the first root
    mObjects[mRoots[0]]->build(inst); // FIXME: what to do with other roots?

    buildMeshAndEntity(inst);

    // build any constraints that were deferred while building the rigid bodies
    for (size_t i = 0; i < inst->mbhkConstraints.size(); ++i)
    {
        inst->mbhkConstraints[i].first->linkBodies(inst, inst->mbhkConstraints[i].second);
    }
}

std::int32_t NiBtOgre::NiModel::getNiNodeParent(std::int32_t child) const
{
    std::map<std::int32_t, std::int32_t>::const_iterator it = mModelData.mNiNodeMap.find(child);
    if (it != mModelData.mNiNodeMap.cend())
        return it->second;
    else
        throw std::logic_error("NiNode parent map: parent not found");
}

// FIXME: maybe pass a parameter here indicating static mesh? (create a "static" group?)
// Or group should come from the classes, e.g. static, misc, furniture, etc
void NiBtOgre::NiModel::buildMeshAndEntity(BtOgreInst* inst)
{
    Ogre::MeshManager& meshManager = Ogre::MeshManager::getSingleton();

    // iterate through the loader map
    //
    // NOTE: If the model/object is static, we only need one child scenenode from the
    // basenode.  Else we need one for each NiNode that has a mesh (and collision shape?).
    // FIXME: how to do this?
    std::map<std::uint32_t, std::pair<std::string, std::unique_ptr<MeshLoader> > >::iterator iter
        = mModelData.mMeshLoaders.begin();

    for (; iter != mModelData.mMeshLoaders.end(); ++iter)
    {
        // iter-second.first = model name + ":" + parent NiNode block name
        // e.g. meshes\\architecture\\imperialcity\\icwalltower01.nif:ICWallTower01

        // iter->second.second = unique_ptr to NiMeshLoader
        //
        // FIXME: work around duplicates for now, should solve it by caching NiModel instead
        //        MeshManager needs a unique string, so probably need to supply the full
        //        name starting from "\\mesh"
        // FIXME: probably room for optimising the use of the "group" parameter
        Ogre::MeshPtr mesh = meshManager.getByName(iter->second.first, mGroup/*"General"*/);
        if (mesh.isNull())
        {
            mesh = meshManager.createManual(iter->second.first, mGroup/*"General"*/, iter->second.second.get());
        }
        mesh->setAutoBuildEdgeLists(false);

        // either use the mesh's name or shared pointer
        Ogre::Entity *entity = inst->mBaseSceneNode->getCreator()->createEntity(/*iter->second.first*/mesh);

        // associate controllers to sub entities
        //
        // WARNING: Assumed that the sub entity order is the same as the order in which the
        //          sub-meshes are created in NiMeshLoader::loadResource

        entity->setVisible(true /*!(flags&Nif::NiNode::Flag_Hidden)*/); // FIXME not for newer NIF

        inst->mObjectScene->mEntities.push_back(entity);

        const std::vector<NiTriBasedGeom*>& subMeshGeometry = iter->second.second->getSubMeshGeometry();
        for (unsigned int j = 0; j < subMeshGeometry.size(); ++j)
        {
            // FIXME: testing only
            //std::cout << "nameIndex " << subMeshGeometry[j]->getNameIndex() << std::endl;
            //std::cout << entity->getSubEntity(j)->getSubMesh()->getMaterialName() << std::endl;
        }

        // FIXME: do we need a map of entities for deleting later?
        //        for now get ObjectScene to do that
    }
#if 0
    std::map<std::uint32_t, EntityConstructionInfo>::iterator iter = mEntityCIMap.begin();

    // one Ogre::Mesh for each NiNode whose children NiGeometry are sub meshes
    for (; iter != mEntityCIMap.end(); ++iter)
    {
        // iter-second.first = model name + ":" + parent NiNode block name
        // iter->second.second = unique_ptr to NiMeshLoader
        //
        // FIXME: work around duplicates for now, should solve it by caching NiModel instead
        //        MeshManager needs a unique string, so probably need to supply the full
        //        name starting from "\\mesh"
        // FIXME: probably room for optimising the use of the "group" parameter
        Ogre::MeshPtr mesh = meshManager.getByName(iter->second.mMeshAndNodeName, "General");
        if (mesh.isNull())
        {
            mesh = meshManager.createManual(iter->second.mMeshAndNodeName, "General",
                                            iter->second.mMeshLoader.get());
        }
        mesh->setAutoBuildEdgeLists(false);

        // either use the mesh's name or shared pointer
        Ogre::Entity *entity = mBaseSceneNode->getCreator()->createEntity(/*iter->second.first*/mesh);





        // associate controllers to sub entities
        //
        // WARNING: Assumed that the sub entity order is the same as the order in which the
        //          sub-meshes are created in NiMeshLoader::loadResource
        unsigned int numSubEntities = entity->getNumSubEntities();

        for (unsigned int j = 0; j < numSubEntities; ++j)
        {
            // FIXME: testing only
            std::cout << entity->getSubEntity(j)->getSubMesh()->getMaterialName() << std::endl;
        }

        // FIXME: do we need a map of entities for deleting later?
    }
#endif
}

void NiBtOgre::ModelData::setNiNodeParent(std::int32_t child, std::int32_t parent)
{
    //if (child == -1) // already checked in NiNode before calling this method
        //return;

    std::map<std::int32_t, std::int32_t >::iterator lb = mNiNodeMap.lower_bound(child);

    if (lb != mNiNodeMap.end() && !(mNiNodeMap.key_comp()(child, lb->first)))
    {
        if (lb->second != parent)
            throw std::logic_error("NiNode parent map: multiple parents");
        // else the same entry already there for some reason, ignore for now
    }
    else
        mNiNodeMap.insert(lb, std::make_pair(child, parent)); // None found, create one
}

// prepare for building the mesh
void NiBtOgre::ModelData::registerNiTriBasedGeom(std::uint32_t nodeIndex, const std::string& name, NiTriBasedGeom* geometry)
{
    std::map<std::uint32_t, std::pair<std::string, std::unique_ptr<MeshLoader> > >::iterator lb
        = mMeshLoaders.lower_bound(nodeIndex);

    if (lb != mMeshLoaders.end() && !(mMeshLoaders.key_comp()(nodeIndex, lb->first)))
    {
        // One exists already, add to it
        lb->second.second->registerSubMeshGeometry(geometry);
    }
    else
    {
        // None found, create one
        std::unique_ptr<MeshLoader> loader(new MeshLoader(mModel));
        loader->registerSubMeshGeometry(geometry);
        mMeshLoaders.insert(lb, std::make_pair(nodeIndex, std::make_pair(name, std::move(loader))));
    }
#if 0
    std::map<std::uint32_t, EntityConstructionInfo>::iterator lb = mEntityCIMap.lower_bound(nodeIndex);
    if (lb != mEntityCIMap.end() && !(mEntityCIMap.key_comp()(nodeIndex, lb->first)))
    {
        // A construction info for this block index exists already, just add to it
        /*std::uint32_t subIndex = */lb->second.mMeshLoader->registerMeshGeometry(geometry);

        // assess properties and add any controllers
        //geometry->assessProperties(lb->second.mSubEntityControllers[subIndex]);
    }
    else
    {
        // A construction info with this block index not found, create one
        std::auto_ptr<NiMeshLoader> loader(new NiMeshLoader(this));
        std::uint32_t subIndex = loader->registerMeshGeometry(geometry); // subIndex should be 0

        EntityConstructionInfo entityCI;
        entityCI.mMeshAndNodeName = name;
        entityCI.mMeshLoader = loader;//std::move(loader);

        // assess properties and add any controllers
        //geometry->assessProperties(entityCI.mSubEntityControllers[subIndex]);

        mEntityCIMap.insert(lb, std::make_pair(nodeIndex, entityCI));
    }
#endif
}
