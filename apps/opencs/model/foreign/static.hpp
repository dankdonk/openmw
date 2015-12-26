#ifndef CSM_FOREIGN_STATIC_H
#define CSM_FOREIGN_STATIC_H

#include <string>

#include <extern/esm4/stat.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Static : public ESM4::Static
    {
        static unsigned int sRecordId;

        Static();
        ~Static();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_STATIC_H
