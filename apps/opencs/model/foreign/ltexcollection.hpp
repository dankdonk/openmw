#ifndef CSM_FOREIGN_LTEXCOLLECTION_H
#define CSM_FOREIGN_LTEXCOLLECTION_H

#include "../world/collection.hpp"
//#include "../world/record.hpp"

#include "landscapetexture.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    class LTEXCollection : public CSMWorld::Collection<LandscapeTexture, CSMWorld::IdAccessor<LandscapeTexture> >
    {
    public:
        LTEXCollection ();
        ~LTEXCollection ();

        // similar to IdCollection but with ESM4::Reader
        int load(ESM4::Reader& reader, bool base);

        // similar to IdCollection but with ESM4::Reader
        int load (const LandscapeTexture& record, bool base, int index = -2);

        virtual void loadRecord (LandscapeTexture& record, ESM4::Reader& reader);

    private:
        LTEXCollection (const LTEXCollection& other);
        LTEXCollection& operator= (const LTEXCollection& other);
    };
}
#endif // CSM_FOREIGN_LTEXCOLLECTION_H
