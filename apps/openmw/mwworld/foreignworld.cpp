#include "foreignworld.hpp"

#include <components/esm/esm4reader.hpp>

#include <extern/esm4/formid.hpp>
#include <extern/esm4/reader.hpp>
#include <extern/esm4/cell.hpp>

#include "foreigncell.hpp"
#include "cellstore.hpp"

unsigned int MWWorld::ForeignWorld::sRecordId = ESM4::REC_WRLD;

MWWorld::ForeignWorld::ForeignWorld() : mDummyCell(0), mVisibleDistCell(nullptr), mVisibleDistCellStore(nullptr)
{
}

MWWorld::ForeignWorld::~ForeignWorld()
{
    if (mVisibleDistCellStore)
        delete mVisibleDistCellStore;

    if (mVisibleDistCell)
        delete mVisibleDistCell;
}

void MWWorld::ForeignWorld::load(ESM::ESMReader& esm, bool isDeleted)
{
    ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

    ESM4::World::load(reader);

    //ESM4::formIdToString(mFormId, mId);
    //ESM4::formIdToString(mParent, mWorldFormId);

    //mName = mFullName;
}

bool MWWorld::ForeignWorld::updateCellGridMap(int x, int y, ESM4::FormId id)
{
    std::pair<std::map<std::pair<int, int>, ESM4::FormId>::iterator, bool> ret
        = mCellGridMap.insert (std::make_pair(std::make_pair(x, y), id));

    return ret.second;
}

const std::map<std::pair<int, int>, ESM4::FormId>& MWWorld::ForeignWorld::getCellGridMap() const
{
    return mCellGridMap;
}

bool MWWorld::ForeignWorld::setDummyCell(ESM4::FormId id)
{
    if (mDummyCell)
        return false;

    mDummyCell = id;
    return true;
}

MWWorld::CellStore *MWWorld::ForeignWorld::getVisibleDistCell()
{
    if (!mVisibleDistCell)
    {
        mVisibleDistCell = new ForeignCell(); // deleted in dtor
        mVisibleDistCell->mCell = new  ESM4::Cell();

        // TODO: populate it with some other dummy entries?
        mVisibleDistCell->mCell->mX = 0;
        mVisibleDistCell->mCell->mY = 0;

        mVisibleDistCellStore = new  CellStore(mVisibleDistCell, true/*isForeignCell*/, false/*isDummy*/);
        mVisibleDistCellStore->setVisibleDistCell();
    }

    return mVisibleDistCellStore;
}

MWWorld::CellStore *MWWorld::ForeignWorld::getVisibleDistCell() const
{
    return mVisibleDistCellStore;
}

void MWWorld::ForeignWorld::blank()
{
    // FIXME: TODO
}
