#include "sigilstone.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::SigilStone::sRecordId = ESM4::REC_SGST;

CSMForeign::SigilStone::SigilStone()
{
}

CSMForeign::SigilStone::~SigilStone()
{
}

void CSMForeign::SigilStone::load(ESM4::Reader& reader)
{
    ESM4::SigilStone::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::SigilStone::blank()
{
    // FIXME: TODO
}
