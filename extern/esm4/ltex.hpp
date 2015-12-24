/*
  Copyright (C) 2015 cc9cii

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
#ifndef ESM4_LTEX_H
#define ESM4_LTEX_H

#include <string>
#include <cstdint>

namespace ESM4
{
    class Reader;
    class Writer;

    struct LandTexture
    {
        std::uint32_t mFormId; // from the header
        std::uint32_t mFlags;  // from the header, see enum type RecordFlag for details

        std::string mEditorId;

        std::uint8_t mHavokFriction;
        std::uint8_t mHavokRestitution;

        std::uint8_t mTextureSpecular; // default 30
        std::uint32_t mGrass;

        // ------ TES4 only -----

        std::string mTextureFile;
        std::uint8_t mHavokMaterial;

        // ------ TES5 only -----

        std::uint32_t mTexture;
        std::uint32_t mMaterial;

        // ----------------------

        LandTexture();
        ~LandTexture();

        void load(ESM4::Reader& reader);
        //void save(ESM4::Writer& reader) const;

        //void blank();
    };
}

#endif // ESM4_LTEX_H