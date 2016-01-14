#ifndef CSM_FOREIGN_APPARATUS_H
#define CSM_FOREIGN_APPARATUS_H

#include <string>

#include <extern/esm4/appa.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Apparatus : public ESM4::Apparatus
    {
        static unsigned int sRecordId;

        Apparatus();
        ~Apparatus();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_APPARATUS_H
