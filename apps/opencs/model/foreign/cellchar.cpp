#include "cellchar.hpp"

#include <extern/esm4/formid.hpp>
#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::CellChar::sRecordId = ESM4::REC_ACHR;

CSMForeign::CellChar::CellChar()
{
}

CSMForeign::CellChar::~CellChar()
{
}

void CSMForeign::CellChar::load(ESM4::Reader& reader)
{
    ESM4::ActorCharacter::load(reader);

    mId = ESM4::formIdToString(mFormId);
    mRefID = ESM4::formIdToString(mBaseObj);

    mPos.pos[0] = mPlacement.pos.x;
    mPos.pos[1] = mPlacement.pos.y;
    mPos.pos[2] = mPlacement.pos.z;
    mPos.rot[0] = mPlacement.rot.x;
    mPos.rot[1] = mPlacement.rot.y;
    mPos.rot[2] = mPlacement.rot.z;
}

void CSMForeign::CellChar::blank()
{
    // FIXME: TODO
}
