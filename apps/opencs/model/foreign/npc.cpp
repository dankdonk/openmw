#include "npc.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Npc::sRecordId = ESM4::REC_NPC_;

CSMForeign::Npc::Npc()
{
}

CSMForeign::Npc::~Npc()
{
}

void CSMForeign::Npc::load(ESM4::Reader& reader)
{
    ESM4::Npc::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Npc::blank()
{
    // FIXME: TODO
}
