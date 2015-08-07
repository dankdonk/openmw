#include "landscape.hpp"

#include <sstream>
#include <iostream> // FIXME

#include <extern/esm4/reader.hpp>

#include "../world/cell.hpp"
#include "../world/idcollection.hpp"

unsigned int CSMForeign::Landscape::sRecordId = ESM4::REC_LAND;

CSMForeign::Landscape::Landscape()
{
}

CSMForeign::Landscape::~Landscape()
{
}

void CSMForeign::Landscape::load(ESM4::Reader& reader, const CSMWorld::IdCollection<CSMWorld::Cell>& cells)
{
    load(reader);

    // correct ID // FIXME
    if (!mId.empty() && mId[0]!='#' && cells.searchId (mId)==-1)
    {
        std::ostringstream stream;

        //stream << "#" << mData.mX << " " << mData.mY;
        //stream << "#" << mLand->mX << " " << mLand->mY;

        mId = stream.str();
    }
}

void CSMForeign::Landscape::load(ESM4::Reader& reader)
{
#if 0
    ESM4::Land::load(esm);

    std::ostringstream stream;
    stream << "#" << mLand->mX << " " << mLand->mY; // FIXME

    mId = stream.str();
#endif
    ESM4::Land::load(reader);

    if (mCell.empty())
    {
        // HACK // FIXME
        std::ostringstream stream;
        stream << "#" << std::floor((float)reader.currCell().grid.x/2)
               << " " << std::floor((float)reader.currCell().grid.y/2);

        mId = stream.str();
    }
    else
        mId = mCell;
#if 0
        static VHGT vhgt;
        if (condLoad(flags, DATA_VHGT, &vhgt, sizeof(vhgt))) {
            float rowOffset = vhgt.mHeightOffset;
            for (int y = 0; y < LAND_SIZE; y++) {
                rowOffset += vhgt.mHeightData[y * LAND_SIZE];

                mLandData->mHeights[y * LAND_SIZE] = rowOffset * HEIGHT_SCALE;

                float colOffset = rowOffset;
                for (int x = 1; x < LAND_SIZE; x++) {
                    colOffset += vhgt.mHeightData[y * LAND_SIZE + x];
                    mLandData->mHeights[x + y * LAND_SIZE] = colOffset * HEIGHT_SCALE;
                }
            }
            mLandData->mUnk1 = vhgt.mUnk1;
            mLandData->mUnk2 = vhgt.mUnk2;
        }
#endif

    std::cout << "Landscape: " << mId << std::endl;
}

void CSMForeign::Landscape::blank()
{
}
