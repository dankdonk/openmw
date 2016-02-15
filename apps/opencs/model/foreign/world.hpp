#ifndef CSM_FOREIGN_WORLD_H
#define CSM_FOREIGN_WORLD_H

#include <string>

#include <extern/esm4/wrld.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct World : public ESM4::World
    {
        static unsigned int sRecordId;

        World();
        ~World();

        std::string mId;
        std::string mName;

        std::string mWorldFormId; // keeping a string allows columnimp.hpp free of formId conversions

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_WORLD_H
