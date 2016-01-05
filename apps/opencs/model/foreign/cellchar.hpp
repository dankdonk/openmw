#ifndef CSM_FOREIGN_CELLCHAR_H
#define CSM_FOREIGN_CELLCHAR_H

#include <string>
#include <vector>

#include <extern/esm4/achr.hpp>

#include <components/esm/defs.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct CellChar : public ESM4::Character
    {
        static unsigned int sRecordId;

        std::string mId;
        std::string mRefID; // mBaseObj
        std::string mCell;  // for region map (#x y for most exterior cells)
        std::string mOriginalCell; // not used

        ESM::Position mPos; // converted to OpenCS format

        CellChar();
        ~CellChar();

        void load(ESM4::Reader& esm);

        void blank();

        /// Calculate cell index based on coordinates (x and y)
        std::pair<int, int> getCellIndex() const;
    };
}

#endif // CSM_FOREIGN_CELLCHAR_H
