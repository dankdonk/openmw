#ifndef CSM_FOREIGN_LANDCOLLECTION_H
#define CSM_FOREIGN_LANDCOLLECTION_H

#include <map>

#include "../world/collection.hpp"

#include "land.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class CellCollection;

    class LandCollection : public CSMWorld::Collection<Land, CSMWorld::IdAccessor<Land> >
    {
        const CSMForeign::CellCollection& mCells;

        // key - x/y coordinates, value - land formid (string)
        typedef std::map<std::pair<int, int>, std::string> CoordinateIndex;

        // key - world formId, value - map of lands
        std::map<std::uint32_t, CoordinateIndex> mPositionIndex;

    public:
        LandCollection (const CellCollection& cells);
        ~LandCollection ();

        // similar to IdCollection but with ESM4::Reader
        int load(ESM4::Reader& reader, bool base);

        // similar to IdCollection but with ESM4::Reader
        int load (const Land& record, bool base, int index = -2);

        virtual void loadRecord (Land& record, ESM4::Reader& reader);

        int searchId(int x, int y, std::uint32_t world = 0x3c) const; // defaults to Tamriel

    private:
        LandCollection ();
        LandCollection (const LandCollection& other);
        LandCollection& operator= (const LandCollection& other);
    };
}
#endif // CSM_FOREIGN_LANDCOLLECTION_H
