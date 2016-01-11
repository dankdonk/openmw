#ifndef COMPONENTS_ESM4TERRAIN_LAND_H
#define COMPONENTS_ESM4TERRAIN_LAND_H

#include <string>

#include <extern/esm4/land.hpp>

namespace ESM4
{
    class Reader;
}

namespace ESM4Terrain
{
    struct LandData
    {
        // Initial reference height for the first vertex, only needed for filling mHeights
        float mHeightOffset;
        // Height in world space for each vertex
        float mHeights[ESM4::Land::VERTS_PER_SIDE * ESM4::Land::VERTS_PER_SIDE];

        // 24-bit normals, these aren't always correct though. Edge and corner normals may be garbage.
        signed char mNormals[ESM4::Land::LAND_NUM_VERTS * 3];

        // 2D array of texture indices. An index can be used to look up an ESM::LandTexture,
        // but to do so you must subtract 1 from the index first!
        // An index of 0 indicates the default texture.
        //uint16_t mTextures[LAND_NUM_TEXTURES];

        // 24-bit RGB color for each vertex
        unsigned char mColours[ESM4::Land::LAND_NUM_VERTS * 3];

        // DataTypes available in this LandData, accessing data that is not available is an undefined operation
        int mDataTypes;

        // low-LOD heightmap (used for rendering the global map)
        //signed char mWnam[81];

        //static void transposeTextureData(const std::uint16_t *in, std::uint16_t *out);
    };

    struct Land : public ESM4::Land
    {
        Land();
        ~Land();

        // copied data structure of ESM::Land so that OpenMW/OpenCS can render terrain
        int mX, mY; // Map coordinates.
        int mPlugin; // Plugin index, used to reference the correct material palette.
        LandData mLandData;

        const LandData *getLandData(int flags = 0) const; // flags is unused for now, all data is loaded up front
        LandData *getLandData();

        void load(ESM4::Reader& reader);
    };
}

#endif
