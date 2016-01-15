#include "hair.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Hair::sRecordId = ESM4::REC_HAIR;

CSMForeign::Hair::Hair()
{
}

CSMForeign::Hair::~Hair()
{
}

void CSMForeign::Hair::load(ESM4::Reader& reader)
{
    ESM4::Hair::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Hair::blank()
{
    // FIXME: TODO
}
