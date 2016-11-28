#include "foreignland.hpp"

#include <components/esm/esm4reader.hpp>

#include <extern/esm4/formid.hpp>
#include <extern/esm4/reader.hpp>

unsigned int MWWorld::ForeignLand::sRecordId = MKTAG('D','L','A','N');

MWWorld::ForeignLand::ForeignLand()
{
}

MWWorld::ForeignLand::~ForeignLand()
{
}

void MWWorld::ForeignLand::load(ESM::ESMReader& esm, bool isDeleted)
{
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

    ESM4Terrain::Land::load(reader);

    ESM4::formIdToString(mFormId, mId);
}

void MWWorld::ForeignLand::blank()
{
    // FIXME: TODO
}
