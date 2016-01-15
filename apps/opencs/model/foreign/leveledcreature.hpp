#ifndef CSM_FOREIGN_LEVELEDCREATURE_H
#define CSM_FOREIGN_LEVELEDCREATURE_H

#include <string>

#include <extern/esm4/lvlc.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct LeveledCreature : public ESM4::LeveledCreature
    {
        static unsigned int sRecordId;

        LeveledCreature();
        ~LeveledCreature();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_LEVELEDCREATURE_H
