#ifndef OPENCS_RENDER_TERRAINSTORAGE_H
#define OPENCS_RENDER_TERRAINSTORAGE_H

#include <components/esm4terrain/storage.hpp>

#include "../../model/world/data.hpp"

namespace ESM4
{
    struct LandTexture;
    struct Land;
    typedef std::uint32_t FormId;
}

namespace CSVForeign
{

    /**
     * @brief A bridge between the terrain component and OpenCS's terrain data storage.
     */
    class TerrainStorage : public ESM4Terrain::Storage
    {
    public:
        TerrainStorage(const CSMWorld::Data& data);

    private:
        const CSMWorld::Data& mData;

        virtual const ESM4::Land* getLand (int cellX, int cellY);
        virtual const ESM4::LandTexture* getLandTexture(ESM4::FormId formId, short plugin);

        virtual void getBounds(float& minX, float& maxX, float& minY, float& maxY);
    };

}

#endif
