#ifndef CSM_FOREIGN_CREATURE_H
#define CSM_FOREIGN_CREATURE_H

#include <string>

#include <extern/esm4/crea.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Creature : public ESM4::Creature
    {
        static unsigned int sRecordId;

        Creature();
        ~Creature();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_CREATURE_H
