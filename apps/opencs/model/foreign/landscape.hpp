#ifndef CSM_FOREIGN_LANDSCAPE_H
#define CSM_FOREIGN_LANDSCAPE_H

#include <string>

#include <extern/esm4/loadland.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMWorld
{
    struct Cell;

    template<typename T>
    struct IdAccessor;

    template<typename T, typename AT>
    class IdCollection;
}

namespace CSMForeign
{
    struct Landscape : public ESM4::Land
    {
        static unsigned int sRecordId;

        Landscape();
        ~Landscape();

        std::string mId;
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
            DATA_VNML = 1,
            DATA_VHGT = 2,
            DATA_WNAM = 4,
            DATA_VCLR = 8,
            DATA_VTEX = 16
        };

        // number of vertices per side
        static const int LAND_SIZE = 65;

        // cell terrain size in world coords
        static const int REAL_SIZE = 8192;

        // total number of vertices
        static const int LAND_NUM_VERTS = LAND_SIZE * LAND_SIZE;

        static const int HEIGHT_SCALE = 8;

        //number of textures per side of land
        static const int LAND_TEXTURE_SIZE = 16;

        //total number of textures per land
        static const int LAND_NUM_TEXTURES = LAND_TEXTURE_SIZE * LAND_TEXTURE_SIZE;

        typedef signed char VNML;

        struct LandData
        {
            float mHeightOffset;
            float mHeights[LAND_NUM_VERTS];
            VNML mNormals[LAND_NUM_VERTS * 3];
            uint16_t mTextures[LAND_NUM_TEXTURES];

            char mColours[3 * LAND_NUM_VERTS];
            int mDataTypes;

            // low-LOD heightmap (used for rendering the global map)
            signed char mWnam[81];

            short mUnk1;
            uint8_t mUnk2;

            //void save(ESMWriter &esm);
            static void transposeTextureData(uint16_t *in, uint16_t *out);
        };

        LandData *mLandData;







        void load(ESM4::Reader& esm,
                const CSMWorld::IdCollection<CSMWorld::Cell, CSMWorld::IdAccessor<CSMWorld::Cell> >& cells);

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_LANDSCAPE_H
