/*
  Copyright (C) 2018 cc9cii

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
#include "btogreinst.hpp"

#include <OgreMeshManager.h>
#include <OgreMesh.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h> // FIXME: testing only
#include <OgreSubMesh.h> // FIXME: testing only

#include "nigeometry.hpp"

NiBtOgre::BtOgreInst::BtOgreInst(Ogre::SceneNode *baseNode)
    : mBaseSceneNode(baseNode), mFlags(0)//, mSkeletonBuilt(false)
{
};

// prepare for building the mesh
void NiBtOgre::BtOgreInst::registerNiGeometry(std::uint32_t nodeIndex, const std::string& name, NiGeometry* geometry)
{
    std::map<std::uint32_t, std::pair<std::string, std::unique_ptr<NiMeshLoader> > >::iterator lb
        = mMeshes.lower_bound(nodeIndex);

    if (lb != mMeshes.end() && !(mMeshes.key_comp()(nodeIndex, lb->first)))
    {
        // One exists already, add to it
        lb->second.second->registerSubMeshGeometry(geometry);
    }
    else
    {
        // None found, create one
        std::unique_ptr<NiMeshLoader> loader(new NiMeshLoader(this));
        loader->registerSubMeshGeometry(geometry);
        mMeshes.insert(lb, std::make_pair(nodeIndex, std::make_pair(name, std::move(loader))));
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

// FIXME: maybe pass a parameter here indicating static mesh? (create a "static" group?)
// Or group should come from the classes, e.g. static, misc, furniture, etc
void NiBtOgre::BtOgreInst::buildMeshAndEntity()
{
    Ogre::MeshManager& meshManager = Ogre::MeshManager::getSingleton();

    // iterate through the loader map
    //
    // NOTE: If the model/object is static, we only need one child scenenode from the
    // basenode.  Else we need one for each NiNode that has a mesh (and collision shape?).
    // FIXME: how to do this?
    std::map<std::uint32_t, std::pair<std::string, std::unique_ptr<NiMeshLoader> > >::iterator iter
        = mMeshes.begin();

    for (; iter != mMeshes.end(); ++iter)
    {
        // iter-second.first = model name + ":" + parent NiNode block name
        // e.g. meshes\\architecture\\imperialcity\\icwalltower01.nif:ICWallTower01

        // iter->second.second = unique_ptr to NiMeshLoader
        //
        // FIXME: work around duplicates for now, should solve it by caching NiModel instead
        //        MeshManager needs a unique string, so probably need to supply the full
        //        name starting from "\\mesh"
        // FIXME: probably room for optimising the use of the "group" parameter
        Ogre::MeshPtr mesh = meshManager.getByName(iter->second.first, "General");
        if (mesh.isNull())
        {
            mesh = meshManager.createManual(iter->second.first, "General", iter->second.second.get());
        }
        mesh->setAutoBuildEdgeLists(false);

        // either use the mesh's name or shared pointer
        Ogre::Entity *entity = mBaseSceneNode->getCreator()->createEntity(/*iter->second.first*/mesh);

        // associate controllers to sub entities
        //
        // WARNING: Assumed that the sub entity order is the same as the order in which the
        //          sub-meshes are created in NiMeshLoader::loadResource

        const std::vector<NiGeometry*>& subMeshGeometry = iter->second.second->getSubMeshGeometry();
        for (unsigned int j = 0; j < subMeshGeometry.size(); ++j)
        {
            // FIXME: testing only
            //std::cout << "nameIndex " << subMeshGeometry[j]->getNameIndex() << std::endl;
            //std::cout << entity->getSubEntity(j)->getSubMesh()->getMaterialName() << std::endl;
        }

        // FIXME: do we need a map of entities for deleting later?
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

void NiBtOgre::BtOgreInst::instantiate()
{
    // FIXME: howto transform nodes to root transform after the meshes have been built?
    // e.g. NiNode CathedralCryptChain11 (18) has 1 mesh CathedralCryptChain11:36 (24)
    //      but it is a child to NiNode CathedralCryptChain (14) which as its own
    //      transform.
    //
    // Is it possible to leave that for Ogre::SceneNode to take care of?
    // i.e. for each NiNode with a mesh create a child scenenode
}
