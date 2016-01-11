#ifndef CSM_FOREIGN_WEAP_H
#define CSM_FOREIGN_WEAP_H

#include <string>

#include <extern/esm4/weap.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Weapon : public ESM4::Weapon
    {
        static unsigned int sRecordId;

        Weapon();
        ~Weapon();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_WEAP_H
