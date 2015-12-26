#include "landscape.hpp"

#include <extern/esm4/reader.hpp>

//#include "cellcollection.hpp"

unsigned int CSMForeign::Landscape::sRecordId = ESM4::REC_LAND;

CSMForeign::Landscape::Landscape()
{
}

CSMForeign::Landscape::~Landscape()
{
}

void CSMForeign::Landscape::load(ESM4::Reader& reader, const CellCollection& cells)
{
    load(reader);
}

void CSMForeign::Landscape::load(ESM4::Reader& reader)
{
    ESM4::Land::load(reader);
}

void CSMForeign::Landscape::blank()
{
}
