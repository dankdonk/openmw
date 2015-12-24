#ifndef CSM_FOREIGN_LANDSCAPETEXTURE_H
#define CSM_FOREIGN_LANDSCAPETEXTURE_H

#include <string>

#include <extern/esm4/ltex.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct LandscapeTexture : public ESM4::LandTexture
    {
        static unsigned int sRecordId;

        LandscapeTexture();
        ~LandscapeTexture();

        std::string mId;
        std::string mName;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_LANDSCAPETEXTURE_H
