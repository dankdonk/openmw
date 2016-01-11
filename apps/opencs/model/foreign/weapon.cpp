#include "weapon.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Weapon::sRecordId = ESM4::REC_WEAP;

CSMForeign::Weapon::Weapon()
{
}

CSMForeign::Weapon::~Weapon()
{
}

void CSMForeign::Weapon::load(ESM4::Reader& reader)
{
    ESM4::Weapon::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Weapon::blank()
{
    // FIXME: TODO
}
