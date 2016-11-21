#include "foreignworld.hpp"

#include <components/esm/esm4reader.hpp>

#include <extern/esm4/formid.hpp>
#include <extern/esm4/reader.hpp>

unsigned int MWWorld::ForeignWorld::sRecordId = ESM4::REC_WRLD;

MWWorld::ForeignWorld::ForeignWorld() : mDummyCell(0)
{
}

MWWorld::ForeignWorld::~ForeignWorld()
{
}

void MWWorld::ForeignWorld::load(ESM::ESMReader& esm, bool isDeleted)
{
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

    ESM4::World::load(reader);

    //ESM4::formIdToString(mFormId, mId);
    //ESM4::formIdToString(mParent, mWorldFormId);

    //mName = mFullName;
}

bool MWWorld::ForeignWorld::insertCellGridMap(int x, int y, ESM4::FormId id)
{
    std::pair<std::map<std::pair<int, int>, ESM4::FormId>::iterator, bool> ret
        = mCells.insert (std::make_pair(std::make_pair(x, y), id));

    return ret.second;
}

bool MWWorld::ForeignWorld::insertDummyCell(ESM4::FormId id)
{
    if (mDummyCell)
        return false;

    mDummyCell = id;
    return true;
}

void MWWorld::ForeignWorld::blank()
{
    // FIXME: TODO
}
