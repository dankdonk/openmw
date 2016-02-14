#ifndef CSM_FOREIGN_LANDCOLLECTION_H
#define CSM_FOREIGN_LANDCOLLECTION_H

#include <map>

#include <extern/esm4/land.hpp>

#include "land.hpp"
#include "idcollection.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class CellGroupCollection;

    class LandCollection : public IdCollection<Land>
    {
        CellGroupCollection& mCellGroups;

        // key - x/y coordinates, value - land formId
        typedef std::map<std::pair<int, int>, ESM4::FormId> CoordinateIndex;

        // key - world formId, value - map of lands
        std::map<ESM4::FormId, CoordinateIndex> mPositionIndex;

    public:
        LandCollection (CellGroupCollection& cellGroups);
        ~LandCollection ();

        virtual int load(ESM4::Reader& reader, bool base);

        int searchId (std::int16_t x, std::int16_t y, ESM4::FormId world = 0x3c) const; // default is Tamriel

    private:
        LandCollection ();
        LandCollection (const LandCollection& other);
        LandCollection& operator= (const LandCollection& other);
    };
}
#endif // CSM_FOREIGN_LANDCOLLECTION_H
