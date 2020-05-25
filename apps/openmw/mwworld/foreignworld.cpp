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
    //if (mDummyCellStore)
        //delete mDummyCellStore;

    //if (mDummyCell)
        //delete mDummyCell;

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

bool MWWorld::ForeignWorld::updateCellGridMap(std::int16_t x, std::int16_t y, ESM4::FormId formId)
//bool MWWorld::ForeignWorld::updateCellGridMap(ForeignCell *cell)
{
    std::pair<std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId>::iterator, bool> ret
        = mCellGridMap.insert(std::make_pair(std::make_pair(x, y), formId));

    // should be the same formid, check just in case
    if (!ret.second)
    {
        ESM4::FormId oldId = ret.first->second;

        if (oldId != formId)
            throw std::runtime_error("Cell GridMap different formid found");
    }

    return ret.second;
}

const std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId>& MWWorld::ForeignWorld::getCellGridMap() const
{
    return mCellGridMap;
}

ESM4::FormId MWWorld::ForeignWorld::getCellId(std::int16_t x, std::int16_t y) const
{
    std::map<std::pair<std::int16_t, std::int16_t>, ESM4::FormId>::const_iterator it
        = mCellGridMap.find(std::make_pair(x, y));

    if (it != mCellGridMap.end())
        return it->second;

    return 0;
}

bool MWWorld::ForeignWorld::setDummyCell(ForeignCell *cell)
{
    if (mDummyCell)
    {
        if (cell->mCell->mFormId != mDummyCell->mCell->mFormId)
            throw std::runtime_error("Dummy cell different formid found");

        return false;
    }

    mDummyCell = cell;

    return true;
}

// FIXME: this is broken, since there can be one for each exterior cell
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

MWWorld::ForeignCell *MWWorld::ForeignWorld::getDummyCell()
{
    return mDummyCell;
}

MWWorld::ForeignCell *MWWorld::ForeignWorld::getDummyCell() const
{
    return mDummyCell;
}

void MWWorld::ForeignWorld::blank()
{
    // FIXME: TODO
}
