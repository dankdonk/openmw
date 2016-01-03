#include "armor.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Armor::sRecordId = ESM4::REC_ARMO;

CSMForeign::Armor::Armor()
{
}

CSMForeign::Armor::~Armor()
{
}

void CSMForeign::Armor::load(ESM4::Reader& reader)
{
    ESM4::Armor::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Armor::blank()
{
    // FIXME: TODO
}
