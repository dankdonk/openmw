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
    std::map<std::string, std::pair<Ogre::Matrix4, btCollisionShape*> >::iterator iter;
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

    std::string modelName = getName(); // remove scale
    NiModelPtr nimodel
        = NiBtOgre::NiModelManager::getSingleton().getByName(modelName.substr(0, modelName.length()-7), getGroup());

    if (!nimodel) // shouldn't happen, since we need the Entities created already
        throw std::runtime_error("NiModel not loaded");

    //           target NiAVObject index               bhkSerializable index (e.g. bhkRigidBody)
    //                   |                                    |
    //                   v                                    v
    const std::map<std::int32_t, /*std::pair<std::string,*/ int32_t/*>*/ >& rigidBodies = nimodel->getBhkRigidBodyMap();
    std::map<std::int32_t, /*std::pair<std::string, */int32_t/*>*/ >::const_iterator iter;
    for (iter = rigidBodies.begin(); iter != rigidBodies.end(); ++iter)
    {
        //if (iter->second/*.second*/ == -1)
            //continue;  // e.g. fire/firetorchlargesmoke.nif@DamageSphere



        // FIXME: check for phantom



        int32_t bhkIndex = iter->second/*.second*/;
        bhkSerializable *bhk = nimodel->getRef<bhkSerializable>(bhkIndex);

///     int32_t ninodeIndex;
///     // get the bullet shape with the parent NiNode as a parameter
        NiAVObject *target = nimodel->getRef<NiAVObject>(iter->first);
///     if (nimodel->blockType(iter->first) != "NiNode")
///         ninodeIndex = nimodel->getNiNodeParent(iter->first); // furniture/middleclass/bearskinrug01.nif
///     else
///         ninodeIndex = iter->first;




//      NiNode *node = nimodel->getRef<NiNode>(ninodeIndex);
//      Ogre::Vector3 pos;
//      Ogre::Vector3 scale; // FIXME: apply scale?
//      Ogre::Quaternion rot;
//      node->getLocalTransform().decomposition(pos, scale, rot);
        //btTransform transform(btQuaternion(rot.x, rot.y, rot.z, rot.w), btVector3(pos.x, pos.y, pos.z));
        //if (node->getNodeName() == "ICWallDoor01")
        //if (node->getNodeName() == "HeavyTargetStructure")
            //std::cout << "stop" << std::endl;

        // crazy experiment to apply translation only
        //Ogre::Matrix4 localTransform;
        //localTransform.makeTransform(pos, Ogre::Vector3(1.f), Ogre::Quaternion::IDENTITY);

        Ogre::Matrix4 worldTransform;

    // original: apply parentNiNode's world transform * bhkRigidBodyT transform
    //           all shapes are in place, but impDunDoor02 rotation doesn't work
    //           (rotates about the center of the door)
    //
    // try1:     apply only bhkRigidBodyT transform
    //           various meshes are off 90 deg (e.g. ICWallDoor01)
    //           TargetHeavy01 all broken
    //           but impDunDoor02 rotation works
    //
    // try2:     apply parentNiNode's local transform * bhkRigidBodyT transform
    //           most meshes look ok except TargetHeavyStructure
    //           impDunDoor02 rotation no longer works (i.e. like the original)
#if 0
        worldTransform = Ogre::Matrix4::IDENTITY;
#else
  #if 1
        worldTransform = target->getWorldTransform();
  #else
        if (node->index() > 0)
            worldTransform = node->getParentNode()->getWorldTransform();
        else
            worldTransform = Ogre::Matrix4::IDENTITY;
  #endif
#endif
        if (nimodel->indexToString(target->getNameIndex()) == "Bone01")
        {
            Ogre::Vector3 pos;
            Ogre::Vector3 nodeScale; // FIXME: apply scale?
            Ogre::Quaternion rot;
            worldTransform.decomposition(pos, nodeScale, rot);
            std::cout << rot.getYaw().valueDegrees() << " " << rot.getPitch().valueDegrees() << " " << rot.getRoll().valueDegrees() << std::endl;
        }

        mBtCollisionShapeMap[nimodel->indexToString(target->getNameIndex())]
            = std::make_pair(worldTransform, bhk->getShape(target));
    }
}

void NiBtOgre::BtRigidBodyCI::unloadImpl()
{
}
