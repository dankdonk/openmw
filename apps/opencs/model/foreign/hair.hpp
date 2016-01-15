#ifndef CSM_FOREIGN_HAIR_H
#define CSM_FOREIGN_HAIR_H

#include <string>

#include <extern/esm4/hair.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Hair : public ESM4::Hair
    {
        static unsigned int sRecordId;

        Hair();
        ~Hair();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_HAIR_H
