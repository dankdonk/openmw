/*
  Copyright (C) 2018, 2019 cc9cii

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
#include "skeletonloader.hpp"

#include <OgreSkeleton.h>
#include <OgreBone.h>

#include "nimodel.hpp"
#include "ninode.hpp"

NiBtOgre::SkeletonLoader::SkeletonLoader(const NiModel& model) : mModel(model)
{
}

// NOTE: can't use NIF block index number as the bone handle since Ogre::Skeleton limits the
//       max handle value to be 0x0100 (256) which is too small for skeleton.nif which has 71
//       bones (or 190 for v20.0.0.5) but the block index goes to 369 (or 817 for v20.0.0.5).
//
//       Ogre probably needs the bone handles to be contiguous, anyway.
//
// That also means we have to maintain a separate lookup map of bone handles and node
// indicies/names.
void NiBtOgre::SkeletonLoader::loadResource(Ogre::Resource *resource)
{
    Ogre::Skeleton* skeleton = static_cast<Ogre::Skeleton*>(resource);
    NiNode *skeletonRoot = mModel.getRef<NiNode>(mModel.getRootIndex());

    skeletonRoot->addBones(skeleton, nullptr, mIndexToHandleMap);
}
