#include "key.hpp"

#include <extern/esm4/reader.hpp>

unsigned int CSMForeign::Key::sRecordId = ESM4::REC_KEYM;

CSMForeign::Key::Key()
{
}

CSMForeign::Key::~Key()
{
}

void CSMForeign::Key::load(ESM4::Reader& reader)
{
    ESM4::Key::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Key::blank()
{
    // FIXME: TODO
}
