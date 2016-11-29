#include "cells.hpp"

#include <extern/esm4/common.hpp>
#include <extern/esm4/formid.hpp>

#include <components/esm/esmreader.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/defs.hpp>
#include <components/esm/cellstate.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "class.hpp"
#include "esmstore.hpp"
#include "containerstore.hpp"
#include "cellstore.hpp"

MWWorld::CellStore *MWWorld::Cells::getCellStore (const ESM::Cell *cell)
{
    if (cell->mData.mFlags & ESM::Cell::Interior)
    {
        std::string lowerName(Misc::StringUtils::lowerCase(cell->mName));
        std::map<std::string, CellStore>::iterator result = mInteriors.find (lowerName);

        if (result==mInteriors.end())
        {
            result = mInteriors.insert (std::make_pair (lowerName, CellStore (cell))).first;
        }

        return &result->second;
    }
    else
    {
        std::map<std::pair<int, int>, CellStore>::iterator result =
            mExteriors.find (std::make_pair (cell->getGridX(), cell->getGridY()));

        if (result==mExteriors.end())
        {
            result = mExteriors.insert (std::make_pair (
                std::make_pair (cell->getGridX(), cell->getGridY()), CellStore (cell))).first;

        }

        return &result->second;
    }
}

void MWWorld::Cells::clear()
{
    mInteriors.clear();
    mExteriors.clear();
    std::fill(mIdCache.begin(), mIdCache.end(), std::make_pair("", (MWWorld::CellStore*)0));
    mIdCacheIndex = 0;
}

MWWorld::Ptr MWWorld::Cells::getPtrAndCache (const std::string& name, CellStore& cellStore)
{
    Ptr ptr = getPtr (name, cellStore);

    if (!ptr.isEmpty() && ptr.isInCell())
    {
        mIdCache[mIdCacheIndex].first = name;
        mIdCache[mIdCacheIndex].second = &cellStore;
        if (++mIdCacheIndex>=mIdCache.size())
            mIdCacheIndex = 0;
    }

    return ptr;
}

void MWWorld::Cells::writeCell (ESM::ESMWriter& writer, CellStore& cell) const
{
    if (cell.getState()!=CellStore::State_Loaded)
        cell.load (mStore, mReader);

    ESM::CellState cellState;

    cell.saveState (cellState);

    writer.startRecord (ESM::REC_CSTA);
    cellState.mId.save (writer);
    cellState.save (writer);
    cell.writeFog(writer);
    cell.writeReferences (writer);
    writer.endRecord (ESM::REC_CSTA);
}

MWWorld::Cells::Cells (const MWWorld::ESMStore& store, std::vector<std::vector<ESM::ESMReader*> >& reader)
: mStore (store), mReader (reader),
  mIdCache (40, std::pair<std::string, CellStore *> ("", (CellStore*)0)), /// \todo make cache size configurable
  mIdCacheIndex (0)
{}

MWWorld::CellStore *MWWorld::Cells::getExterior (int x, int y)
{
    std::map<std::pair<int, int>, CellStore>::iterator result =
        mExteriors.find (std::make_pair (x, y));

    if (result==mExteriors.end())
    {
        const ESM::Cell *cell = mStore.get<ESM::Cell>().search(x, y);

        if (!cell)
        {
            // Cell isn't predefined. Make one on the fly.
            ESM::Cell record;

            record.mData.mFlags = ESM::Cell::HasWater;
            record.mData.mX = x;
            record.mData.mY = y;
            record.mWater = 0;
            record.mMapColor = 0;

            cell = MWBase::Environment::get().getWorld()->createRecord (record);
        }

        result = mExteriors.insert (std::make_pair (
            std::make_pair (x, y), CellStore (cell))).first;
    }

    if (result->second.getState()!=CellStore::State_Loaded)
    {
        // Multiple plugin support for landscape data is much easier than for references. The last plugin wins.
        result->second.load (mStore, mReader);
    }

    return &result->second;
}

MWWorld::CellStore *MWWorld::Cells::getInterior (const std::string& name)
{
    std::string lowerName = Misc::StringUtils::lowerCase(name);
    std::map<std::string, CellStore>::iterator result = mInteriors.find (lowerName);

    if (result==mInteriors.end())
    {
        const ESM::Cell *cell = mStore.get<ESM::Cell>().search(lowerName);
        if (!cell) // might be a foreign cell
            return getForeignInterior(name);

        //const ESM::Cell *cell = mStore.get<ESM::Cell>().find(lowerName);

        result = mInteriors.insert (std::make_pair (lowerName, CellStore (cell))).first;
    }

    if (result->second.getState()!=CellStore::State_Loaded)
    {
        result->second.load (mStore, mReader);
    }

    return &result->second;
}

MWWorld::CellStore *MWWorld::Cells::getCell (const ESM::CellId& id)
{
    if (id.mPaged)
        return getExterior (id.mIndex.mX, id.mIndex.mY);

    return getInterior (id.mWorldspace);
}

// in line with COE to COW
MWWorld::CellStore *MWWorld::Cells::getForeignWorld (const std::string& world, int x, int y)
{
#if 0
    // -- start of tests
    {
    std::string lowerWorld = Misc::StringUtils::lowerCase(world);
    const MWWorld::ForeignWorld *foreignWorld = mStore.get<MWWorld::ForeignWorld>().find(lowerWorld);
    if (!foreignWorld)
        return 0; // FIXME: print an error? maybe it should have been tested before this method was called and we should throw instead

    std::map<std::pair<int, int>, ESM4::FormId>::const_iterator it = foreignWorld->mCells.begin();
    for (; it != foreignWorld->mCells.end(); ++it)
    {
        std::cout << "cell: " << ESM4::formIdToString(it->second)
            << std::dec << ", x: " << it->first.first << ", y: " << it->first.second
            << std::endl; // FIXME: debug

        if (it->first.first == x && it->first.second == y)
        {
            const MWWorld::ForeignCell *foreignCell = mStore.get<MWWorld::ForeignCell>().find(it->second);
            if (!foreignCell)
                return 0; // FIXME: print an error? maybe it should have been tested before this method was called and we should throw instead

            std::cout << "cell found: " << ESM4::formIdToString(it->second) << std::endl;
        }

    //return 0;
    }
    }
    // -- end of tests
#endif
    ESM4::FormId formId = mStore.get<ForeignWorld>().getFormId(world);
    return getForeignWorld(formId, x, y);
}

MWWorld::CellStore *MWWorld::Cells::getForeignWorld (ESM4::FormId worldId, int x, int y)
{
    typedef std::map<std::pair<int, int>, CellStore> CellStoreIndex;

    // find the world for the given form id
    const ForeignWorld *world = mStore.get<ForeignWorld>().find(worldId);
    if (!world)
        return 0;// FIXME: maybe exception?

    // now find the cell's formid for the given x, y
    std::map<std::pair<int, int>, ESM4::FormId>::const_iterator it = world->mCells.find(std::make_pair(x, y));
    if (it == world->mCells.end())
        return 0; // FIXME: maybe exception?

    // get the cell given the formid
    const ForeignCell *cell = mStore.get<ForeignCell>().find(it->second);
    if (!cell)
        return 0; // FIXME: maybe exception?

    // insert into the map
    std::map<ESM4::FormId, CellStoreIndex>::iterator lb = mForeignWorlds.lower_bound(worldId);

    if (lb != mForeignWorlds.end() && !(mForeignWorlds.key_comp()(worldId, lb->first)))
    {
        // found world
        std::pair<CellStoreIndex::iterator, bool> res
            = lb->second.insert({ std::pair<int, int>(x, y), CellStore(cell, true) });

        // There may be a CellStore already at that location!
        // This can happen if a mod updates the record, just overwrite it for now // FIXME
        // FIXME: Is CellStore constructed twice here?
        // FIXME: always loading!!!
        //if (!res.second)
            //res.first->second = CellStore(cell, true); // can't use [] as it needs a default constructor
    }
    else // insert a new world
        mForeignWorlds.insert(lb, std::map<ESM4::FormId, CellStoreIndex>::value_type(worldId,
            { {std::pair<int, int>(x, y), CellStore(cell, true) } }));

    // FIXME: do we have an iterator already to avoid calling find() again?
    CellStoreIndex::iterator result = mForeignWorlds[worldId].find(std::pair<int, int>(x, y));
    if (result->second.getState() != CellStore::State_Loaded)
    {
        // Multiple plugin support for landscape data is much easier than for references. The last plugin wins.
        result->second.load(mStore, mReader);

        // inherit parent world's land?
        // save context
        //ESM4::ReaderContext ctx = mReader

        // restore context

        //if (lb->first->mParent != 0)
        // FIXME: TODO weather, etc
    }

    return &result->second;
}

// If one does not exist insert in mForeignInteriors. Load as required.
MWWorld::CellStore *MWWorld::Cells::getForeignInterior (const std::string& name)
{
    std::string lowerName = Misc::StringUtils::lowerCase(name);
    std::map<std::string, CellStore>::iterator result = mForeignInteriors.find(lowerName);

    if (result == mForeignInteriors.end())
    {
        const ForeignCell *cell = mStore.get<ForeignCell>().find(lowerName);

        //if (cell) // FIXME: why null?
            result = mForeignInteriors.insert(std::make_pair(lowerName, CellStore(cell, true))).first;
        //else
            //return 0;
    }

    if (result->second.getState() != CellStore::State_Loaded)
    {
        result->second.load(mStore, mReader);
    }

    return &result->second;
}

MWWorld::Ptr MWWorld::Cells::getPtr (const std::string& name, CellStore& cell,
    bool searchInContainers)
{
    if (cell.getState()==CellStore::State_Unloaded)
        cell.preload (mStore, mReader);

    if (cell.getState()==CellStore::State_Preloaded)
    {
        if (cell.hasId (name))
        {
            cell.load (mStore, mReader);
        }
        else
            return Ptr();
    }

    Ptr ptr = cell.search (name);

    if (!ptr.isEmpty())
        return ptr;

    if (searchInContainers)
        return cell.searchInContainer (name);

    return Ptr();
}

MWWorld::Ptr MWWorld::Cells::getPtr (const std::string& name)
{
    // First check the cache
    for (std::vector<std::pair<std::string, CellStore *> >::iterator iter (mIdCache.begin());
        iter!=mIdCache.end(); ++iter)
        if (iter->first==name && iter->second)
        {
            Ptr ptr = getPtr (name, *iter->second);
            if (!ptr.isEmpty())
                return ptr;
        }

    // Then check cells that are already listed
    // Search in reverse, this is a workaround for an ambiguous chargen_plank reference in the vanilla game.
    // there is one at -22,16 and one at -2,-9, the latter should be used.
    for (std::map<std::pair<int, int>, CellStore>::reverse_iterator iter = mExteriors.rbegin();
        iter!=mExteriors.rend(); ++iter)
    {
        Ptr ptr = getPtrAndCache (name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    for (std::map<std::string, CellStore>::iterator iter = mInteriors.begin();
        iter!=mInteriors.end(); ++iter)
    {
        Ptr ptr = getPtrAndCache (name, iter->second);
        if (!ptr.isEmpty())
            return ptr;
    }

    // Now try the other cells
    const MWWorld::Store<ESM::Cell> &cells = mStore.get<ESM::Cell>();
    MWWorld::Store<ESM::Cell>::iterator iter;

    for (iter = cells.extBegin(); iter != cells.extEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    for (iter = cells.intBegin(); iter != cells.intEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            return ptr;
    }

    // giving up
    return Ptr();
}

void MWWorld::Cells::getExteriorPtrs(const std::string &name, std::vector<MWWorld::Ptr> &out)
{
    const MWWorld::Store<ESM::Cell> &cells = mStore.get<ESM::Cell>();
    for (MWWorld::Store<ESM::Cell>::iterator iter = cells.extBegin(); iter != cells.extEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            out.push_back(ptr);
    }
}

void MWWorld::Cells::getInteriorPtrs(const std::string &name, std::vector<MWWorld::Ptr> &out)
{
    const MWWorld::Store<ESM::Cell> &cells = mStore.get<ESM::Cell>();
    for (MWWorld::Store<ESM::Cell>::iterator iter = cells.intBegin(); iter != cells.intEnd(); ++iter)
    {
        CellStore *cellStore = getCellStore (&(*iter));

        Ptr ptr = getPtrAndCache (name, *cellStore);

        if (!ptr.isEmpty())
            out.push_back(ptr);
    }
}

int MWWorld::Cells::countSavedGameRecords() const
{
    int count = 0;

    for (std::map<std::string, CellStore>::const_iterator iter (mInteriors.begin());
        iter!=mInteriors.end(); ++iter)
        if (iter->second.hasState())
            ++count;

    for (std::map<std::pair<int, int>, CellStore>::const_iterator iter (mExteriors.begin());
        iter!=mExteriors.end(); ++iter)
        if (iter->second.hasState())
            ++count;

    return count;
}

void MWWorld::Cells::write (ESM::ESMWriter& writer, Loading::Listener& progress) const
{
    for (std::map<std::pair<int, int>, CellStore>::iterator iter (mExteriors.begin());
        iter!=mExteriors.end(); ++iter)
        if (iter->second.hasState())
        {
            writeCell (writer, iter->second);
            progress.increaseProgress();
        }

    for (std::map<std::string, CellStore>::iterator iter (mInteriors.begin());
        iter!=mInteriors.end(); ++iter)
        if (iter->second.hasState())
        {
            writeCell (writer, iter->second);
            progress.increaseProgress();
        }
}

bool MWWorld::Cells::readRecord (ESM::ESMReader& reader, uint32_t type,
    const std::map<int, int>& contentFileMap)
{
    if (type==ESM::REC_CSTA)
    {
        ESM::CellState state;
        state.mId.load (reader);

        CellStore *cellStore = 0;

        try
        {
            cellStore = getCell (state.mId);
        }
        catch (...)
        {
            // silently drop cells that don't exist anymore
            reader.skipRecord();
            return true;
            /// \todo log
        }

        state.load (reader);
        cellStore->loadState (state);

        if (state.mHasFogOfWar)
            cellStore->readFog(reader);

        if (cellStore->getState()!=CellStore::State_Loaded)
            cellStore->load (mStore, mReader);

        cellStore->readReferences (reader, contentFileMap);

        return true;
    }

    return false;
}
