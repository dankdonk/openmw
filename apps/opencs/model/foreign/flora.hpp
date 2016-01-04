#ifndef CSM_FOREIGN_FLORA_H
#define CSM_FOREIGN_FLORA_H

#include <string>

#include <extern/esm4/flor.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Flora : public ESM4::Flora
    {
        static unsigned int sRecordId;

        Flora();
        ~Flora();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_FLORA_H
