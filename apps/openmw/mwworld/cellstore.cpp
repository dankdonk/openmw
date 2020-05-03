#include "cellstore.hpp"

#include <iostream>
#include <algorithm>

#include <extern/esm4/formid.hpp>
#include <extern/esm4/cell.hpp>
#include <extern/esm4/refr.hpp>
#include <extern/esm4/achr.hpp>
#include <extern/esm4/acre.hpp>
#include <extern/esm4/pgrd.hpp>

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

// FIXME: testing audio markers
#if 0
                if (ref.mAudioLocation)
                    std::cout << "audio loc " << ref.mEditorId << " "
                        << ref.mPosition.pos.x/4096 << "," << ref.mPosition.pos.y/4096 << " "
                    << mList.back().mBase->mEditorId << std::endl; // FIXME: debug only
#endif
            }
        }
        else
        {
            std::cerr
                << "Error: could not resolve foreign cell reference " << ref.mEditorId
                << " (dropping reference)" << std::endl;
        }
    }

    template <typename X>
    void CellRefList<X>::load(ESM4::ActorCreature &ref, bool deleted, const MWWorld::ESMStore &esmStore)
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

    template <typename X>
    void CellRefList<X>::load(ESM4::ActorCharacter &ref, bool deleted, const MWWorld::ESMStore &esmStore)
    {
        const MWWorld::ForeignStore<X> &store = esmStore.getForeign<X>();

        if (const X *ptr = store.search (ref.mBaseObj))
        {
            // see operator== in livecellref.hpp, check for matching formId
            typename std::list<LiveRef>::iterator iter =
                std::find(mList.begin(), mList.end(), ref);
#if 0
                            for (size_t i = 0; i < ptr->mInventory.size(); ++i)
                            {
                                uint32_t type = esmStore.find(ptr->mInventory[i].item);
                                if (type == MKTAG('K', 'B', 'O', 'O'))
                                {
                        const ESM4::Book* note = esmStore.getForeign<ESM4::Book>().search(ptr->mInventory[i].item);
                        std::cout << ptr->mEditorId << " " << note->mEditorId << std::endl;
                                }
                                else
                                    std::cout << ESM4::printName(type) << std::endl;
                            }
#endif
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

    CellStore::CellStore (const ESM::Cell *cell, bool isForeignCell, bool isDummyCell)
      : mCell (cell), mState (State_Unloaded), mHasState (false), mLastRespawn(0,0),
        mIsForeignCell(isForeignCell), mIsDummyCell(isDummyCell), mIsVisibleDistCell(false), mForeignLand(0)
    {
        if (!mIsForeignCell)
            mWaterLevel = cell->mWater;
        else
            mWaterLevel = 0.f; // FIXME: should lookup formid and determine?
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

        //

        if (LiveCellRef<ESM4::Activator> *ref = mForeignActivators.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Container> *ref = mForeignContainers.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Book>      *ref = mForeignBooks.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Door>      *ref = mForeignDoors.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Light>     *ref = mForeignLights.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::MiscItem>  *ref = mForeignMiscItems.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Sound>     *ref = mSounds.find (id))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Static>    *ref = mForeignStatics.find (id))
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

        //

        if (LiveCellRef<ESM4::Activator> *ref = mForeignActivators.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Apparatus> *ref = mForeignApparatus.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Armor>     *ref = mForeignArmors.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Book>      *ref = mForeignBooks.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Clothing>  *ref = mForeignClothes.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Container> *ref = mForeignContainers.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Door>      *ref = mForeignDoors.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Ingredient> *ref = mForeignIngredients.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Light>     *ref = mForeignLights.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::MiscItem>  *ref = mForeignMiscItems.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Sound>     *ref = mSounds.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Static>    *ref = mForeignStatics.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Grass>     *ref = mForeignGrasses.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Tree>      *ref = mForeignTrees.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Flora>     *ref = mForeignFloras.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Furniture> *ref = mForeignFurnitures.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Weapon>    *ref = mForeignWeapons.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Ammo>      *ref = mForeignAmmos.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Npc>       *ref = mForeignNpcs.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Creature>  *ref = mForeignCreatures.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::LevelledCreature> *ref = mLevelledCreatures.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::SoulGem>   *ref = mSoulGems.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Key>       *ref = mForeignKeys.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Potion>    *ref = mForeignPotions.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Subspace>  *ref = mSubspaces.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::SigilStone> *ref = mSigilStones.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::LevelledItem> *ref = mLevelledItems.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::LevelledNpc> *ref = mLevelledNpcs.searchViaHandle (handle))
            return Ptr (ref, this);

        if (LiveCellRef<ESM4::Note>      *ref = mNotes.searchViaHandle (handle))
            return Ptr (ref, this);

        mHasState = oldState;

        return Ptr();
    }

    Ptr CellStore::searchViaActorId (int id)
    {
        if (Ptr ptr = ::searchViaActorId (mForeignNpcs, id, this))
            return ptr;

        if (Ptr ptr = ::searchViaActorId (mForeignCreatures, id, this))
            return ptr;

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

    int CellStore::getRefrEstimate(std::int32_t groupType) const
    {
        assert(mIsForeignCell);
        const ForeignCell *cell = static_cast<const ForeignCell*>(mCell);

        return (int)cell->getRefrEstimate(groupType);
    }

    int CellStore::getPersistentRefrCount() const
    {
        assert(mIsForeignCell);
        const ForeignCell *cell = static_cast<const ForeignCell*>(mCell);

        return (int)cell->getPersistentRefrCount();
    }

    void CellStore::load (const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm)
    {
        if (!mIsForeignCell)
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

                // Foreign::TerrainStorage::getLand() calls MWWorld::Cells::getWorldCell()
                // which could mean a lot of unnecessary loading - build PathgridGraph later
                // (but then again the cell would be empty and the overhead won't be much anyway?)
            }
        }
    }

    void CellStore::preload (const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm)
    {
        if (mCell && !mIsForeignCell)
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
            mCell->restore (*esm[0][index], (int)i); // FIXME: hardcoded 0 means TES3

            ESM::CellRef ref;

            // Get each reference in turn
            bool deleted = false;
            while (mCell->getNextRef (*esm[0][index], ref, deleted)) // FIXME hardcoded 0 means TES3
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
            mCell->restore (*esm[0][index], (int)i); // FIXME: hardcoded 0 means TES3

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

    // FIXME: this can be optimised by checking if a cell has any refs - see ForeignCell::mHasChildren
    void CellStore::loadForeignRefs(const MWWorld::ESMStore& store, std::vector<std::vector<ESM::ESMReader*> >& esm)
    {
        assert(mIsForeignCell);
        const ForeignCell *cell = static_cast<const ForeignCell*>(mCell);

        if (!cell)
            return; // FIXME: probably spelling error from console

        // Load references from all plugins that do something with this cell.
        for (size_t i = 0; i < cell->mModList.size(); i++)
        {
            // Reopen the ESM reader and seek to the right position.
            int modIndex = cell->mModList.at(i).modIndex >> 24; // FIXME: Morroblivion
            // HACK: Should find another way...  Anyway, TES4 has 20 byte header, TES5 24.
            int modType = (cell->mModList.at(i).recHeaderSize == 20) ? 1 : 2; // FIXME: hard coded 1 and 2
            ESM::ESM4Reader *esm4 = static_cast<ESM::ESM4Reader*>(esm[modType][modIndex]);
            esm4->restoreCellChildrenContext(cell->mModList.at(i));
            // FIXME: need a way to load the cell children (just the refs?)
            //
            // 1. skip cell itself (should have been preloaded)
            // 2. load cell child group
            //std::cout << "file " << cell->mModList.at(i).filename << std::endl;

            // hasMoreRecs() here depends on the hack in restoreCellChildrenContext()
            while(esm[modType][modIndex]->hasMoreRecs())
            {
                ESM4::Reader& reader = esm4->reader();
                reader.checkGroupStatus();

                loadTes4Group(store, *esm[modType][modIndex]);
            }

            const ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            const ForeignWorld *world
                = store.get<ForeignWorld>().find(static_cast<const ForeignCell*>(mCell)->mCell->mParent);
            if (!world)
                continue;

            if ((world->mParentUseFlags & 0x01/*use land data*/) == 0)
                continue;

            if (world->mParent != 0) // ok, use parent world land but do I have a parent world?
            {
                CellStore * parentCell
                    = MWBase::Environment::get().getWorld()->getWorldCell(world->mParent,
                        mCell->getGridX(),
                        mCell->getGridY());

                if (parentCell)
                    mForeignLand = parentCell->getForeignLandId();
            }
        }
    }

    void CellStore::loadTes4Group (const MWWorld::ESMStore& store, ESM::ESMReader& esm)
    {
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();

        reader.getRecordHeader();
        const ESM4::RecordHeader& hdr = reader.hdr();

        if (hdr.record.typeId != ESM4::REC_GRUP)
            return loadTes4Record(store, esm);

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
#if 0 // FIXME: temporary testing only
            case ESM4::Grp_CellPersistentChild:
            case ESM4::Grp_CellVisibleDistChild:
            {
                //if (hdr.group.type == ESM4::Grp_CellPersistentChild)
                    std::cout << ESM4::printLabel(reader.hdr().group.label, reader.hdr().group.type) << std::endl;
                reader.skipGroup();
                break;
            }
#endif
            case ESM4::Grp_RecordType:
            case ESM4::Grp_WorldChild:
            case ESM4::Grp_TopicChild:
            case ESM4::Grp_ExteriorCell:
            case ESM4::Grp_ExteriorSubCell:
            case ESM4::Grp_InteriorCell:
            case ESM4::Grp_InteriorSubCell:
            default:
                std::cout << "unexpected group..." << std::endl; // FIXME
                reader.skipGroup();
                break;
        }

        return;
    }

    void CellStore::loadTes4Record (const MWWorld::ESMStore& store, ESM::ESMReader& esm)
    {
        // Assumes that the reader has just read the record header only.
        ESM4::Reader& reader = static_cast<ESM::ESM4Reader*>(&esm)->reader();
        const ESM4::RecordHeader& hdr = reader.hdr();

        switch (hdr.record.typeId)
        {
            case ESM4::REC_REFR:
            {
                // FIXME: testing WhiteRun
                if ((reader.hdr().record.flags & ESM4::Rec_Disabled) != 0)
                {
#if 1
                    reader.skipRecordData();
#else
                    reader.getRecordData();
                    ESM4::Reference record;
                    record.load(reader);
                    std::cout << "disabled " << record.mEditorId << std::endl;

                    if (record.mEditorId == "CWSiegeWhiterunFireEnblerMrk")
                        std::cout << std::hex << record.mFormId << std::endl;

                    if (record.mBaseObj == 0x00100d2f)
                        std::cout << "? " << record.mEditorId << std::endl;
                    switch (store.find(record.mEsp.parent))
                    {
                        case MKTAG('R','R','E','F'): std::cout << "parent refr " << std::endl; break;
                        default: std::cout << std::hex << store.find(record.mEsp.parent) << std::endl; break;
                    }
#endif
                    break;
                }

                bool deleted = (reader.hdr().record.flags & ESM4::Rec_Deleted) != 0;
                reader.getRecordData();
                ESM4::Reference record;
                record.load(reader);
                //if (!record.mEditorId.empty())
                    //std::cout << "REFR: " << record.mEditorId << std::endl; // FIXME

#if 0
                if (reader.grp().type == ESM4::Grp_CellPersistentChild)
                    //if (record.mIsMapMarker)
                    std::cout << "Persistent REFR: " << record.mEditorId << " 0x"
                        << ESM4::formIdToString(record.mFormId) << std::endl;
#endif

                const int cellSize = 4096;

                int newX = static_cast<int>(std::floor(record.mPosition.pos.x / cellSize));
                int newY = static_cast<int>(std::floor(record.mPosition.pos.y / cellSize));




                // FIXME: for temp testing
                //if (newX < (5-16) || newX > (5+16) || newY < (12-16) || newY > (12+16))
                    //break;





                if (record.mEsp.parent != 0)
                {
                    int res = store.find(record.mEsp.parent);
                    if (res == MKTAG('R','R','E','F'))
                        std::cout << "flags " << std::hex << record.mEsp.flags << std::endl;

                    if ((record.mEsp.parent == 0x000376e0 || record.mEsp.parent == 0x000fc90b ||
                         record.mEsp.parent == 0x00100d2c || record.mEsp.parent == 0x00100d98)
                            && ((record.mEsp.flags & 0x01) == 0))
                    {
                        std::cout << "CWSiegeWhiterunFireEnablerMrk" << std::endl;
                        std::cout << "CWWhiterunIntEnableOnly" << std::endl;
                        std::cout << "CWSolClutterToggle" << std::endl;
                        std::cout << "CWSolClutterEnableForever" << std::endl;
                        std::cout << std::hex << record.mEsp.flags << std::endl;
                        break;
                    }
                }
#if 0
                if ((record.mFlags & ESM4::Rec_DistVis) != 0 && reader.getContext().groupStack.back().first.type != ESM4::Grp_CellVisibleDistChild)
                {
                    std::string padding = ""; // FIXME: debugging only
                    padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                    std::cout << padding << "CellStore REFR " << record.mEditorId << " "
                              << ESM4::formIdToString(record.mFormId) << " visible dist" << std::endl;
                }

                if ((record.mFlags & ESM4::Rec_DistVis) == 0 && reader.getContext().groupStack.back().first.type == ESM4::Grp_CellVisibleDistChild)
                {
                    std::string padding = ""; // FIXME: debugging only
                    padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                    std::cout << padding << "CellStore REFR " << record.mEditorId << " "
                              << ESM4::formIdToString(record.mFormId) << " NOT visible dist" << std::endl;
                }
#endif
                // unfortunately at this point most of the worlds are not yet loaded and
                // hence nothing will be found - try interior only
                //if (record.mBaseObj == 0x23) // AudioMarker
                if (record.mAudioLocation && !getCell()->isExterior())
                {
                    ESM4::FormId worldId
                        = static_cast<const MWWorld::ForeignCell*>(getCell())->mCell->mParent;
                    const ForeignWorld *world = store.get<ForeignWorld>().find(worldId);
                    if (world)
                    {
                        std::string worldName = world->mEditorId;
                        std::cout << "REFR audio " << worldName << std::endl;
                    }
                    else // shouldn't be any interior dummy cells
                        //std::cout << "REFR audio" << (isDummyCell() ? " dummy cell" : "") << std::endl;
                        std::cout << "REFR audio" << /*15d026*/ ESM4::formIdToString(record.mAudioLocation) << std::endl;

                    mAudioLocation = record.mAudioLocation; // will be processed later when cell becomes active
                }

                switch (store.find(record.mBaseObj))
                {
                    case MKTAG('N','S','O','U'): mSounds.load(record, deleted, store); break;
#if 1
                    case MKTAG('I','A','C','T'): mForeignActivators.load(record, deleted, store); break;
#else
                    case MKTAG('I','A','C','T'):
                    {
                        const MWWorld::ForeignStore<ESM4::Activator>& actiStore
                            = store.getForeign<ESM4::Activator>();
                        const ESM4::Activator *acti = actiStore.search(record.mBaseObj);
                        if (acti)
                            std::cout << "activator " << acti->mEditorId << std::endl;

                        mForeignActivators.load(record, deleted, store);
                        break;
                    }
#endif
                    case MKTAG('A','A','P','P'): mForeignApparatus.load(record, deleted, store); break;
                    case MKTAG('O','A','R','M'): mForeignArmors.load(record, deleted, store); break;
                    case MKTAG('K','B','O','O'): mForeignBooks.load(record, deleted, store); break;
                    case MKTAG('T','C','L','O'): mForeignClothes.load(record, deleted, store); break;
                    case MKTAG('T','C','O','N'): mForeignContainers.load(record, deleted, store); break;
                    case MKTAG('R','D','O','O'):
                    {
                        mForeignDoors.load(record, deleted, store);
                        store.setDoorCell(record.mFormId, reader.currCell());
                        break;
                    }
                    case MKTAG('R','I','N','G'): mForeignIngredients.load(record, deleted, store); break;
                    case MKTAG('H','L','I','G'): mForeignLights.load(record, deleted, store); break;
                    case MKTAG('C','M','I','S'): mForeignMiscItems.load(record, deleted, store); break;
                    case MKTAG('T','S','T','A'):
                                                 mForeignStatics.load(record, deleted, store);
                                                 if (record.mEditorId == "DoorMarker")
                                                     std::cout << "DoorMarker: " << " 0x"
                                                         << ESM4::formIdToString(record.mFormId) << std::endl;
#if 0
                //if (reader.getContext().currWorld == 0x0001D0BC && reader.getContext().groupStack.back().first.type == ESM4::Grp_CellVisibleDistChild)
                if (reader.getContext().currWorld == 0x0000003C &&
                        reader.getContext().groupStack.back().first.type == ESM4::Grp_CellPersistentChild)
                {
                    std::string padding = ""; // FIXME: debugging only
                    padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                    std::cout << padding << "CellStore REFR " << record.mEditorId << " "
                              //<< ESM4::formIdToString(reader.getContext().currCell) << " visible dist "
                              << ESM4::formIdToString(reader.getContext().currCell) << " persistent "
                              << ESM4::formIdToString(record.mBaseObj) << std::endl;
                }
#endif
                                                 break;
                    case MKTAG('S','G','R','A'): mForeignGrasses.load(record, deleted, store); break;
                    //case MKTAG('E','T','R','E'): mForeignTrees.load(record, deleted, store); break;
                    case MKTAG('R','F','L','O'): mForeignFloras.load(record, deleted, store); break;
                    case MKTAG('N','F','U','R'): mForeignFurnitures.load(record, deleted, store); break;
                    case MKTAG('P','W','E','A'): mForeignWeapons.load(record, deleted, store); break;
                    case MKTAG('O','A','M','M'): mForeignAmmos.load(record, deleted, store); break;
                    case MKTAG('C','L','V','L'):
                    {
                        mLevelledCreatures.load(record, deleted, store); break; // only TES4
                    }
                    case MKTAG('M','S','L','G'): mSoulGems.load(record, deleted, store); break;
                    case MKTAG('M','K','E','Y'): mForeignKeys.load(record, deleted, store); break;
                    case MKTAG('H','A','L','C'): mForeignPotions.load(record, deleted, store); break;
                    case MKTAG('P','S','B','S'): mSubspaces.load(record, deleted, store); break;
                    case MKTAG('T','S','G','S'): mSigilStones.load(record, deleted, store); break;
                    case MKTAG('I','L','V','L'): mLevelledItems.load(record, deleted, store); break;
                    case MKTAG('E','N','O','T'): mNotes.load(record, deleted, store); break;
                    case MKTAG('N','L','V','L'): // Leveled NPC
                    {
                        mLevelledNpcs.load(record, deleted, store); break; // never occurs?
                    }

                    case MKTAG('E','T','R','E'): //mForeignTrees.load(record, deleted, store); break;
                    case MKTAG('M','I','D','L'): // Idle Marker
                    case MKTAG('T','M','S','T'): // Movable Static
                    case MKTAG('T','T','X','S'): // Texture Set
                    case MKTAG('L','S','C','R'): // Scroll
                    case MKTAG('A','A','R','M'): // Armor Addon
                    case MKTAG('M','T','E','R'): // Terminal
                    case MKTAG('T','T','A','C'): // Talking Activator
                    case MKTAG('C','A','S','P'): // Acoustic Space
                    case MKTAG('D','I','M','O'): // Item Mod
                    case MKTAG('T','P','W','A'): // Placeable Water
                    case MKTAG('L','S','C','O'): // Static Collection
                         break;

                    // FO3 adds ASPC, IDLM, ARMA, MSTT, NOTE, PWAT, SCOL, TACT, TERM and TXST
                    // FONV adds CCRD, IMOD and CMNY
                    case 0: std::cerr << "Cell refr " + record.mEditorId +" "+ ESM4::formIdToString(record.mBaseObj) + " not found!\n"; break;

                    default:
                        std::cerr << "WARNING: Ignoring TES4 reference '"
                                  << ESM4::formIdToString(record.mBaseObj) << "' of unhandled type\n";
                }

// FIXME: testing only
#if 0
                if (record.mEsp.parent != 0)
                {
                    uint32_t baseType = store.find(record.mBaseObj);
                    std::cout << "base type "
                              << ESM4::printName(((baseType & 0xffffff00)>>8)|((baseType & 0xff)<<24))
                              << " parent formid " << ESM4::formIdToString(record.mEsp.parent) << std::endl;
                    // parent type appears to be REFR
                }
#endif
                break;
            }
            case ESM4::REC_ACHR:
            {
                if (0)//(reader.hdr().record.flags & ESM4::Rec_Disabled) != 0) // FIXME: temp testing
                {
                    reader.skipRecordData();
                    break;
                }

                bool deleted = (reader.hdr().record.flags & ESM4::Rec_Deleted) != 0;
                reader.getRecordData();
                ESM4::ActorCharacter record;
                record.load(reader);
                //if (!record.mEditorId.empty())
                    //std::cout << ESM4::printName(hdr.record.typeId) << ": " << record.mEditorId << std::endl; // FIXME

                switch (store.find(record.mBaseObj))
                {
                    case MKTAG('_','N','P','C'):
                    {
#if 0
                        // this might be a levelled actor (e.g. FO3) - check if the model is empty
                        const ESM4::Npc* npc = store.getForeign<ESM4::Npc>().search(record.mBaseObj);
                        //
                        //if ((npc->mBaseConfig.fo3.flags & 0x1) == 0) // FIXME: temp testing
                            //std::cout << npc->mEditorId << " male" << std::endl;
                        //
                        if (npc && (/*npc->mModel.empty() || */npc->mModel == "marker_creature.nif"))
                            for (size_t i = 0; i < npc->mInventory.size(); ++i)
                            {
                                uint32_t type = store.find(npc->mInventory[i].item);
                                if (type == MKTAG('K', 'B', 'O', 'O'))
                                //if (type == MKTAG('E', 'N', 'O', 'T'))
                                {
                        const ESM4::Book* note = store.getForeign<ESM4::Book>().search(npc->mInventory[i].item);
                        std::cout << npc->mEditorId << " " << note->mEditorId << std::endl;
                                }
                                else
                                    std::cout << npc->mEditorId << " " << ESM4::printName(type) << std::endl;
                            }
                        if (npc && npc->mBaseTemplate != 0
                                && (npc->mModel.empty() || npc->mModel == "marker_creature.nif"))
                        {
#if 0
            if (npc->mBaseTemplate != 0)
            {
                // has baseconfig, inventory, hair colour, hair length, race, class; all else 0
                const ESM4::Race* race = store.getForeign<ESM4::Race>().search(npc->mRace);
                const ESM4::Npc& newNpc = *npc;
                std::vector<const ESM4::Npc*> sts;
                sts.push_back(npc);
                bool isFemale = (npc->mBaseConfig.fo3.flags & 0x000001) != 0;
                //if (isFemale)
                std::cout << "ori " << newNpc.mEditorId << (isFemale? " female" : "") << std::endl;
                std::cout << race->mEditorId << std::endl;
                std::cout << ESM4::formIdToString(newNpc.mClass) << std::endl;
            }
#endif
                            ESM4::FormId id = npc->mBaseTemplate;
                            uint32_t type = store.find(id);
                            bool found = false;

                            while (!found) {
                                if (type == MKTAG('_', 'N', 'P', 'C'))
                                {
                                    // FIXME: for TES5 need to check male/female model
                                    record.mBaseObj = id;
                                    found = true;
#if 0 // 32 64 23 209, inv 2
            //bool isFemale = (record.mBaseConfig.fo3.flags & 0x000001) != 0;
            //std::cout << record.mEditorId << (isFemale? " female" : " male") << std::endl;
            const ESM4::Npc* newNpc = store.getForeign<ESM4::Npc>().search(id);
            const ESM4::Race* race = store.getForeign<ESM4::Race>().search(newNpc->mRace);
            std::vector<const ESM4::Npc*> sts;
            sts.push_back(newNpc);
            bool isFemale = (newNpc->mBaseConfig.fo3.flags & 0x000001) != 0;
            std::cout << newNpc->mEditorId << (isFemale? " female" : "") << std::endl;
            std::cout << race->mEditorId << std::endl;
            std::cout << ESM4::formIdToString(newNpc->mClass) << std::endl;
                            for (size_t i = 0; i < newNpc->mInventory.size(); ++i)
                            {
                                uint32_t type = store.find(newNpc->mInventory[i].item);
                                if (type == MKTAG('K', 'B', 'O', 'O'))
                                //if (type == MKTAG('E', 'N', 'O', 'T'))
                                {
                        const ESM4::Book* note = store.getForeign<ESM4::Book>().search(newNpc->mInventory[i].item);
                        std::cout << newNpc->mEditorId << " " << note->mEditorId << std::endl;
                                }
                                else
                                    std::cout << newNpc->mEditorId << " " << ESM4::printName(type) << std::endl;
                            }
#endif
                                }
                                else if (type == MKTAG('N', 'L', 'V', 'L'))
                                {
                                    const ESM4::LevelledNpc* lvlActor
                                        = store.getForeign<ESM4::LevelledNpc>().search(id);

                                    if (lvlActor && lvlActor->mLvlObject.size() != 0)
                                    {
                                        // FIXME: this should be based on player's level, etc
                                        id = lvlActor->mLvlObject[0].item;
                                        type = store.find(id);
                                    }
                                    else
                                        throw std::runtime_error ("levelled actor not found!");
                                }
                            }
                        }

#endif
#if 1
                        mForeignNpcs.load(record, deleted, store);
#else
        const MWWorld::ForeignStore<ESM4::Npc> &store = esmStore.getForeign<ESM4::Npc>();

        if (const ESM4::Npc *ptr = store.search (record.mBaseObj))
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
#endif
                        break;
                    }

                    case 0: std::cerr << "Cell achr " + ESM4::formIdToString(record.mBaseObj) + " not found!\n"; break;

                    default:
                        std::cerr
                            << "WARNING: Ignoring ACHR '" << ESM4::formIdToString(record.mBaseObj) << "' of unhandled type\n";
                }
                break;
            }
            case ESM4::REC_ACRE: // Oblivion only?
            {
                if (0)//(reader.hdr().record.flags & ESM4::Rec_Disabled) != 0) // FIXME: temp testing
                {
                    reader.skipRecordData();
                    break;
                }

                bool deleted = (reader.hdr().record.flags & ESM4::Rec_Deleted) != 0;
                reader.getRecordData();
                ESM4::ActorCreature record;
                record.load(reader);
                //if (!record.mEditorId.empty())
                    //std::cout << ESM4::printName(hdr.record.typeId) << ": " << record.mEditorId << std::endl; // FIXME

                switch (store.find(record.mBaseObj))
                {
                    case MKTAG('A','C','R','E'):
                    {
#if 0
                        // this might be a levelled creature (e.g. FO3) - check if the model is empty
                        const ESM4::Creature* crea = store.getForeign<ESM4::Creature>().search(record.mBaseObj);
                        if (crea && crea->mBaseTemplate != 0
                                 && (crea->mModel.empty() || crea->mModel == "marker_creature.nif"))
                        {
                            ESM4::FormId id = crea->mBaseTemplate;
                            uint32_t type = store.find(id);
                            bool found = false;

                            while (!found) {
                                if (type == MKTAG('A', 'C', 'R', 'E'))
                                {
                                    record.mBaseObj = id;
                                    found = true;
                                }
                                else if (type == MKTAG('C', 'L', 'V', 'L'))
                                {
                                    const ESM4::LevelledCreature* lvlCrea
                                        = store.getForeign<ESM4::LevelledCreature>().search(id);

                                    if (lvlCrea && lvlCrea->mLvlObject.size() != 0)
                                    {
                                        // FIXME: this should be based on player's level, etc
                                        id = lvlCrea->mLvlObject[0].item;
                                        type = store.find(id);
                                    }
                                    else
                                        throw std::runtime_error ("levelled creature not found!");
                                }
                            }
                        }

#else
                        mForeignCreatures.load(record, deleted, store);
#endif
                        break;
                    }

                    case 0: std::cerr << "Cell acre " + ESM4::formIdToString(record.mBaseObj) + " not found!\n"; break;

                    default:
                        std::cerr
                            << "WARNING: Ignoring ACRE '" << ESM4::formIdToString(record.mBaseObj) << "' of unhandled type\n";
                }
                break;
            }
            case ESM4::REC_LAND:
            {

                // Can't store land record in CellStore if we want to keep it around rather
                // than loading it each time like references.  But then, how do we keep it
                // growing too large?  Need some way of keeping track of "recently accessed"
                // cells.
                //
                // The other issue is that of const correctness. We only get access to a const
                // pointer or reference to the ESMStore...
                //
                // HACK: Workaround by going through a method in World
                //
                //ESM4::FormId worldId = mCell->mCell->mParent;
                //ESM4::FormId cellId  = mCell->mCell->mFormId;

                // Save context just in case
                //ESM4::ReaderContext ctx = static_cast<ESM::ESM4Reader*>(&esm)->getESM4Context();

                // Load land, note may not be used
                mForeignLand = MWBase::Environment::get().getWorld()->loadForeignLand(esm);

                // load parent world's land; but that triggers Tamriel to be loaded, is there another way?
#if 0
                const ForeignCell *cell = static_cast<const ForeignCell*>(mCell);
                const ForeignWorld *world = store.get<ForeignWorld>().find(cell->mCell->mParent);

                if (world && world->mParent != 0)
                {
                    CellStore * parentCell
                        = MWBase::Environment::get().getWorld()->getWorldCell(world->mParent,
                            mCell->getGridX(),
                            mCell->getGridY());

                    if (parentCell)
                        mForeignLand = parentCell->getForeignLandId();
                }
#endif
                break;
            }
            case ESM4::REC_PGRD: // TES4 only
            {
                bool deleted = (reader.hdr().record.flags & ESM4::Rec_Deleted) != 0;
                ESM4::FormId cellId = static_cast<const MWWorld::ForeignCell*>(mCell)->mCell->mFormId;

                // SEtestGateRoom in Oblivion.esm has broken PGRD sub-record
                if (deleted || cellId == 0x000132b1)
                {
                    reader.skipRecordData();
                }
                else
                {
                    reader.getRecordData();
                    mForeignPathgrids.load(esm);

                    mPathgridGraph.load(this);
                    buildTES3Pathgrid(); // FIXME: just a workaround
                }

                break;
            }
            case ESM4::REC_ROAD: // TES4 only
            case ESM4::REC_NAVM:
            {
                //std::string padding = "";
                //padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                //std::cout << padding << ESM4::printName(hdr.record.typeId) << " skipping..." << std::endl;
                reader.skipRecordData();
                break;
            }
            case ESM4::REC_PGRE: // FO3/FONV
            {
#if 0
                reader.getRecordData();
                ESM4::PlacedGrenade record;
                record.load(reader);
                std::string padding = "";
                padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                std::cout << padding << ESM4::printName(hdr.record.typeId) << " " << record.mEditorId << std::endl;
#else
                //std::string padding = "";
                //padding.insert(0, reader.getContext().groupStack.size()*2, ' ');
                //std::cout << padding << ESM4::printName(hdr.record.typeId) << " skipping..." << std::endl;
                reader.skipRecordData();
#endif
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

    void CellStore::setLoadedState()
    {
        if (mState!=State_Loaded)
        {
            // first create a grid index



            mState = State_Loaded;
        }
        else
            throw std::runtime_error ("CellStore: unexpected call to setLoadedState");
    }

    // used by MWRender::Debugging
    const ESM4::Pathgrid *CellStore::getTES4Pathgrid() const
    {
        // NOTE: cellId is not what we want - we need the Pathgrid FormId
        ESM4::FormId cellId = static_cast<const MWWorld::ForeignCell*>(mCell)->mCell->mFormId;

        // FIXME: can there be more than one? if not why do we need a ForeignStore?
        // Is there a way to store these in ESMStore? (we only get a const reference)
        std::vector<ESM4::FormId> list;
        mForeignPathgrids.listForeignIdentifier(list);
        if (list.size())
        {
            //std::cout << "CELL formId " << ESM4::formIdToString(cellId) << std::endl;
            //std::cout << "PGRD formId " << ESM4::formIdToString(list[0]) << std::endl;
            if (list.size() > 1)
                std::cout << cellId << " cellid Pathgrid more than one " << list.size() << std::endl;

            return mForeignPathgrids.search(list[0]);
        }
        else
            return nullptr;
    }

    // used by MWMechanics::PathgridGraph
    const ESM::Pathgrid *CellStore::getTES3Pathgrid() const
    {
        return &mPathgrid;
    }

    // FIXME: quick workaround to avoid rewriting code, however it is likely that new logic for
    //        connecting to roads and other external cells will be required eventually
    void CellStore::buildTES3Pathgrid()
    {
        std::vector<ESM4::FormId> list;
        mForeignPathgrids.listForeignIdentifier(list);
        if (!list.size())
            return;

        const ESM4::Pathgrid *pathgrid = mForeignPathgrids.search(list[0]);

        mPathgrid.mData.mX = static_cast<const MWWorld::ForeignCell*>(mCell)->getGridX();
        mPathgrid.mData.mY = static_cast<const MWWorld::ForeignCell*>(mCell)->getGridY();
        mPathgrid.mData.mS2 = pathgrid->mData;

        mPathgrid.mPoints.resize(pathgrid->mNodes.size());
        for (std::size_t i = 0; i < pathgrid->mNodes.size(); ++i)
        {
            mPathgrid.mPoints.at(i).mX = pathgrid->mNodes[i].x;
            mPathgrid.mPoints.at(i).mY = pathgrid->mNodes[i].y;
            mPathgrid.mPoints.at(i).mZ = pathgrid->mNodes[i].z;
            mPathgrid.mPoints.at(i).mAutogenerated = pathgrid->mNodes[i].priority;
            mPathgrid.mPoints.at(i).mConnectionNum = pathgrid->mNodes[i].numLinks;
        }

        mPathgrid.mEdges.resize(pathgrid->mLinks.size());
        for (std::size_t i = 0; i < pathgrid->mLinks.size(); ++i)
        {
            mPathgrid.mEdges.at(i).mV0 = pathgrid->mLinks[i].startNode;
            mPathgrid.mEdges.at(i).mV1 = pathgrid->mLinks[i].endNode;
        }
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
