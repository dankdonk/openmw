#ifndef CSM_FOREIGN_SOUN_H
#define CSM_FOREIGN_SOUN_H

#include <string>

#include <extern/esm4/soun.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Sound : public ESM4::Sound
    {
        static unsigned int sRecordId;

        Sound();
        ~Sound();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_SOUN_H
