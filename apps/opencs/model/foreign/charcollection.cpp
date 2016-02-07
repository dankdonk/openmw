#include "charcollection.hpp"

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
    void Collection<CSMForeign::CellChar, IdAccessor<CSMForeign::CellChar> >::removeRows (int index, int count)
    {
        mRecords.erase(mRecords.begin()+index, mRecords.begin()+index+count);

        // index map is updated in CharCollection::removeRows()
    }

    template<>
    void Collection<CSMForeign::CellChar, IdAccessor<CSMForeign::CellChar> >::insertRecord (
            std::unique_ptr<RecordBase> record, int index, UniversalId::Type type)
    {
        int size = static_cast<int>(mRecords.size());
        if (index < 0 || index > size)
            throw std::runtime_error("index out of range");

        std::unique_ptr<Record<CSMForeign::CellChar> > record2
            (static_cast<Record<CSMForeign::CellChar>*>( record.release()));

        if (index == size)
            mRecords.push_back(std::move(record2));
        else
            mRecords.insert(mRecords.begin()+index, std::move(record2));

        // index map is updated in CharCollection::insertRecord()
    }
}

CSMForeign::CharCollection::CharCollection (CellCollection& cells)
  : mCells (cells)
{
}

CSMForeign::CharCollection::~CharCollection ()
{
}

int CSMForeign::CharCollection::load (ESM4::Reader& reader, bool base)
{
    CSMForeign::CellChar record;

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
    if (reader.hasCellGrid())
    {
        //std::cout << "baseobj " << ESM4::formIdToString(record.mBaseObj) << std::endl;
        std::string pos;
        ESM4::gridToString(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y, pos);
        //std::cout << pos << std::endl;
    }

    return load(record, base, index);
}

int CSMForeign::CharCollection::load (const CellChar& record, bool base, int index)
{
    if (index == -2) // unknown index
        index = searchId(static_cast<ESM4::FormId>(std::stoi(record.mId, nullptr, 16))); // hex base

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<CellChar> > record2(new CSMWorld::Record<CellChar>);

        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<CellChar> > record2(new CSMWorld::Record<CellChar>(
                CSMWorld::Collection<CellChar, CSMWorld::IdAccessor<CellChar> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

int CSMForeign::CharCollection::searchId (const std::string& id) const
{
    return searchId(static_cast<ESM4::FormId>(std::stoi(id, nullptr, 16))); // hex base
}

int CSMForeign::CharCollection::getIndex (ESM4::FormId formId) const
{
    int index = searchId(formId);

    if (index == -1)
        throw std::runtime_error("CharCollection: invalid formId: " + ESM4::formIdToString(formId));

    return index;
}

void CSMForeign::CharCollection::removeRows (int index, int count)
{
    CSMWorld::Collection<CellChar, CSMWorld::IdAccessor<CellChar> >::removeRows(index, count); // erase records only

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

int CSMForeign::CharCollection::searchId (ESM4::FormId formId) const
{
    RefIndexMap::const_iterator iter = mRefIndex.find(formId);

    if (iter == mRefIndex.end())
        return -1;

    return iter->second;
}

void CSMForeign::CharCollection::insertRecord (std::unique_ptr<CSMWorld::RecordBase> record, int index,
    CSMWorld::UniversalId::Type type)
{
    int size = getAppendIndex(/*id*/"", type); // id is ignored
    std::string id = static_cast<CSMWorld::Record<CellChar>*>(record.get())->get().mId;
    ESM4::FormId formId = static_cast<CSMWorld::Record<CellChar>*>(record.get())->get().mFormId;

    // first add records only
    CSMWorld::Collection<CellChar, CSMWorld::IdAccessor<CellChar> >::insertRecord(std::move(record), index, type);

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
