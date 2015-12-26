#include "cellcollection.hpp"

#include <stdexcept>
#include <cassert>

#include <iostream> // FIXME
#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

#include <libs/platform/strings.h>

#include <extern/esm4/reader.hpp>

#include "../world/record.hpp"
#include "../world/universalid.hpp"

#include "worldcollection.hpp"

namespace CSMWorld
{
    template<>
    void Collection<CSMForeign::Cell, IdAccessor<CSMForeign::Cell> >::removeRows (int index, int count)
    {
        mRecords.erase(mRecords.begin()+index, mRecords.begin()+index+count);

        // index map is updated in CellCollection::removeRows()
    }

    template<>
    void Collection<CSMForeign::Cell, IdAccessor<CSMForeign::Cell> >::insertRecord (std::unique_ptr<RecordBase> record,
        int index, UniversalId::Type type)
    {
        int size = static_cast<int>(mRecords.size());
        if (index < 0 || index > size)
            throw std::runtime_error("index out of range");

        std::unique_ptr<Record<CSMForeign::Cell> > record2(static_cast<Record<CSMForeign::Cell>*>(record.release()));

        if (index == size)
            mRecords.push_back(std::move(record2));
        else
            mRecords.insert(mRecords.begin()+index, std::move(record2));

        // index map is updated in CellCollection::insertRecord()
    }
}

CSMForeign::CellCollection::CellCollection (const WorldCollection& worlds) :mWorlds(worlds)
{
}

CSMForeign::CellCollection::~CellCollection ()
{
}

// http://www.uesp.net/wiki/Tes4Mod:Mod_File_Format/CELL
//
// The block and subblock groups for an interior cell are determined by the last two decimal
// digits of the lower 3 bytes of the cell form ID (the modindex is not included in the
// calculation). For example, for form ID 0x000CF2=3314, the block is 4 and the subblock is 1.
//
// The block and subblock groups for an exterior cell are determined by the X-Y coordinates of
// the cell. Each block contains 16 subblocks (4x4) and each subblock contains 64 cells (8x8).
// So each block contains 1024 cells (32x32).

int CSMForeign::CellCollection::load (ESM4::Reader& reader, bool base)
{
    CSMForeign::Cell record;
    std::string id;
    ESM4::FormId formId = reader.hdr().record.id;
    ESM4::formIdToString(formId, id);

    // reader.currCellGrid() is set during the load (sub record XCLC for an exterior cell)
    loadRecord(record, reader);
    if (reader.hasCellGrid())
    {
        assert((reader.grp().type == ESM4::Grp_ExteriorSubCell ||
                reader.grp().type == ESM4::Grp_WorldChild) && "Unexpected group while loading cell");

        ESM4::gridToString(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y, record.mCellId);
    }
    else
    {
        // Toddland, EmptyWorld, Bloated Float at Sea, Skingrad, Plane of Oblivion, Bravil,
        // etc, etc, are in Grp_WorldChild but without any grids
        //
        // TestRender, EmptyCell, OblivionMQKvatchBridge, thehill, Hawkhaven02, 000009bf, 0000169f,
        // etc, etc, are in Grp_ExteriorSubCell but witout any grids
        assert((reader.grp().type == ESM4::Grp_ExteriorSubCell ||
                reader.grp().type == ESM4::Grp_InteriorSubCell ||
                reader.grp().type == ESM4::Grp_WorldChild) && "Unexpected group while loading cell");

        if (!record.mEditorId.empty()) // can't use Full Name since they are not unique
            record.mCellId = record.mEditorId; // FIXME: check if editor id's are uplicated
        else
            record.mCellId = id; // use formId string instead of "#x y"
    }

    int index = searchId(formId);

    if (index == -1)
        record.mId = id; // new record
    else
    {
        std::cout << "record overwritten" << std::endl;
        //record = this->getRecord(index).get(); // FIXME: record (just loaded) is being overwritten
    }

    record.mWorld = mWorlds.getIdString(record.mParent); // FIXME: assumes our world is already loaded
    if (!record.mRegions.empty())
        ESM4::formIdToString(record.mRegions.back(), record.mRegion);

    return load(record, base, index);
}

void CSMForeign::CellCollection::loadRecord (Cell& record, ESM4::Reader& reader)
{
    record.load(reader);
}

int CSMForeign::CellCollection::load (const Cell& record, bool base, int index)
{
    if (index == -2) // unknown index
        index = searchId(static_cast<ESM4::FormId>(std::stoi(record.mId, nullptr, 16)));

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<Cell> > record2(new CSMWorld::Record<Cell>);
        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<Cell> > record2(new CSMWorld::Record<Cell>(
                    CSMWorld::Collection<Cell, CSMWorld::IdAccessor<Cell> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

int CSMForeign::CellCollection::searchId (const std::string& id) const
{
    return searchId(static_cast<ESM4::FormId>(std::stoi(id, nullptr, 16))); // hex
}

int CSMForeign::CellCollection::getIndex (ESM4::FormId formId) const
{
    int index = searchId(formId);

    if (index == -1)
        throw std::runtime_error("CellCollection: invalid formId: " + ESM4::formIdToString(formId));

    return index;
}

void CSMForeign::CellCollection::removeRows (int index, int count)
{
    CSMWorld::Collection<Cell, CSMWorld::IdAccessor<Cell> >::removeRows(index, count); // erase records only

    CellIndexMap::iterator iter = mCellIndex.begin();
    while (iter != mCellIndex.end())
    {
        if (iter->second>=index)
        {
            if (iter->second >= index+count)
            {
                iter->second -= count;
                ++iter;
            }
            else
                mCellIndex.erase(iter++);
        }
        else
            ++iter;
    }
}

int CSMForeign::CellCollection::searchId (ESM4::FormId formId) const
{
    CellIndexMap::const_iterator iter = mCellIndex.find(formId);

    if (iter == mCellIndex.end())
        return -1;

    return iter->second;
}

void CSMForeign::CellCollection::insertRecord (std::unique_ptr<CSMWorld::RecordBase> record, int index,
    CSMWorld::UniversalId::Type type)
{
    int size = getAppendIndex(/*id*/"", type); // id is ignored
    std::string id = static_cast<CSMWorld::Record<Cell>*>(record.get())->get().mId;
    ESM4::FormId formId = static_cast<CSMWorld::Record<Cell>*>(record.get())->get().mFormId;

    // first add records only
    CSMWorld::Collection<Cell, CSMWorld::IdAccessor<Cell> >::insertRecord(std::move(record), index, type);

    // then update cell index
    if (index < size-1)
    {
        for (CellIndexMap::iterator iter(mCellIndex.begin()); iter != mCellIndex.end(); ++iter)
        {
            if (iter->second >= index)
                ++(iter->second);
        }
    }

    mCellIndex.insert(std::make_pair(formId, index));
}
