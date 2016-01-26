/*
  Copyright (C) 2016 cc9cii

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
#ifndef ESM4_LVLC_H
#define ESM4_LVLC_H

#include <vector>

#include "common.hpp"

namespace ESM4
{
    class Reader;
    class Writer;
    typedef std::uint32_t FormId;

    struct LeveledCreature
    {
        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;

        FormId      mScript;
        FormId      mTemplate;
        std::int8_t mChanceNone;
        std::uint8_t mLvlCreaFlags;
        std::vector<LVLO> mLvlObject;

        LeveledCreature();
        ~LeveledCreature();

        void load(ESM4::Reader& reader);
        //void save(ESM4::Writer& reader) const;

        //void blank();
    };
}

#endif // ESM4_LVLC_H
