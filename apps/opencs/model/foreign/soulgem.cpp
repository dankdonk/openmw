#include "soulgem.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::SoulGem::sRecordId = ESM4::REC_SLGM;

CSMForeign::SoulGem::SoulGem()
{
}

CSMForeign::SoulGem::~SoulGem()
{
}

void CSMForeign::SoulGem::load(ESM4::Reader& reader)
{
    ESM4::SoulGem::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::SoulGem::blank()
{
    // FIXME: TODO
}
