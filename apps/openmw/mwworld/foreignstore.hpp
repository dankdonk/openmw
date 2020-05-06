#ifndef OPENMW_MWWORLD_FOREIGNSTORE_H
#define OPENMW_MWWORLD_FOREIGNSTORE_H

#include <string>
#include <vector>
#include <map>

#include <components/loadinglistener/loadinglistener.hpp>

#include "storebase.hpp"

namespace ESM4
{
    class Reader;
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

    struct ForeignId
    {
        ESM4::FormId mId;
        bool mIsDeleted;

        ForeignId(ESM4::FormId id = 0, bool isDeleted = false);
    };

    // TODO: not so sure if we needed to inherit from StoreBase;
    //       we could probably define another base class instead
    template <class T>
    class ForeignStore : public StoreBase
    {
        std::map<ESM4::FormId, T>      mStatic;
        std::vector<T *>    mShared; // Preserves the record order as it came from the content files (this
                                     // is relevant for the spell autocalc code and selection order
                                     // for heads/hairs in the character creation)
        std::map<ESM4::FormId, T> mDynamic; // probably for saved games

        typedef std::map<ESM4::FormId, T> Dynamic;
        typedef std::map<ESM4::FormId, T> Static;

    public:
        ForeignStore();
        ForeignStore(const ForeignStore<T>& orig);

        typedef SharedIterator<T> iterator;

        // setUp needs to be called again after
        virtual void clearDynamic();
        void setUp();

        const T *search(const std::string& id) const; // search EditorId

        const T *search(ESM4::FormId id) const; // search BaseObj or DIAL

        /**
         * Does the record with this ID come from the dynamic store?
         */
        bool isDynamic(const std::string& id) const;

        // TODO: seems to be only used for TES3 werewolf related stuff
        const T *searchRandom(const std::string& id) const { return nullptr; }

        const T *find(const std::string& id) const;

        // TODO: seems to be only used for TES3 werewolf related stuff (always throws an exception)
        const T *findRandom(const std::string& id) const;

        iterator begin() const;
        iterator end() const;

        size_t getSize() const;
        int getDynamicSize() const;

        /// @note The record identifiers are listed in the order that the records were defined by the content files.
        virtual void listIdentifier(std::vector<std::string>& list) const;

        void listForeignIdentifier(std::vector<ESM4::FormId>& list) const;

        T *insert(const T& item);
        T *insertStatic(const T& item);

        virtual bool eraseStatic(const std::string& id);
        virtual bool erase(const std::string& id);
        bool erase(ESM4::FormId formId);
        virtual bool erase(const T& item);

        virtual RecordId load(ESM::ESMReader& esm);
        ForeignId loadForeign(ESM4::Reader& reader);

        virtual void write(ESM::ESMWriter& writer, Loading::Listener& progress) const;

        ///< Read into dynamic storage
        // seems to be used only by ESMStore::readRecord, called by World::readRecord
        // which is called by StateManager for loading a save file
        virtual RecordId read(ESM::ESMReader& reader);
    };

} //end namespace

#endif
