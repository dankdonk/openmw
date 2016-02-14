#include "land.hpp"

#include <extern/esm4/formid.hpp>
#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Land::sRecordId = ESM4::REC_LAND;

CSMForeign::Land::Land()
{
}

CSMForeign::Land::~Land()
{
}

void CSMForeign::Land::load(ESM4::Reader& reader)
{
    ESM4Terrain::Land::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Land::blank()
{
}
