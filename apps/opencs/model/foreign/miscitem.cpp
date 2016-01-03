#include "miscitem.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::MiscItem::sRecordId = ESM4::REC_MISC;

CSMForeign::MiscItem::MiscItem()
{
}

CSMForeign::MiscItem::~MiscItem()
{
}

void CSMForeign::MiscItem::load(ESM4::Reader& reader)
{
    ESM4::MiscItem::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::MiscItem::blank()
{
    // FIXME: TODO
}
