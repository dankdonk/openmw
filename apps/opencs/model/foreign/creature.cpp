#include "creature.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Creature::sRecordId = ESM4::REC_CREA;

CSMForeign::Creature::Creature()
{
}

CSMForeign::Creature::~Creature()
{
}

void CSMForeign::Creature::load(ESM4::Reader& reader)
{
    ESM4::Creature::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Creature::blank()
{
    // FIXME: TODO
}
