#ifndef CSM_FOREIGN_MISCITEM_H
#define CSM_FOREIGN_MISCITEM_H

#include <string>

#include <extern/esm4/misc.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct MiscItem : public ESM4::MiscItem
    {
        static unsigned int sRecordId;

        MiscItem();
        ~MiscItem();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_MISCITEM_H
