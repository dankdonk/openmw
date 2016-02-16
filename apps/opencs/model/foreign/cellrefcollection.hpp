#ifndef CSM_FOREIGN_CELLREFCOLLECTION_H
#define CSM_FOREIGN_CELLREFCOLLECTION_H

#include <algorithm>

#include <extern/esm4/reader.hpp>

#include "idcollection.hpp"
#include "cellgroupcollection.hpp"

namespace ESM4
{
    typedef std::uint32_t FormId;
}

namespace CSMForeign
{
    template<typename RecordT>
    class CellRefCollection : public IdCollection<RecordT>
    {
        CellGroupCollection& mCellGroups;

    public:
        CellRefCollection (CellGroupCollection& cellGroups);
        ~CellRefCollection ();

        virtual int load(ESM4::Reader& reader, bool base);

    private:
        CellRefCollection ();
        CellRefCollection (const CellRefCollection& other);
        CellRefCollection& operator= (const CellRefCollection& other);

        void update (const RecordT& record, std::vector<ESM4::FormId>& cellRefs);
    };

    template<typename RecordT>
    CellRefCollection<RecordT>::CellRefCollection (CellGroupCollection& cellGroups)
    : mCellGroups(cellGroups)
    {}

    template<typename RecordT>
    CellRefCollection<RecordT>::~CellRefCollection ()
    {}

    template<typename RecordT>
    void CellRefCollection<RecordT>::update (const RecordT& record, std::vector<ESM4::FormId>& cellRefs)
    {
        if ((record.mFlags & ESM4::Rec_Deleted) != 0)
        {
            std::vector<ESM4::FormId>::iterator it =
                std::find(cellRefs.begin(), cellRefs.end(), record.mFormId);

            if (it != cellRefs.end())
            {
                std::cout << "deleted " << record.mId << std::endl;
                cellRefs.erase(it);
            }
        }
        else
            cellRefs.push_back(record.mFormId);
    }

    template<typename RecordT>
    int CellRefCollection<RecordT>::load (ESM4::Reader& reader, bool base)
    {
        using CSMWorld::Record;

        // load the record
        RecordT record; // REFR, ACHR or ACRE
        IdCollection<RecordT>::loadRecord(record, reader);

        // update mCell to make it easier to locate the ref
        if (reader.hasCellGrid())
            ESM4::gridToString(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y, record.mCell);
        else
            record.mCell = record.mId; // use formId string instead

        // first cache the record's formId to its parent cell group
        int cellIndex = mCellGroups.searchFormId(reader.currCell());
        if (cellIndex == -1)
        {
            // new cell group
            CellGroup cellGroup;

            switch (reader.grp().type)
            {
                case ESM4::Grp_CellPersistentChild:  update(record, cellGroup.mPersistent);     break;
                case ESM4::Grp_CellVisibleDistChild: update(record, cellGroup.mVisibleDistant); break;
                case ESM4::Grp_CellTemporaryChild:   update(record, cellGroup.mTemporary);      break;
                default:
                    throw std::runtime_error("unexpected group while loading cellref");
            }

            std::unique_ptr<Record<CellGroup> > record2(new Record<CellGroup>);
            record2->mState = CSMWorld::RecordBase::State_BaseOnly;
            record2->mBase = cellGroup;

            mCellGroups.insertRecord(std::move(record2), mCellGroups.getSize());
        }
        else
        {
            // existing cell group
            std::unique_ptr<Record<CellGroup> > record2(new Record<CellGroup>);
            record2->mBase = mCellGroups.getRecord(cellIndex).get();
            record2->mState = CSMWorld::RecordBase::State_BaseOnly; // FIXME: State_Modified if new modindex?
            CellGroup &cellGroup = record2->get();

            switch (reader.grp().type)
            {
                case ESM4::Grp_CellPersistentChild:  update(record, cellGroup.mPersistent);     break;
                case ESM4::Grp_CellVisibleDistChild: update(record, cellGroup.mVisibleDistant); break;
                case ESM4::Grp_CellTemporaryChild:   update(record, cellGroup.mTemporary);      break;
                default:
                    throw std::runtime_error("unexpected group while loading cellref");
            }

            mCellGroups.setRecord(cellIndex, std::move(record2));
        }

        // continue with the rest of the loading
        int index = this->searchFormId(record.mFormId);

        // deal with deleted records
        if ((record.mFlags & ESM4::Rec_Deleted) != 0)
        {
            std::cout << "deleting " << record.mId << std::endl; // FIXME
            if (base)
            {
                if (index == -1)
                {
                    // cannot delete a non-existent record - may have been deleted by one of
                    // the (master) dependencies or another file loaded before this file
                    return -1;
                }
                else
                {
                    // being deleted by one of the (master) dependencies or another file loaded
                    // before the content file
                    this->removeRows(index, 1);
                    return -1;
// FIXME below can't work since mFormId must match...
#if 0
                    // the removeRows() operation can be slow - just mark it deleted for now
                    std::unique_ptr<Record<RecordT> > baseRecord(new Record<RecordT>);
                    baseRecord->mBase = this->getRecord(index).get();
                    baseRecord->mBase.mFormId |= 0xff000000; // hack to indicate erased
                    baseRecord->mState = CSMWorld::RecordBase::State_Deleted;
                    this->setRecord(index, std::move(baseRecord));
                    return index;
#endif
                }
            }
            else
            {
                std::unique_ptr<Record<RecordT> > baseRecord(new Record<RecordT>);

                if (index == -1)
                {
                    // need to keep a deleted marker when saving
                    baseRecord->mBase = record;
                    baseRecord->mState = CSMWorld::RecordBase::State_Deleted;
                    index = this->getSize();
                    this->insertRecord(std::move(baseRecord), index);
                }
                else
                {
                    // TODO: when saving remember that the deleted record size is usually zero
                    baseRecord->mBase = this->getRecord(index).get();
                    baseRecord->mState = CSMWorld::RecordBase::State_Deleted;
                    this->setRecord(index, std::move(baseRecord));
                }
                return index;
            }
        }

        return IdCollection<RecordT>::load(record, base, index);
    }
}
#endif // CSM_FOREIGN_CELLREFCOLLECTION_H
