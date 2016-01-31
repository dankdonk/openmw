#ifndef CSM_FOREIGN_EYES_H
#define CSM_FOREIGN_EYES_H

#include <string>

#include <extern/esm4/eyes.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Eyes : public ESM4::Eyes
    {
        static unsigned int sRecordId;

        Eyes();
        ~Eyes();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_EYES_H