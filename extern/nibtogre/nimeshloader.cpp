/*
  Copyright (C) 2019, 2020 cc9cii

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
#include "nimeshloader.hpp"

#include <cassert>

#include <OgreMesh.h>
#include <OgreMeshManager.h>

#include "nimodel.hpp"
#include "ninode.hpp"

template<> NiBtOgre::NiMeshLoader* Ogre::Singleton<NiBtOgre::NiMeshLoader>::msSingleton = 0;

namespace NiBtOgre
{
    NiMeshLoader::NiMeshLoader()
    {
    }

    NiMeshLoader::~NiMeshLoader()
    {
    }

    NiMeshLoader* NiMeshLoader::getSingletonPtr(void)
    {
        return msSingleton;
    }

    NiMeshLoader& NiMeshLoader::getSingleton(void)
    {
        assert( msSingleton ); return ( *msSingleton );
    }

    Ogre::MeshPtr NiMeshLoader::createMesh(const Ogre::String& name,
            const Ogre::String& group, NiModel *model, std::int32_t ninode)
    {
        // Create manual model which calls back self to load
        Ogre::MeshPtr pMesh = createManual(name, group);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Object;
        bInfo.model = model;
        bInfo.ninode = ninode;
        mModelBuildInfoMap[pMesh.get()] = bInfo;

        return pMesh; // at this point the mesh is created but not yet loaded
    }

    // WARN: must ensure that 'name' includes the NPC EditorID and 'model' must also be the NPC
    // specific one.
    Ogre::MeshPtr NiMeshLoader::createMorphedMesh(const Ogre::String& name,
            const Ogre::String& group, const Ogre::String& morphedTexture, NiModel *model, std::int32_t ninode)
    {
        // Create manual model which calls back self to load
        Ogre::MeshPtr pMesh = createManual(name, group);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Morphed;
        bInfo.model = model;
        bInfo.ninode = ninode;
        // create an Ogre::Material but not yet load and store in bInfo
        mModelBuildInfoMap[pMesh.get()] = bInfo;

        return pMesh; // at this point the mesh is created but not yet loaded
    }

    Ogre::MeshPtr NiMeshLoader::createManual(const Ogre::String& name, const Ogre::String& group)
    {
        Ogre::MeshManager& meshManager = Ogre::MeshManager::getSingleton();

        // Don't try to get existing, create should fail if already exists
        if( meshManager.getResourceByName( name, group ) )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_DUPLICATE_ITEM,
                         "Mesh with name '" + name + "' already exists.",
                         "NiMeshLoader::createManual" );
        }

        return meshManager.createManual(name, group, this);
    }

    // this method is not called until the associated Ogre::Entity is created
    void NiMeshLoader::loadResource(Ogre::Resource *res)
    {
        Ogre::Mesh *mesh = static_cast<Ogre::Mesh*>(res);

        // Find build parameters
        std::map<Ogre::Resource*, ModelBuildInfo>::iterator it = mModelBuildInfoMap.find(res);
        if (it == mModelBuildInfoMap.end())
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot find build parameters for " + res->getName(),
                         "NiMeshLoader::loadResource");
        }
        ModelBuildInfo& bInfo = it->second;

        switch(bInfo.type)
        {
        case MBT_Object:
            loadManualMesh(mesh, bInfo);
            break;
        case MBT_Morphed:
            loadManualMorphedMesh(mesh, bInfo);
            break;
        default:
            OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                         "Unknown build parameters for " + res->getName(),
                         "NiMeshLoader::loadResource");
        }
    }

    void NiMeshLoader::loadManualMesh(Ogre::Mesh* pMesh, const ModelBuildInfo& params)
    {
        NiNode *node = params.model->getRef<NiNode>(params.ninode); // get NiNode
        node->buildMesh(pMesh);
    }

    void NiMeshLoader::loadManualMorphedMesh(Ogre::Mesh* pMesh, const ModelBuildInfo& params)
    {
        NiNode *node = params.model->getRef<NiNode>(params.ninode); // get NiNode
        // load material from params
        //Ogre::MaterialPtr mat =
        //node->buildMesh(pMesh, mat);
    }
}
