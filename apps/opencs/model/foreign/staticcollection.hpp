#ifndef CSM_FOREIGN_STATICCOLLECTION_H
#define CSM_FOREIGN_STATICCOLLECTION_H

#include "../world/collection.hpp"
//#include "../world/record.hpp"

#include "static.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class WorldCollection;

    class StaticCollection : public CSMWorld::Collection<Static, CSMWorld::IdAccessor<Static> >
    {
    public:
        StaticCollection ();
        ~StaticCollection ();

        // similar to IdCollection but with ESM4::Reader
        int load(ESM4::Reader& reader, bool base);

        // similar to IdCollection but with ESM4::Reader
        int load (const Static& record, bool base, int index = -2);

        virtual void loadRecord (Static& record, ESM4::Reader& reader);

        // for populating World name strings (FULL or EDID)
        void updateWorldNames (const WorldCollection& worlds);

    private:
        StaticCollection (const StaticCollection& other);
        StaticCollection& operator= (const StaticCollection& other);
    };
}
#endif // CSM_FOREIGN_STATICCOLLECTION_H
