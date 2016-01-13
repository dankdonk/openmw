#ifndef CSM_FOREIGN_AMMO_H
#define CSM_FOREIGN_AMMO_H

#include <string>

#include <extern/esm4/ammo.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Ammo : public ESM4::Ammo
    {
        static unsigned int sRecordId;

        Ammo();
        ~Ammo();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_AMMO_H
