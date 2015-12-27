#include "cellref.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::CellRef::sRecordId = ESM4::REC_REFR;

CSMForeign::CellRef::CellRef()
{
}

CSMForeign::CellRef::~CellRef()
{
}

void CSMForeign::CellRef::load(ESM4::Reader& reader)
{
    ESM4::Reference::load(reader);

    mPos.pos[0] = mPosition.pos.x;
    mPos.pos[1] = mPosition.pos.y;
    mPos.pos[2] = mPosition.pos.z;
    mPos.rot[0] = mPosition.rot.x;
    mPos.rot[1] = mPosition.rot.y;
    mPos.rot[2] = mPosition.rot.z;
}

void CSMForeign::CellRef::blank()
{
    // FIXME: TODO
}
