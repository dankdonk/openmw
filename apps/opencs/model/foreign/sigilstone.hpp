#ifndef CSM_FOREIGN_SIGILSTONE_H
#define CSM_FOREIGN_SIGILSTONE_H

#include <string>

#include <extern/esm4/sgst.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct SigilStone : public ESM4::SigilStone
    {
        static unsigned int sRecordId;

        SigilStone();
        ~SigilStone();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_SIGILSTONE_H
