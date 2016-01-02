#include "container.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Container::sRecordId = ESM4::REC_CONT;

CSMForeign::Container::Container()
{
}

CSMForeign::Container::~Container()
{
}

void CSMForeign::Container::load(ESM4::Reader& reader)
{
    ESM4::Container::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Container::blank()
{
    // FIXME: TODO
}
