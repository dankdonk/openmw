#ifndef CSM_FOREIGN_ARMOR_H
#define CSM_FOREIGN_ARMOR_H

#include <string>

#include <extern/esm4/armo.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Armor : public ESM4::Armor
    {
        static unsigned int sRecordId;

        Armor();
        ~Armor();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_ARMOR_H
