#include "static.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Static::sRecordId = ESM4::REC_STAT;

CSMForeign::Static::Static()
{
}

CSMForeign::Static::~Static()
{
}

void CSMForeign::Static::load(ESM4::Reader& reader)
{
    ESM4::Static::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Static::blank()
{
    // FIXME: TODO
}
