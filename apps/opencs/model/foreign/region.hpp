#ifndef CSM_FOREIGN_REGION_H
#define CSM_FOREIGN_REGION_H

#include <string>

#include <extern/esm4/regn.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Region : public ESM4::Region
    {
        static unsigned int sRecordId;

        Region();
        ~Region();

        std::string mId;
        std::string mWorld; // converted from mWorldId (formId)
        std::uint32_t mMapColor;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_REGION_H
