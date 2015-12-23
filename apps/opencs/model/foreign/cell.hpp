#ifndef CSM_FOREIGN_CELL_H
#define CSM_FOREIGN_CELL_H

#include <string>

#include <extern/esm4/loadcell.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Cell : public ESM4::Cell
    {
        static unsigned int sRecordId;

        std::string mId; // required by Collection<T>
        std::string mName;

        void load (ESM4::Reader& reader);

        void blank(); // required by Collection<T>
    };
}

#endif // CSM_FOREIGN_CELL_H
