#ifndef CSM_FOREIGN_KEY_H
#define CSM_FOREIGN_KEY_H

#include <string>

#include <extern/esm4/keym.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Key : public ESM4::Key
    {
        static unsigned int sRecordId;

        Key();
        ~Key();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_KEY_H
