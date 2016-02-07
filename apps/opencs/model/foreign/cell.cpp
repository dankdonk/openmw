#include "cell.hpp"

#include <extern/esm4/reader.hpp>

void CSMForeign::Cell::load (ESM4::Reader& reader)
{
    ESM4::Cell::load (reader);

    ESM4::formIdToString(mFormId, mId);
}

void CSMForeign::Cell::blank()
{
}
