#ifndef OPENMW_MWWORLD_STOREBASE_H
#define OPENMW_MWWORLD_STOREBASE_H

#include <string>
#include <vector>

namespace ESM4
{
    typedef uint32_t FormId;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    struct RecordId
    {
        std::string mId;
        bool mIsDeleted;

        RecordId(const std::string &id = "", bool isDeleted = false);
    };

    template <class T>
    class SharedIterator
    {
        typedef typename std::vector<T *>::const_iterator Iter;

        Iter mIter;

    public:
        SharedIterator() {}

        SharedIterator(const SharedIterator &orig)
          : mIter(orig.mIter)
        {}

        SharedIterator(const Iter &iter)
          : mIter(iter)
        {}

        SharedIterator &operator++() {
            ++mIter;
            return *this;
        }

        SharedIterator operator++(int) {
            SharedIterator iter = *this;
            ++mIter;

            return iter;
        }

        SharedIterator &operator--() {
            --mIter;
            return *this;
        }

        SharedIterator operator--(int) {
            SharedIterator iter = *this;
            --mIter;

            return iter;
        }

        bool operator==(const SharedIterator &x) const {
            return mIter == x.mIter;
        }

        bool operator!=(const SharedIterator &x) const {
            return !(*this == x);
        }

        const T &operator*() const {
            return **mIter;
        }

        const T *operator->() const {
            return &(**mIter);
        }
    };

    class StoreBase
    {
    public:
        virtual ~StoreBase() {}

        virtual void setUp() {}

        /// List identifiers of records contained in this Store (case-smashed). No-op for Stores that don't use string IDs.
        virtual void listIdentifier(std::vector<std::string> &list) const {}
        virtual void listForeignIdentifier(std::vector<ESM4::FormId> &list) const {}

        virtual size_t getSize() const = 0;
        virtual int getDynamicSize() const { return 0; }
        virtual RecordId load(ESM::ESMReader &esm) = 0;

        virtual bool eraseStatic(const std::string &id) {return false;}
        virtual void clearDynamic() {}

        virtual void write (ESM::ESMWriter& writer, Loading::Listener& progress) const {}

        virtual RecordId read (ESM::ESMReader& reader) { return RecordId(); }
        ///< Read into dynamic storage

        virtual std::string getLastAddedRecordId() const { return ""; }
        ///< Returns the last loaded/read ID or empty string if a loaded record has no ID
        virtual bool isLastAddedRecordDeleted() const { return false; }
    };

} //end namespace

#endif
