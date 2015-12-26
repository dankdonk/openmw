#ifndef CSM_FOREIGN_LANDTEXTURE_H
#define CSM_FOREIGN_LANDTEXTURE_H

#include <string>

#include <extern/esm4/ltex.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct LandTexture : public ESM4::LandTexture
    {
        static unsigned int sRecordId;

        LandTexture();
        ~LandTexture();

        std::string mId;
        std::string mName;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_LANDTEXTURE_H
