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

  Below code is basically Ogre::SkeletonManager with a different class name.

*/
#ifndef NIBTOGRE_NIMODELMANAGER_H
#define NIBTOGRE_NIMODELMANAGER_H

#include <OgreResourceManager.h>
#include <OgreSingleton.h>

namespace NiBtOgre
{
    class NiModel;
}
typedef Ogre::SharedPtr<NiBtOgre::NiModel> NiModelPtr;

namespace NiBtOgre
{
    class NiModelManager : public Ogre::ResourceManager, public Ogre::Singleton<NiModelManager>
    {
    public:
        /// Constructor
        NiModelManager();
        ~NiModelManager();

        /// Create a new NiModel
        /// @see ResourceManager::createResource
        NiModelPtr create(const Ogre::String& name, const Ogre::String& group,
                          bool isManual = false, Ogre::ManualResourceLoader* loader = 0,
                          const Ogre::NameValuePairList* createParams = 0);

        /// Get a resource by name
        /// @see ResourceManager::getResourceByName
        NiModelPtr getByName(const Ogre::String& name,
                             const Ogre::String& groupName
                             = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

        NiModelPtr getOrLoadByName(const Ogre::String& name,
                                   const Ogre::String& groupName
                                   = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

        /// @copydoc Singleton::getSingleton()
        static NiModelManager& getSingleton(void);
        /// @copydoc Singleton::getSingleton()
        static NiModelManager* getSingletonPtr(void);

    protected:
        /// @copydoc ResourceManager::createImpl
        Ogre::Resource* createImpl(const Ogre::String& name, Ogre::ResourceHandle handle,
                                   const Ogre::String& group, bool isManual,
                                   Ogre::ManualResourceLoader* loader,
                                   const Ogre::NameValuePairList* createParams);
    };
}

#endif // NIBTOGRE_NIMODELMANAGER_H
