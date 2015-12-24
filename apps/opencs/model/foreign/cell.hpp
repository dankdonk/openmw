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

        void load (ESM4::Reader& reader);

        void blank();
    };
}

#endif // CSM_FOREIGN_CELL_H
