/*
  Copyright (C) 2019 cc9cii

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
#include "btrigidbodyci.hpp"

#include <stdexcept>
#include <iostream> // FIXME: debugging only

#include <OgreResourceManager.h>

#include "nimodel.hpp"
#include "nimodelmanager.hpp"
#include "ninode.hpp"
#include "bhkrefobject.hpp"

namespace
{
    void deleteShape(btCollisionShape* shape)
    {
        if(shape)
        {
            if(shape->isCompound())
            {
                btCompoundShape* compoundShape = static_cast<btCompoundShape*>(shape);
                for(int i = 0; i < compoundShape->getNumChildShapes() ;++i)
                {
                    deleteShape(compoundShape->getChildShape(i));
                }
            }
            delete shape;
        }
    }
}

NiBtOgre::BtRigidBodyCI::BtRigidBodyCI(Ogre::ResourceManager *creator, const Ogre::String& name,
        Ogre::ResourceHandle handle, const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader)
    : Resource(creator, name, handle, group, isManual, loader)
{
}

NiBtOgre::BtRigidBodyCI::~BtRigidBodyCI()
{
    std::map<int32_t, btCollisionShape*>::iterator iter;
    for (iter = mBtCollisionShapeMap.begin(); iter != mBtCollisionShapeMap.end(); ++iter)
    {
        delete iter->second;
    }
}

// only called if this resource is not being loaded from a ManualResourceLoader
void NiBtOgre::BtRigidBodyCI::loadImpl()
{
//  // mName looks like meshes\\architecture\\imperialcity\\icwalltower01.nif@ICWallTower01
//  std::string modelName = getName();
//  size_t modelNameSize = modelName.find_first_of('@');

//  if (modelNameSize == std::string::npos)
//      throw std::runtime_error(modelName + " is of unexpected format");

//  NiModelPtr nimodel
//      = NiBtOgre::NiModelManager::getSingleton().getByName(modelName.substr(0, modelNameSize), getGroup());

    std::string modelName = getName();
    NiModelPtr nimodel
        = NiBtOgre::NiModelManager::getSingleton().getByName(modelName.substr(0, modelName.length()-7), getGroup());

    if (!nimodel) // shouldn't happen, since we need the Entities created already
        throw std::runtime_error("NiModel not loaded");

    const std::map<std::int32_t, std::pair<std::string, int32_t> >& loaders = nimodel->getBhkRigidBodyMap();
    std::map<std::int32_t, std::pair<std::string, int32_t> >::const_iterator iter;
    for (iter = loaders.begin(); iter != loaders.end(); ++iter)
    {
        if (iter->second.second == -1)
            continue;  // e.g. fire/firetorchlargesmoke.nif@DamageSphere

        int32_t bhkIndex = iter->second.second;
        bhkRigidBody *bhk = nimodel->getRef<bhkRigidBody>(bhkIndex);

        mBtCollisionShapeMap[bhkIndex] = bhk->getShape(nimodel->getRef<NiNode>(iter->first));
    }
}

void NiBtOgre::BtRigidBodyCI::unloadImpl()
{
}
