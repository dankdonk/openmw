#include "landcollection.hpp"

#include <iostream> // FIXME
#ifdef NDEBUG // FIXME for debugging only
#undef NDEBUG
#endif

#include <extern/esm4/reader.hpp>

#include "cellgroupcollection.hpp"

CSMForeign::LandCollection::LandCollection (CellGroupCollection& cellGroups)
  : mCellGroups (cellGroups)
{
}

CSMForeign::LandCollection::~LandCollection ()
{
}

// Can't reliably use cell grid as the id for LAND, since some cells can be "empty" or do not
// have an XCLC sub-record. e.g. OblivionMQKvatchBridge, TheFrostFireGlade and CheydinhalOblivion
//
// NOTE: the lack of XCLC for some cells was worked-around, but should't the id for land be the
// cell formId, anyway?
int CSMForeign::LandCollection::load (ESM4::Reader& reader, bool base)
{
    assert(reader.grp().type == ESM4::Grp_CellTemporaryChild && "Unexpected Group type for LAND");

    // load the record
    Land record;
    IdCollection<Land>::loadRecord(record, reader);

    // cache the ref's formId to its parent cell
    int cellIndex = mCellGroups.searchFormId(reader.currCell());
    if (cellIndex == -1)
        throw std::runtime_error("no CELL for LAND");

    std::unique_ptr<CSMWorld::Record<CellGroup> > record2(
            new CSMWorld::Record<CellGroup>(mCellGroups.getRecord(cellIndex)));
    record2->mState = CSMWorld::RecordBase::State_BaseOnly; // FIXME: set Modified for new modindex?
    CellGroup &cellGroup = record2->get();
    //assert(cellGroup.mLandTemporary == 0 && "CELL already has a LAND child");
    cellGroup.mLand = record.mFormId;
    mCellGroups.setRecord(cellIndex, std::move(record2));

    // check if parent cell left some bread crumbs
    if (reader.hasCellGrid())
    {
//#if 0
        // FIXME DLCFrostcrag.esp triggers this
        // LAND records should only occur in a exterior cell.
        assert(reader.grp(3).type == ESM4::Grp_ExteriorCell &&
               reader.grp(2).type == ESM4::Grp_ExteriorSubCell &&
               reader.grp(1).type == ESM4::Grp_CellChild &&
               reader.grp(0).type == ESM4::Grp_CellTemporaryChild &&
               "LAND record found in an unexpected group hierarchy");
//#endif
        ESM4::FormId worldId = reader.currWorld();
        std::map<ESM4::FormId, CoordinateIndex>::iterator lb = mPositionIndex.lower_bound(worldId);

        if (lb != mPositionIndex.end() && !(mPositionIndex.key_comp()(worldId, lb->first)))
        {
            std::pair<CoordinateIndex::iterator, bool> res = lb->second.insert(
                { std::pair<int, int>(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y), record.mFormId });

            // this can happen if a mod updates the record, just overwrite it for now // FIXME
            //assert(res.second && "existing LAND record found for the given coordinates");
            if (!res.second)
                lb->second[std::pair<int, int>(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y)] = record.mFormId;
        }
        else
            mPositionIndex.insert(lb, std::map<ESM4::FormId, CoordinateIndex>::value_type(worldId,
                { {std::pair<int, int>(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y), record.mFormId } }));

        ESM4::gridToString(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y, record.mCellId);
    }
    else
        record.mCellId = ESM4::formIdToString(record.mFormId);

    // continue with the rest of the loading
    int index = this->searchFormId(record.mFormId);

    // FIXME: how to deal with deleted records?
    //if ((record.mFlags & ESM4::Rec_Deleted) != 0)

    return IdCollection<Land>::load(record, base, index);
}

// returns record index
int CSMForeign::LandCollection::searchId(std::int16_t x, std::int16_t y, ESM4::FormId world) const
{
    std::map<ESM4::FormId, CoordinateIndex>::const_iterator iter = mPositionIndex.find(world);
    if (iter == mPositionIndex.end())
        return -1; // can't find world

    CoordinateIndex::const_iterator it = iter->second.find(std::make_pair(x, y));
    if (it == iter->second.end())
        return -1; // cann't find coordinate

    return this->searchFormId(it->second);
}
