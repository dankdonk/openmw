#ifndef CSM_FOREIGN_CELLREFCOLLECTION_H
#define CSM_FOREIGN_CELLREFCOLLECTION_H

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
    };

    template<typename RecordT>
    CellRefCollection<RecordT>::CellRefCollection (CellGroupCollection& cellGroups)
    : mCellGroups(cellGroups)
    {}

    template<typename RecordT>
    CellRefCollection<RecordT>::~CellRefCollection ()
    {}

    template<typename RecordT>
    int CellRefCollection<RecordT>::load (ESM4::Reader& reader, bool base)
    {
        using CSMWorld::Record;

        // load the record
        RecordT record; // REFR, ACHR or ACRE
        IdCollection<RecordT>::loadRecord(record, reader);

        // first cache the record's formId to its parent cell group
        int cellIndex = mCellGroups.searchFormId(reader.currCell());
        if (cellIndex == -1)
        {
            // new cell group
            CellGroup cellGroup;

            switch (reader.grp().type)
            {
                case ESM4::Grp_CellPersistentChild:
                {
                    cellGroup.mPersistent.push_back(record.mFormId);
                    break;
                }
                case ESM4::Grp_CellVisibleDistChild:
                {
                    cellGroup.mVisibleDistant.push_back(record.mFormId);
                    break;
                }
                case ESM4::Grp_CellTemporaryChild:
                {
                    cellGroup.mTemporary.push_back(record.mFormId);
                    break;
                }
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
            std::unique_ptr<Record<CellGroup> > record2(new Record<CellGroup>(mCellGroups.getRecord(cellIndex)));
            record2->mState = CSMWorld::RecordBase::State_BaseOnly; // FIXME: State_Modified if new modindex?
            CellGroup &cellGroup = record2->get();

            // FIXME: how to deal with deleted or modified formId's?
            switch (reader.grp().type)
            {
                case ESM4::Grp_CellPersistentChild:
                {
                    cellGroup.mPersistent.push_back(record.mFormId);
                    break;
                }
                case ESM4::Grp_CellVisibleDistChild:
                {
                    cellGroup.mVisibleDistant.push_back(record.mFormId);
                    break;
                }
                case ESM4::Grp_CellTemporaryChild:
                {
                    cellGroup.mTemporary.push_back(record.mFormId);
                    break;
                }
                default:
                    throw std::runtime_error("unexpected group while loading cellref");
            }

            mCellGroups.setRecord(cellIndex, std::move(record2));
        }

        // continue with the rest of the loading
        int index = this->searchFormId(record.mFormId);

        // FIXME: how to deal with deleted records?
        if ((record.mFlags & ESM4::Rec_Deleted) != 0)
        {
            if (index == -1)
            {
                // deleting a record that does not exist
                // ignore it for now
                /// \todo report the problem to the user
                return -1;
            }

            if (base)
            {
                this->removeRows(index, 1);
                return -1;
            }

            std::unique_ptr<Record<RecordT> > baseRecord(new Record<RecordT>(this->getRecord(index)));
            baseRecord->mState = CSMWorld::RecordBase::State_Deleted;
            this->setRecord(index, std::move(baseRecord));
            return index;
        }

        return IdCollection<RecordT>::load(record, base, index);
    }
}
#endif // CSM_FOREIGN_CELLREFCOLLECTION_H
