#ifndef CSM_FOREIGN_LANDTEXTURECOLLECTION_H
#define CSM_FOREIGN_LANDTEXTURECOLLECTION_H

#include "../world/collection.hpp"

#include "landtexture.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class LandTextureCollection : public CSMWorld::Collection<LandTexture, CSMWorld::IdAccessor<LandTexture> >
    {
    public:
        LandTextureCollection ();
        ~LandTextureCollection ();

        int load(ESM4::Reader& reader, bool base);

        int load (const LandTexture& record, bool base, int index = -2);

        virtual void loadRecord (LandTexture& record, ESM4::Reader& reader);

    private:
        LandTextureCollection (const LandTextureCollection& other);
        LandTextureCollection& operator= (const LandTextureCollection& other);
    };
}
#endif // CSM_FOREIGN_LANDTEXTURECOLLECTION_H
