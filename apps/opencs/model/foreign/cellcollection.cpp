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
    ESM4::formIdToString(reader.hdr().record.id, id);
#if 0
    std::string padding = "";
    padding.insert(0, reader.stackSize()*2, ' ');
    std::cout << padding << "CELL: formId " << std::hex << reader.hdr().record.id << std::endl;
    std::cout << padding << "CELL type " << std::hex << reader.grp().type << std::endl;
#endif

    // reader.currCellGrid() is set during the load (sub record XCLC for an exterior cell)
    loadRecord(record, reader);
    if (reader.hasCellGrid())
    {
        assert((reader.grp().type == ESM4::Grp_ExteriorSubCell ||
                reader.grp().type == ESM4::Grp_WorldChild) && "Unexpected group while loading cell");

#if 0
        std::string padding = "";
        padding.insert(0, reader.stackSize()*2, ' ');
        std::cout << padding << "CELL X " << std::dec << reader.currCellGrid().grid.x
            << ", Y " << reader.currCellGrid().grid.y << std::endl;

        std::ostringstream stream;
        stream << "#" << reader.currCellGrid().grid.x << " " << reader.currCellGrid().grid.y;
        //stream << "#" << std::floor((float)reader.currCellGrid().grid.x/2)
               //<< " " << std::floor((float)reader.currCellGrid().grid.y/2);
        id = stream.str();
#endif
        char buf[100];
        int res = snprintf(buf, 100, "#%d %d", reader.currCellGrid().grid.x, reader.currCellGrid().grid.y);
        if (res > 0 && res < 100)
            id.assign(buf);
        else
            throw std::runtime_error("Cell Collection possible buffer overflow");
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

        record.mName = id; // use formId string instead of "#x y" for name
    }

    int index = searchId(reader.hdr().record.id);

    if (index == -1)
        CSMWorld::IdAccessor<CSMForeign::Cell>().getId(record) = id; // new record, set mId
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

void CSMForeign::CellCollection::loadRecord (CSMForeign::Cell& record, ESM4::Reader& reader)
{
    record.load(reader);
}

int CSMForeign::CellCollection::load (const CSMForeign::Cell& record, bool base, int index)
{
    if (index == -2) // unknown index
        index = this->searchId(CSMWorld::IdAccessor<CSMForeign::Cell>().getId(record)); // FIXME

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<CSMForeign::Cell> > record2(new CSMWorld::Record<CSMForeign::Cell>);
        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<CSMForeign::Cell> > record2(
                new CSMWorld::Record<CSMForeign::Cell>(
                    CSMWorld::Collection<CSMForeign::Cell, CSMWorld::IdAccessor<CSMForeign::Cell> >::getRecord(index)));

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
    return searchId(static_cast<std::uint32_t>(std::stoi(id, nullptr, 16))); // hex
}

int CSMForeign::CellCollection::getIndex (std::uint32_t id) const
{
    int index = searchId(id);

    if (index == -1)
        throw std::runtime_error("invalid formId: " + std::to_string(id));

    return index;
}

void CSMForeign::CellCollection::removeRows (int index, int count)
{
    CSMWorld::Collection<Cell, CSMWorld::IdAccessor<Cell> >::removeRows(index, count); // erase records only

    std::map<std::uint32_t, int>::iterator iter = mCellIndex.begin();
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

int CSMForeign::CellCollection::searchId (std::uint32_t id) const
{
    std::map<std::uint32_t, int>::const_iterator iter = mCellIndex.find(id);

    if (iter == mCellIndex.end())
        return -1;

    return iter->second;
}

void CSMForeign::CellCollection::insertRecord (std::unique_ptr<CSMWorld::RecordBase> record, int index,
    CSMWorld::UniversalId::Type type)
{
    int size = getAppendIndex(/*id*/"", type); // id is ignored
    std::string id = static_cast<CSMWorld::Record<Cell>*>(record.get())->get().mId;
    std::uint32_t formId = static_cast<CSMWorld::Record<Cell>*>(record.get())->get().mFormId;

    CSMWorld::Collection<Cell, CSMWorld::IdAccessor<Cell> >::insertRecord(std::move(record), index, type); // add records only

    if (index < size-1)
    {
        for (std::map<std::uint32_t, int>::iterator iter(mCellIndex.begin()); iter != mCellIndex.end(); ++iter)
        {
            if (iter->second >= index)
                ++(iter->second);
        }
    }

    mCellIndex.insert(std::make_pair(formId, index));
}
