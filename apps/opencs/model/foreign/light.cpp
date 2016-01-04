#include "light.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Light::sRecordId = ESM4::REC_LIGH;

CSMForeign::Light::Light()
{
}

CSMForeign::Light::~Light()
{
}

void CSMForeign::Light::load(ESM4::Reader& reader)
{
    ESM4::Light::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Light::blank()
{
    // FIXME: TODO
}
