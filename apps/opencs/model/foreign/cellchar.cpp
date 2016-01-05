#include "cellchar.hpp"

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
    ESM4::Character::load(reader);

    mRefID = ESM4::formIdToString(mBaseObj);

    mPos.pos[0] = mPosition.pos.x;
    mPos.pos[1] = mPosition.pos.y;
    mPos.pos[2] = mPosition.pos.z;
    mPos.rot[0] = mPosition.rot.x;
    mPos.rot[1] = mPosition.rot.y;
    mPos.rot[2] = mPosition.rot.z;
}

void CSMForeign::CellChar::blank()
{
    // FIXME: TODO
}
