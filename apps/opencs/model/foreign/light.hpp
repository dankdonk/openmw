#ifndef CSM_FOREIGN_LIGHT_H
#define CSM_FOREIGN_LIGHT_H

#include <string>

#include <extern/esm4/ligh.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Light : public ESM4::Light
    {
        static unsigned int sRecordId;

        Light();
        ~Light();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_LIGHT_H
