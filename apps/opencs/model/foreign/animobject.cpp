#include "animobject.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::AnimObject::sRecordId = ESM4::REC_ANIO;

CSMForeign::AnimObject::AnimObject()
{
}

CSMForeign::AnimObject::~AnimObject()
{
}

void CSMForeign::AnimObject::load(ESM4::Reader& reader)
{
    ESM4::AnimObject::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::AnimObject::blank()
{
    // FIXME: TODO
}
