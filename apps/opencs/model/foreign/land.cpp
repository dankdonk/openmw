#include "land.hpp"

#include <extern/esm4/reader.hpp>

//#include "cellcollection.hpp"

unsigned int CSMForeign::Land::sRecordId = ESM4::REC_LAND;

CSMForeign::Land::Land()
{
}

CSMForeign::Land::~Land()
{
}

void CSMForeign::Land::load(ESM4::Reader& reader, const CellCollection& cells)
{
    load(reader);
}

void CSMForeign::Land::load(ESM4::Reader& reader)
{
    ESM4::Land::load(reader);
}

void CSMForeign::Land::blank()
{
}
