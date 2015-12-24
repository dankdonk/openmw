#ifndef CSM_FOREIGN_LANDSCAPECOLLECTION_H
#define CSM_FOREIGN_LANDSCAPECOLLECTION_H

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
        //Landscape mNavMesh;

    public:
        LandscapeCollection (const CellCollection& cells);
        ~LandscapeCollection ();

        // similar to IdCollection but with ESM4::Reader
        int load(ESM4::Reader& reader, bool base);

        // similar to IdCollection but with ESM4::Reader
        int load (const Landscape& record, bool base, int index = -2);

        virtual void loadRecord (Landscape& record, ESM4::Reader& reader);

    private:
        LandscapeCollection ();
        LandscapeCollection (const LandscapeCollection& other);
        LandscapeCollection& operator= (const LandscapeCollection& other);
    };
}
#endif // CSM_FOREIGN_LANDSCAPECOLLECTION_H