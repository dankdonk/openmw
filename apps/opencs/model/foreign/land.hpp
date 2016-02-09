#ifndef CSM_FOREIGN_LAND_H
#define CSM_FOREIGN_LAND_H

#include <string>

#include <components/esm4terrain/land.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class CellCollection;

    struct Land : public ESM4Terrain::Land // NOTE: not derived from ESM4::Land
    {
        static unsigned int sRecordId;

        Land();
        ~Land();

        std::string mId;     // used by OpenCS to identify records (string instead of FormId)
        std::string mCellId; // for region map (#x y for most exterior cells)
        std::string mCellName; // Cell name

        void load(ESM4::Reader& reader);

        void blank();
    };
}

#endif // CSM_FOREIGN_LAND_H
