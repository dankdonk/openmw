#include "eyes.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Eyes::sRecordId = ESM4::REC_EYES;

CSMForeign::Eyes::Eyes()
{
}

CSMForeign::Eyes::~Eyes()
{
}

void CSMForeign::Eyes::load(ESM4::Reader& reader)
{
    ESM4::Eyes::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Eyes::blank()
{
    // FIXME: TODO
}
