#include "landcollection.hpp"

#include <iostream> // FIXME
#ifdef NDEBUG // FIXME for debugging only
#undef NDEBUG
#endif

#include <libs/platform/strings.h>

#include <extern/esm4/reader.hpp>

#include "../world/record.hpp"

#include "cellcollection.hpp"

CSMForeign::LandCollection::LandCollection (CellCollection& cells)
  : mCells (cells)
{
}

CSMForeign::LandCollection::~LandCollection ()
{
}

// Can't reliably use cell grid as the id for LAND, since some cells can be "empty" or do not
// have an XCLC sub-record. e.g. OblivionMQKvatchBridge, TheFrostFireGlade and CheydinhalOblivion
int CSMForeign::LandCollection::load (ESM4::Reader& reader, bool base)
{
    CSMForeign::Land record;

    std::string id;
    ESM4::FormId formId = reader.hdr().record.id;
    ESM4::formIdToString(formId, id);

    // cache the ref's formId to its parent cell
    Cell& cell = mCells.getCell(reader.currCell()); // FIXME: const issue with Collection

    assert(reader.grp().type == ESM4::Grp_CellTemporaryChild && "Unexpected Group type for LAND");
    assert(cell.mLandTemporary == 0 && "CELL already has a LAND child");
    cell.mLandTemporary = formId;

    // check if parent cell left some bread crumbs
    if (reader.hasCellGrid())
    {
        // LAND records should only occur in a exterior cell.
        assert(reader.grp(3).type == ESM4::Grp_ExteriorCell &&
               reader.grp(2).type == ESM4::Grp_ExteriorSubCell &&
               reader.grp(1).type == ESM4::Grp_CellChild &&
               reader.grp(0).type == ESM4::Grp_CellTemporaryChild &&
               "LAND record found in an unexpected group heirarchy");

        ESM4::FormId worldId = reader.currWorld();
        std::map<ESM4::FormId, CoordinateIndex>::iterator lb = mPositionIndex.lower_bound(worldId);

        if (lb != mPositionIndex.end() && !(mPositionIndex.key_comp()(worldId, lb->first)))
        {
            std::pair<CoordinateIndex::iterator, bool> res = lb->second.insert(
                { std::pair<int, int>(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y), id });

            assert(res.second && "existing LAND record found for the given coordinates");
        }
        else
            mPositionIndex.insert(lb, std::map<ESM4::FormId, CoordinateIndex>::value_type(worldId,
                { {std::pair<int, int>(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y), id } }));

        ESM4::gridToString(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y, record.mCellId);
    }
    else
        record.mCellId = id; // use formId string instead

    // FIXME; should be using the formId as the lookup key rather than its string form
    int index = CSMWorld::Collection<Land, CSMWorld::IdAccessor<Land> >::searchId(id);

    if (index == -1)
        record.mId = id; // new record
    else
        record = this->getRecord(index).get();

    loadRecord(record, reader);

    return load(record, base, index);
}

void CSMForeign::LandCollection::loadRecord (Land& record, ESM4::Reader& reader)
{
    record.load(reader, mCells);
}

int CSMForeign::LandCollection::load (const Land& record, bool base, int index)
{
    if (index == -2)
        index = CSMWorld::Collection<Land, CSMWorld::IdAccessor<Land> >::searchId(
            CSMWorld::IdAccessor<Land>().getId(record));

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<Land> > record2(new CSMWorld::Record<Land>);

        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<Land> > record2(new CSMWorld::Record<Land>(
                CSMWorld::Collection<Land, CSMWorld::IdAccessor<Land> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

int CSMForeign::LandCollection::searchId(ESM4::FormId formId) const
{
    return CSMWorld::Collection<Land, CSMWorld::IdAccessor<Land> >::searchId(ESM4::formIdToString(formId));
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

    return CSMWorld::Collection<Land, CSMWorld::IdAccessor<Land> >::searchId(it->second);
}
