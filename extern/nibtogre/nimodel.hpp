/*
  Copyright (C) 2015-2018 cc9cii

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
#ifndef NIBTOGRE_NIMODEL_H
#define NIBTOGRE_NIMODEL_H

#include <vector>
#include <string>
#include <cstdint>

#include "nistream.hpp"
#include "header.hpp"

namespace Ogre
{
    class SceneNode;
}

namespace NiBtOgre
{
    class NiObject;
    struct BtOgreInst;

    //
    //   +--& NiModel
    //   |     o o o
    //   |     | | |
    //   |     | | +--- NiStream
    //   |     | |        o o
    //   |     | |        | |
    //   |     | |        | +-- NIF version numbers (local copy)
    //   |     | |        *
    //   |     | +----- Header
    //   |     |         o o
    //   |     |         | |
    //   |     |         | +--- NIF version numbers
    //   |     |         |
    //   |     |         +----- vector<string>
    //   |     |
    //   |     +------- vector<NiObject*>
    //   |                       o
    //   |                       |
    //   +-----------------------+
    //
    // NOTE: NiStream has a pointer to Header for the appending long strings (TES3/TES4)
    //       NiObject has a reference to NiModel for getting strings and other NiObject Ptrs/Refs
    //
    class NiModel
    {
        // NOTE the initialisation order
        NiStream mNiStream;
        Header   mHeader;

        std::vector<std::unique_ptr<NiObject> > mObjects;
        std::vector<std::uint32_t> mRoots;

        int mCurrIndex; // FIXME: for debugging Ptr
        std::string filename; // FIXME: for testing only

        // default, copy and assignment not allowed
        NiModel();
        NiModel(const NiModel& other);
        NiModel& operator=(const NiModel& other);

    public:
        // The parameter 'name' refers to those in the Ogre::ResourceGroupManager
        // e.g. full path from the directory/BSA added in Bsa::registerResources(), etc
        //
        // NOTE: the constructor may throw
        NiModel(const std::string &name);
        ~NiModel();

        const std::string& getName() const { return filename; } // FIXME: for testing only

        template<class T>
        inline T *getRef(std::int32_t index) const {

            if (index >= 0 && (index > mCurrIndex)) // FIXME: for debugging Ptr
                throw std::runtime_error("Ptr");

            return (index < 0) ? nullptr : static_cast<T*>(mObjects[index].get());
        }

        // WARNING: SceneNode in inst should have the scale (assumed uniform)
        void build(BtOgreInst *inst);

        // returns NiObject type name
        const std::string& blockType(std::uint32_t index) const {
            return mHeader.blockType(index);
        }

        // needed for the NIF version and strings (TES5)
        inline std::uint32_t nifVer() const { return mHeader.nifVer(); }

        inline const std::string& getLongString(std::int32_t index) const {
            return mHeader.getLongString(index);
        }
    };
}

#endif // NIBTOGRE_NIMODEL_H
