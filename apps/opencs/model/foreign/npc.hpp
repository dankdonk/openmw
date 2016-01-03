#ifndef CSM_FOREIGN_NPC_H
#define CSM_FOREIGN_NPC_H

#include <string>

#include <extern/esm4/npc_.hpp>

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    struct Npc : public ESM4::Npc
    {
        static unsigned int sRecordId;

        Npc();
        ~Npc();

        std::string mId;

        void load(ESM4::Reader& esm);

        void blank();
    };
}

#endif // CSM_FOREIGN_NPC_H
