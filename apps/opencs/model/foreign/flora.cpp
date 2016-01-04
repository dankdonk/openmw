#include "flora.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Flora::sRecordId = ESM4::REC_FLOR;

CSMForeign::Flora::Flora()
{
}

CSMForeign::Flora::~Flora()
{
}

void CSMForeign::Flora::load(ESM4::Reader& reader)
{
    ESM4::Flora::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Flora::blank()
{
    // FIXME: TODO
}
