#include "furniture.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Furniture::sRecordId = ESM4::REC_FURN;

CSMForeign::Furniture::Furniture()
{
}

CSMForeign::Furniture::~Furniture()
{
}

void CSMForeign::Furniture::load(ESM4::Reader& reader)
{
    ESM4::Furniture::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Furniture::blank()
{
    // FIXME: TODO
}
