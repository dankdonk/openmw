#ifndef OPENMW_FOREIGN_TERRAINSTORAGE_H
#define OPENMW_FOREIGN_TERRAINSTORAGE_H

#include <components/esm4terrain/storage.hpp>

namespace ESM4
{
    struct LandTexture;
    typedef std::uint32_t FormId;
}

namespace ESM4Terrain
{
    struct Land;
}

namespace Foreign
{
    class TerrainStorage : public ESM4Terrain::Storage
    {
    public:
        TerrainStorage(ESM4::FormId world);

    private:
        ESM4::FormId mWorld;

        virtual const ESM4Terrain::Land* getLand (int cellX, int cellY);
        virtual const ESM4::LandTexture* getLandTexture(ESM4::FormId formId);

    public:
        virtual void getBounds(float& minX, float& maxX, float& minY, float& maxY);
    };

}

#endif
