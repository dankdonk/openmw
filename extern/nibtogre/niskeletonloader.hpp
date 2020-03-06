/*
  Copyright (C) 2018-2020 cc9cii

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
#ifndef NIBTOGRE_NISKELETONLOADER_H
#define NIBTOGRE_NISKELETONLOADER_H

#include <map>
#include <cstdint>

#include <OgreResource.h>

namespace Ogre
{
    class Skeleton;
}

namespace NiBtOgre
{
    class NiModel;
    class NiNode;

    // There is one skeleton per NIF model.  Hence the NIF name (i.e. full path to the NIF) is
    // used as the unique name for the Ogre::SkeletonManager.
    //
    // TODO: different games might be using the same name, so some kind of namespace concept
    //       may be needed
    class NiSkeletonLoader : public Ogre::ManualResourceLoader
    {
        //const NiModel& mModel;

        //      NiNode block index
        //        |
        //        |            bone handle
        //        |              |
        //        v              v
        std::map<std::uint32_t, std::uint16_t> mIndexToHandleMap; // NOTE: not used currently

        enum ModelBuildType
        {
            MBT_Object,
            MBT_Skeleton
        };

        struct ModelBuildInfo
        {
            ModelBuildType type;
            NiModel *model; // better if const but want to call skeletonRoot()
            //std::int32_t ninode;  // NiNodeRef
        };

        static std::map<Ogre::Resource*, ModelBuildInfo> sModelBuildInfoMap;

        Ogre::SkeletonPtr createManual(const Ogre::String& name, const Ogre::String& group);

        void loadManualSkeleton(Ogre::Skeleton* pSkel, NiNode *skeletonRoot);
        void loadFullSkeleton(Ogre::Skeleton* pSkel, NiNode *skeletonRoot);

    public:
        NiSkeletonLoader();
        ~NiSkeletonLoader();

        Ogre::SkeletonPtr createSkeleton(const Ogre::String& name, const Ogre::String& group,
                NiModel *model/*, std::int32_t ninode*/);

        Ogre::SkeletonPtr createFullSkeleton(const Ogre::String& name, const Ogre::String& group,
                NiModel *model);

        // reimplement Ogre::ManualResourceLoader
        void loadResource(Ogre::Resource *res);
    };
}

#endif // NIBTOGRE_NISKELETONLOADER_H
