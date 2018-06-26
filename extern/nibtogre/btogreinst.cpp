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

#include "niobject.hpp" // Flag_IgnoreEditorMarker (but probably no longer needed)

namespace NiBtOgre
{
    // FIXME: Flag_IgnoreEditorMarker is probably no longer required
    BtOgreInst::BtOgreInst(Ogre::SceneNode *baseNode) : mBaseNode(baseNode), mFlags(Flag_IgnoreEditorMarker)
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
            std::unique_ptr<NiMeshLoader> loader(new NiMeshLoader());
            loader->addMeshGeometry(geometry);
            mMeshes.insert(lb, std::make_pair(index, std::make_pair(name, std::move(loader))));
        }
    }

    // FIXME: maybe pass a parameter here indicating static mesh? (create a "static" group?)
    void BtOgreInst::buildMeshAndEntity()
    {
        Ogre::MeshManager& meshManager = Ogre::MeshManager::getSingleton();

        // iterate through the loader map
        std::map<std::uint32_t,
                 std::pair<std::string, std::unique_ptr<NiMeshLoader> > >::iterator iter = mMeshes.begin();
        for (; iter != mMeshes.end(); ++iter)
        {
            // iter-second.first = model name + parent NiNode block name
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
        }
    }
}
