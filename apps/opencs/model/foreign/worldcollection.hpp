#ifndef CSM_FOREIGN_WORLDCOLLECTION_H
#define CSM_FOREIGN_WORLDCOLLECTION_H

#include "idcollection.hpp"
#include "world.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class WorldCollection : public IdCollection<World>
    {
    public:
        WorldCollection ();
        ~WorldCollection ();

        std::string getIdString(std::uint32_t formId) const;

        World *getWorld(ESM4::FormId formId); // for updating world children

    private:
        WorldCollection (const WorldCollection& other);
        WorldCollection& operator= (const WorldCollection& other);
    };
}
#endif // CSM_FOREIGN_WORLDCOLLECTION_H
