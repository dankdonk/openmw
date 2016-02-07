#include "refcollection.hpp"

#include <iostream> // FIXME
#ifdef NDEBUG // FIXME for debugging only
#undef NDEBUG
#endif

#include <libs/platform/strings.h>

#include <extern/esm4/reader.hpp>

#include "../world/record.hpp"

#include "cellcollection.hpp"

namespace CSMWorld
{
    template<>
    void Collection<CSMForeign::CellRef, IdAccessor<CSMForeign::CellRef> >::removeRows (int index, int count)
    {
        mRecords.erase(mRecords.begin()+index, mRecords.begin()+index+count);

        // index map is updated in RefCollection::removeRows()
    }

    template<>
    void Collection<CSMForeign::CellRef, IdAccessor<CSMForeign::CellRef> >::insertRecord (
            std::unique_ptr<RecordBase> record, int index, UniversalId::Type type)
    {
        int size = static_cast<int>(mRecords.size());
        if (index < 0 || index > size)
            throw std::runtime_error("index out of range");

        std::unique_ptr<Record<CSMForeign::CellRef> > record2
            (static_cast<Record<CSMForeign::CellRef>*>( record.release()));

        if (index == size)
            mRecords.push_back(std::move(record2));
        else
            mRecords.insert(mRecords.begin()+index, std::move(record2));

        // index map is updated in RefCollection::insertRecord()
    }
}

CSMForeign::RefCollection::RefCollection (CellCollection& cells)
  : mCells (cells)
{
}

CSMForeign::RefCollection::~RefCollection ()
{
}

int CSMForeign::RefCollection::load (ESM4::Reader& reader, bool base)
{
    CSMForeign::CellRef record;

    std::string id;
    ESM4::FormId formId = reader.adjustFormId(reader.hdr().record.id); // FIXME: use master adjusted?
    ESM4::formIdToString(formId, id);

    // cache the ref's formId to its parent cell
    Cell *cell = mCells.getCell(reader.currCell()); // FIXME: const issue with Collection

    if (cell)
        switch (reader.grp().type)
        {
            case ESM4::Grp_CellPersistentChild:
            {
                cell->mRefPersistent.push_back(formId);
                break;
            }
            case ESM4::Grp_CellVisibleDistChild:
            {
                cell->mRefVisibleDistant.push_back(formId);
                break;
            }
            case ESM4::Grp_CellTemporaryChild:
            {
                cell->mRefTemporary.push_back(formId);
                break;
            }
            default: break; // do nothing
        }

    if (reader.hasCellGrid())
        ESM4::gridToString(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y, record.mCell);
    else
        record.mCell = id; // use formId string instead

    int index = searchId(formId);

    if (index == -1)
        record.mId = id; // new record
    else
        record = this->getRecord(index).get();

    record.load(reader);

    return load(record, base, index);
}

int CSMForeign::RefCollection::load (const CellRef& record, bool base, int index)
{
    if (index == -2) // unknown index
        index = searchId(static_cast<ESM4::FormId>(std::stoi(record.mId, nullptr, 16))); // hex base

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<CellRef> > record2(new CSMWorld::Record<CellRef>);

        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<CellRef> > record2(new CSMWorld::Record<CellRef>(
                CSMWorld::Collection<CellRef, CSMWorld::IdAccessor<CellRef> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

int CSMForeign::RefCollection::searchId (const std::string& id) const
{
    return searchId(static_cast<ESM4::FormId>(std::stoi(id, nullptr, 16))); // hex base
}

int CSMForeign::RefCollection::getIndex (ESM4::FormId formId) const
{
    int index = searchId(formId);

    if (index == -1)
        throw std::runtime_error("RefCollection: invalid formId: " + ESM4::formIdToString(formId));

    return index;
}

void CSMForeign::RefCollection::removeRows (int index, int count)
{
    CSMWorld::Collection<CellRef, CSMWorld::IdAccessor<CellRef> >::removeRows(index, count); // erase records only

    RefIndexMap::iterator iter = mRefIndex.begin();
    while (iter != mRefIndex.end())
    {
        if (iter->second>=index)
        {
            if (iter->second >= index+count)
            {
                iter->second -= count;
                ++iter;
            }
            else
                mRefIndex.erase(iter++);
        }
        else
            ++iter;
    }
}

int CSMForeign::RefCollection::searchId (ESM4::FormId formId) const
{
    RefIndexMap::const_iterator iter = mRefIndex.find(formId);

    if (iter == mRefIndex.end())
        return -1;

    return iter->second;
}

void CSMForeign::RefCollection::insertRecord (std::unique_ptr<CSMWorld::RecordBase> record, int index,
    CSMWorld::UniversalId::Type type)
{
    int size = getAppendIndex(/*id*/"", type); // id is ignored
    std::string id = static_cast<CSMWorld::Record<CellRef>*>(record.get())->get().mId;
    ESM4::FormId formId = static_cast<CSMWorld::Record<CellRef>*>(record.get())->get().mFormId;

    // first add records only
    CSMWorld::Collection<CellRef, CSMWorld::IdAccessor<CellRef> >::insertRecord(std::move(record), index, type);

    // then update cell index
    if (index < size-1)
    {
        for (RefIndexMap::iterator iter(mRefIndex.begin()); iter != mRefIndex.end(); ++iter)
        {
            if (iter->second >= index)
                ++(iter->second);
        }
    }

    mRefIndex.insert(std::make_pair(formId, index));
}
