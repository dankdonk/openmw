#ifndef CSM_FOREIGN_CELL_H
#define CSM_FOREIGN_CELL_H

#include <string>

#include <extern/esm4/cell.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Cell : public ESM4::Cell
    {
        static unsigned int sRecordId;

        std::string mId;
        std::string mName;

        std::string mWorld;
        std::string mRegion; // for region map, probably will be removed

        void load (ESM4::Reader& reader);

        std::string getRegion() const;

        void blank();
    };
}

#endif // CSM_FOREIGN_CELL_H
