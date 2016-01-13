#include "door.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Door::sRecordId = ESM4::REC_DOOR;

CSMForeign::Door::Door()
{
}

CSMForeign::Door::~Door()
{
}

void CSMForeign::Door::load(ESM4::Reader& reader)
{
    ESM4::Door::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Door::blank()
{
    // FIXME: TODO
}
