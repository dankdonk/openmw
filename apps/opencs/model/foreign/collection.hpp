#ifndef CSM_FOREIGN_COLLECTION_H
#define CSM_FOREIGN_COLLECTION_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <stdexcept>
#include <memory>

#include <QVariant>

#include <extern/esm4/formid.hpp>

#include "../world/columnbase.hpp"
#include "../world/collectionbase.hpp"

// Basically exactly the same as the one in CSMWorld but using FormId as the record id's.
// Each record keeps mId (string form of FormId) as well as FormId.  For compatibility with
// OpenCS, each record type in extern/esm4 will need a wrapper in apps/opencs/model/foreign.
namespace CSMForeign
{

    template<typename RecordT>
    class Collection : public CSMWorld::CollectionBase
    {
        std::vector<std::unique_ptr<CSMWorld::Record<RecordT> > > mRecords;
        std::map<ESM4::FormId, int> mIndex;
        std::vector<CSMWorld::Column<RecordT>*> mColumns;

        // not implemented
        Collection (const Collection&);
        Collection& operator= (const Collection&);

    protected:

        // FIXME: used by regioncollection and staticcollection, is there another way?
        const std::vector<std::unique_ptr<CSMWorld::Record<RecordT> > >& getRecords() const;

        //bool reorderRowsImp (int baseIndex, const std::vector<int>& newOrder);
        ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
        /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).
        ///
        /// \return Success?

    public:

        Collection();

        virtual ~Collection();

        virtual int getSize() const;

        virtual ESM4::FormId getFormId (int index) const;
        virtual std::string getId (int index) const;

        virtual int getIndex (const ESM4::FormId id) const;
        // assumes that id is a string representation of FormId as a hex number
        // (throws if out of range or cannot convert)
        virtual int getIndex (const std::string& id) const;

        virtual int getColumns() const;

        virtual const CSMWorld::ColumnBase& getColumn (int column) const;

        virtual QVariant getData (int index, int column) const;

        virtual void setData (int index, int column, const QVariant& data);

        virtual void merge();

        virtual void purge();

        virtual void removeRows (int index, int count);

        // assumes that id is a string representation of FormId as a hex number
        virtual void appendBlankRecord (const std::string& id,
            CSMWorld::UniversalId::Type type = CSMWorld::UniversalId::Type_None);

        virtual int searchFormId (const ESM4::FormId id) const;
        virtual int searchId (const std::string& id) const;
        ////< Search record with \a id.
        /// \return index of record (if found) or -1 (not found)

        virtual void replace (int index, std::unique_ptr<CSMWorld::RecordBase> record);
        ///< If the record type does not match, an exception is thrown.
        ///
        /// \attention \a record must not change the ID.

        virtual void appendRecord (std::unique_ptr<CSMWorld::RecordBase> record,
            CSMWorld::UniversalId::Type type = CSMWorld::UniversalId::Type_None);
        ///< If the record type does not match, an exception is thrown.
        ///< \param type Will be ignored, unless the collection supports multiple record types

        // assumes that id is a string representation of FormId as a hex number
        virtual void cloneRecord(const std::string& origin,
                                 const std::string& destination,
                                 const CSMWorld::UniversalId::Type type);

        virtual const CSMWorld::Record<RecordT>& getForeignRecord (const ESM4::FormId id) const;
        virtual const CSMWorld::Record<RecordT>& getRecord (const std::string& id) const;

        CSMWorld::Record<RecordT>& getModifiableRecord (int index); // FIXME
        virtual const CSMWorld::Record<RecordT>& getRecord (int index) const;

        virtual int getAppendIndex (const std::string& id,
            CSMWorld::UniversalId::Type type = CSMWorld::UniversalId::Type_None) const;
        ///< \param type Will be ignored, unless the collection supports multiple record types

        virtual std::vector<std::string> getIds (bool listDeleted = true) const;
        ///< Return a sorted collection of all IDs
        ///
        /// \param listDeleted include deleted record in the list

        virtual bool reorderRows (int baseIndex, const std::vector<int>& newOrder);
        ///< Reorder the rows [baseIndex, baseIndex+newOrder.size()) according to the indices
        /// given in \a newOrder (baseIndex+newOrder[0] specifies the new index of row baseIndex).
        ///
        /// \return Success?

        virtual void insertRecord (std::unique_ptr<CSMWorld::RecordBase> record, int index,
            CSMWorld::UniversalId::Type type = CSMWorld::UniversalId::Type_None);
        ///< Insert record before index.
        ///
        /// If the record type does not match, an exception is thrown.
        ///
        /// If the index is invalid either generally (by being out of range) or for the particular
        /// record, an exception is thrown.

        void addColumn (CSMWorld::Column<RecordT> *column);

        void setRecord (int index, std::unique_ptr<CSMWorld::Record<RecordT> > record);
        ///< \attention This function must not change the ID.
    };

    template<typename RecordT>
    Collection<RecordT>::Collection()
    {}

    template<typename RecordT>
    Collection<RecordT>::~Collection()
    {
        using CSMWorld::Column;

        for (typename std::vector<Column<RecordT>*>::iterator iter(mColumns.begin()); iter != mColumns.end(); ++iter)
            delete *iter;
    }

    template<typename RecordT>
    const std::vector<std::unique_ptr<CSMWorld::Record<RecordT> > >& Collection<RecordT>::getRecords() const
    {
        return mRecords;
    }

    template<typename RecordT>
    int Collection<RecordT>::getSize() const
    {
        return mRecords.size();
    }

    template<typename RecordT>
    std::string Collection<RecordT>::getId (int index) const
    {
        return mRecords.at(index)->get().mId;
    }

    template<typename RecordT>
    ESM4::FormId Collection<RecordT>::getFormId (int index) const
    {
        return mRecords.at(index)->get().mFormId;
    }

    template<typename RecordT>
    int  Collection<RecordT>::getIndex (const ESM4::FormId id) const
    {
        int index = searchFormId(id);

        if (index == -1)
            throw std::runtime_error("CSMForeign::Collection::getIndex invalid FormId: "
                                    + ESM4::formIdToString(id));
        return index;
    }

    template<typename RecordT>
    int  Collection<RecordT>::getIndex (const std::string& id) const
    {
        return getIndex(ESM4::stringToFormId(id));
    }

    template<typename RecordT>
    int Collection<RecordT>::getColumns() const
    {
        return mColumns.size();
    }

    template<typename RecordT>
    const CSMWorld::ColumnBase& Collection<RecordT>::getColumn (int column) const
    {
        return *mColumns.at(column);
    }

    template<typename RecordT>
    QVariant Collection<RecordT>::getData (int index, int column) const
    {
        return mColumns.at(column)->get(*mRecords.at(index));
    }

    template<typename RecordT>
    void Collection<RecordT>::setData (int index, int column, const QVariant& data)
    {
        return mColumns.at(column)->set(*mRecords.at(index), data);
    }

    template<typename RecordT>
    void Collection<RecordT>::merge()
    {
        using CSMWorld::Record;

        for (typename std::vector<std::unique_ptr<Record<RecordT> > >::iterator iter(mRecords.begin()); iter != mRecords.end(); ++iter)
            (*iter)->merge();

        purge();
    }

    template<typename RecordT>
    void  Collection<RecordT>::purge()
    {
        int i = 0;

        while (i<static_cast<int>(mRecords.size()))
        {
            if (mRecords[i]->isErased())
                removeRows(i, 1);
            else
                ++i;
        }
    }

    template<typename RecordT>
    void Collection<RecordT>::removeRows (int index, int count)
    {
        mRecords.erase(mRecords.begin()+index, mRecords.begin()+index+count);

        typename std::map<ESM4::FormId, int>::iterator iter = mIndex.begin();

        while (iter != mIndex.end())
        {
            if (iter->second >= index)
            {
                if (iter->second >= index+count)
                {
                    iter->second -= count;
                    ++iter;
                }
                else
                    mIndex.erase(iter++);
            }
            else
                ++iter;
        }
    }

    template<typename RecordT>
    void  Collection<RecordT>::appendBlankRecord (const std::string& id, CSMWorld::UniversalId::Type type)
    {
        RecordT record;
        record.mId = id;
        // FIXME: add here?  How to ensure that the FormId is unique?
        record.mFormId = ESM4::stringToFormId(id);
        record.blank();

        std::unique_ptr<CSMWorld::Record<RecordT> > record2(new CSMWorld::Record<RecordT>);
        record2->mState = CSMWorld::Record<RecordT>::State_ModifiedOnly;
        record2->mModified = record;

        insertRecord(std::move(record2), getAppendIndex(id, type), type);
    }

    template<typename RecordT>
    int Collection<RecordT>::searchFormId (const ESM4::FormId id) const
    {
        std::map<ESM4::FormId, int>::const_iterator iter = mIndex.find(id);

        if (iter == mIndex.end())
            return -1;

        return iter->second;
    }

    template<typename RecordT>
    int Collection<RecordT>::searchId (const std::string& id) const
    {
        ESM4::FormId formId;

        if (!ESM4::isFormId(id, &formId))
            return -1;

        return searchFormId(formId);
    }

    template<typename RecordT>
    void Collection<RecordT>::replace (int index, std::unique_ptr<CSMWorld::RecordBase> record)
    {
        using CSMWorld::Record;

        std::unique_ptr<Record<RecordT> > tmp(static_cast<Record<RecordT>*>(record.release()));
        mRecords.at(index) = std::move(tmp);
    }

    template<typename RecordT>
    void Collection<RecordT>::appendRecord (std::unique_ptr<CSMWorld::RecordBase> record,
        CSMWorld::UniversalId::Type type)
    {
        using CSMWorld::Record;

        int index = getAppendIndex(static_cast<Record<RecordT>*>(record.get())->get().mId, type);

        insertRecord(std::move(record), index, type);
    }

    template<typename RecordT>
    void Collection<RecordT>::cloneRecord(const std::string& origin,
                                          const std::string& destination,
                                          const CSMWorld::UniversalId::Type type)
    {
        using CSMWorld::Record;

        std::unique_ptr<Record<RecordT> > copy(new Record<RecordT>);
        copy->mModified = getRecord(origin).get();
        copy->mState = CSMWorld::RecordBase::State_ModifiedOnly;
        copy->get().mId = destination;
        // FIXME: add here?  But FormId needs to be unique?
        //copy->get().mFormId = ESM4::stringToFormId(destination);

        insertRecord(std::move(copy), getAppendIndex(destination, type));
    }

    template<typename RecordT>
    int Collection<RecordT>::getAppendIndex (const std::string& id, CSMWorld::UniversalId::Type type) const
    {
        return static_cast<int>(mRecords.size());
    }
#if 0
    template<typename RecordT>
    bool Collection<RecordT>::reorderRowsImp (int baseIndex, const std::vector<int>& newOrder)
    {
        if (!newOrder.empty())
        {
            int size = static_cast<int>(newOrder.size());

            // check that all indices are present
            std::vector<int> test(newOrder);
            std::sort(test.begin(), test.end());
            if (*test.begin() != 0 || *--test.end() != size-1)
                return false;

            // reorder records
            std::vector<std::unique_ptr<CSMWorld::Record<RecordT> > > buffer(size);

            for (int i = 0; i < size; ++i)
            {
                buffer[newOrder[i]] = std::move(mRecords[baseIndex+i]);
                buffer[newOrder[i]]->setModified(buffer[newOrder[i]]->get());
            }

            std::move(buffer.begin(), buffer.end(), mRecords.begin()+baseIndex);

            // adjust index
            for (std::map<ESM4::FormId, int>::iterator iter(mIndex.begin()); iter != mIndex.end(); ++iter)
                if (iter->second >= baseIndex && iter->second < baseIndex+size)
                    iter->second = newOrder.at(iter->second-baseIndex)+baseIndex;
        }

        return true;
    }
#endif
    template<typename RecordT>
    void Collection<RecordT>::addColumn (CSMWorld::Column<RecordT> *column)
    {
        mColumns.push_back(column);
    }

    template<typename RecordT>
    const CSMWorld::Record<RecordT>& Collection<RecordT>::getForeignRecord (const ESM4::FormId id) const
    {
        return *mRecords.at(getIndex(id));
    }

    template<typename RecordT>
    const CSMWorld::Record<RecordT>& Collection<RecordT>::getRecord (const std::string& id) const
    {
        return *mRecords.at(getIndex(id));
    }

    template<typename RecordT>
    CSMWorld::Record<RecordT>& Collection<RecordT>::getModifiableRecord (int index)
    {
        return *mRecords.at (index);
    }

    template<typename RecordT>
    const CSMWorld::Record<RecordT>& Collection<RecordT>::getRecord (int index) const
    {
        return *mRecords.at(index);
    }

    template<typename RecordT>
    std::vector<std::string> Collection<RecordT>::getIds (bool listDeleted) const
    {
        std::vector<std::string> ids;

        for (typename std::map<ESM4::FormId, int>::const_iterator iter = mIndex.begin();
            iter != mIndex.end(); ++iter)
        {
            if (listDeleted || !mRecords[iter->second]->isDeleted())
                ids.push_back(mRecords[iter->second]->get().mId);
        }

        return ids;
    }

    template<typename RecordT>
    void Collection<RecordT>::insertRecord (std::unique_ptr<CSMWorld::RecordBase> record, int index,
        CSMWorld::UniversalId::Type type)
    {
        using CSMWorld::Record;

        int size = static_cast<int>(mRecords.size());
        if (index < 0 || index > size)
            throw std::runtime_error("CSMForeign::Collection::insertRecord index out of range");

        std::unique_ptr<Record<RecordT> > record2(static_cast<Record<RecordT>*>(record.release()));
        ESM4::FormId formId = record2->get().mFormId;

        if (index == size)
            mRecords.push_back(std::move(record2));
        else
            mRecords.insert(mRecords.begin()+index, std::move(record2));

        if (index < size-1)
        {
            for (std::map<ESM4::FormId, int>::iterator iter(mIndex.begin()); iter != mIndex.end(); ++iter)
            {
                if (iter->second >= index)
                    ++(iter->second);
            }
        }

        mIndex.insert(std::make_pair(formId, index));
    }

    template<typename RecordT>
    void Collection<RecordT>::setRecord (int index, std::unique_ptr<CSMWorld::Record<RecordT> > record)
    {
        if (mRecords.at(index)->get().mFormId != record->get().mFormId)
            throw std::runtime_error("CSMForeign::Collection::setRecord attempt to change the FormId of a record");

        mRecords.at(index) = std::move(record);
    }

    template<typename RecordT>
    bool Collection<RecordT>::reorderRows (int baseIndex, const std::vector<int>& newOrder)
    {
        return false;
    }
}

#endif // CSM_FOREIGN_COLLECTION_H
