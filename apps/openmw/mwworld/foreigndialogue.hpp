#ifndef OPENMW_MWWORLD_FOREIGNDIALOGUE_H
#define OPENMW_MWWORLD_FOREIGNDIALOGUE_H

#include <map>

#include <extern/esm4/dial.hpp>

namespace ESM
{
    class ESMWriter;
}

namespace ESM4
{
    class Reader;
    struct DialogInfo;
}

namespace MWWorld
{
    class ForeignDialogue : public ESM4::Dialogue
    {
        std::map<ESM4::FormId, ESM4::DialogInfo*> mInfos;

    public:
        ForeignDialogue();
        ~ForeignDialogue();

        void loadInfo(ESM4::Reader& reader, bool merge);

        const std::map<ESM4::FormId, ESM4::DialogInfo*>& getInfos() const { return mInfos; }

        void load (ESM4::Reader& reader);
        void save (ESM::ESMWriter& esm, bool isDeleted = false) const {} // FIXME: TODO

        void blank();
    };
}

#endif // OPENMW_MWWORLD_FOREIGNDIALOGUE_H
