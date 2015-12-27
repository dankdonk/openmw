#ifndef CSM_FOREIGN_CELLREF_H
#define CSM_FOREIGN_CELLREF_H

#include <string>
#include <vector>

#include <extern/esm4/refr.hpp>

#include <components/esm/defs.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct CellRef : public ESM4::Reference
    {
        static unsigned int sRecordId;

        std::string mId;
        std::string mRefID; // not used
        std::string mCell;  // for region map (#x y for most exterior cells)
        std::string mOriginalCell; // not used

        ESM::Position mPos; // converted to OpenCS format

        CellRef();
        ~CellRef();

        void load(ESM4::Reader& esm);

        void blank();

        /// Calculate cell index based on coordinates (x and y)
        std::pair<int, int> getCellIndex() const;
    };
}

#endif // CSM_FOREIGN_CELLREF_H
