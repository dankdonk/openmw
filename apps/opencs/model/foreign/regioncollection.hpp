#ifndef CSM_FOREIGN_REGIONCOLLECTION_H
#define CSM_FOREIGN_REGIONCOLLECTION_H

#include "../world/collection.hpp"
//#include "../world/record.hpp"

#include "region.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class WorldCollection;

    class RegionCollection : public CSMWorld::Collection<Region, CSMWorld::IdAccessor<Region> >
    {
    public:
        RegionCollection ();
        ~RegionCollection ();

        // similar to IdCollection but with ESM4::Reader
        int load(ESM4::Reader& reader, bool base);

        // similar to IdCollection but with ESM4::Reader
        int load (const Region& record, bool base, int index = -2);

        virtual void loadRecord (Region& record, ESM4::Reader& reader);

        // for populating World name strings (FULL or EDID)
        void updateWorldNames (const WorldCollection& worlds);

    private:
        RegionCollection (const RegionCollection& other);
        RegionCollection& operator= (const RegionCollection& other);
    };
}
#endif // CSM_FOREIGN_REGIONCOLLECTION_H
