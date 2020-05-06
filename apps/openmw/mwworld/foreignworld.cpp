#include "foreignworld.hpp"

#include <components/esm/esm4reader.hpp>

#include <extern/esm4/reader.hpp>
#include <extern/esm4/cell.hpp>

#include "foreigncell.hpp"
#include "cellstore.hpp"

MWWorld::ForeignWorld::ForeignWorld() : /*mDummyCell(0), */mVisibleDistCell(nullptr), mVisibleDistCellStore(nullptr)
                                      , mDummyCell(nullptr), mDummyCellStore(nullptr)
{
}

MWWorld::ForeignWorld::~ForeignWorld()
{
    if (mDummyCellStore)
        delete mDummyCellStore;

    if (mDummyCell)
        delete mDummyCell;

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

#if 0
bool MWWorld::ForeignWorld::setDummyCell(ESM4::FormId id)
{
    if (mDummyCell)
        return false;

    mDummyCell = id;
    return true;
}
#endif

MWWorld::CellStore *MWWorld::ForeignWorld::getVisibleDistCell()
{
    if (!mVisibleDistCell)
    {
        // FIXME: if we add a new ctor to CellStore we don't need a ForeignCell?
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

MWWorld::CellStore *MWWorld::ForeignWorld::getDummyCell()
{
    if (!mDummyCell)
    {
        // FIXME: if we add a new ctor to CellStore we don't need a ForeignCell?
        mDummyCell = new ForeignCell(); // deleted in dtor
        mDummyCell->mCell = new  ESM4::Cell();

        // TODO: populate it with some other dummy entries?
        mDummyCell->mCell->mX = 0;
        mDummyCell->mCell->mY = 0;

        mDummyCellStore = new  CellStore(mDummyCell, true/*isForeignCell*/, true/*isDummy*/);
        //mDummyCellStore->setVisibleDistCell();
    }

    return mDummyCellStore;
}

MWWorld::CellStore *MWWorld::ForeignWorld::getDummyCell() const
{
    return mDummyCellStore;
}

void MWWorld::ForeignWorld::blank()
{
    // FIXME: TODO
}
