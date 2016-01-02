#ifndef CSM_FOREIGN_ANIMOBJECT_H
#define CSM_FOREIGN_ANIMOBJECT_H

#include <string>

#include <extern/esm4/anio.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct AnimObject : public ESM4::AnimObject
    {
        static unsigned int sRecordId;

        AnimObject();
        ~AnimObject();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_ANIMOBJECT_H
