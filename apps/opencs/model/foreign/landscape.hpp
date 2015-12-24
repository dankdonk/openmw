#ifndef CSM_FOREIGN_LANDSCAPE_H
#define CSM_FOREIGN_LANDSCAPE_H

#include <string>

#include <extern/esm4/land.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class CellCollection;

    struct Landscape : public ESM4::Land
    {
        static unsigned int sRecordId;

        Landscape();
        ~Landscape();

        std::string mId;
        std::string mName;
        std::string mCell; // Cell name

        // copied data structure of ESM::Land so that OpenMW/OpenCS can render terrain




        int mX, mY; // Map coordinates.
        int mPlugin; // Plugin index, used to reference the correct material palette.

        // File context. This allows the ESM reader to be 'reset' to this
        // location later when we are ready to load the full data set.
        //ESMReader* mEsm;
        //ESM_Context mContext;

        int mDataTypes;
        int mDataLoaded;

        enum
        {
            DATA_VNML = 1, // vertex normals
            DATA_VHGT = 2, // vertex gradient height map
            DATA_WNAM = 4,
            DATA_VCLR = 8, // vertex colour
            DATA_VTEX = 16 // formId of LTEX records (TES4)
        };

        // number of vertices per side
        static const int FOREIGN_LAND_SIZE = 33;

        // cell terrain size in world coords
        static const int REA_FOREIGNL_SIZE = 4096;

        // total number of vertices
        static const int FOREIGN_LAND_NUM_VERTS = FOREIGN_LAND_SIZE * FOREIGN_LAND_SIZE;

        static const int FOREIGN_HEIGHT_SCALE = 8;

        //number of textures per side of land
        static const int FOREIGN_LAND_TEXTURE_SIZE = 16;

        //total number of textures per land
        static const int FOREIGN_LAND_NUM_TEXTURES = FOREIGN_LAND_TEXTURE_SIZE * FOREIGN_LAND_TEXTURE_SIZE;

        typedef signed char VNML;

        struct LandData
        {
            float mHeightOffset;
            float mHeights[FOREIGN_LAND_NUM_VERTS];
            VNML mNormals[FOREIGN_LAND_NUM_VERTS * 3];
            uint16_t mTextures[FOREIGN_LAND_NUM_TEXTURES];

            char mColours[3 * FOREIGN_LAND_NUM_VERTS];
            int mDataTypes;

            // low-LOD heightmap (used for rendering the global map)
            signed char mWnam[81];

            short mUnk1;
            uint8_t mUnk2;

            //void save(ESMWriter &esm);
            static void transposeTextureData(uint16_t *in, uint16_t *out);
        };

        LandData *mLandData;







        void load(ESM4::Reader& esm, const CellCollection& cells);

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_LANDSCAPE_H
