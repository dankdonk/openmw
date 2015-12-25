#ifndef CSM_FOREIGN_LANDSCAPECOLLECTION_H
#define CSM_FOREIGN_LANDSCAPECOLLECTION_H

#include <map>

#include "../world/collection.hpp"
//#include "../world/record.hpp"
#include "landscape.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class CellCollection;

    class LandscapeCollection : public CSMWorld::Collection<Landscape, CSMWorld::IdAccessor<Landscape> >//, public NestedCollection
    {
        const CSMForeign::CellCollection& mCells;

        // key - x/y coordinates, value - land formid (string)
        typedef std::map<std::pair<int, int>, std::string> CoordinateIndex;

        // key - world formId, value - map of lands
        std::map<std::uint32_t, CoordinateIndex> mPositionIndex;

    public:
        LandscapeCollection (const CellCollection& cells);
        ~LandscapeCollection ();

        // similar to IdCollection but with ESM4::Reader
        int load(ESM4::Reader& reader, bool base);

        // similar to IdCollection but with ESM4::Reader
        int load (const Landscape& record, bool base, int index = -2);

        virtual void loadRecord (Landscape& record, ESM4::Reader& reader);

        int searchId(int x, int y, std::uint32_t world = 0x3c) const; // defaults to Tamriel

    private:
        LandscapeCollection ();
        LandscapeCollection (const LandscapeCollection& other);
        LandscapeCollection& operator= (const LandscapeCollection& other);
    };
}
#endif // CSM_FOREIGN_LANDSCAPECOLLECTION_H
