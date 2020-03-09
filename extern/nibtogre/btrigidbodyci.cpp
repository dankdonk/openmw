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
    std::map<std::int32_t, std::pair<Ogre::Matrix4, btCollisionShape*> >::iterator iter;
    for (iter = mBtCollisionShapeMap.begin(); iter != mBtCollisionShapeMap.end(); ++iter)
    {
        delete iter->second.second;
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

    std::string modelName = getName(); // remove scale from the name (see -7 below)

    if (0)//modelName.find("idgate") != std::string::npos)
        std::cout << modelName << std::endl;

    NiModelPtr nimodel
        = NiBtOgre::NiModelManager::getSingleton().getByName(modelName.substr(0, modelName.length()-7), getGroup());

    if (!nimodel) // shouldn't happen, since we need the Entities created already
        throw std::runtime_error("BtRigidBodyCI: NiModel not loaded");

    //           target NiAVObject ref               bhkSerializable ref (e.g. bhkRigidBody)
    //                   |                                    |
    //                   v                                    v
    const std::map<std::int32_t, /*std::pair<std::string,*/ int32_t/*>*/ >& rigidBodies = nimodel->getBhkRigidBodyMap();
    std::map<std::int32_t, /*std::pair<std::string, */int32_t/*>*/ >::const_iterator iter(rigidBodies.begin());
    for (; iter != rigidBodies.end(); ++iter)
    {
        //if (iter->second/*.second*/ == -1)
            //continue;  // e.g. fire/firetorchlargesmoke.nif@DamageSphere
        // FIXME: check for phantom

        std::int32_t bhkRef = iter->second/*.second*/;
        bhkSerializable *bhk = nimodel->getRef<bhkSerializable>(bhkRef);
        std::int32_t targetRef = iter->first;
        NiAVObject *target = nimodel->getRef<NiAVObject>(targetRef);

        mTargetNames[targetRef] = nimodel->indexToString(target->getNameIndex());

        // expectation is that each target has only one bhkRigidBody
        if (mBtCollisionShapeMap.find(targetRef) != mBtCollisionShapeMap.end())
            throw std::logic_error("target name collision "+nimodel->indexToString(targetRef));


        // get the bullet shape with the target as a parameter
        // TODO: cloning pre-pade shape (e.g. bhkRigidBody via unique_ptr) may be faster?
        mBtCollisionShapeMap[targetRef] = std::make_pair(target->getWorldTransform(), bhk->getShape(*target));
    }
}

void NiBtOgre::BtRigidBodyCI::unloadImpl()
{
}
