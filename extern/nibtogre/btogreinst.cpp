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

namespace NiBtOgre
{
    BtOgreInst::BtOgreInst(Ogre::SceneNode *baseNode) : mBaseNode(baseNode), mFlags(0)
    {
    };

    // prepare for building the mesh
    void BtOgreInst::addMeshGeometry(std::uint32_t index, const std::string& name, NiGeometry* geometry)
    {
        std::map<std::uint32_t,
                 std::pair<std::string, std::unique_ptr<NiMeshLoader> > >::iterator lb = mMeshes.lower_bound(index);
        if (lb != mMeshes.end() && !(mMeshes.key_comp()(index, lb->first)))
        {
            // One exists already, add to it
            lb->second.second->addMeshGeometry(geometry);
        }
        else
        {
            // None found, create one
            std::unique_ptr<NiMeshLoader> loader(new NiMeshLoader(this));
            loader->addMeshGeometry(geometry);
            mMeshes.insert(lb, std::make_pair(index, std::make_pair(name, std::move(loader))));
        }
    }

    // FIXME: maybe pass a parameter here indicating static mesh? (create a "static" group?)
    // Or group should come from the classes, e.g. static, misc, furniture, etc
    void BtOgreInst::buildMeshAndEntity()
    {
        Ogre::MeshManager& meshManager = Ogre::MeshManager::getSingleton();

        // iterate through the loader map
        //
        // NOTE: If the model/object is static, we only need one child scenenode from the
        // basenode.  Else we need one for each NiNode that has a mesh (and collision shape?).
        // FIXME: how to do this?
        std::map<std::uint32_t,
                 std::pair<std::string, std::unique_ptr<NiMeshLoader> > >::iterator iter = mMeshes.begin();
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
                mesh = meshManager.createManual(iter->second.first, "General", iter->second.second.get());
            mesh->setAutoBuildEdgeLists(false);

            // either use the mesh's name or shared pointer
            Ogre::Entity *entity = mBaseNode->getCreator()->createEntity(/*iter->second.first*/mesh);

            // FIXME: do we need a map of entities for deleting later?
        }
    }

    void BtOgreInst::instantiate()
    {
        // FIXME: howto transform nodes to root transform after the meshes have been built?
        // e.g. NiNode CathedralCryptChain11 (18) has 1 mesh CathedralCryptChain11:36 (24)
        //      but it is a child to NiNode CathedralCryptChain (14) which as its own
        //      transform.
        //
        // Is it possible to leave that for Ogre::SceneNode to take care of?
        // i.e. for each NiNode with a mesh create a child scenenode
    }
}
