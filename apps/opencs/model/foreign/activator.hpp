#ifndef CSM_FOREIGN_ACTIVATOR_H
#define CSM_FOREIGN_ACTIVATOR_H

#include <string>

#include <extern/esm4/acti.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Activator : public ESM4::Activator
    {
        static unsigned int sRecordId;

        Activator();
        ~Activator();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_ACTIVATOR_H
