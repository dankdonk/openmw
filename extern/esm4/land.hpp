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
        // FIXME
        enum
        {
            DATA_VNML = 1,
            DATA_VHGT = 2,
            DATA_WNAM = 4,
            DATA_VCLR = 8,
            DATA_VTEX = 16
        };

        int mDataTypes; // FIXME

        // number of vertices per side
        static const int VERTS_SIDE = 33;

        // cell terrain size in world coords
        static const int REAL_SIZE = 4096;

        // total number of vertices
        //static const int LAND_NUM_VERTS = VERTS_SIDE * VERTS_SIDE;

        static const int HEIGHT_SCALE = 8;

        //number of textures per side of land
        static const int LAND_TEXTURE_SIZE = 16;

        //total number of textures per land
        //static const int LAND_NUM_TEXTURES = LAND_TEXTURE_SIZE * LAND_TEXTURE_SIZE;

#pragma pack(push,1)
        struct VHGT
        {
            float         heightOffset;
            std::int8_t   gradientData[VERTS_SIDE * VERTS_SIDE];
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

        std::uint32_t mFormId; // from the header
        std::uint32_t mFlags;  // from the header, see enum type RecordFlag for details

        std::uint32_t mLandFlags;         // from DATA subrecord

        // FIXME
        struct LandData
        {
#if 0
            // Initial reference height for the first vertex, only needed for filling mHeights
            float mHeightOffset;
            // Height in world space for each vertex
            float mHeights[LAND_NUM_VERTS];

            // 24-bit normals, these aren't always correct though. Edge and corner normals may be garbage.
            signed char mNormals[LAND_NUM_VERTS * 3];

            // 2D array of texture indices. An index can be used to look up an ESM::LandTexture,
            // but to do so you must subtract 1 from the index first!
            // An index of 0 indicates the default texture.
            uint16_t mTextures[LAND_NUM_TEXTURES];

            // 24-bit RGB color for each vertex
            unsigned char mColours[3 * LAND_NUM_VERTS];

            // DataTypes available in this LandData, accessing data that is not available is an undefined operation
            int mDataTypes;

            // low-LOD heightmap (used for rendering the global map)
            signed char mWnam[81];

            // ???
            short mUnk1;
            uint8_t mUnk2;
#endif
            signed char   mVertNorm[VERTS_SIDE * VERTS_SIDE * 3]; // from VNML subrecord
            signed char   mVertColr[VERTS_SIDE * VERTS_SIDE * 3]; // from VCLR subrecord
            //VHGT          mHeightMap;
            //float         mHeightOffset; // probably not used
            float         mHeights[VERTS_SIDE * VERTS_SIDE]; // filled during load
            Texture       mTextures[4]; // 0 = bottom left. 1 = bottom right. 2 = upper-left. 3 = upper-right
            std::vector<std::uint32_t> mIds;  // land texture (LTEX) formids

            //void save(Writer &writer) const;
            static void transposeTextureData(const std::uint16_t *in, std::uint16_t *out);
        };

        LandData mLandData;

        Land();
        ~Land();

        // FIXME
        const LandData *getLandData(int flags) const;
        const LandData *getLandData() const;
        LandData *getLandData();

        void load(Reader& reader);
        //void save(Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_LAND_H
