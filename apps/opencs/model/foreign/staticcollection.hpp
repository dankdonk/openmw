#ifndef CSM_FOREIGN_STATICCOLLECTION_H
#define CSM_FOREIGN_STATICCOLLECTION_H

#include "idcollection.hpp"
#include "static.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class WorldCollection;

    class StaticCollection : public IdCollection<Static>
    {
    public:
        StaticCollection ();
        ~StaticCollection ();

        // for populating World name strings (FULL or EDID)
        void updateWorldNames (const WorldCollection& worlds);

    private:
        StaticCollection (const StaticCollection& other);
        StaticCollection& operator= (const StaticCollection& other);
    };
}
#endif // CSM_FOREIGN_STATICCOLLECTION_H
