#ifndef CSM_FOREIGN_DOOR_H
#define CSM_FOREIGN_DOOR_H

#include <string>

#include <extern/esm4/door.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Door : public ESM4::Door
    {
        static unsigned int sRecordId;

        Door();
        ~Door();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_DOOR_H
