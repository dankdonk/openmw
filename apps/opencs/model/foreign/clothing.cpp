#include "clothing.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Clothing::sRecordId = ESM4::REC_CLOT;

CSMForeign::Clothing::Clothing()
{
}

CSMForeign::Clothing::~Clothing()
{
}

void CSMForeign::Clothing::load(ESM4::Reader& reader)
{
    ESM4::Clothing::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Clothing::blank()
{
    // FIXME: TODO
}
