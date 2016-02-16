#ifndef CSM_FOREIGN_CELLREFCOLLECTION_H
#define CSM_FOREIGN_CELLREFCOLLECTION_H

#include <algorithm>
#include <stdexcept>

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

        void update (const RecordT& record,
                std::vector<ESM4::FormId>& cellRefs, std::vector<ESM4::FormId>& delRefs);

        void update (std::int32_t type, CellGroup& cellGroup);
    };

    template<typename RecordT>
    CellRefCollection<RecordT>::CellRefCollection (CellGroupCollection& cellGroups)
    : mCellGroups(cellGroups)
    {}

    template<typename RecordT>
    CellRefCollection<RecordT>::~CellRefCollection ()
    {}

    template<typename RecordT>
    void CellRefCollection<RecordT>::update (const RecordT& record,
            std::vector<ESM4::FormId>& cellRefs, std::vector<ESM4::FormId>& delRefs)
    {
        if ((record.mFlags & ESM4::Rec_Deleted) != 0)
        {
            std::vector<ESM4::FormId>::iterator it =
                std::find(cellRefs.begin(), cellRefs.end(), record.mFormId);

            if (it != cellRefs.end())
            {
                cellRefs.erase(it);
                delRefs.push_back(record.mFormId);
            }
        }
        else
            cellRefs.push_back(record.mFormId);
    }

    template<typename RecordT>
    void CellRefCollection<RecordT>::update (std::int32_t type, CellGroup& cellGroup)
    {
        switch (type)
        {
            case ESM4::Grp_CellPersistentChild:
                update(record, cellGroup.mPersistent, cellGroup.mdelPersistent);
                break;
            case ESM4::Grp_CellVisibleDistChild:
                update(record, cellGroup.mVisibleDist, cellGroup.mdelVisibleDist);
                break;
            case ESM4::Grp_CellTemporaryChild:
                update(record, cellGroup.mTemporary, cellGroup.mdelTemporary);
                break;
            default:
                throw std::runtime_error("Unexpected group found while loading a cell ref");
        }
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
            update(reader.grp().type, cellGroup);

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
            update(reader.grp().type, cellGroup);

            mCellGroups.setRecord(cellIndex, std::move(record2));
        }

        // continue with the rest of the loading
        int index = this->searchFormId(record.mFormId);

        // deal with deleted records
        // NOTE: keep the status of the content file's deleted records in cell group
        // TODO: when saving remember that the deleted record size is usually zero
        if ((record.mFlags & ESM4::Rec_Deleted) != 0)
        {
            if (index == -1)
            {
                // cannot delete a non-existent record - may have been deleted by one of
                // the (master) dependencies or another file loaded before this file
                //
                // NOTE: a dummy record may need to be created when saving the content file
                return -1;
            }

            if (base)
            {
                // being deleted by one of the (master) dependencies or another file loaded
                // before the content file
                //
                // the removeRows() operation can be slow but can't just mark it deleted
                // for base because it can be confusing while editing as the deletion
                // can't be reverted
                this->removeRows(index, 1);
                return -1;
            }

            std::unique_ptr<Record<RecordT> > baseRecord(new Record<RecordT>);
            baseRecord->mBase = this->getRecord(index).get();
            baseRecord->mState = CSMWorld::RecordBase::State_Deleted;
            this->setRecord(index, std::move(baseRecord));
            return index;
        }

        return IdCollection<RecordT>::load(record, base, index);
    }
}
#endif // CSM_FOREIGN_CELLREFCOLLECTION_H
