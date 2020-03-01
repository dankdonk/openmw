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

  Below code is basically Ogre::SkeletonManager with a new class name.

*/
#include "btrigidbodycimanager.hpp"

#include <OgreResourceGroupManager.h>

#include "btrigidbodyci.hpp"

template<> NiBtOgre::BtRigidBodyCIManager* Ogre::Singleton<NiBtOgre::BtRigidBodyCIManager>::msSingleton = 0;

namespace NiBtOgre
{
    BtRigidBodyCIManager* BtRigidBodyCIManager::getSingletonPtr(void)
    {
        return msSingleton;
    }

    BtRigidBodyCIManager& BtRigidBodyCIManager::getSingleton(void)
    {
        assert( msSingleton ); return ( *msSingleton );
    }

    BtRigidBodyCIManager::BtRigidBodyCIManager()
    {
        mLoadOrder = 450.0f; // FIXME: how to choose an appropriate value?
        mResourceType = "BtRigidBodyCI";

        Ogre::ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
    }

    BtRigidBodyCIManager::~BtRigidBodyCIManager()
    {
        Ogre::ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
    }

    BtRigidBodyCIPtr BtRigidBodyCIManager::getByName(const Ogre::String& name, const Ogre::String& groupName)
    {
        return Ogre::static_pointer_cast<BtRigidBodyCI>(getResourceByName(name, groupName));
    }

    BtRigidBodyCIPtr BtRigidBodyCIManager::getOrLoadByName(const Ogre::String& name, const Ogre::String& groupName)
    {
        Ogre::ResourcePtr res = getResourceByName(name, groupName);

        if (!res)
            res = load(name, groupName);

        return Ogre::static_pointer_cast<BtRigidBodyCI>(res);
    }

    BtRigidBodyCIPtr BtRigidBodyCIManager::create (const Ogre::String& name, const Ogre::String& group,
                                       bool isManual, Ogre::ManualResourceLoader* loader,
                                       const Ogre::NameValuePairList* createParams)
    {
        return Ogre::static_pointer_cast<BtRigidBodyCI>(createResource(name, group, isManual, loader, createParams));
    }

    Ogre::Resource* BtRigidBodyCIManager::createImpl(const Ogre::String& name,
            Ogre::ResourceHandle handle, const Ogre::String& group, bool isManual,
            Ogre::ManualResourceLoader* loader, const Ogre::NameValuePairList* createParams)
    {
        // maybe pass the scale value as a parameter?  what about the NiNode block index?
        return OGRE_NEW BtRigidBodyCI(this, name, handle, group, isManual, loader);
    }
}
