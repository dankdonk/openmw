#include "foreignland.hpp"

#include <extern/esm4/formid.hpp>
#include <extern/esm4/reader.hpp>

MWWorld::ForeignLand::ForeignLand()
{
}

MWWorld::ForeignLand::~ForeignLand()
{
}

void MWWorld::ForeignLand::load(ESM4::Reader& reader, bool isDeleted)
{
    ESM4Terrain::Land::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void MWWorld::ForeignLand::blank()
{
    // FIXME: TODO
}
