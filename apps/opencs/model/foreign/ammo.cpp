#include "ammo.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Ammo::sRecordId = ESM4::REC_AMMO;

CSMForeign::Ammo::Ammo()
{
}

CSMForeign::Ammo::~Ammo()
{
}

void CSMForeign::Ammo::load(ESM4::Reader& reader)
{
    ESM4::Ammo::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Ammo::blank()
{
    // FIXME: TODO
}
