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
#ifndef ESM4_LAND_H
#define ESM4_LAND_H

#include <vector>
#include <string>
#include <cstdint>

namespace ESM4
{
    class Reader;

    struct Land
    {
#pragma pack(push,1)
        struct VHGT
        {
            float         heightOffset;
            std::int8_t   gradientData[33*33];
            std::uint16_t unknown1;
            unsigned char unknown2;
        };

        struct BTXT
        {
            std::uint32_t formId;
            std::uint8_t  quadrant; // 0 = bottom left. 1 = bottom right. 2 = upper-left. 3 = upper-right
            std::uint8_t  unknown1;
            std::uint16_t unknown2;
        };

        struct ATXT
        {
            std::uint32_t formId;
            std::uint8_t  quadrant; // 0 = bottom left. 1 = bottom right. 2 = upper-left. 3 = upper-right
            std::uint8_t  unknown;
            std::uint16_t layer;    // texture layer, 0..7
        };

        struct VTXT
        {
            std::uint16_t position; // 0..288 (17x17 grid)
            std::uint8_t  unknown1;
            std::uint8_t  unknown2;
            float         opacity;
        };
#pragma pack(pop)

        struct Texture
        {
            BTXT          base;
            ATXT          additional;
            std::vector<VTXT> data;
        };

        std::uint32_t mFlags;             // from DATA subrecord
        signed char   mVertNorm[33*33*3]; // from VNML subrecord
        signed char   mVertColr[33*33*3]; // from VCLR subrecord
        VHGT          mHeightMap;
        Texture       mTextures[4]; // 0 = bottom left. 1 = bottom right. 2 = upper-left. 3 = upper-right
        std::vector<std::uint32_t> mIds;  // land texture (LTEX) formids

        Land();
        ~Land();

        void load(ESM4::Reader& reader);
        //void save(ESM4::Writer& reader) const;

    //private:
        //Land(const Land& other);
        //Land& operator=(const Land& other);
    };
}

#endif // ESM4_LAND_H
