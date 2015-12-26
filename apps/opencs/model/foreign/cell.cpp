#include "cell.hpp"

#include <sstream>

void CSMForeign::Cell::load (ESM4::Reader& reader)
{
    ESM4::Cell::load (reader);
}

void CSMForeign::Cell::blank()
{
}
