#include "potion.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Potion::sRecordId = ESM4::REC_ALCH;

CSMForeign::Potion::Potion()
{
}

CSMForeign::Potion::~Potion()
{
}

void CSMForeign::Potion::load(ESM4::Reader& reader)
{
    ESM4::Potion::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Potion::blank()
{
    // FIXME: TODO
}
