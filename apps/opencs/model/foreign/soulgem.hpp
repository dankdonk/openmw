#ifndef CSM_FOREIGN_SOULGEM_H
#define CSM_FOREIGN_SOULGEM_H

#include <string>

#include <extern/esm4/slgm.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct SoulGem : public ESM4::SoulGem
    {
        static unsigned int sRecordId;

        SoulGem();
        ~SoulGem();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_SOULGEM_H
