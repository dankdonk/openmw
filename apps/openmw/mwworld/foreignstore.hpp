#ifndef OPENMW_MWWORLD_FOREIGNSTORE_H
#define OPENMW_MWWORLD_FOREIGNSTORE_H

#include <string>
#include <vector>
#include <map>

#include <openengine/misc/rng.hpp>

//#include <components/esm/esmwriter.hpp>
//#include <components/esm/util.hpp>

#include <components/loadinglistener/loadinglistener.hpp>

//#include "recordcmp.hpp"
#include "storebase.hpp"

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
    class ESMStore;

    template <class T>
    class ForeignStore : public StoreBase
    {
        std::map<std::string, T>      mStatic;
        std::vector<T *>    mShared; // Preserves the record order as it came from the content files (this
                                     // is relevant for the spell autocalc code and selection order
                                     // for heads/hairs in the character creation)
        std::map<std::string, T> mDynamic;

        typedef std::map<std::string, T> Dynamic;
        typedef std::map<std::string, T> Static;

        friend class ESMStore;

    public:
        ForeignStore();
        ForeignStore(const ForeignStore<T> &orig);

        typedef SharedIterator<T> iterator;

        // setUp needs to be called again after
        virtual void clearDynamic();
        void setUp();

        const T *search(const std::string &id) const; // search EditorId

        const T *search(ESM4::FormId id) const; // search BaseObj

        /**
         * Does the record with this ID come from the dynamic store?
         */
        bool isDynamic(const std::string &id) const;

        /** Returns a random record that starts with the named ID, or NULL if not found. */
        const T *searchRandom(const std::string &id) const;

        const T *find(const std::string &id) const;

        /** Returns a random record that starts with the named ID. An exception is thrown if none
         * are found. */
        const T *findRandom(const std::string &id) const;

        iterator begin() const;
        iterator end() const;

        size_t getSize() const;
        int getDynamicSize() const;

        /// @note The record identifiers are listed in the order that the records were defined by the content files.
        void listIdentifier(std::vector<std::string> &list) const;
        void listForeignIdentifier(std::vector<ESM4::FormId> &list) const;

        T *insert(const T &item);
        T *insertStatic(const T &item);

        bool eraseStatic(const std::string &id);
        bool erase(const std::string &id);
        bool erase(const T &item);

        RecordId load(ESM::ESMReader &esm);
        void write(ESM::ESMWriter& writer, Loading::Listener& progress) const;
        RecordId read(ESM::ESMReader& reader);
    };

} //end namespace

#endif
