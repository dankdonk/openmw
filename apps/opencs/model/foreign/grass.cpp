#include "grass.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Grass::sRecordId = ESM4::REC_GRAS;

CSMForeign::Grass::Grass()
{
}

CSMForeign::Grass::~Grass()
{
}

void CSMForeign::Grass::load(ESM4::Reader& reader)
{
    ESM4::Grass::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Grass::blank()
{
    // FIXME: TODO
}
