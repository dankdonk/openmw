#ifndef CSM_FOREIGN_CONTAINER_H
#define CSM_FOREIGN_CONTAINER_H

#include <string>

#include <extern/esm4/cont.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Container : public ESM4::Container
    {
        static unsigned int sRecordId;

        Container();
        ~Container();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_CONTAINER_H
