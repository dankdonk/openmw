#ifndef CSM_FOREIGN_GRASS_H
#define CSM_FOREIGN_GRASS_H

#include <string>

#include <extern/esm4/gras.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Grass : public ESM4::Grass
    {
        static unsigned int sRecordId;

        Grass();
        ~Grass();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_GRASS_H
