#include "land.hpp"

#include <extern/esm4/reader.hpp>

ESM4Terrain::Land::Land()
{
}

ESM4Terrain::Land::~Land()
{
}

// According to http://www.uesp.net/wiki/Tes5Mod:Mod_File_Format/LAND
//
//   float[33,33] heightmap; // heightmap in game units
//
//   float offset = ReadFloat() * 8; // whole cell offset each unit equals 8 game units
//   float row_offset = 0;
//
//   for (int i = 0; i < 1089; i++)
//   {
//       float value = ReadSByte() * 8; // signed byte, each unit equals 8 game units
//
//       int r = i / 33;
//       int c = i % 33;
//
//       if (c == 0) // first column value controls height for all remaining points in cell
//       {
//           row_offset = 0;
//           offset += value;
//       }
//       else // other col values control height of all points in the same row
//       {
//           row_offset += value;
//       }
//
//       heightmap[c, r] = offset + row_offset;
//   }

void ESM4Terrain::Land::load(ESM4::Reader& reader)
{
    ESM4::Land::load(reader);

    mLandData.mHeightOffset = mHeightMap.heightOffset;
    float rowOffset = mHeightMap.heightOffset;

    for (int y = 0; y < ESM4::Land::VERTS_PER_SIDE; y++)
    {
        rowOffset += mHeightMap.gradientData[y * ESM4::Land::VERTS_PER_SIDE];

        mLandData.mHeights[y * ESM4::Land::VERTS_PER_SIDE] = rowOffset * ESM4::Land::HEIGHT_SCALE;

        float colOffset = rowOffset;
        for (int x = 1; x < ESM4::Land::VERTS_PER_SIDE; x++)
        {
            colOffset += mHeightMap.gradientData[y * ESM4::Land::VERTS_PER_SIDE + x];
            mLandData.mHeights[x + y * ESM4::Land::VERTS_PER_SIDE] = colOffset * ESM4::Land::HEIGHT_SCALE;
        }
    }

    //mLandData.mNormals = &mVertNorm;

    //static uint16_t vtex[LAND_NUM_TEXTURES];
    //LandData::transposeTextureData(vtex, mLandData.mTextures);

    //mLandData.mColours = mVertColr;

    mLandData.mDataTypes = mDataTypes;
}

const ESM4Terrain::LandData *ESM4Terrain::Land::getLandData(int flags) const
{
    return &mLandData;
}

ESM4Terrain::LandData *ESM4Terrain::Land::getLandData()
{
    return &mLandData;
}

#if 0
void ESM4Terrain::LandData::transposeTextureData(const std::uint16_t *in, std::uint16_t *out)
{
    int readPos = 0; //bit ugly, but it works
    for ( int y1 = 0; y1 < 4; y1++ )
        for ( int x1 = 0; x1 < 4; x1++ )
            for ( int y2 = 0; y2 < 4; y2++)
                for ( int x2 = 0; x2 < 4; x2++ )
                    out[(y1*4+y2)*16+(x1*4+x2)] = in[readPos++];
}
#endif
