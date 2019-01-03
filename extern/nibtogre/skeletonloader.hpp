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
#ifndef NIBTOGRE_SKELETONLOADER_H
#define NIBTOGRE_SKELETONLOADER_H

#include <map>

#include <OgreResource.h>

namespace NiBtOgre
{
    class NiModel;

    // There is one skeleton per NIF model.  Hence the unique name for the
    // Ogre::SkeletonManager is the NIF name.
    //
    // TODO: different games might be using the same name, so some kind of
    //       namespace concept is needed
    class SkeletonLoader : public Ogre::ManualResourceLoader
    {
        const NiModel& mModel;

        //      NiNode block index
        //        |
        //        |            bone handle
        //        |              |
        //        v              v
        std::map<std::uint32_t, std::uint16_t> mIndexToHandleMap;

    public:

        SkeletonLoader(const NiModel& model);

        // reimplement Ogre::ManualResourceLoader
        void loadResource(Ogre::Resource *resource);
    };
}

#endif // NIBTOGRE_SKELETONLOADER_H
