#ifndef CSM_FOREIGN_IDCOLLECTION_H
#define CSM_FOREIGN_IDCOLLECTION_H

#include "../world/collection.hpp"

namespace ESM4
{
    class Reader;
}

namespace CSMForeign
{
    template<typename ESXRecordT, typename IdAccessorT = CSMWorld::IdAccessor<ESXRecordT> >
    class IdCollection : public CSMWorld::Collection<ESXRecordT, IdAccessorT>
    {
        virtual void loadRecord (ESXRecordT& record, ESM4::Reader& reader, bool& isDeleted);

    public:
        int load(ESM4::Reader& reader, bool base);

        int load (const ESXRecordT& record, bool base, int index = -2);
    };

    template<typename ESXRecordT, typename IdAccessorT>
    void IdCollection<ESXRecordT, IdAccessorT>::loadRecord (ESXRecordT& record,
                                                            ESM4::Reader& reader, bool& isDeleted)
    {
        record.load (reader/*, isDeleted*/);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int IdCollection<ESXRecordT, IdAccessorT>::load (ESM4::Reader& reader, bool base)
    {
        ESXRecordT record;
        bool isDeleted = false;

        loadRecord (record, reader, isDeleted);

        std::string id = ESM4::formIdToString(reader.hdr().record.id);
        int index = this->searchId (id);

        if (isDeleted)
        {
            if (index==-1)
            {
                // deleting a record that does not exist
                // ignore it for now
                /// \todo report the problem to the user
                return -1;
            }

            if (base)
            {
                this->removeRows (index, 1);
                return -1;
            }

            std::unique_ptr<CSMWorld::Record<ESXRecordT> > baseRecord(
                    new CSMWorld::Record<ESXRecordT>(this->getRecord(index)));
            baseRecord->mState = CSMWorld::RecordBase::State_Deleted;
            this->setRecord(index, std::move(baseRecord));
            return index;
        }

        return load (record, base, index);
    }

    template<typename ESXRecordT, typename IdAccessorT>
    int IdCollection<ESXRecordT, IdAccessorT>::load (const ESXRecordT& record, bool base, int index)
    {
        if (index==-2) // index unknown
            index = this->searchId (IdAccessorT().getId (record));

        if (index==-1)
        {
            // new record
            std::unique_ptr<CSMWorld::Record<ESXRecordT> > record2(new CSMWorld::Record<ESXRecordT>);
            record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly
                                   : CSMWorld::RecordBase::State_ModifiedOnly;
            (base ? record2->mBase : record2->mModified) = record;

            index = this->getSize();
            this->appendRecord(std::move(record2));
        }
        else
        {
            // old record
            std::unique_ptr<CSMWorld::Record<ESXRecordT> > record2(
                    new CSMWorld::Record<ESXRecordT>(this->getRecord(index)));

            if (base)
                record2->mBase = record;
            else
                record2->setModified(record);

            this->setRecord(index, std::move(record2));
        }

        return index;
    }
}

#endif // CSM_FOREIGN_IDCOLLECTION_H
