#ifndef CSM_FOREIGN_REGIONCOLLECTION_H
#define CSM_FOREIGN_REGIONCOLLECTION_H

#include "idcollection.hpp"
#include "region.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class WorldCollection;

    class RegionCollection : public IdCollection<Region>
    {
    public:
        RegionCollection ();
        ~RegionCollection ();

        // for populating World name strings (FULL or EDID)
        void updateWorldNames (const WorldCollection& worlds);

    private:
        RegionCollection (const RegionCollection& other);
        RegionCollection& operator= (const RegionCollection& other);
    };
}
#endif // CSM_FOREIGN_REGIONCOLLECTION_H
