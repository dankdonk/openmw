#ifndef OPENMW_MWWORLD_STORE_H
#define OPENMW_MWWORLD_STORE_H

#include <string>
#include <vector>
#include <map>

#include <components/misc/rng.hpp>

#include <components/esm/esmwriter.hpp>
#include <components/esm/util.hpp>

#include "foreignworld.hpp"
#include "foreigncell.hpp"
#include "foreignland.hpp"

#include "recordcmp.hpp"
#include "storebase.hpp"

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    template <class T>
    class IndexedStore
    {
    protected:
        typedef typename std::map<int, T> Static;
        Static mStatic;

    public:
        typedef typename std::map<int, T>::const_iterator iterator;

        IndexedStore();

        iterator begin() const;
        iterator end() const;

        void load(ESM::ESMReader &esm);

        int getSize() const;
        void setUp();

        const T *search(int index) const;
        const T *find(int index) const;
    };

    class ESMStore;

    template <class T>
    class Store : public StoreBase
    {
        std::map<std::string, T>      mStatic;
        std::vector<T *>    mShared; // Preserves the record order as it came from the content files (this
                                     // is relevant for the spell autocalc code and selection order
                                     // for heads/hairs in the character creation)
        std::map<std::string, T> mDynamic;

        typedef std::map<std::string, T> Dynamic;
        typedef std::map<std::string, T> Static;

        class GetRecords {
            const std::string mFind;
            std::vector<const T*> *mRecords;

        public:
            GetRecords(const std::string &str, std::vector<const T*> *records)
              : mFind(Misc::StringUtils::lowerCase(str)), mRecords(records)
            { }

            void operator()(const T *item)
            {
                if(Misc::StringUtils::ciCompareLen(mFind, item->mId, mFind.size()) == 0)
                    mRecords->push_back(item);
            }
        };

        T mLastAddedRecord;

        friend class ESMStore;

    public:
        Store();
        Store(const Store<T> &orig);

        typedef SharedIterator<T> iterator;

        // setUp needs to be called again after
        virtual void clearDynamic();
        void setUp();

        const T *search(const std::string &id) const;

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

        T *insert(const T &item);
        T *insertStatic(const T &item);

        bool eraseStatic(const std::string &id);
        bool erase(const std::string &id);
        bool erase(const T &item);

        RecordId load(ESM::ESMReader &esm);
        void write(ESM::ESMWriter& writer, Loading::Listener& progress) const;
        RecordId read(ESM::ESMReader& reader);
#if 0
        std::string getLastAddedRecordId() const
        {
            return ESM::getRecordId(mLastAddedRecord);
        }

        bool isLastAddedRecordDeleted() const
        {
            return ESM::isRecordDeleted(mLastAddedRecord);
        }
#endif
    };

#if 0
    template <>
    inline void Store<ESM::Dialogue>::load(ESM::ESMReader &esm) {
        // The original letter case of a dialogue ID is saved, because it's printed
        ESM::Dialogue dialogue;
        bool isDeleted;
        dialogue.load(esm, isDeleted);

        std::string idLower = Misc::StringUtils::lowerCase(dialogue.mId);
        std::map<std::string, ESM::Dialogue>::iterator found = mStatic.find(idLower);
        if (found == mStatic.end())
        {
            mStatic.insert(std::make_pair(idLower, dialogue));
        }
        else
        {
            //found->second.mIsDeleted = mIsDeleted;
            found->second.mType = dialogue.mType;
        }

        mLastAddedRecord = dialogue;
    }

    template <>
    inline void Store<ESM::Script>::load(ESM::ESMReader &esm) {
        ESM::Script script;
        bool isDeleted;
        script.load(esm, isDeleted);
        Misc::StringUtils::toLower(script.mId);

        std::pair<typename Static::iterator, bool> inserted = mStatic.insert(std::make_pair(script.mId, script));
        if (inserted.second)
            mShared.push_back(&inserted.first->second);
        else
            inserted.first->second = script;

        mLastAddedRecord = script;
    }

    template <>
    inline void Store<ESM::StartScript>::load(ESM::ESMReader &esm)
    {
        ESM::StartScript script;
        bool isDeleted;
        script.load(esm, isDeleted);
        Misc::StringUtils::toLower(script.mId);

        std::pair<typename Static::iterator, bool> inserted = mStatic.insert(std::make_pair(script.mId, script));
        if (inserted.second)
            mShared.push_back(&inserted.first->second);
        else
            inserted.first->second = script;

        mLastAddedRecord = script;
    }
#endif

    template <>
    class Store<ESM::LandTexture> : public StoreBase
    {
        // For multiple ESM/ESP files we need one list per file.
        typedef std::vector<ESM::LandTexture> LandTextureList;
        std::vector<LandTextureList> mStatic;
        //ESM::LandTexture mLastLoadedTexture;

    public:
        Store();

        typedef std::vector<ESM::LandTexture>::const_iterator iterator;

        // Must be threadsafe! Called from terrain background loading threads.
        // Not a big deal here, since ESM::LandTexture can never be modified or inserted/erased
        const ESM::LandTexture *search(size_t index, size_t plugin) const;
        const ESM::LandTexture *find(size_t index, size_t plugin) const;

        /// Resize the internal store to hold at least \a num plugins.
        void resize(size_t num);

        size_t getSize() const;
        size_t getSize(size_t plugin) const;

        RecordId load(ESM::ESMReader &esm, size_t plugin);
        RecordId load(ESM::ESMReader &esm);

        iterator begin(size_t plugin) const;
        iterator end(size_t plugin) const;
#if 0
        std::string getLastAddedRecordId() const
        {
            return "";// ESM::getRecordId(mLastLoadedTexture);
        }

        bool isLastAddedRecordDeleted() const
        {
            return 0;// ESM::isRecordDeleted(mLastLoadedTexture);
        }
#endif
    };

    template <>
    class Store<ESM::Land> : public StoreBase
    {
        std::vector<ESM::Land *> mStatic;

    public:
        typedef SharedIterator<ESM::Land> iterator;

        virtual ~Store();

        size_t getSize() const;
        iterator begin() const;
        iterator end() const;

        // Must be threadsafe! Called from terrain background loading threads.
        // Not a big deal here, since ESM::Land can never be modified or inserted/erased
        ESM::Land *search(int x, int y) const;
        ESM::Land *find(int x, int y) const;

        RecordId load(ESM::ESMReader &esm);
        void setUp();
    };

    template <>
    class Store<ESM::Cell> : public StoreBase
    {
        struct DynamicExtCmp
        {
            bool operator()(const std::pair<int, int> &left, const std::pair<int, int> &right) const {
                if (left.first == right.first && left.second == right.second)
                    return false;

                if (left.first == right.first)
                    return left.second > right.second;

                // Exterior cells are listed in descending, row-major order,
                // this is a workaround for an ambiguous chargen_plank reference in the vanilla game.
                // there is one at -22,16 and one at -2,-9, the latter should be used.
                return left.first > right.first;
            }
        };

        typedef std::map<std::string, ESM::Cell>                           DynamicInt;
        typedef std::map<std::pair<int, int>, ESM::Cell, DynamicExtCmp>    DynamicExt;

        DynamicInt      mInt;
        DynamicExt      mExt;

        std::vector<ESM::Cell *>    mSharedInt;
        std::vector<ESM::Cell *>    mSharedExt;

        DynamicInt mDynamicInt;
        DynamicExt mDynamicExt;

        const ESM::Cell *search(const ESM::Cell &cell) const;
        void handleMovedCellRefs(ESM::ESMReader& esm, ESM::Cell* cell);

    public:
        typedef SharedIterator<ESM::Cell> iterator;

        const ESM::Cell *search(const std::string &id) const;
        const ESM::Cell *search(int x, int y) const;
        const ESM::Cell *searchOrCreate(int x, int y);

        const ESM::Cell *find(const std::string &id) const;
        const ESM::Cell *find(int x, int y) const;

        void setUp();

        RecordId load(ESM::ESMReader &esm);

        iterator intBegin() const;
        iterator intEnd() const;
        iterator extBegin() const;
        iterator extEnd() const;

        // Return the northernmost cell in the easternmost column.
        const ESM::Cell *searchExtByName(const std::string &id) const;

        // Return the northernmost cell in the easternmost column.
        const ESM::Cell *searchExtByRegion(const std::string &id) const;

        size_t getSize() const;

        void listIdentifier(std::vector<std::string> &list) const;

        ESM::Cell *insert(const ESM::Cell &cell);

        bool erase(const ESM::Cell &cell);
        bool erase(const std::string &id);

        bool erase(int x, int y);
    };

    template <>
    class Store<ESM::Pathgrid> : public StoreBase
    {
    private:
        typedef std::map<std::string, ESM::Pathgrid> Interior;
        typedef std::map<std::pair<int, int>, ESM::Pathgrid> Exterior;

        Interior mInt;
        Exterior mExt;

        Store<ESM::Cell>* mCells;

    public:

        Store();

        void setCells(Store<ESM::Cell>& cells);
        RecordId load(ESM::ESMReader &esm);
        size_t getSize() const;

        void setUp();

        const ESM::Pathgrid *search(int x, int y) const;
        const ESM::Pathgrid *search(const std::string& name) const;
        const ESM::Pathgrid *find(int x, int y) const;
        const ESM::Pathgrid* find(const std::string& name) const;
        const ESM::Pathgrid *search(const ESM::Cell &cell) const;
        const ESM::Pathgrid *find(const ESM::Cell &cell) const;
    };


    template <>
    class Store<ESM::Skill> : public IndexedStore<ESM::Skill>
    {
    public:
        Store();
    };

    template <>
    class Store<ESM::MagicEffect> : public IndexedStore<ESM::MagicEffect>
    {
    public:
        Store();
    };

    template <>
    class Store<ESM::Attribute> : public IndexedStore<ESM::Attribute>
    {
        std::vector<ESM::Attribute> mStatic;

    public:
        typedef std::vector<ESM::Attribute>::const_iterator iterator;

        Store();

        const ESM::Attribute *search(size_t index) const;
        const ESM::Attribute *find(size_t index) const;

        void setUp();

        size_t getSize() const;
        iterator begin() const;
        iterator end() const;
    };

#if 0
    template<>
    inline void Store<ESM::Dialogue>::setUp()
    {
        // DialInfos marked as deleted are kept during the loading phase, so that the linked list
        // structure is kept intact for inserting further INFOs. Delete them now that loading is done.
        for (Static::iterator it = mStatic.begin(); it != mStatic.end(); ++it)
        {
            ESM::Dialogue& dial = it->second;
            dial.clearDeletedInfos();
        }

        mShared.clear();
        mShared.reserve(mStatic.size());
        std::map<std::string, ESM::Dialogue>::iterator it = mStatic.begin();
        for (; it != mStatic.end(); ++it) {
            mShared.push_back(&(it->second));
        }
    }
#endif

    // FIXME: only one of either TES4 or TES5 allowed (else FormId's may clash)
    //
    // One option might be to use a 64bit version of formid to identify which game,
    // which can also allow more than 255 mods (will need to use some hack such as detecting
    // the base game from the header dependency/master lists)

    template <>
    class Store<ForeignWorld> : public StoreBase
    {
    private:

        std::map<ESM4::FormId, ForeignWorld*> mWorlds;

    public:
        //typedef SharedIterator<ForeignWorld> iterator; // FIXME: is this needed?

        virtual ~Store();

        // Would like to make it const, but Store<ForeignCell> needs to update
        ForeignWorld *getWorld(ESM4::FormId worldId);

        const ForeignWorld *find(ESM4::FormId worldId) const;

        // Assumes editorId to be lower case.
        const ForeignWorld *find(const std::string& editorId) const; // FIXME: deprecated

        // Returns 0 if not found. Does not assume editorId to be lower case.
        ESM4::FormId getFormId(const std::string& editorId) const;

        size_t getSize() const;
        //iterator begin() const; // FIXME: is this needed?
        //iterator end() const; // FIXME: is this needed?

        // FIXME: need to overload eraseStatic()?

        RecordId load(ESM::ESMReader& esm);
        void setUp(); // FIXME: is this needed?
    };

    template <>
    class Store<MWWorld::ForeignCell> : public StoreBase
    {
    private:

        std::map<ESM4::FormId, ForeignCell*> mCells;

        std::map<std::string, ESM4::FormId> mEditorIdMap;

        ESM4::FormId mLastPreloadedCell;       // FIXME for testing only

    public:

        virtual ~Store();

        // probably need some search functions here
        // also utilities e.g. get formId based on EditorId/FullName

        // Used by World::findForeignWorldPosition for teleporting the player, e.g. from
        // console command COC (center on cell)
        const MWWorld::ForeignCell *searchExtByName(const std::string &name) const;

        size_t getSize() const;

        void preload(ESM::ESMReader& esm, Store<MWWorld::ForeignWorld>& worlds);

        RecordId load(ESM::ESMReader& esm) { return RecordId("", false); } // noop
        RecordId load(ESM::ESMReader& esm, Store<ForeignWorld>& worlds);
        void setUp(); // FIXME: is this needed?

        const ESM::Cell *find(int x, int y) const; // FIXME: returns wrong cell type
        const ForeignCell *find(ESM4::FormId formId) const;
        const ForeignCell *find(const std::string& name) const;

        void testPreload(ESM::ESMReader& esm); // FIXME for testing only
    };

    template <>
    class Store<MWWorld::ForeignLand> : public StoreBase
    {
    private:

        std::map<ESM4::FormId, ForeignLand*> mLands;

    public:

        virtual ~Store();

        size_t getSize() const;

        RecordId load(ESM::ESMReader& esm);
        void setUp(); // FIXME: is this needed?

        const ForeignLand *find(ESM4::FormId formId) const;

        ForeignLand *search(ESM4::FormId worldId, int x, int y) const;
        ForeignLand *find(ESM4::FormId worldId, int x, int y) const;
    };

} //end namespace

#endif
