#ifndef GAME_MWWORLD_CELLSTORE_H
#define GAME_MWWORLD_CELLSTORE_H

#include <algorithm>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <boost/shared_ptr.hpp>

#include <extern/esm4/hair.hpp>
#include <extern/esm4/eyes.hpp>
#include <extern/esm4/soun.hpp>
#include <extern/esm4/ltex.hpp>
#include <extern/esm4/acti.hpp>
#include <extern/esm4/appa.hpp>
#include <extern/esm4/armo.hpp>
#include <extern/esm4/book.hpp>
#include <extern/esm4/clot.hpp>
#include <extern/esm4/cont.hpp>
#include <extern/esm4/door.hpp>
#include <extern/esm4/ingr.hpp>
#include <extern/esm4/ligh.hpp>
#include <extern/esm4/misc.hpp>
#include <extern/esm4/stat.hpp>
#include <extern/esm4/gras.hpp>
#include <extern/esm4/tree.hpp>
#include <extern/esm4/flor.hpp>
#include <extern/esm4/furn.hpp>
#include <extern/esm4/weap.hpp>
#include <extern/esm4/ammo.hpp>
#include <extern/esm4/npc_.hpp>
#include <extern/esm4/crea.hpp>
#include <extern/esm4/lvlc.hpp>
#include <extern/esm4/slgm.hpp>
#include <extern/esm4/keym.hpp>
#include <extern/esm4/alch.hpp>
#include <extern/esm4/sbsp.hpp>
#include <extern/esm4/sgst.hpp>
#include <extern/esm4/lvli.hpp>
#include <extern/esm4/lvln.hpp>
#include <extern/esm4/regn.hpp>
#include <extern/esm4/land.hpp>
#include <extern/esm4/anio.hpp>
#include <extern/esm4/term.hpp>
#include <extern/esm4/tact.hpp>
#include <extern/esm4/note.hpp>
#include <extern/esm4/pgrd.hpp>
#include <extern/esm4/aspc.hpp>
#include <extern/esm4/pwat.hpp>
#include <extern/esm4/scol.hpp>

#include "livecellref.hpp"
#include "cellreflist.hpp"

#include <components/esm/fogstate.hpp>
#include <components/esm/records.hpp>

#include "../mwmechanics/pathgrid.hpp"  // TODO: maybe belongs in mwworld

#include "foreignstore.hpp"
#include "timestamp.hpp"

namespace ESM4
{
    typedef uint32_t FormId;
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
            CellRefList<ESM4::Ammo>             mForeignAmmos;
            CellRefList<ESM4::Npc>              mForeignNpcs;
            CellRefList<ESM4::Creature>         mForeignCreatures;
            CellRefList<ESM4::LevelledCreature>  mLevelledCreatures;
            CellRefList<ESM4::SoulGem>          mSoulGems;
            CellRefList<ESM4::Key>              mForeignKeys;
            CellRefList<ESM4::Potion>           mForeignPotions;
            CellRefList<ESM4::Subspace>         mSubspaces;
            CellRefList<ESM4::SigilStone>       mSigilStones;
            CellRefList<ESM4::LevelledItem>     mLevelledItems;
            CellRefList<ESM4::LevelledNpc>      mLevelledNpcs;
            CellRefList<ESM4::AcousticSpace>    mAcousticSpaces;
            CellRefList<ESM4::Terminal>         mTerminals;
            CellRefList<ESM4::TalkingActivator>  mTalkingActivators;
            CellRefList<ESM4::Note>             mNotes;
            CellRefList<ESM4::PlaceableWater>   mPlaceableWaters;
            CellRefList<ESM4::StaticCollection> mStaticCollections;

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

            bool hasId (const std::string& id) const;
            ///< May return true for deleted IDs when in preload state. Will return false, if cell is
            /// unloaded.

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
                    //
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
                    forEachImp (functor, mForeignAmmos) &&
                    forEachImp (functor, mForeignNpcs) &&
                    forEachImp (functor, mForeignCreatures) &&
                    forEachImp (functor, mLevelledCreatures) &&
                    forEachImp (functor, mSoulGems) &&
                    forEachImp (functor, mForeignKeys) &&
                    forEachImp (functor, mForeignPotions) &&
                    forEachImp (functor, mSubspaces) &&
                    forEachImp (functor, mSigilStones) &&
                    forEachImp (functor, mLevelledItems) &&
                    forEachImp (functor, mTerminals) &&
                    forEachImp (functor, mTalkingActivators) &&
                    forEachImp (functor, mNotes) &&
                    forEachImp (functor, mPlaceableWaters) &&
                    forEachImp (functor, mStaticCollections);
            }

            template<class Functor>
            bool forEachContainer (Functor& functor)
            {
                mHasState = true;

                return
                    forEachImp (functor, mContainers) &&
                    forEachImp (functor, mCreatures) &&
                    forEachImp (functor, mNpcs);
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
    inline const CellRefList<ESM4::Door>& CellStore::getReadOnly<ESM4::Door>()
    {
        return mForeignDoors;
    }

    template<>
    inline CellRefList<ESM4::Static>& CellStore::get<ESM4::Static>()
    {
        mHasState = true;
        return mForeignStatics;
    }

    template<>
    inline CellRefList<ESM4::Npc>& CellStore::get<ESM4::Npc>()
    {
        return mForeignNpcs;
    }

    template<>
    inline CellRefList<ESM4::Creature>& CellStore::get<ESM4::Creature>()
    {
        return mForeignCreatures;
    }

    bool operator== (const CellStore& left, const CellStore& right);
    bool operator!= (const CellStore& left, const CellStore& right);
}

#endif
