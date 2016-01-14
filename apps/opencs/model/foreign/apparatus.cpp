#include "apparatus.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Apparatus::sRecordId = ESM4::REC_APPA;

CSMForeign::Apparatus::Apparatus()
{
}

CSMForeign::Apparatus::~Apparatus()
{
}

void CSMForeign::Apparatus::load(ESM4::Reader& reader)
{
    ESM4::Apparatus::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Apparatus::blank()
{
    // FIXME: TODO
}
