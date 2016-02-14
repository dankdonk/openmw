#include "world.hpp"

#include <extern/esm4/formid.hpp>
#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::World::sRecordId = ESM4::REC_WRLD;

CSMForeign::World::World()
{
}

CSMForeign::World::~World()
{
}

void CSMForeign::World::load(ESM4::Reader& reader)
{
    ESM4::World::load(reader);

    ESM4::formIdToString(mFormId, mId);

    mName = mFullName;
}

void CSMForeign::World::blank()
{
    // FIXME: TODO
}
