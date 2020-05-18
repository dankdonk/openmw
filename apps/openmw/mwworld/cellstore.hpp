#ifndef GAME_MWWORLD_CELLSTORE_H
#define GAME_MWWORLD_CELLSTORE_H

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
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
            std::vector<ESM4::FormId> mForeignIds; // FIXME: currently unused
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
            CellRefList<ESM4::Sound>            mSounds;
            CellRefList<ESM4::Activator>        mForeignActivators;
            CellRefList<ESM4::Apparatus>        mForeignApparatus;
            CellRefList<ESM4::Armor>            mForeignArmors;
            CellRefList<ESM4::Book>             mForeignBooks;
            CellRefList<ESM4::Clothing>         mForeignClothes;
            CellRefList<ESM4::Container>        mForeignContainers;
            CellRefList<ESM4::Door>             mForeignDoors;
            CellRefList<ESM4::Ingredient>       mForeignIngredients;
            CellRefList<ESM4::Light>            mForeignLights;
            CellRefList<ESM4::MiscItem>         mForeignMiscItems;
            CellRefList<ESM4::Static>           mForeignStatics;
            CellRefList<ESM4::Grass>            mForeignGrasses;
            CellRefList<ESM4::Tree>             mForeignTrees;
            CellRefList<ESM4::Flora>            mForeignFloras;
            CellRefList<ESM4::Furniture>        mForeignFurnitures;
            CellRefList<ESM4::Weapon>           mForeignWeapons;
            CellRefList<ESM4::Ammunition>       mAmmunitions;
            CellRefList<ESM4::Npc>              mForeignNpcs;
            CellRefList<ESM4::Creature>         mForeignCreatures;
            CellRefList<ESM4::LevelledCreature>  mLevelledCreatures;
            CellRefList<ESM4::IdleMarker>       mIdleMarkers;
            CellRefList<ESM4::SoulGem>          mSoulGems;
            CellRefList<ESM4::Key>              mForeignKeys;
            CellRefList<ESM4::Potion>           mForeignPotions;
            CellRefList<ESM4::SubSpace>         mSubSpaces;
            CellRefList<ESM4::SigilStone>       mSigilStones;
            CellRefList<ESM4::LevelledItem>     mLevelledItems;
            CellRefList<ESM4::LevelledNpc>      mLevelledNpcs;
            CellRefList<ESM4::AcousticSpace>    mAcousticSpaces;
            CellRefList<ESM4::MovableStatic>    mMovableStatics;
            CellRefList<ESM4::Terminal>         mTerminals;
            CellRefList<ESM4::TalkingActivator>  mTalkingActivators;
            CellRefList<ESM4::Note>             mNotes;
            CellRefList<ESM4::PlaceableWater>   mPlaceableWaters;
            CellRefList<ESM4::StaticCollection> mStaticCollections;

            std::map<int/*store type*/, CellRefStoreBase*> mStores;
            std::map<ESM4::FormId, int> mStoreTypes;
            int getStoreType(ESM4::FormId formId) const;

            std::unordered_map<std::string, ESM4::FormId> mSceneNodeMap; // for searching via handle

            //std::vector<LiveCellRefBase*> mDummyActive;
            //std::vector<LiveCellRefBase*> mDummyVisible;

            ForeignStore<ESM4::Pathgrid>        mForeignPathgrids;
            ESM::Pathgrid mPathgrid; // FIXME: just a quick workaround
            void buildTES3Pathgrid();

            void loadTes4Group (const MWWorld::ESMStore& store, ESM::ESMReader& esm);

        public:

            CellStore (const ESM::Cell *cell_, bool isForeignCell = false, bool isDummyCell = false);
            //~CellStore ();
            CellStore (const CellStore& other) = delete;
            CellStore& operator=(const CellStore& other) = delete;
            CellStore (CellStore&& other); // move
            CellStore& operator=(CellStore&& other); // move

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

            Ptr search (ESM4::FormId formId);
            Ptr search (const std::string& id);
            ///< Will return an empty Ptr if cell is not loaded. Does not check references in
            /// containers.

            // TODO: manual updates are error prone - is there another way?
            void removeLookupKeys (const std::string& handle, ESM4::FormId formId);
            void updateHandleMap (const std::string& handle, ESM4::FormId formId); // update mSceneNodeMap
            // update mForeignIds, mStoreTypes and mSceneNodeMap
            void updateLookupMaps (ESM4::FormId formId, LiveCellRefBase *ref = nullptr, int storeType = -1);

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

                return (!mIsForeignCell ? (
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
                    forEachImp (functor, mCreatureLists)
                    ) : (
                    // in order to use ListAndResetHandles in Scene::unloadCell()
                    // we need to add below stores
                    forEachImp (functor, mSounds) &&
                    forEachImp (functor, mForeignActivators) &&
                    forEachImp (functor, mForeignApparatus) &&
                    forEachImp (functor, mForeignArmors) &&
                    forEachImp (functor, mForeignBooks) &&
                    forEachImp (functor, mForeignClothes) &&
                    forEachImp (functor, mForeignContainers) &&
                    forEachImp (functor, mForeignDoors) &&
                    forEachImp (functor, mForeignIngredients) &&
                    forEachImp (functor, mForeignLights) &&
                    forEachImp (functor, mForeignMiscItems) &&
                    forEachImp (functor, mForeignStatics) &&
                    forEachImp (functor, mForeignGrasses) &&
                    forEachImp (functor, mForeignTrees) &&
                    forEachImp (functor, mForeignFloras) &&
                    forEachImp (functor, mForeignFurnitures) &&
                    forEachImp (functor, mForeignWeapons) &&
                    forEachImp (functor, mAmmunitions) &&
                    forEachImp (functor, mSoulGems) &&
                    forEachImp (functor, mForeignKeys) &&
                    forEachImp (functor, mForeignPotions) &&
                    forEachImp (functor, mSubSpaces) &&
                    forEachImp (functor, mSigilStones) &&
                    forEachImp (functor, mLevelledItems) &&
                    forEachImp (functor, mTerminals) &&
                    forEachImp (functor, mTalkingActivators) &&
                    forEachImp (functor, mNotes) &&
                    forEachImp (functor, mPlaceableWaters) &&
                    forEachImp (functor, mStaticCollections) &&
                    forEachImp (functor, mForeignNpcs) &&
                    forEachImp (functor, mForeignCreatures) &&
                    forEachImp (functor, mLevelledCreatures)
                    ));
            }

            // FIXME: not sure why Creatures and NPCs are handled last.
            template<class Functor>
            bool forEachDummy (Functor& functor, int x = 0, int y = 0, size_t range = 0, size_t exclude = 0)
            {
                mHasState = true;

                return
                    forEachDummyImp (functor, mSounds,             x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignActivators,  x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignApparatus,   x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignArmors,      x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignBooks,       x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignClothes,     x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignContainers,  x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignDoors,       x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignIngredients, x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignLights,      x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignMiscItems,   x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignStatics,     x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignGrasses,     x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignTrees,       x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignFloras,      x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignFurnitures,  x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignWeapons,     x, y, range, exclude) &&
                    forEachDummyImp (functor, mAmmunitions,        x, y, range, exclude) &&
                    forEachDummyImp (functor, mSoulGems,           x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignKeys,        x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignPotions,     x, y, range, exclude) &&
                    forEachDummyImp (functor, mSubSpaces,          x, y, range, exclude) &&
                    forEachDummyImp (functor, mSigilStones,        x, y, range, exclude) &&
                    forEachDummyImp (functor, mLevelledItems,      x, y, range, exclude) &&
                    forEachDummyImp (functor, mTerminals,          x, y, range, exclude) &&
                    forEachDummyImp (functor, mTalkingActivators,  x, y, range, exclude) &&
                    forEachDummyImp (functor, mNotes,              x, y, range, exclude) &&
                    forEachDummyImp (functor, mPlaceableWaters,    x, y, range, exclude) &&
                    forEachDummyImp (functor, mStaticCollections,  x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignNpcs,        x, y, range, exclude) &&
                    forEachDummyImp (functor, mForeignCreatures,   x, y, range, exclude) &&
                    forEachDummyImp (functor, mLevelledCreatures,  x, y, range, exclude);
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
            CellRefList<T>& getForeign() {
                throw std::runtime_error ("Storage for type " + std::string(typeid(T).name()) + " does not exist in cells");
            }

            template <class T>
            const CellRefList<T>& getForeignReadOnly() {
                throw std::runtime_error ("Read Only CellRefList access not available for type " + std::string(typeid(T).name()) );
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
            bool forEachDummyImp(Functor& functor, List& vect,
                    int x = 0, int y = 0, std::size_t range = 0, std::size_t exclude = 0, bool insert = true)
            {
// index update is now triggered from the caller (i.e. MWWorld::Scene)
#if 0
                //if (range == 0 && exclude == 0) // FIXME: hack for non-dummy
                if (!mIsDummyCell)
                {
                    for (typename List::List::iterator iter(vect.mList.begin());
                            iter != vect.mList.end(); ++iter)
                    {
                        if (iter->mData.isDeletedByContentFile())
                            continue;

                        // should have an Ogre::SceneNode handle after this;
                        // see Objects::insertBegin() and Actors::insertBegin()
                        if (!functor(MWWorld::Ptr(&*iter, this)))
                            return false;

                        ESM4::FormId formId = (*iter).mRef.getFormId();
                        // FIXME: currently unused because getPtr() only uses hasId()
                      //mForeignIds.push_back(formId);  // for hasFormId()

                        if ((*iter).mData.getBaseNode())
                        {
                            std::string handle = (*iter).mData.getHandle();
                            mSceneNodeMap[handle] = formId; // for searchViaHandle()
                        }
                        // some (e.g. REFR with a SOUN baseObj) do not have a NIF model hence no BaseNode
                    }
                }
                else // dummy cell
#endif
                {
                    std::vector<LiveCellRefBase*> sublist = vect.search(x, y, range, exclude);

                    // remove objects no longer needed
                    if (exclude == 0) // those with physics
                    {
                        for (std::vector<LiveCellRefBase*>::iterator iter(vect.mDummyActive.begin());
                            iter != vect.mDummyActive.end(); ++iter)
                        {
                            if (std::find(sublist.begin(), sublist.end(), *iter) == sublist.end())
                            {
                                // used to be, but no longer

                                // get the SceneNode and handle from the object (how?)
                                //
                                // use the SceneNode to remove the physics object
                                // (how to get access to PhysicsSystem *mPhysics ?)
                                // remove handle from mSceneNodeMap
                                //
                                // need modified Objects::removeCell() and Actors::removeCell()
                                //
                                // remove formid from mStoreTypes and mForeignIds







                                //ListAndResetHandles functor;


          //Ogre::SceneNode* handle = ptr.getRefData().getBaseNode();
          //if (handle)
          //    mHandles.push_back (handle);

          //ptr.getRefData().setBaseNode(0);




                              //(*iter)->forEach<ListAndResetHandles>(functor);
                              //{
                              //    // silence annoying g++ warning
                              //    for (std::vector<Ogre::SceneNode*>::const_iterator iter2 (functor.mHandles.begin());
                              //        iter2!=functor.mHandles.end(); ++iter2)
                              //    {
                              //        Ogre::SceneNode* node = *iter2;

                              //        // ragdoll objects have physics objects attached to child SceneNodes
                              //        Ogre::Node::ChildNodeIterator childIter = node->getChildIterator();
                              //        while (childIter.hasMoreElements())
                              //        {
                              //            mPhysics->removeObject (childIter.current()->first);
                              //            childIter.getNext();
                              //        }

                              //        mPhysics->removeObject (node->getName());

                              //        removeLookupKeys(object.getBase()->mData.getHandle(), formId);
                              //    }
                              //}





                            }
                        }
                    }
                    else // those without physics
                    {
                        for (std::vector<LiveCellRefBase*>::iterator iter(vect.mDummyVisible.begin());
                            iter != vect.mDummyVisible.end(); ++iter)
                        {
                            if (std::find(sublist.begin(), sublist.end(), *iter) == sublist.end())
                            {
                                // used to be, but no longer
                            }
                        }
                    }

                    for (std::vector<LiveCellRefBase*>::iterator iter(sublist.begin());
                            iter != sublist.end(); ++iter)
                    {
                        if ((*iter)->mData.isDeletedByContentFile())
                            continue;

                        // should have an Ogre::SceneNode handle after this;
                        // see Objects::insertBegin() and Actors::insertBegin()
                        MWWorld::Ptr ptr(*iter, this);
                        if (!functor(ptr))
                        //if (!functor(MWWorld::Ptr(*iter, this)))
                            return false;

                        //if (ptr.getTypeName() == typeid(ESM4::Door).name())
                            //std::cout << "dummy insert door " << std::endl;

                        ESM4::FormId formId = (*iter)->mRef.getFormId();
                        // FIXME: currently unused because getPtr() only uses hasId()
                      //mForeignIds.push_back(formId);  // for hasFormId()

                        // update index for searchViaHandle()
                        if (exclude == 0 && // for those with collision shapes only
                                (*iter)->mData.getBaseNode()) // and if it was actually created
                        {
                            std::string handle = (*iter)->mData.getHandle();
                            mSceneNodeMap[handle] = formId;
                        }
                        // some (e.g. REFR with a SOUN baseObj) do not have a NIF model hence no BaseNode
                    }

                    if (exclude == 0)
                        vect.mDummyActive.swap(sublist);
                    else
                        vect.mDummyVisible.swap(sublist);
                }

                return true;
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
    inline const CellRefList<ESM4::Activator>& CellStore::getForeignReadOnly<ESM4::Activator>()
    {
        return mForeignActivators;
    }

    template<>
    inline CellRefList<ESM4::Apparatus>& CellStore::getForeign<ESM4::Apparatus>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignApparatus;
    }

    template<>
    inline CellRefList<ESM4::Armor>& CellStore::getForeign<ESM4::Armor>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignArmors;
    }

    template<>
    inline CellRefList<ESM4::Book>& CellStore::getForeign<ESM4::Book>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignBooks;
    }

    template<>
    inline CellRefList<ESM4::Clothing>& CellStore::getForeign<ESM4::Clothing>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignClothes;
    }

    template<>
    inline CellRefList<ESM4::Door>& CellStore::getForeign<ESM4::Door>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignDoors;
    }

    template<>
    inline const CellRefList<ESM4::Door>& CellStore::getForeignReadOnly<ESM4::Door>()
    {
        return mForeignDoors;
    }

    template<>
    inline CellRefList<ESM4::Ingredient>& CellStore::getForeign<ESM4::Ingredient>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignIngredients;
    }

    template<>
    inline CellRefList<ESM4::Light>& CellStore::getForeign<ESM4::Light>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignLights;
    }

    template<>
    inline CellRefList<ESM4::MiscItem>& CellStore::getForeign<ESM4::MiscItem>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignMiscItems;
    }

    template<>
    inline CellRefList<ESM4::Static>& CellStore::getForeign<ESM4::Static>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignStatics;
    }

    template<>
    inline const CellRefList<ESM4::Static>& CellStore::getForeignReadOnly<ESM4::Static>()
    {
        return mForeignStatics;
    }

    template<>
    inline CellRefList<ESM4::Weapon>& CellStore::getForeign<ESM4::Weapon>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignWeapons;
    }

    template<>
    inline CellRefList<ESM4::Ammunition>& CellStore::getForeign<ESM4::Ammunition>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mAmmunitions;
    }

    template<>
    inline CellRefList<ESM4::Npc>& CellStore::getForeign<ESM4::Npc>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignNpcs;
    }

    template<>
    inline CellRefList<ESM4::Creature>& CellStore::getForeign<ESM4::Creature>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignCreatures;
    }

    template<>
    inline CellRefList<ESM4::SoulGem>& CellStore::getForeign<ESM4::SoulGem>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mSoulGems;
    }

    template<>
    inline CellRefList<ESM4::Key>& CellStore::getForeign<ESM4::Key>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignKeys;
    }

    template<>
    inline CellRefList<ESM4::Potion>& CellStore::getForeign<ESM4::Potion>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mForeignPotions;
    }

    template<>
    inline CellRefList<ESM4::SigilStone>& CellStore::getForeign<ESM4::SigilStone>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mSigilStones;
    }

    template<>
    inline const CellRefList<ESM4::TalkingActivator>& CellStore::getForeignReadOnly<ESM4::TalkingActivator>()
    {
        return mTalkingActivators;
    }

    template<>
    inline CellRefList<ESM4::Note>& CellStore::getForeign<ESM4::Note>()
    {
        mHasState = true; // FIXME: what is this used for?
        return mNotes;
    }

    bool operator== (const CellStore& left, const CellStore& right);
    bool operator!= (const CellStore& left, const CellStore& right);
}

#endif
