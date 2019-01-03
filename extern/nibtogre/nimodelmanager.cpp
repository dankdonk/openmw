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

  Below code is basically Ogre::SkeletonManager with a new class name.

*/
#include "nimodelmanager.hpp"

#include <OgreResourceGroupManager.h>

#include "nimodel.hpp"

template<> NiBtOgre::NiModelManager* Ogre::Singleton<NiBtOgre::NiModelManager>::msSingleton = 0;

namespace NiBtOgre
{
    NiModelManager* NiModelManager::getSingletonPtr(void)
    {
        return msSingleton;
    }

    NiModelManager& NiModelManager::getSingleton(void)
    {
        assert( msSingleton ); return ( *msSingleton );
    }

    NiModelManager::NiModelManager()
    {
        mLoadOrder = 350.0f; // FIXME: how to choose an appropriate value?
        mResourceType = "NiModel";

        Ogre::ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
    }

    NiModelManager::~NiModelManager()
    {
        Ogre::ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
    }

    NiModelPtr NiModelManager::getByName(const Ogre::String& name, const Ogre::String& groupName)
    {
        return Ogre::static_pointer_cast<NiModel>(getResourceByName(name, groupName));
    }

    NiModelPtr NiModelManager::getOrLoadByName(const Ogre::String& name, const Ogre::String& groupName)
    {
        Ogre::ResourcePtr res = getResourceByName(name, groupName);

        if (!res)
            res = load(name, groupName);

        return Ogre::static_pointer_cast<NiModel>(res);
    }

    NiModelPtr NiModelManager::create (const Ogre::String& name, const Ogre::String& group,
                                       bool isManual, Ogre::ManualResourceLoader* loader,
                                       const Ogre::NameValuePairList* createParams)
    {
        return Ogre::static_pointer_cast<NiModel>(createResource(name, group, isManual, loader, createParams));
    }

    Ogre::Resource* NiModelManager::createImpl(const Ogre::String& name,
            Ogre::ResourceHandle handle, const Ogre::String& group, bool isManual,
            Ogre::ManualResourceLoader* loader, const Ogre::NameValuePairList* createParams)
    {
        // no use for createParams here
        return OGRE_NEW NiModel(this, name, handle, group, isManual, loader);
    }
}
