#ifndef GAME_MWWORLD_CELLSTORE_H
#define GAME_MWWORLD_CELLSTORE_H

#include <algorithm>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <boost/shared_ptr.hpp>

#include <extern/esm4/records.hpp>

#include "livecellref.hpp"
#include "cellreflist.hpp"

#include <components/esm/fogstate.hpp>
#include <components/esm/records.hpp>

#include "../mwmechanics/pathgrid.hpp"  // TODO: maybe belongs in mwworld

#include "foreignstore.hpp"
#include "timestamp.hpp"

namespace ESM4
{
    union RecordHeader;
}

namespace ESM
{
    struct CellState;
    struct FogState;
}

namespace MWWorld
{
    class Ptr;
    class ESMStore;
    struct ForeignCell;


    /// \brief Mutable state of a cell
    class CellStore
    {
        public:

            enum State
            {
                State_Unloaded, State_Preloaded, State_Loaded
            };

        private:

            // Even though fog actually belongs to the player and not cells,
            // it makes sense to store it here since we need it once for each cell.
            // Note this is NULL until the cell is explored to save some memory
            boost::shared_ptr<ESM::FogState> mFogState;

            const ESM::Cell *mCell;
            State mState;
            bool mHasState;
            std::vector<std::string> mIds;
            std::vector<ESM4::FormId> mForeignIds;
            float mWaterLevel;
            //
            bool mIsForeignCell;
            bool mIsDummyCell;
            bool mIsVisibleDistCell;
            ESM4::FormId mForeignLand; // ForeignCell only, can't store in mCell due to const ptr
            ESM4::FormId mAudioLocation; // from REFR, interior only

            MWWorld::TimeStamp mLastRespawn;

            CellRefList<ESM::Activator>         mActivators;
            CellRefList<ESM::Potion>            mPotions;
            CellRefList<ESM::Apparatus>         mAppas;
            CellRefList<ESM::Armor>             mArmors;
            CellRefList<ESM::Book>              mBooks;
            CellRefList<ESM::Clothing>          mClothes;
            CellRefList<ESM::Container>         mContainers;
            CellRefList<ESM::Creature>          mCreatures;
            CellRefList<ESM::Door>              mDoors;
            CellRefList<ESM::Ingredient>        mIngreds;
            CellRefList<ESM::CreatureLevList>   mCreatureLists;
            CellRefList<ESM::ItemLevList>       mItemLists;
            CellRefList<ESM::Light>             mLights;
            CellRefList<ESM::Lockpick>          mLockpicks;
            CellRefList<ESM::Miscellaneous>     mMiscItems;
            CellRefList<ESM::NPC>               mNpcs;
            CellRefList<ESM::Probe>             mProbes;
            CellRefList<ESM::Repair>            mRepairs;
            CellRefList<ESM::Static>            mStatics;
            CellRefList<ESM::Weapon>            mWeapons;
            //
            CellRefVect<ESM4::Sound>            mSounds;
            CellRefVect<ESM4::Activator>        mForeignActivators;
            CellRefVect<ESM4::Apparatus>        mForeignApparatus;
            CellRefVect<ESM4::Armor>            mForeignArmors;
            CellRefVect<ESM4::Book>             mForeignBooks;
            CellRefVect<ESM4::Clothing>         mForeignClothes;
            CellRefVect<ESM4::Container>        mForeignContainers;
            CellRefVect<ESM4::Door>             mForeignDoors;
            CellRefVect<ESM4::Ingredient>       mForeignIngredients;
            CellRefVect<ESM4::Light>            mForeignLights;
            CellRefVect<ESM4::MiscItem>         mForeignMiscItems;
            CellRefVect<ESM4::Static>           mForeignStatics;
            CellRefVect<ESM4::Grass>            mForeignGrasses;
            CellRefVect<ESM4::Tree>             mForeignTrees;
            CellRefVect<ESM4::Flora>            mForeignFloras;
            CellRefVect<ESM4::Furniture>        mForeignFurnitures;
            CellRefVect<ESM4::Weapon>           mForeignWeapons;
            CellRefVect<ESM4::Ammunition>       mAmmunitions;
            CellRefVect<ESM4::Npc>              mForeignNpcs;
            CellRefVect<ESM4::Creature>         mForeignCreatures;
            CellRefVect<ESM4::LevelledCreature>  mLevelledCreatures;
            CellRefVect<ESM4::IdleMarker>       mIdleMarkers;
            CellRefVect<ESM4::SoulGem>          mSoulGems;
            CellRefVect<ESM4::Key>              mForeignKeys;
            CellRefVect<ESM4::Potion>           mForeignPotions;
            CellRefVect<ESM4::SubSpace>         mSubSpaces;
            CellRefVect<ESM4::SigilStone>       mSigilStones;
            CellRefVect<ESM4::LevelledItem>     mLevelledItems;
            CellRefVect<ESM4::LevelledNpc>      mLevelledNpcs;
            CellRefVect<ESM4::AcousticSpace>    mAcousticSpaces;
            CellRefVect<ESM4::MovableStatic>    mMovableStatics;
            CellRefVect<ESM4::Terminal>         mTerminals;
            CellRefVect<ESM4::TalkingActivator>  mTalkingActivators;
            CellRefVect<ESM4::Note>             mNotes;
            CellRefVect<ESM4::PlaceableWater>   mPlaceableWaters;
            CellRefVect<ESM4::StaticCollection> mStaticCollections;

            std::map<int/*store type*/, CellRefStoreBase*> mStores;
            std::map<ESM4::FormId, int> mStoreTypes;
            int getStoreType(ESM4::FormId formId) const;

            std::map<std::string, ESM4::FormId> mSceneNodeMap; // for searching via handle

            ForeignStore<ESM4::Pathgrid>        mForeignPathgrids;
            ESM::Pathgrid mPathgrid; // FIXME: just a quick workaround
            void buildTES3Pathgrid();

            void loadTes4Group (const MWWorld::ESMStore& store, ESM::ESMReader& esm);

        public:

            CellStore (const ESM::Cell *cell_, bool isForeignCell = false, bool isDummyCell = false);

            const ESM::Cell *getCell() const;
            inline const bool isForeignCell() const { return mIsForeignCell; }
            inline const bool isDummyCell() const { return mIsDummyCell; }
            inline const bool isVisibleDistCell() const { return mIsVisibleDistCell; }
            void setVisibleDistCell() { mIsVisibleDistCell = true; }
            inline ESM4::FormId getAudioLocation() const { return mAudioLocation; }

            inline ESM4::FormId getForeignLandId() const { return mForeignLand; }
            void loadTes4Record (const MWWorld::ESMStore& store, ESM::ESMReader& esm);
            void setLoadedState();

            State getState() const;

            bool hasState() const;
            ///< Does this cell have state that needs to be stored in a saved game file?

            bool hasFormId (ESM4::FormId formId) const;
            bool hasId (const std::string& id) const;
            ///< May return true for deleted IDs when in preload state. Will return false, if cell is
            /// unloaded.

            //Ptr search (ESM4::FormId formId);
            Ptr search (const std::string& id);
            ///< Will return an empty Ptr if cell is not loaded. Does not check references in
            /// containers.

            Ptr searchViaHandle (const std::string& handle);
            ///< Will return an empty Ptr if cell is not loaded.

            Ptr searchViaActorId (int id);
            ///< Will return an empty Ptr if cell is not loaded.

            float getWaterLevel() const;

            void setWaterLevel (float level);

            void setFog (ESM::FogState* fog);
            ///< \note Takes ownership of the pointer

            ESM::FogState* getFog () const;

            int count() const;
            ///< Return total number of references, including deleted ones.
            int getRefrEstimate(std::int32_t groupType) const;
            int getPersistentRefrCount() const;

            void load (const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm);
            ///< Load references from content file.

            void preload (const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm);
            ///< Build ID list from content file.

            /// Call functor (ref) for each reference. functor must return a bool. Returning
            /// false will abort the iteration.
            /// \attention This function also lists deleted (count 0) objects!
            /// \return Iteration completed?
            ///
            /// \note Creatures and NPCs are handled last.
            template<class Functor>
            bool forEach (Functor& functor)
            {
                mHasState = true;

                return
                    forEachImp (functor, mActivators) &&
                    forEachImp (functor, mPotions) &&
                    forEachImp (functor, mAppas) &&
                    forEachImp (functor, mArmors) &&
                    forEachImp (functor, mBooks) &&
                    forEachImp (functor, mClothes) &&
                    forEachImp (functor, mContainers) &&
                    forEachImp (functor, mDoors) &&
                    forEachImp (functor, mIngreds) &&
                    forEachImp (functor, mItemLists) &&
                    forEachImp (functor, mLights) &&
                    forEachImp (functor, mLockpicks) &&
                    forEachImp (functor, mMiscItems) &&
                    forEachImp (functor, mProbes) &&
                    forEachImp (functor, mRepairs) &&
                    forEachImp (functor, mStatics) &&
                    forEachImp (functor, mWeapons) &&
                    forEachImp (functor, mCreatures) &&
                    forEachImp (functor, mNpcs) &&
                    forEachImp (functor, mCreatureLists) &&
                    // FIXME: don't understand why this is needed here
                    forEachImpForeign(functor, 0, 0, 0, 0, mSounds) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignActivators) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignApparatus) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignArmors) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignBooks) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignClothes) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignContainers) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignDoors) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignIngredients) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignLights) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignMiscItems) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignStatics) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignGrasses) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignTrees) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignFloras) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignFurnitures) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignWeapons) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mAmmunitions) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mSoulGems) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignKeys) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignPotions) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mSubSpaces) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mSigilStones) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mLevelledItems) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mTerminals) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mTalkingActivators) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mNotes) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mPlaceableWaters) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mStaticCollections) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignNpcs) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mForeignCreatures) &&
                    forEachImpForeign(functor, 0, 0, 0, 0, mLevelledCreatures);
            }

            // FIXME: not sure why Creatures and NPCs are handled last.
            template<class Functor>
            bool forEachForeign (Functor& functor, int x=0, int y=0, size_t range=0, size_t exclude=0)
            {
                mHasState = true;

                return
                    forEachImpForeign(functor, x, y, range, exclude, mSounds) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignActivators) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignApparatus) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignArmors) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignBooks) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignClothes) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignContainers) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignDoors) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignIngredients) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignLights) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignMiscItems) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignStatics) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignGrasses) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignTrees) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignFloras) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignFurnitures) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignWeapons) &&
                    forEachImpForeign(functor, x, y, range, exclude, mAmmunitions) &&
                    forEachImpForeign(functor, x, y, range, exclude, mSoulGems) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignKeys) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignPotions) &&
                    forEachImpForeign(functor, x, y, range, exclude, mSubSpaces) &&
                    forEachImpForeign(functor, x, y, range, exclude, mSigilStones) &&
                    forEachImpForeign(functor, x, y, range, exclude, mLevelledItems) &&
                    forEachImpForeign(functor, x, y, range, exclude, mTerminals) &&
                    forEachImpForeign(functor, x, y, range, exclude, mTalkingActivators) &&
                    forEachImpForeign(functor, x, y, range, exclude, mNotes) &&
                    forEachImpForeign(functor, x, y, range, exclude, mPlaceableWaters) &&
                    forEachImpForeign(functor, x, y, range, exclude, mStaticCollections) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignNpcs) &&
                    forEachImpForeign(functor, x, y, range, exclude, mForeignCreatures) &&
                    forEachImpForeign(functor, x, y, range, exclude, mLevelledCreatures);
            }

            template<class Functor>
            bool forEachContainer (Functor& functor)
            {
                mHasState = true;

                return
                    forEachImp (functor, mContainers) &&
                    forEachImp (functor, mCreatures) &&
                    forEachImp (functor, mNpcs);

                // FIXME: foreign
            }

            bool isExterior() const;

            Ptr searchInContainer (const std::string& id);

            void loadState (const ESM::CellState& state);

            void saveState (ESM::CellState& state) const;

            void writeFog (ESM::ESMWriter& writer) const;

            void readFog (ESM::ESMReader& reader);

            void writeReferences (ESM::ESMWriter& writer) const;

            void readReferences (ESM::ESMReader& reader, const std::map<int, int>& contentFileMap);

            void respawn ();
            ///< Check mLastRespawn and respawn references if necessary. This is a no-op if the cell is not loaded.

            template <class T>
            CellRefList<T>& get() {
                throw std::runtime_error ("Storage for type " + std::string(typeid(T).name())+ " does not exist in cells");
            }

            template <class T>
            const CellRefList<T>& getReadOnly() {
                throw std::runtime_error ("Read Only CellRefList access not available for type " + std::string(typeid(T).name()) );
            }

            template <class T>
            CellRefVect<T>& getForeign() {
                throw std::runtime_error ("Storage for type " + std::string(typeid(T).name()) + " does not exist in cells");
            }

            template <class T>
            const CellRefVect<T>& getForeignReadOnly() {
                throw std::runtime_error ("Read Only CellRefVect access not available for type " + std::string(typeid(T).name()) );
            }

            bool isPointConnected(const int start, const int end) const;

            std::list<ESM::Pathgrid::Point> aStarSearch(const int start, const int end) const;

            const ESM4::Pathgrid *getTES4Pathgrid() const;
            const ESM::Pathgrid *getTES3Pathgrid() const;

        private:

            template<class Functor, class List>
            bool forEachImp (Functor& functor, List& list)
            {
                for (typename List::List::iterator iter (list.mList.begin()); iter!=list.mList.end();
                    ++iter)
                {
                    if (iter->mData.isDeletedByContentFile())
                        continue;
                    if (!functor (MWWorld::Ptr(&*iter, this)))
                        return false;
                }
                return true;
            }

            template<class Functor, class List>
            bool forEachImpForeign(Functor& functor,
                    int x, int y, std::size_t range, std::size_t exclude, List& vect)
            {
#if 1
                for (typename List::List::iterator iter (vect.mList.begin()); iter!=vect.mList.end();
                    ++iter)
                {
                    if (iter->mData.isDeletedByContentFile())
                        continue;
                    if (!functor (MWWorld::Ptr(&*iter, this)))
                        return false;
                }
                return true;
#else
                if (range == 0 && exclude == 0) // FIXME: hack for non-dummy
                {
                    for (std::vector<LiveCellRef<X> >::iterator iter(vect.mList.begin());
                            iter != vect.mList.end(); ++iter)
                    {
                        if (iter->mData.isDeletedByContentFile())
                            continue;

                        MWWorld::Ptr ptr(&*iter, this);
                        bool res = functor(ptr); // we should have an Ogre::SceneNode handle after this
                        if (!res)
                            return false;

                        ESM4::FormId formId = (*iter).mRef.getFormId();

                        mForeignIds.push_back(formId);  // for hasFormId()

                        if ((*iter).mData.getBaseNode())
                        {
                            std::string handle = (*iter).mData.getHandle(); // see Objects::insertBegin()
                            mSceneNodeMap[handle] = formId; // for searchViaHandle()
                        }
                    }
                }
                else
                {
                    std::vector<LiveCellRefBase*> sublist = vect.search(x, y, range, exclude);

                    for (std::vector<LiveCellRefBase*>::iterator iter(sublist.begin());
                            iter != sublist.end(); ++iter)
                    {
                        if (static_cast<LiveCellRef<X>*>(*iter)->mData.isDeletedByContentFile())
                            continue;

                        //ESM::Position pos = (*iter)->mRef.getPosition();
                        //CellStore *cell
                        //    = MWBase::Environment::get().getWorld()->getWorldCell(mWorldId, pos.pos[0], pos.pos[1]);

                        MWWorld::Ptr ptr(*iter, this/*cell*/);
                        bool res = functor(ptr); // we should have an Ogre::SceneNode handle after this
                        if (!res)
                            return false;

                        ESM4::FormId formId = (*iter)->mRef.getFormId();

                        mForeignIds.push_back(formId);  // for hasFormId()

                        if (exclude == 0 && // do this for those with collision shapes only ?
                                (*iter)->mData.getBaseNode()) // and if it was actually created
                        {
                            std::string handle = (*iter)->mData.getHandle(); // see Objects::insertBegin()
                            mSceneNodeMap[handle] = formId; // for searchViaHandle()
                        }

                        //MWBase::Environment::get().getWorld()->moveObject(ptr, cell, pos.pos[0], pos.pos[1], pos.pos[2]);
                    }
                }

                return true;
#endif
            }

            /// Run through references and store IDs
            void listRefs(const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm);

            void loadRefs(const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm);
            void loadForeignRefs(const MWWorld::ESMStore &store, std::vector<std::vector<ESM::ESMReader*> > &esm);

            void loadRef (ESM::CellRef& ref, bool deleted, const ESMStore& store);
            ///< Make case-adjustments to \a ref and insert it into the respective container.
            ///
            /// Invalid \a ref objects are silently dropped.

            MWMechanics::PathgridGraph mPathgridGraph;
    };

    template<>
    inline CellRefList<ESM::Activator>& CellStore::get<ESM::Activator>()
    {
        mHasState = true;
        return mActivators;
    }

    template<>
    inline CellRefList<ESM::Potion>& CellStore::get<ESM::Potion>()
    {
        mHasState = true;
        return mPotions;
    }

    template<>
    inline CellRefList<ESM::Apparatus>& CellStore::get<ESM::Apparatus>()
    {
        mHasState = true;
        return mAppas;
    }

    template<>
    inline CellRefList<ESM::Armor>& CellStore::get<ESM::Armor>()
    {
        mHasState = true;
        return mArmors;
    }

    template<>
    inline CellRefList<ESM::Book>& CellStore::get<ESM::Book>()
    {
        mHasState = true;
        return mBooks;
    }

    template<>
    inline CellRefList<ESM::Clothing>& CellStore::get<ESM::Clothing>()
    {
        mHasState = true;
        return mClothes;
    }

    template<>
    inline CellRefList<ESM::Container>& CellStore::get<ESM::Container>()
    {
        mHasState = true;
        return mContainers;
    }

    template<>
    inline CellRefList<ESM::Creature>& CellStore::get<ESM::Creature>()
    {
        mHasState = true;
        return mCreatures;
    }

    template<>
    inline CellRefList<ESM::Door>& CellStore::get<ESM::Door>()
    {
        mHasState = true;
        return mDoors;
    }

    template<>
    inline CellRefList<ESM::Ingredient>& CellStore::get<ESM::Ingredient>()
    {
        mHasState = true;
        return mIngreds;
    }

    template<>
    inline CellRefList<ESM::CreatureLevList>& CellStore::get<ESM::CreatureLevList>()
    {
        mHasState = true;
        return mCreatureLists;
    }

    template<>
    inline CellRefList<ESM::ItemLevList>& CellStore::get<ESM::ItemLevList>()
    {
        mHasState = true;
        return mItemLists;
    }

    template<>
    inline CellRefList<ESM::Light>& CellStore::get<ESM::Light>()
    {
        mHasState = true;
        return mLights;
    }

    template<>
    inline CellRefList<ESM::Lockpick>& CellStore::get<ESM::Lockpick>()
    {
        mHasState = true;
        return mLockpicks;
    }

    template<>
    inline CellRefList<ESM::Miscellaneous>& CellStore::get<ESM::Miscellaneous>()
    {
        mHasState = true;
        return mMiscItems;
    }

    template<>
    inline CellRefList<ESM::NPC>& CellStore::get<ESM::NPC>()
    {
        mHasState = true;
        return mNpcs;
    }

    template<>
    inline CellRefList<ESM::Probe>& CellStore::get<ESM::Probe>()
    {
        mHasState = true;
        return mProbes;
    }

    template<>
    inline CellRefList<ESM::Repair>& CellStore::get<ESM::Repair>()
    {
        mHasState = true;
        return mRepairs;
    }

    template<>
    inline CellRefList<ESM::Static>& CellStore::get<ESM::Static>()
    {
        mHasState = true;
        return mStatics;
    }

    template<>
    inline CellRefList<ESM::Weapon>& CellStore::get<ESM::Weapon>()
    {
        mHasState = true;
        return mWeapons;
    }

    template<>
    inline const CellRefList<ESM::Door>& CellStore::getReadOnly<ESM::Door>()
    {
        return mDoors;
    }

    template<>
    inline const CellRefVect<ESM4::Activator>& CellStore::getForeignReadOnly<ESM4::Activator>()
    {
        return mForeignActivators;
    }

    template<>
    inline CellRefVect<ESM4::Apparatus>& CellStore::getForeign<ESM4::Apparatus>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignApparatus;
    }

    template<>
    inline CellRefVect<ESM4::Armor>& CellStore::getForeign<ESM4::Armor>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignArmors;
    }

    template<>
    inline CellRefVect<ESM4::Book>& CellStore::getForeign<ESM4::Book>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignBooks;
    }

    template<>
    inline CellRefVect<ESM4::Clothing>& CellStore::getForeign<ESM4::Clothing>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignClothes;
    }

    template<>
    inline CellRefVect<ESM4::Door>& CellStore::getForeign<ESM4::Door>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignDoors;
    }

    template<>
    inline const CellRefVect<ESM4::Door>& CellStore::getForeignReadOnly<ESM4::Door>()
    {
        return mForeignDoors;
    }

    template<>
    inline CellRefVect<ESM4::Ingredient>& CellStore::getForeign<ESM4::Ingredient>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignIngredients;
    }

    template<>
    inline CellRefVect<ESM4::Light>& CellStore::getForeign<ESM4::Light>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignLights;
    }

    template<>
    inline CellRefVect<ESM4::MiscItem>& CellStore::getForeign<ESM4::MiscItem>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignMiscItems;
    }

    template<>
    inline CellRefVect<ESM4::Static>& CellStore::getForeign<ESM4::Static>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignStatics;
    }

    template<>
    inline const CellRefVect<ESM4::Static>& CellStore::getForeignReadOnly<ESM4::Static>()
    {
        return mForeignStatics;
    }

    template<>
    inline CellRefVect<ESM4::Weapon>& CellStore::getForeign<ESM4::Weapon>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignWeapons;
    }

    template<>
    inline CellRefVect<ESM4::Ammunition>& CellStore::getForeign<ESM4::Ammunition>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mAmmunitions;
    }

    template<>
    inline CellRefVect<ESM4::Npc>& CellStore::getForeign<ESM4::Npc>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignNpcs;
    }

    template<>
    inline CellRefVect<ESM4::Creature>& CellStore::getForeign<ESM4::Creature>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignCreatures;
    }

    template<>
    inline CellRefVect<ESM4::SoulGem>& CellStore::getForeign<ESM4::SoulGem>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mSoulGems;
    }

    template<>
    inline CellRefVect<ESM4::Key>& CellStore::getForeign<ESM4::Key>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignKeys;
    }

    template<>
    inline CellRefVect<ESM4::Potion>& CellStore::getForeign<ESM4::Potion>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignPotions;
    }

    template<>
    inline CellRefVect<ESM4::SigilStone>& CellStore::getForeign<ESM4::SigilStone>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mSigilStones;
    }

    template<>
    inline const CellRefVect<ESM4::TalkingActivator>& CellStore::getForeignReadOnly<ESM4::TalkingActivator>()
    {
        return mTalkingActivators;
    }

    template<>
    inline CellRefVect<ESM4::Note>& CellStore::getForeign<ESM4::Note>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mNotes;
    }

    bool operator== (const CellStore& left, const CellStore& right);
    bool operator!= (const CellStore& left, const CellStore& right);
}

#endif
