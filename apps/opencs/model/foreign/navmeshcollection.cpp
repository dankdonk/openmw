#include "navmeshcollection.hpp"

#include <stdexcept>
#include <iostream> // FIXME

#include <extern/esm4/reader.hpp>

#include "../world/idcollection.hpp"
#include "../world/cell.hpp"
#include "../world/record.hpp"

// FIXME: refactor as foreign/idcollection
namespace CSMWorld
{
    template<>
    void Collection<CSMForeign::NavMesh, IdAccessor<CSMForeign::NavMesh> >::removeRows (int index, int count)
    {
        mRecords.erase(mRecords.begin()+index, mRecords.begin()+index+count);

        // index map is updated in NavMeshCollection::removeRows()
    }

    template<>
    void Collection<CSMForeign::NavMesh, IdAccessor<CSMForeign::NavMesh> >::insertRecord (std::unique_ptr<RecordBase> record,
        int index, UniversalId::Type type)
    {
        int size = static_cast<int>(mRecords.size());
        if (index < 0 || index > size)
            throw std::runtime_error("index out of range");

        std::unique_ptr<Record<CSMForeign::NavMesh> > record2(static_cast<Record<CSMForeign::NavMesh>*>(record.release()));

        if (index == size)
            mRecords.push_back(std::move(record2));
        else
            mRecords.insert(mRecords.begin()+index, std::move(record2));

        // index map is updated in NavMeshCollection::insertRecord()
    }
}

CSMForeign::NavMeshCollection::NavMeshCollection (const CSMWorld::IdCollection<CSMWorld::Cell, CSMWorld::IdAccessor<CSMWorld::Cell> >& cells)
  : mCells (cells)
{
}

CSMForeign::NavMeshCollection::~NavMeshCollection ()
{
}

int CSMForeign::NavMeshCollection::load (ESM4::Reader& reader, bool base)
{
    CSMForeign::NavMesh record;
    ESM4::FormId formId = reader.hdr().record.id;
    reader.adjustFormId(formId);
    //std::cout << "new NavMesh " << std::hex << &record << std::endl; // FIXME

    std::string id;
#if 0
    // HACK // FIXME
    if (reader.grp().type != ESM4::Grp_CellTemporaryChild)
        return -1; // FIXME
    else if (reader.grp(2).type == ESM4::Grp_InteriorCell)
    {
        // FIXME: another id?
        id = "";
    }
    else
    {
        // FIXME: navmesh can occur in interior cells
        std::ostringstream stream;
        //stream << "#" << reader.currCellGrid().grid.x << " " << reader.currCellGrid().grid.y;
        //stream << "#" << std::floor((float)reader.currCellGrid().grid.x/2)
               //<< " " << std::floor((float)reader.currCellGrid().grid.y/2);
        id = ""; //stream.str();
        //std::cout << "loading Cell " << id << std::endl; // FIXME
    }
#endif
    id = std::to_string(formId); // use formId instead

    int index = searchId(formId);

    if (index == -1)
        CSMWorld::IdAccessor<CSMForeign::NavMesh>().getId(record) = id;
    else
    {
        record = this->getRecord(index).get();
    }

    loadRecord(record, reader);

    return load(record, base, index);
}

void CSMForeign::NavMeshCollection::loadRecord (CSMForeign::NavMesh& record, ESM4::Reader& reader)
{
    record.load(reader, mCells);
}

int CSMForeign::NavMeshCollection::load (const CSMForeign::NavMesh& record, bool base, int index)
{
    if (index == -2)
        index = this->searchId(CSMWorld::IdAccessor<CSMForeign::NavMesh>().getId(record));

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<CSMForeign::NavMesh> > record2(new CSMWorld::Record<CSMForeign::NavMesh>);
        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<CSMForeign::NavMesh> > record2(
                new CSMWorld::Record<CSMForeign::NavMesh>(CSMWorld::Collection<CSMForeign::NavMesh,
                    CSMWorld::IdAccessor<CSMForeign::NavMesh> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

int CSMForeign::NavMeshCollection::searchId (const std::string& id) const
{
    return searchId(static_cast<ESM4::FormId>(std::stoi(id, nullptr, 16))); // hex base
}

int CSMForeign::NavMeshCollection::getIndex (ESM4::FormId id) const
{
    int index = searchId(id);

    if (index == -1)
        throw std::runtime_error("invalid formId: " + std::to_string(id));

    return index;
}

void CSMForeign::NavMeshCollection::removeRows (int index, int count)
{
    CSMWorld::Collection<NavMesh, CSMWorld::IdAccessor<NavMesh> >::removeRows(index, count); // erase records only

    std::map<ESM4::FormId, int>::iterator iter = mNavMeshIndex.begin();
    while (iter != mNavMeshIndex.end())
    {
        if (iter->second>=index)
        {
            if (iter->second >= index+count)
            {
                iter->second -= count;
                ++iter;
            }
            else
                mNavMeshIndex.erase(iter++);
        }
        else
            ++iter;
    }
}

int CSMForeign::NavMeshCollection::searchId (ESM4::FormId id) const
{
    std::map<ESM4::FormId, int>::const_iterator iter = mNavMeshIndex.find(id);

    if (iter == mNavMeshIndex.end())
        return -1;

    return iter->second;
}

void CSMForeign::NavMeshCollection::insertRecord (std::unique_ptr<CSMWorld::RecordBase> record, int index,
    CSMWorld::UniversalId::Type type)
{
    int size = getAppendIndex(/*id*/"", type); // id is ignored
    ESM4::FormId formId = static_cast<CSMWorld::Record<NavMesh>*>(record.get())->get().mFormId;

    CSMWorld::Collection<NavMesh, CSMWorld::IdAccessor<NavMesh> >::insertRecord(std::move(record), index, type); // add records only

    if (index < size-1)
    {
        for (std::map<ESM4::FormId, int>::iterator iter(mNavMeshIndex.begin()); iter != mNavMeshIndex.end(); ++iter)
        {
            if (iter->second >= index)
                ++(iter->second);
        }
    }

    mNavMeshIndex.insert(std::make_pair(formId, index));
}
