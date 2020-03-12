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
#include "niskeletonloader.hpp"

#include <iostream> // FIXME

#include <OgreSkeleton.h>
#include <OgreSkeletonManager.h>
#include <OgreBone.h>

#include "nimodel.hpp"
#include "nimodelmanager.hpp"
#include "ninode.hpp"

namespace NiBtOgre
{
    std::map<Ogre::Resource*, NiSkeletonLoader::ModelBuildInfo> NiSkeletonLoader::sModelBuildInfoMap;

    NiSkeletonLoader::NiSkeletonLoader()
    {
    }

    NiSkeletonLoader::~NiSkeletonLoader()
    {
    }

    Ogre::SkeletonPtr NiSkeletonLoader::createSkeleton(const Ogre::String& name,
            const Ogre::String& group, NiModel *model/*, std::int32_t ninode*/)
    {
        // Create manual model which calls back self to load
        Ogre::SkeletonPtr pSkel = createManual(name, group);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Object;
        bInfo.model = model;
        //bInfo.ninode = ninode;
        sModelBuildInfoMap[pSkel.get()] = bInfo;

        return pSkel; // at this point the mesh is created but not yet loaded
    }

    Ogre::SkeletonPtr NiSkeletonLoader::createFullSkeleton(const Ogre::String& name,
            const Ogre::String& group, NiModel *model/*, std::int32_t ninode*/)
    {
        // Create manual model which calls back self to load
        Ogre::SkeletonPtr pSkel = createManual(name, group);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Skeleton;
        bInfo.model = model;
        sModelBuildInfoMap[pSkel.get()] = bInfo;

        return pSkel; // at this point the mesh is created but not yet loaded
    }

    Ogre::SkeletonPtr NiSkeletonLoader::createManual(const Ogre::String& name, const Ogre::String& group)
    {
        Ogre::SkeletonManager& skelManager = Ogre::SkeletonManager::getSingleton();

        // Don't try to get existing, create should fail if already exists
        if( skelManager.getResourceByName( name, group ) )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_DUPLICATE_ITEM,
                         "Skeleton with name '" + name + "' already exists.",
                         "NiSkeletonLoader::createManual" );
        }

        return skelManager.create(name, group, true/*manual*/, this);
    }

    // NOTE: can't use NIF block index number as the bone handle since Ogre::Skeleton limits the
    //       max handle value to be 0x0100 (256) which is too small for skeleton.nif which has 71
    //       bones (or 190 for v20.0.0.5) but the block index goes to 369 (or 817 for v20.0.0.5).
    //
    //       Ogre probably needs the bone handles to be contiguous, anyway.
    //
    // That also means we have to maintain a separate lookup map of bone handles and node
    // indicies/names.
    void NiSkeletonLoader::loadResource(Ogre::Resource *res)
    {
        Ogre::Skeleton *skel = static_cast<Ogre::Skeleton*>(res);

        // Find build parameters
        std::map<Ogre::Resource*, ModelBuildInfo>::iterator it = sModelBuildInfoMap.find(res);
        if (it == sModelBuildInfoMap.end())
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot find build parameters for " + res->getName(),
                         "NiMeshLoader::loadResource");
        }

        NiNode *skeletonRoot = nullptr;
        ModelBuildInfo& bInfo = it->second;

        try
        {
#if 1
            skeletonRoot = bInfo.model->getRef<NiNode>(bInfo.model->rootIndex()); // get NiNode
            skeletonRoot->getName();
#else
            skeletonRoot = bInfo.model->skeletonRoot(); // get NiNode
            skeletonRoot->getName(); // see if the pointer is good
#endif
        }
        catch (...)
        {
            NiModelManager& modelManager = NiModelManager::getSingleton();
            NiModelPtr model = modelManager.getOrLoadByName(skel->getName(), skel->getGroup());

#if 1
            skeletonRoot = model->getRef<NiNode>(model->rootIndex());
#else
            skeletonRoot = bInfo.model->skeletonRoot(); // get NiNode
#endif
        }

        switch(bInfo.type)
        {
        case MBT_Object:
            loadManualSkeleton(skel, skeletonRoot);
            break;
        case MBT_Skeleton:
            loadManualSkeleton(skel, skeletonRoot); // NOTE: was loadFullSkeleton
            break;
        default:
            OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                         "Unknown build parameters for " + res->getName(),
                         "NiSkeletonLoader::loadResource");
        }
    }

    void NiSkeletonLoader::loadManualSkeleton(Ogre::Skeleton* pSkel, NiNode *skeletonRoot)
    {
        skeletonRoot->addBones(pSkel, nullptr, mIndexToHandleMap);
    }

    // deprecated
    void NiSkeletonLoader::loadFullSkeleton(Ogre::Skeleton* pSkel, NiNode *skeletonRoot)
    {
        skeletonRoot->addAllBones(pSkel, nullptr);
    }
}
