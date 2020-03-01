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

  Below code is based on Ogre::SkeletonManager and Ogre::MeshManager.

*/
#ifndef NIBTOGRE_NIMODELMANAGER_H
#define NIBTOGRE_NIMODELMANAGER_H

#include <map>

#include <OgreResourceManager.h>
#include <OgreSingleton.h>
#include <OgreResource.h>

namespace ESM4
{
    struct Npc;
    struct Race;
}

namespace NiBtOgre
{
    class NiModel;
}
typedef Ogre::SharedPtr<NiBtOgre::NiModel> NiModelPtr;

namespace NiBtOgre
{
    class NiModelManager
        : public Ogre::ResourceManager, public Ogre::Singleton<NiModelManager>, public Ogre::ManualResourceLoader
    {
        enum ModelBuildType
        {
            MBT_Object,
            MBT_Skinned,
            MBT_Skeleton,
            MBT_Morphed
        };

        enum ModelBodyPart
        {
            Head        = 0,
            EarMale     = 1,
            EarFemale   = 2,
            Mouth       = 3,
            TeethLower  = 4,
            TeethUpper  = 5,
            Tongue      = 6,
            EyeLeft     = 7,
            EyeRight    = 8,
            UpperBody   = 9,
            Hair        = 10
        };

        struct ModelBuildInfo
        {
            ModelBuildType type;
            const ESM4::Npc *npc;   // FIXME: danger in holding pointers here?
            const ESM4::Race *race;
            ModelBodyPart bodyPart;
            Ogre::String baseNif;
            Ogre::String baseTexture;
        };

        std::map<Ogre::Resource*, ModelBuildInfo> mModelBuildInfoMap;

        void loadManualMorphedModel(NiModel* pModel, const ModelBuildInfo& params);

    public:
        NiModelManager();
        ~NiModelManager();

        NiModelPtr getByName(const Ogre::String& name,
                const Ogre::String& group = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

        NiModelPtr create(const Ogre::String& name, const Ogre::String& group,
                bool isManual = false,
                Ogre::ManualResourceLoader* loader = 0,
                const Ogre::NameValuePairList* createParams = 0);

        NiModelPtr createManual(const Ogre::String& name, const Ogre::String& group,
                const Ogre::String& nif, Ogre::ManualResourceLoader* loader);

        static NiModelManager& getSingleton(void);
        static NiModelManager* getSingletonPtr(void);

        void loadResource(Ogre::Resource* res);

        NiModelPtr createMorphed(const Ogre::String& nif, const Ogre::String& group,
                const ESM4::Npc *npc, const ESM4::Race *race, const Ogre::String& texture);

        NiModelPtr getOrLoadByName(const Ogre::String& name,
                const Ogre::String& group = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

    protected:
        Ogre::Resource* createImpl(const Ogre::String& name, Ogre::ResourceHandle handle,
                const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
                const Ogre::NameValuePairList* createParams);
    };
}

#endif // NIBTOGRE_NIMODELMANAGER_H
