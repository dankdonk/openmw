#ifndef CSM_FOREIGN_FURNITURE_H
#define CSM_FOREIGN_FURNITURE_H

#include <string>

#include <extern/esm4/furn.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Furniture : public ESM4::Furniture
    {
        static unsigned int sRecordId;

        Furniture();
        ~Furniture();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_FURNITURE_H
