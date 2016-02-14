#include "region.hpp"

#include <extern/esm4/formid.hpp>
#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Region::sRecordId = ESM4::REC_REGN;

CSMForeign::Region::Region()
{
    mWorld.clear();
}

CSMForeign::Region::~Region()
{
}

void CSMForeign::Region::load(ESM4::Reader& reader)
{
    ESM4::Region::load(reader);

    ESM4::formIdToString(mFormId, mId);

    mMapColor = mColour;
}

void CSMForeign::Region::blank()
{
    // FIXME: TODO
}
