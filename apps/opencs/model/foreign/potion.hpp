#ifndef CSM_FOREIGN_POTION_H
#define CSM_FOREIGN_POTION_H

#include <string>

#include <extern/esm4/alch.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Potion : public ESM4::Potion
    {
        static unsigned int sRecordId;

        Potion();
        ~Potion();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_POTION_H
