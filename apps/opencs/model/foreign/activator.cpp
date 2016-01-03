#include "activator.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Activator::sRecordId = ESM4::REC_ACTI;

CSMForeign::Activator::Activator()
{
}

CSMForeign::Activator::~Activator()
{
}

void CSMForeign::Activator::load(ESM4::Reader& reader)
{
    ESM4::Activator::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Activator::blank()
{
    // FIXME: TODO
}
