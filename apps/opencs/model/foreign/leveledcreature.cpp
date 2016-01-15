#include "leveledcreature.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::LeveledCreature::sRecordId = ESM4::REC_LVLC;

CSMForeign::LeveledCreature::LeveledCreature()
{
}

CSMForeign::LeveledCreature::~LeveledCreature()
{
}

void CSMForeign::LeveledCreature::load(ESM4::Reader& reader)
{
    ESM4::LeveledCreature::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::LeveledCreature::blank()
{
    // FIXME: TODO
}
