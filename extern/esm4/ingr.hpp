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
#ifndef ESM4_INGR_H
#define ESM4_INGR_H

#include <string>
#include <cstdint>
#include <vector>

namespace ESM4
{
    class Reader;
    class Writer;
    typedef std::uint32_t FormId;

    struct Ingredient
    {
#pragma pack(push, 1)
        struct Data
        {
            float         weight;
        };

        struct ENIT
        {
            std::uint32_t value;
            std::uint32_t flags;
        };
#pragma pack(pop)

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;
        std::string mIcon; // inventory

        std::vector<std::string> mScriptEffect; // FIXME: prob. should be in a struct
        FormId mScript;
        ENIT   mEnchantment;

        Data mData;

        Ingredient();
        ~Ingredient();

        void load(ESM4::Reader& reader);
        //void save(ESM4::Writer& reader) const;

        //void blank();
    };
}

#endif // ESM4_INGR_H
