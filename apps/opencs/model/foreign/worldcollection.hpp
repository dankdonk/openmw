#ifndef CSM_FOREIGN_WORLDCOLLECTION_H
#define CSM_FOREIGN_WORLDCOLLECTION_H

#include "../world/collection.hpp"
//#include "../world/record.hpp"

#include "world.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class WorldCollection : public CSMWorld::Collection<World, CSMWorld::IdAccessor<World> >
    {
    public:
        WorldCollection ();
        ~WorldCollection ();

        // similar to IdCollection but with ESM4::Reader
        int load(ESM4::Reader& reader, bool base);

        // similar to IdCollection but with ESM4::Reader
        int load (const World& record, bool base, int index = -2);

        virtual void loadRecord (World& record, ESM4::Reader& reader);

        std::string getIdString(std::uint32_t formId) const;

        World *WorldCollection::getWorld(ESM4::FormId formId); // for updating world children

    private:
        WorldCollection (const WorldCollection& other);
        WorldCollection& operator= (const WorldCollection& other);
    };
}
#endif // CSM_FOREIGN_WORLDCOLLECTION_H
