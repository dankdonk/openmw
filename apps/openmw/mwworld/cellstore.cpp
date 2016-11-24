#include "cellstore.hpp"

#include <iostream>
#include <algorithm>

#include <extern/esm4/refr.hpp>
#include <extern/esm4/formid.hpp>

#include <components/esm/cellstate.hpp>
#include <components/esm/cellid.hpp>
#include <components/esm/esmwriter.hpp>
#include <components/esm/objectstate.hpp>
#include <components/esm/containerstate.hpp>
#include <components/esm/npcstate.hpp>
#include <components/esm/creaturestate.hpp>
#include <components/esm/fogstate.hpp>
#include <components/esm/creaturelevliststate.hpp>
#include <components/esm/doorstate.hpp>
#include <components/esm/esm4reader.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "ptr.hpp"
#include "esmstore.hpp"
#include "class.hpp"
#include "containerstore.hpp"

namespace
{
    template<typename T>
    MWWorld::Ptr searchInContainerList (MWWorld::CellRefList<T>& containerList, const std::string& id)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (containerList.mList.begin());
             iter!=containerList.mList.end(); ++iter)
        {
            MWWorld::Ptr container (&*iter, 0);

            MWWorld::Ptr ptr =
                container.getClass().getContainerStore (container).search (id);

            if (!ptr.isEmpty())
                return ptr;
        }

        return MWWorld::Ptr();
    }

    template<typename T>
    MWWorld::Ptr searchViaActorId (MWWorld::CellRefList<T>& actorList, int actorId,
        MWWorld::CellStore *cell)
    {
        for (typename MWWorld::CellRefList<T>::List::iterator iter (actorList.mList.begin());
             iter!=actorList.mList.end(); ++iter)
        {
            MWWorld::Ptr actor (&*iter, cell);

            if (actor.getClass().getCreatureStats (actor).matchesActorId (actorId) && actor.getRefData().getCount() > 0)
                return actor;
        }

        return MWWorld::Ptr();
    }

    template<typename RecordType, typename T>
    void writeReferenceCollection (ESM::ESMWriter& writer,
        const MWWorld::CellRefList<T>& collection)
    {
        if (!collection.mList.empty())
        {
            // references
            for (typename MWWorld::CellRefList<T>::List::const_iterator
                iter (collection.mList.begin());
                iter!=collection.mList.end(); ++iter)
            {
                if (!iter->mData.hasChanged() && !iter->mRef.hasChanged() && iter->mRef.hasContentFile())
                {
                    // Reference that came from a content file and has not been changed -> ignore
                    continue;
                }
                if (iter->mData.getCount()==0 && !iter->mRef.hasContentFile())
                {
                    // Deleted reference that did not come from a content file -> ignore
                    continue;
                }

                RecordType state;
                iter->save (state);

                // recordId currently unused
                writer.writeHNT ("OBJE", collection.mList.front().mBase->sRecordId);

                state.save (writer);
            }
        }
    }

    template<typename RecordType, typename T>
    void readReferenceCollection (ESM::ESMReader& reader,
        MWWorld::CellRefList<T>& collection, const ESM::CellRef& cref, const std::map<int, int>& contentFileMap)
    {
        const MWWorld::ESMStore& esmStore = MWBase::Environment::get().getWorld()->getStore();

        RecordType state;
        state.mRef = cref;
        state.load(reader);

        // If the reference came from a content file, make sure this content file is loaded
        if (state.mRef.mRefNum.hasContentFile())
        {
            std::map<int, int>::const_iterator iter =
                contentFileMap.find (state.mRef.mRefNum.mContentFile);

            if (iter==contentFileMap.end())
                return; // content file has been removed -> skip

            state.mRef.mRefNum.mContentFile = iter->second;
        }

        if (!MWWorld::LiveCellRef<T>::checkState (state))
            return; // not valid anymore with current content files -> skip

        const T *record = esmStore.get<T>().search (state.mRef.mRefID);

        if (!record)
            return;

        if (state.mRef.mRefNum.hasContentFile())
        {
            for (typename MWWorld::CellRefList<T>::List::iterator iter (collection.mList.begin());
                iter!=collection.mList.end(); ++iter)
                if (iter->mRef.getRefNum()==state.mRef.mRefNum)
                {
                    // overwrite existing reference
                    iter->load (state);
                    return;
                }
        }

        // new reference
        MWWorld::LiveCellRef<T> ref (record);
        ref.load (state);
        collection.mList.push_back (ref);
    }
}

namespace MWWorld
{
    template <typename X>
    void CellRefList<X>::load(ESM::CellRef &ref, bool deleted, const MWWorld::ESMStore &esmStore)
    {
        const MWWorld::Store<X> &store = esmStore.get<X>();

        if (const X *ptr = store.search (ref.mRefID))
        {
            typename std::list<LiveRef>::iterator iter =
                std::find(mList.begin(), mList.end(), ref.mRefNum);

            LiveRef liveCellRef (ref, ptr);

            if (deleted)
                liveCellRef.mData.setDeleted(true);

            if (iter != mList.end())
                *iter = liveCellRef;
            else
                mList.push_back (liveCellRef);
        }
        else
        {
            std::cerr
                << "Error: could not resolve cell reference " << ref.mRefID
                << " (dropping reference)" << std::endl;
        }
    }

    template <typename X>
    void CellRefList<X>::load(ESM4::Reference &ref, bool deleted, const MWWorld::ESMStore &esmStore)
    {
        const MWWorld::ForeignStore<X> &store = esmStore.getForeign<X>();

        if (const X *ptr = store.search (ref.mBaseObj))
        {
            // see operator== in livecellref.hpp, check for matching formId
            typename std::list<LiveRef>::iterator iter =
                std::find(mList.begin(), mList.end(), ref);

            LiveRef liveCellRef (ref, ptr); // ref is the reference, ptr is the base object

            if (deleted)
                liveCellRef.mData.setDeleted(true);

            if (iter != mList.end())
                *iter = liveCellRef;
            else
            {
                mList.push_back (liveCellRef);
                //std::cout << "CellRefList::load "
                    //<< ESM4::formIdToString(mList.back().mBase->mFormId) << std::endl; // FIXME: debug only
            }
        }
        else
        {
            std::cerr
                << "Error: could not resolve foreign cell reference " << ref.mEditorId
                << " (dropping reference)" << std::endl;
        }
    }

    template<typename X> bool operator==(const LiveCellRef<X>& ref, int pRefnum)
    {
        return (ref.mRef.mRefnum == pRefnum);
    }

    CellStore::CellStore (const ESM::Cell *cell, bool isForeignCell)
      : mCell (cell), mState (State_Unloaded), mHasState (false), mLastRespawn(0,0), mIsForeignCell(isForeignCell)
    {
        mWaterLevel = cell->mWater;
    }

    const ESM::Cell *CellStore::getCell() const
    {
        return mCell;
    }

    CellStore::State CellStore::getState() const
    {
        return mState;
    }

    bool CellStore::hasState() const
    {
        return mHasState;
    }

    bool CellStore::hasId (const std::string& id) const
    {
        if (mState==State_Unloaded)
            return false;

        if (mState==State_Preloaded)
            return std::binary_search (mIds.begin(), mIds.end(), id);

        /// \todo address const-issues
        return const_cast<CellStore *> (this)->search (id).isEmpty();
    }

    Ptr CellStore::search (const std::string& id)
    {
        bool oldState = mHasState;

        mHasState = true;

        if (LiveCellRef<ESM::Activator> *ref = mActivators.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Potion> *ref = mPotions.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Apparatus> *ref = mAppas.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Armor> *ref = mArmors.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Book> *ref = mBooks.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Clothing> *ref = mClothes.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Container> *ref = mContainers.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Creature> *ref = mCreatures.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Door> *ref = mDoors.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Ingredient> *ref = mIngreds.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::CreatureLevList> *ref = mCreatureLists.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::ItemLevList> *ref = mItemLists.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Light> *ref = mLights.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Lockpick> *ref = mLockpicks.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Miscellaneous> *ref = mMiscItems.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::NPC> *ref = mNpcs.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Probe> *ref = mProbes.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Repair> *ref = mRepairs.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Static> *ref = mStatics.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Weapon> *ref = mWeapons.find (id))
            return Ptr (ref, this);

        mHasState = oldState;

        return Ptr();
    }

    Ptr CellStore::searchViaHandle (const std::string& handle)
    {
        bool oldState = mHasState;

        mHasState = true;

        if (LiveCellRef<ESM::Activator> *ref = mActivators.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Potion> *ref = mPotions.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Apparatus> *ref = mAppas.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Armor> *ref = mArmors.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Book> *ref = mBooks.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Clothing> *ref = mClothes.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Container> *ref = mContainers.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Creature> *ref = mCreatures.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Door> *ref = mDoors.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Ingredient> *ref = mIngreds.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::CreatureLevList> *ref = mCreatureLists.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::ItemLevList> *ref = mItemLists.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Light> *ref = mLights.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Lockpick> *ref = mLockpicks.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Miscellaneous> *ref = mMiscItems.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::NPC> *ref = mNpcs.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Probe> *ref = mProbes.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Repair> *ref = mRepairs.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Static> *ref = mStatics.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM::Weapon> *ref = mWeapons.searchViaHandle (handle))
            return Ptr (ref, this);

        mHasState = oldState;

        return Ptr();
    }

    Ptr CellStore::searchViaActorId (int id)
    {
        if (Ptr ptr = ::searchViaActorId (mNpcs, id, this))
            return ptr;

        if (Ptr ptr = ::searchViaActorId (mCreatures, id, this))
            return ptr;

        return Ptr();
    }

    float CellStore::getWaterLevel() const
    {
        return mWaterLevel;
    }

    void CellStore::setWaterLevel (float level)
    {
        mWaterLevel = level;
        mHasState = true;
    }

    int CellStore::count() const
    {
        return
            mActivators.mList.size()
            + mPotions.mList.size()
            + mAppas.mList.size()
            + mArmors.mList.size()
            + mBooks.mList.size()
            + mClothes.mList.size()
            + mContainers.mList.size()
            + mDoors.mList.size()
            + mIngreds.mList.size()
            + mCreatureLists.mList.size()
            + mItemLists.mList.size()
            + mLights.mList.size()
            + mLockpicks.mList.size()
            + mMiscItems.mList.size()
            + mProbes.mList.size()
            + mRepairs.mList.size()
            + mStatics.mList.size()
            + mWeapons.mList.size()
            + mCreatures.mList.size()
            + mNpcs.mList.size();
    }

    void CellStore::load (const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm)
    {
        if (mCell)
        {
            if (mState!=State_Loaded)
            {
                if (mState==State_Preloaded)
                    mIds.clear();

                loadRefs (store, esm);

                mState = State_Loaded;

                // TODO: the pathgrid graph only needs to be loaded for active cells, so move this somewhere else.
                // In a simple test, loading the graph for all cells in MW + expansions took 200 ms
                mPathgridGraph.load(this);
            }
        }
        else // mForeignCell
        {
            if (mState!=State_Loaded)
            {
                if (mState == State_Preloaded)
                    mForeignIds.clear();

                loadForeignRefs(store, esm);

                mState = State_Loaded;
            }
        }
    }

    void CellStore::preload (const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm)
    {
        if (mCell)
        {
            if (mState==State_Unloaded)
            {
                listRefs (store, esm);

                mState = State_Preloaded;
            }
        }
    }

    void CellStore::listRefs(const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm)
    {
        assert (mCell);

        if (mCell->mContextList.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < mCell->mContextList.size(); i++)
        {
            // Reopen the ESM reader and seek to the right position.
            int index = mCell->mContextList.at(i).index;
            mCell->restore (*esm[0][index], i); // FIXME

            ESM::CellRef ref;

            // Get each reference in turn
            bool deleted = false;
            while (mCell->getNextRef (*esm[0][index], ref, deleted)) // FIXME
            {
                if (deleted)
                    continue;

                // Don't list reference if it was moved to a different cell.
                ESM::MovedCellRefTracker::const_iterator iter =
                    std::find(mCell->mMovedRefs.begin(), mCell->mMovedRefs.end(), ref.mRefNum);
                if (iter != mCell->mMovedRefs.end()) {
                    continue;
                }

                mIds.push_back (Misc::StringUtils::lowerCase (ref.mRefID));
            }
        }

        // List moved references, from separately tracked list.
        for (ESM::CellRefTracker::const_iterator it = mCell->mLeasedRefs.begin(); it != mCell->mLeasedRefs.end(); ++it)
        {
            const ESM::CellRef &ref = *it;

            mIds.push_back(Misc::StringUtils::lowerCase(ref.mRefID));
        }

        std::sort (mIds.begin(), mIds.end());
    }

    void CellStore::loadRefs(const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm)
    {
        assert (mCell);

        if (mCell->mContextList.empty())
            return; // this is a dynamically generated cell -> skipping.

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < mCell->mContextList.size(); i++)
        {
            // Reopen the ESM reader and seek to the right position.
            int index = mCell->mContextList.at(i).index;
            mCell->restore (*esm[0][index], i); // FIXME: 0 means TES3

            ESM::CellRef ref;
            ref.mRefNum.mContentFile = ESM::RefNum::RefNum_NoContentFile;

            // Get each reference in turn
            bool deleted = false;
            while(mCell->getNextRef(*esm[0][index], ref, deleted)) // FIXME: 0 means TES3
            {
                // Don't load reference if it was moved to a different cell.
                ESM::MovedCellRefTracker::const_iterator iter =
                    std::find(mCell->mMovedRefs.begin(), mCell->mMovedRefs.end(), ref.mRefNum);
                if (iter != mCell->mMovedRefs.end()) {
                    continue;
                }

                loadRef (ref, deleted, store);
            }
        }

        // Load moved references, from separately tracked list.
        for (ESM::CellRefTracker::const_iterator it = mCell->mLeasedRefs.begin();
                it != mCell->mLeasedRefs.end(); ++it)
        {
            ESM::CellRef &ref = const_cast<ESM::CellRef&>(*it);

            loadRef (ref, false, store);
        }
    }

    void CellStore::loadForeignRefs(const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm)
    {
        assert(mCell->isForeignCell());
        const ForeignCell *cell = static_cast<const ForeignCell*>(mCell);

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < cell->mModList.size(); i++)
        {
            // Reopen the ESM reader and seek to the right position.
            int modIndex = cell->mModList.at(i).modIndex;
            ESM::ESM4Reader *esm4 = static_cast<ESM::ESM4Reader*>(esm[1][modIndex]); // FIXME: hard coded '1'
            esm4->restoreCellChildrenContext(cell->mModList.at(i));
            // FIXME: need a way to load the cell children (just the refs?)
            //
            // 1. skip cell itself (should have been preloaded)
            // 2. load cell child group
            std::cout << "file " << cell->mModList.at(i).filename << std::endl;

            // hasMoreRecs() here depends on the hack in restoreCellChildrenContext()
            while(esm[1][modIndex]->hasMoreRecs()) // FIXME: hard coded '1'
            {
                ESM4::Reader& reader = esm4->reader();
                reader.checkGroupStatus();

                loadTes4Group(store, *esm[1][modIndex]); // FIXME: hard coded '1'
            }
        }
    }

    void CellStore::loadTes4Group (const MWWorld::ESMStore &store, ESM::ESMReader &esm)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

        reader.getRecordHeader();
        const ESM4::RecordHeader& hdr = reader.hdr();

        if (hdr.record.typeId != ESM4::REC_GRUP)
            return loadTes4Record(store, esm, hdr);

        switch (hdr.group.type)
        {
            case ESM4::Grp_CellChild:
            case ESM4::Grp_CellPersistentChild:
            case ESM4::Grp_CellTemporaryChild:
            case ESM4::Grp_CellVisibleDistChild:
            {
                reader.adjustGRUPFormId();  // not needed or even shouldn't be done? (only labels anyway)
                reader.saveGroupStatus();
                if (!esm.hasMoreRecs())
                    return; // may have been an empty group followed by EOF

                loadTes4Group(store, esm);

                break;
            }
            case ESM4::Grp_RecordType:
            case ESM4::Grp_WorldChild:
            case ESM4::Grp_TopicChild:
            case ESM4::Grp_ExteriorCell:
            case ESM4::Grp_ExteriorSubCell:
            case ESM4::Grp_InteriorCell:
            case ESM4::Grp_InteriorSubCell:
            default:
                std::cout << "unknown group..." << std::endl; // FIXME
                reader.skipGroup();
                break;
        }

        return;
    }

    void CellStore::loadTes4Record (const MWWorld::ESMStore &store, ESM::ESMReader& esm, const ESM4::RecordHeader& hdr)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

        switch (hdr.record.typeId)
        {
            case ESM4::REC_REFR:
            {
                bool deleted = (reader.hdr().record.flags & ESM4::Rec_Deleted) != 0;
                reader.getRecordData();
                ESM4::Reference record;
                record.load(reader);
                if (!record.mEditorId.empty())
                    std::cout << "REFR: " << record.mEditorId << std::endl;




                switch (store.find(record.mBaseObj))
                {
                    case MKTAG('R','H','A','I'): std::cout << " hair " << std::endl; break;
                    case MKTAG('S','E','Y','E'): std::cout << " eyes " << std::endl; break;
                    case MKTAG('N','S','O','U'):
                    {
                        std::cout << " sound " << std::endl;
                        mForeignSounds.load(record, deleted, store); break;
                    }
                    case MKTAG('I','A','C','T'):
                    {
                        mForeignActivators.load(record, deleted, store); break;
                    }
                    case MKTAG('A','A','P','P'): std::cout << " apparatus " << std::endl; break;
                    case MKTAG('O','A','R','M'): std::cout << " armor " << std::endl; break;
                    case MKTAG('K','B','O','O'):
                    {
                        mForeignBooks.load(record, deleted, store); break;
                    }
                    case MKTAG('T','C','L','O'): std::cout << " clothing " << std::endl; break;
                    case MKTAG('T','C','O','N'):
                    {
                        mForeignContainers.load(record, deleted, store); break;
                    }
                    case MKTAG('R','D','O','O'):
                    {
                        std::cout << " door " << std::endl;
                        mForeignDoors.load(record, deleted, store); break;
                    }
                    case MKTAG('R','I','N','G'): std::cout << " ingredient " << std::endl; break;
                    case MKTAG('H','L','I','G'):
                    {
                        mForeignLights.load(record, deleted, store); break;
                    }
                    case MKTAG('C','M','I','S'):
                    {
                        std::cout << " misc " << std::endl;
                        mForeignMiscItems.load(record, deleted, store); break;
                    }
                    case MKTAG('T','S','T','A'):
                    {
                        mForeignStatics.load(record, deleted, store); break;
                    }
                    case MKTAG('P','W','E','A'): std::cout << " weapon " << std::endl; break;
                    case MKTAG('_','N','P','C'): std::cout << " npc " << std::endl; break;
                    case MKTAG('A','C','R','E'): std::cout << " creature " << std::endl; break;
                    case MKTAG('C','L','V','L'): std::cout << " lvlcreature " << std::endl; break;
                    case MKTAG('M','S','L','G'): std::cout << " soulgem " << std::endl; break;
                    case MKTAG('H','A','L','C'): std::cout << " potion " << std::endl; break;
                    case MKTAG('T','S','G','S'): std::cout << " sigilstone " << std::endl; break;

                    case 0: std::cerr << "Cell reference " + ESM4::formIdToString(record.mBaseObj) + " not found!\n"; break;

                    default:
                        std::cerr
                            << "WARNING: Ignoring reference '" << ESM4::formIdToString(record.mBaseObj) << "' of unhandled type\n";
                }




                break;
            }
            case ESM4::REC_ACHR:
#if 0
            {
                reader.getRecordData();
                ESM4::ActorCharacter record;
                record.load(reader);
                break;
            }
#endif
            case ESM4::REC_LAND: //reader.getRecordData(); mForeignLands.load(esm, mForeignCells); break;
            case ESM4::REC_PGRD: // Oblivion only?
            case ESM4::REC_ACRE: // Oblivion only?
            case ESM4::REC_ROAD: // Oblivion only?
            {
                std::cout << ESM4::printName(hdr.record.typeId) << " skipping..." << std::endl;
                reader.skipRecordData();
                break;
            }
            default:
            {
                std::cout << "CellStore unsupported TES4 record type: " + ESM4::printName(hdr.record.typeId) << std::endl;
                reader.skipRecordData();
            }
        }

        return;
    }

    bool CellStore::isExterior() const
    {
        return mCell->isExterior();
    }

    Ptr CellStore::searchInContainer (const std::string& id)
    {
        bool oldState = mHasState;

        mHasState = true;

        if (Ptr ptr = searchInContainerList (mContainers, id))
            return ptr;

        if (Ptr ptr = searchInContainerList (mCreatures, id))
            return ptr;

        if (Ptr ptr = searchInContainerList (mNpcs, id))
            return ptr;

        mHasState = oldState;

        return Ptr();
    }

    void CellStore::loadRef (ESM::CellRef& ref, bool deleted, const ESMStore& store)
    {
        Misc::StringUtils::lowerCaseInPlace (ref.mRefID);

        switch (store.find (ref.mRefID))
        {
            case ESM::REC_ACTI: mActivators.load(ref, deleted, store); break;
            case ESM::REC_ALCH: mPotions.load(ref, deleted,store); break;
            case ESM::REC_APPA: mAppas.load(ref, deleted, store); break;
            case ESM::REC_ARMO: mArmors.load(ref, deleted, store); break;
            case ESM::REC_BOOK: mBooks.load(ref, deleted, store); break;
            case ESM::REC_CLOT: mClothes.load(ref, deleted, store); break;
            case ESM::REC_CONT: mContainers.load(ref, deleted, store); break;
            case ESM::REC_CREA: mCreatures.load(ref, deleted, store); break;
            case ESM::REC_DOOR: mDoors.load(ref, deleted, store); break;
            case ESM::REC_INGR: mIngreds.load(ref, deleted, store); break;
            case ESM::REC_LEVC: mCreatureLists.load(ref, deleted, store); break;
            case ESM::REC_LEVI: mItemLists.load(ref, deleted, store); break;
            case ESM::REC_LIGH: mLights.load(ref, deleted, store); break;
            case ESM::REC_LOCK: mLockpicks.load(ref, deleted, store); break;
            case ESM::REC_MISC: mMiscItems.load(ref, deleted, store); break;
            case ESM::REC_NPC_: mNpcs.load(ref, deleted, store); break;
            case ESM::REC_PROB: mProbes.load(ref, deleted, store); break;
            case ESM::REC_REPA: mRepairs.load(ref, deleted, store); break;
            case ESM::REC_STAT: mStatics.load(ref, deleted, store); break;
            case ESM::REC_WEAP: mWeapons.load(ref, deleted, store); break;

            case 0: std::cerr << "Cell reference " + ref.mRefID + " not found!\n"; break;

            default:
                std::cerr
                    << "WARNING: Ignoring reference '" << ref.mRefID << "' of unhandled type\n";
        }
    }

    void CellStore::loadState (const ESM::CellState& state)
    {
        mHasState = true;

        if (mCell->mData.mFlags & ESM::Cell::Interior && mCell->mData.mFlags & ESM::Cell::HasWater)
            mWaterLevel = state.mWaterLevel;

        mWaterLevel = state.mWaterLevel;
        mLastRespawn = MWWorld::TimeStamp(state.mLastRespawn);
    }

    void CellStore::saveState (ESM::CellState& state) const
    {
        state.mId = mCell->getCellId();

        if (mCell->mData.mFlags & ESM::Cell::Interior && mCell->mData.mFlags & ESM::Cell::HasWater)
            state.mWaterLevel = mWaterLevel;

        state.mWaterLevel = mWaterLevel;
        state.mHasFogOfWar = (mFogState.get() ? 1 : 0);
        state.mLastRespawn = mLastRespawn.toEsm();
    }

    void CellStore::writeFog(ESM::ESMWriter &writer) const
    {
        if (mFogState.get())
        {
            mFogState->save(writer, mCell->mData.mFlags & ESM::Cell::Interior);
        }
    }

    void CellStore::readFog(ESM::ESMReader &reader)
    {
        mFogState.reset(new ESM::FogState());
        mFogState->load(reader);
    }

    void CellStore::writeReferences (ESM::ESMWriter& writer) const
    {
        writeReferenceCollection<ESM::ObjectState> (writer, mActivators);
        writeReferenceCollection<ESM::ObjectState> (writer, mPotions);
        writeReferenceCollection<ESM::ObjectState> (writer, mAppas);
        writeReferenceCollection<ESM::ObjectState> (writer, mArmors);
        writeReferenceCollection<ESM::ObjectState> (writer, mBooks);
        writeReferenceCollection<ESM::ObjectState> (writer, mClothes);
        writeReferenceCollection<ESM::ContainerState> (writer, mContainers);
        writeReferenceCollection<ESM::CreatureState> (writer, mCreatures);
        writeReferenceCollection<ESM::DoorState> (writer, mDoors);
        writeReferenceCollection<ESM::ObjectState> (writer, mIngreds);
        writeReferenceCollection<ESM::CreatureLevListState> (writer, mCreatureLists);
        writeReferenceCollection<ESM::ObjectState> (writer, mItemLists);
        writeReferenceCollection<ESM::ObjectState> (writer, mLights);
        writeReferenceCollection<ESM::ObjectState> (writer, mLockpicks);
        writeReferenceCollection<ESM::ObjectState> (writer, mMiscItems);
        writeReferenceCollection<ESM::NpcState> (writer, mNpcs);
        writeReferenceCollection<ESM::ObjectState> (writer, mProbes);
        writeReferenceCollection<ESM::ObjectState> (writer, mRepairs);
        writeReferenceCollection<ESM::ObjectState> (writer, mStatics);
        writeReferenceCollection<ESM::ObjectState> (writer, mWeapons);
    }

    void CellStore::readReferences (ESM::ESMReader& reader,
        const std::map<int, int>& contentFileMap)
    {
        mHasState = true;

        while (reader.isNextSub ("OBJE"))
        {
            unsigned int unused;
            reader.getHT (unused);

            // load the RefID first so we know what type of object it is
            ESM::CellRef cref;
            cref.loadId(reader, true);

            int type = MWBase::Environment::get().getWorld()->getStore().find(cref.mRefID);
            if (type == 0)
            {
                std::cerr << "Dropping reference to '" << cref.mRefID << "' (object no longer exists)" << std::endl;
                reader.skipHSubUntil("OBJE");
                continue;
            }

            switch (type)
            {
                case ESM::REC_ACTI:

                    readReferenceCollection<ESM::ObjectState> (reader, mActivators, cref, contentFileMap);
                    break;

                case ESM::REC_ALCH:

                    readReferenceCollection<ESM::ObjectState> (reader, mPotions, cref, contentFileMap);
                    break;

                case ESM::REC_APPA:

                    readReferenceCollection<ESM::ObjectState> (reader, mAppas, cref, contentFileMap);
                    break;

                case ESM::REC_ARMO:

                    readReferenceCollection<ESM::ObjectState> (reader, mArmors, cref, contentFileMap);
                    break;

                case ESM::REC_BOOK:

                    readReferenceCollection<ESM::ObjectState> (reader, mBooks, cref, contentFileMap);
                    break;

                case ESM::REC_CLOT:

                    readReferenceCollection<ESM::ObjectState> (reader, mClothes, cref, contentFileMap);
                    break;

                case ESM::REC_CONT:

                    readReferenceCollection<ESM::ContainerState> (reader, mContainers, cref, contentFileMap);
                    break;

                case ESM::REC_CREA:

                    readReferenceCollection<ESM::CreatureState> (reader, mCreatures, cref, contentFileMap);
                    break;

                case ESM::REC_DOOR:

                    readReferenceCollection<ESM::DoorState> (reader, mDoors, cref, contentFileMap);
                    break;

                case ESM::REC_INGR:

                    readReferenceCollection<ESM::ObjectState> (reader, mIngreds, cref, contentFileMap);
                    break;

                case ESM::REC_LEVC:

                    readReferenceCollection<ESM::CreatureLevListState> (reader, mCreatureLists, cref, contentFileMap);
                    break;

                case ESM::REC_LEVI:

                    readReferenceCollection<ESM::ObjectState> (reader, mItemLists, cref, contentFileMap);
                    break;

                case ESM::REC_LIGH:

                    readReferenceCollection<ESM::ObjectState> (reader, mLights, cref, contentFileMap);
                    break;

                case ESM::REC_LOCK:

                    readReferenceCollection<ESM::ObjectState> (reader, mLockpicks, cref, contentFileMap);
                    break;

                case ESM::REC_MISC:

                    readReferenceCollection<ESM::ObjectState> (reader, mMiscItems, cref, contentFileMap);
                    break;

                case ESM::REC_NPC_:

                    readReferenceCollection<ESM::NpcState> (reader, mNpcs, cref, contentFileMap);
                    break;

                case ESM::REC_PROB:

                    readReferenceCollection<ESM::ObjectState> (reader, mProbes, cref, contentFileMap);
                    break;

                case ESM::REC_REPA:

                    readReferenceCollection<ESM::ObjectState> (reader, mRepairs, cref, contentFileMap);
                    break;

                case ESM::REC_STAT:

                    readReferenceCollection<ESM::ObjectState> (reader, mStatics, cref, contentFileMap);
                    break;

                case ESM::REC_WEAP:

                    readReferenceCollection<ESM::ObjectState> (reader, mWeapons, cref, contentFileMap);
                    break;

                default:

                    throw std::runtime_error ("unknown type in cell reference section");
            }
        }
    }

    bool operator== (const CellStore& left, const CellStore& right)
    {
        return left.getCell()->getCellId()==right.getCell()->getCellId();
    }

    bool operator!= (const CellStore& left, const CellStore& right)
    {
        return !(left==right);
    }

    bool CellStore::isPointConnected(const int start, const int end) const
    {
        return mPathgridGraph.isPointConnected(start, end);
    }

    std::list<ESM::Pathgrid::Point> CellStore::aStarSearch(const int start, const int end) const
    {
        return mPathgridGraph.aStarSearch(start, end);
    }

    void CellStore::setFog(ESM::FogState *fog)
    {
        mFogState.reset(fog);
    }

    ESM::FogState* CellStore::getFog() const
    {
        return mFogState.get();
    }

    void CellStore::respawn()
    {
        if (mState == State_Loaded)
        {
            static int iMonthsToRespawn = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("iMonthsToRespawn")->getInt();
            if (MWBase::Environment::get().getWorld()->getTimeStamp() - mLastRespawn > 24*30*iMonthsToRespawn)
            {
                mLastRespawn = MWBase::Environment::get().getWorld()->getTimeStamp();
                for (CellRefList<ESM::Container>::List::iterator it (mContainers.mList.begin()); it!=mContainers.mList.end(); ++it)
                {
                    Ptr ptr (&*it, this);
                    ptr.getClass().respawn(ptr);
                }
                for (CellRefList<ESM::Creature>::List::iterator it (mCreatures.mList.begin()); it!=mCreatures.mList.end(); ++it)
                {
                    Ptr ptr (&*it, this);
                    ptr.getClass().respawn(ptr);
                }
                for (CellRefList<ESM::NPC>::List::iterator it (mNpcs.mList.begin()); it!=mNpcs.mList.end(); ++it)
                {
                    Ptr ptr (&*it, this);
                    ptr.getClass().respawn(ptr);
                }
                for (CellRefList<ESM::CreatureLevList>::List::iterator it (mCreatureLists.mList.begin()); it!=mCreatureLists.mList.end(); ++it)
                {
                    Ptr ptr (&*it, this);
                    ptr.getClass().respawn(ptr);
                }
            }
        }
    }
}
