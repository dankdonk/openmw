/*
  Copyright (C) 2015-2017 cc9cii

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
#ifndef NIBTOGRE_HEADER_H
#define NIBTOGRE_HEADER_H

#include <string>
#include <vector>

namespace NiBtOgre
{
    class NiStream;

    // The header contains the NIF file version and a list of all the object names used in the file.
    // The object names are then used to create the corresponding classes.
    // TODO: factory pattern using a simple map keyed on NIF object names?
    //
    // NOTE: Any exceptions thrown by the stream reader are not caught in the constructor.  It is
    // expected that exeptions are caught and handed upstream (e.g. abort loading the whole file)
    class Header
    {
        std::uint32_t mVer;
        std::uint32_t mUserVer;
        std::uint32_t mUserVer2;

        std::uint32_t mNumBlocks;

        std::string   mCreator;      // not used
        std::string   mExportInfo1;  // not used
        std::string   mExportInfo2;  // not used

        std::vector<std::string>   mBlockTypes;
        std::vector<std::int16_t>  mBlockTypeIndex;
        std::vector<std::uint32_t> mBlockSize; // TES5 only, probably for sanity check
        std::vector<std::string>   mStrings;   // TES5 only

        // default, copy and assignment not allowed
        Header();
        Header(const Header& other);
        Header& operator=(const Header& other);

    public:
        Header(NiStream& stream); // may throw std::runtime_error
        ~Header() {}

        // may throw (does not check bounds)
        inline const std::string& getLongString(std::int32_t index) const {
            return mStrings.at(index);
        }

        // returns index
        std::uint32_t appendLongString(std::string&& str);

        // returns the number of NiObjects
        inline std::uint32_t numBlocks() const { return mNumBlocks; }

        // returns NiObject type name for creation via a factory
        const std::string& blockType(std::uint32_t index) const;
    };
}

#endif // NIBTOGRE_HEADER_H
