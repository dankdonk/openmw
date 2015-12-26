#ifndef CSM_FOREIGN_LANDCOLLECTION_H
#define CSM_FOREIGN_LANDCOLLECTION_H

#include <map>

#include "../world/collection.hpp"

#include "land.hpp"

namespace ESM4
{
    class Reader;
    typedef std::uint32_t FormId;
}

namespace CSMForeign
{
    class CellCollection;

    class LandCollection : public CSMWorld::Collection<Land, CSMWorld::IdAccessor<Land> >
    {
        const CSMForeign::CellCollection& mCells; // FIXME: not used, delete?

        // key - x/y coordinates, value - land formid (string)
        typedef std::map<std::pair<int, int>, std::string> CoordinateIndex;

        // key - world formId, value - map of lands
        std::map<ESM4::FormId, CoordinateIndex> mPositionIndex;

    public:
        LandCollection (const CellCollection& cells);
        ~LandCollection ();

        // similar to IdCollection but with ESM4::Reader
        int load(ESM4::Reader& reader, bool base);

        // similar to IdCollection but with ESM4::Reader
        int load (const Land& record, bool base, int index = -2);

        virtual void loadRecord (Land& record, ESM4::Reader& reader);

        int searchId (std::int16_t x, std::int16_t y, ESM4::FormId world = 0x3c) const; // default is Tamriel

    private:
        LandCollection ();
        LandCollection (const LandCollection& other);
        LandCollection& operator= (const LandCollection& other);
    };
}
#endif // CSM_FOREIGN_LANDCOLLECTION_H
