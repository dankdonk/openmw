#ifndef OPENMW_MWWORLD_ESMSTORE_H
#define OPENMW_MWWORLD_ESMSTORE_H

#include <stdexcept>

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
#include <extern/esm4/sgst.hpp>
#include <extern/esm4/lvli.hpp>
#include <extern/esm4/regn.hpp>
#include <extern/esm4/land.hpp>
#include <extern/esm4/anio.hpp>

#include <components/esm/records.hpp>
#include "store.hpp"
#include "foreignstore.hpp"

namespace ESM4
{
    class Reader;
    union RecordHeader;
}

namespace Loading
{
    class Listener;
}

namespace MWWorld
{
    class ESMStore
    {
        Store<ESM::Activator>       mActivators;
        Store<ESM::Potion>          mPotions;
        Store<ESM::Apparatus>       mAppas;
        Store<ESM::Armor>           mArmors;
        Store<ESM::BodyPart>        mBodyParts;
        Store<ESM::Book>            mBooks;
        Store<ESM::BirthSign>       mBirthSigns;
        Store<ESM::Class>           mClasses;
        Store<ESM::Clothing>        mClothes;
        Store<ESM::Container>       mContainers;
        Store<ESM::Creature>        mCreatures;
        Store<ESM::Dialogue>        mDialogs;
        Store<ESM::Door>            mDoors;
        Store<ESM::Enchantment>     mEnchants;
        Store<ESM::Faction>         mFactions;
        Store<ESM::Global>          mGlobals;
        Store<ESM::Ingredient>      mIngreds;
        Store<ESM::CreatureLevList> mCreatureLists;
        Store<ESM::ItemLevList>     mItemLists;
        Store<ESM::Light>           mLights;
        Store<ESM::Lockpick>        mLockpicks;
        Store<ESM::Miscellaneous>   mMiscItems;
        Store<ESM::NPC>             mNpcs;
        Store<ESM::Probe>           mProbes;
        Store<ESM::Race>            mRaces;
        Store<ESM::Region>          mRegions;
        Store<ESM::Repair>          mRepairs;
        Store<ESM::SoundGenerator>  mSoundGens;
        Store<ESM::Sound>           mSounds;
        Store<ESM::Spell>           mSpells;
        Store<ESM::StartScript>     mStartScripts;
        Store<ESM::Static>          mStatics;
        Store<ESM::Weapon>          mWeapons;

        Store<ESM::GameSetting>     mGameSettings;
        Store<ESM::Script>          mScripts;

        // Lists that need special rules
        Store<ESM::Cell>        mCells;
        Store<ESM::Land>        mLands;
        Store<ESM::LandTexture> mLandTextures;
        Store<ESM::Pathgrid>    mPathgrids;

        Store<ESM::MagicEffect> mMagicEffects;
        Store<ESM::Skill>       mSkills;

        // Special entry which is hardcoded and not loaded from an ESM
        Store<ESM::Attribute>   mAttributes;

        // Lists that are foreign
        Store<MWWorld::ForeignWorld>   mForeignWorlds;
        Store<MWWorld::ForeignCell>    mForeignCells;
        Store<MWWorld::ForeignLand>    mForeignLands;
        //
        ForeignStore<ESM4::Hair>       mForeignHairs;
        ForeignStore<ESM4::Eyes>       mForeignEyesSet;
        ForeignStore<ESM4::Sound>      mForeignSounds;
        ForeignStore<ESM4::LandTexture> mForeignLandTextures;
        // Foreign referenceables
        ForeignStore<ESM4::Activator>  mForeignActivators;
        ForeignStore<ESM4::Apparatus>  mForeignApparatuses;
        ForeignStore<ESM4::Armor>      mForeignArmors;
        ForeignStore<ESM4::Book>       mForeignBooks;
        ForeignStore<ESM4::Clothing>   mForeignClothes;
        ForeignStore<ESM4::Container>  mForeignContainers;
        ForeignStore<ESM4::Door>       mForeignDoors;
        ForeignStore<ESM4::Ingredient> mForeignIngredients;
        ForeignStore<ESM4::Light>      mForeignLights;
        ForeignStore<ESM4::MiscItem>   mForeignMiscItems;
        ForeignStore<ESM4::Static>     mForeignStatics;
        ForeignStore<ESM4::Grass>      mForeignGrasses;
        ForeignStore<ESM4::Tree>       mForeignTrees;
        ForeignStore<ESM4::Flora>      mForeignFloras;
        ForeignStore<ESM4::Furniture>  mForeignFurnitures;
        ForeignStore<ESM4::Weapon>     mForeignWeapons;
        ForeignStore<ESM4::Ammo>       mForeignAmmos;
        ForeignStore<ESM4::Npc>        mForeignNpcs;
        ForeignStore<ESM4::Creature>   mForeignCreatures;
        ForeignStore<ESM4::LeveledCreature> mForeignLvlCreatures;
        ForeignStore<ESM4::SoulGem>    mForeignSoulGems;
        ForeignStore<ESM4::Key>        mForeignKeys;
        ForeignStore<ESM4::Potion>     mForeignPotions;
        ForeignStore<ESM4::SigilStone> mForeignSigilStones;
        ForeignStore<ESM4::LeveledItem> mForeignLvlItems;
        //
        ForeignStore<ESM4::AnimObject> mForeignAnimObjs;

        // Lookup of all IDs. Makes looking up references faster. Just
        // maps the id name to the record type.
        std::map<std::string, int> mIds;
        std::map<ESM4::FormId, int> mForeignIds;
        std::map<int, StoreBase *> mStores;

        // Unlike TES3, the destination cell is not specified in the reference record.
        // Need a lookup map to work around this issue.
        std::map<ESM4::FormId, ESM4::FormId> mDoorDestCell;

        ESM::NPC mPlayerTemplate;

        unsigned int mDynamicCount;

        void loadTes4Group (ESM::ESMReader& esm);
        void loadTes4Record (ESM::ESMReader& esm);

    public:
        /// \todo replace with SharedIterator<StoreBase>
        typedef std::map<int, StoreBase *>::const_iterator iterator;

        iterator begin() const {
            return mStores.begin();
        }

        iterator end() const {
            return mStores.end();
        }

        /// Look up the given ID in 'all'. Returns 0 if not found.
        /// \note id must be in lower case.
        int find(const std::string &id) const
        {
            std::map<std::string, int>::const_iterator it = mIds.find(id);
            if (it == mIds.end()) {
                return 0;
            }
            return it->second;
        }

        int find(ESM4::FormId formId) const
        {
            std::map<ESM4::FormId, int>::const_iterator it = mForeignIds.find(formId);
            if (it == mForeignIds.end())
                return 0;
            else
                return it->second;
        }

        ESM4::FormId getDoorCell(ESM4::FormId doorId) const
        {
            std::map<ESM4::FormId, ESM4::FormId>::const_iterator it
                = mDoorDestCell.find(doorId);

            if (it != mDoorDestCell.end())
                return it->second;

            return 0;
        }

        ESMStore()
          : mDynamicCount(0)
        {
            mStores[ESM::REC_ACTI] = &mActivators;
            mStores[ESM::REC_ALCH] = &mPotions;
            mStores[ESM::REC_APPA] = &mAppas;
            mStores[ESM::REC_ARMO] = &mArmors;
            mStores[ESM::REC_BODY] = &mBodyParts;
            mStores[ESM::REC_BOOK] = &mBooks;
            mStores[ESM::REC_BSGN] = &mBirthSigns;
            mStores[ESM::REC_CELL] = &mCells;
            mStores[ESM::REC_CLAS] = &mClasses;
            mStores[ESM::REC_CLOT] = &mClothes;
            mStores[ESM::REC_CONT] = &mContainers;
            mStores[ESM::REC_CREA] = &mCreatures;
            mStores[ESM::REC_DIAL] = &mDialogs;
            mStores[ESM::REC_DOOR] = &mDoors;
            mStores[ESM::REC_ENCH] = &mEnchants;
            mStores[ESM::REC_FACT] = &mFactions;
            mStores[ESM::REC_GLOB] = &mGlobals;
            mStores[ESM::REC_GMST] = &mGameSettings;
            mStores[ESM::REC_INGR] = &mIngreds;
            mStores[ESM::REC_LAND] = &mLands;
            mStores[ESM::REC_LEVC] = &mCreatureLists;
            mStores[ESM::REC_LEVI] = &mItemLists;
            mStores[ESM::REC_LIGH] = &mLights;
            mStores[ESM::REC_LOCK] = &mLockpicks;
            mStores[ESM::REC_LTEX] = &mLandTextures;
            mStores[ESM::REC_MISC] = &mMiscItems;
            mStores[ESM::REC_NPC_] = &mNpcs;
            mStores[ESM::REC_PGRD] = &mPathgrids;
            mStores[ESM::REC_PROB] = &mProbes;
            mStores[ESM::REC_RACE] = &mRaces;
            mStores[ESM::REC_REGN] = &mRegions;
            mStores[ESM::REC_REPA] = &mRepairs;
            mStores[ESM::REC_SCPT] = &mScripts;
            mStores[ESM::REC_SNDG] = &mSoundGens;
            mStores[ESM::REC_SOUN] = &mSounds;
            mStores[ESM::REC_SPEL] = &mSpells;
            mStores[ESM::REC_SSCR] = &mStartScripts;
            mStores[ESM::REC_STAT] = &mStatics;
            mStores[ESM::REC_WEAP] = &mWeapons;

            // NOTE: to avoid clash with TES3, these are rotated by one
            mStores[MKTAG('R','H','A','I')] = &mForeignHairs;
            mStores[MKTAG('S','E','Y','E')] = &mForeignEyesSet;
            mStores[MKTAG('N','S','O','U')] = &mForeignSounds;
            mStores[MKTAG('X','L','T','E')] = &mForeignLandTextures;

            mStores[MKTAG('I','A','C','T')] = &mForeignActivators;
            mStores[MKTAG('A','A','P','P')] = &mForeignApparatuses;
            mStores[MKTAG('O','A','R','M')] = &mForeignArmors;
            mStores[MKTAG('K','B','O','O')] = &mForeignBooks;
            mStores[MKTAG('T','C','L','O')] = &mForeignClothes;
            mStores[MKTAG('T','C','O','N')] = &mForeignContainers;
            mStores[MKTAG('R','D','O','O')] = &mForeignDoors;
            mStores[MKTAG('R','I','N','G')] = &mForeignIngredients;
            mStores[MKTAG('H','L','I','G')] = &mForeignLights;
            mStores[MKTAG('C','M','I','S')] = &mForeignMiscItems;
            mStores[MKTAG('T','S','T','A')] = &mForeignStatics;
            mStores[MKTAG('S','G','R','A')] = &mForeignGrasses;
            mStores[MKTAG('E','T','R','E')] = &mForeignTrees;
            mStores[MKTAG('R','F','L','O')] = &mForeignFloras;
            mStores[MKTAG('N','F','U','R')] = &mForeignFurnitures;
            mStores[MKTAG('P','W','E','A')] = &mForeignWeapons;
            mStores[MKTAG('O','A','M','M')] = &mForeignAmmos;
            mStores[MKTAG('_','N','P','C')] = &mForeignNpcs;
            mStores[MKTAG('A','C','R','E')] = &mForeignCreatures;
            mStores[MKTAG('C','L','V','L')] = &mForeignLvlCreatures;
            mStores[MKTAG('M','S','L','G')] = &mForeignSoulGems;
            mStores[MKTAG('M','K','E','Y')] = &mForeignKeys;
            mStores[MKTAG('H','A','L','C')] = &mForeignPotions;
            mStores[MKTAG('T','S','G','S')] = &mForeignSigilStones;
            mStores[MKTAG('I','L','V','L')] = &mForeignLvlItems;

            mStores[MKTAG('O','A','N','I')] = &mForeignAnimObjs;

            mStores[MKTAG('D','W','R','L')] = &mForeignWorlds;
            mStores[MKTAG('L','C','E','L')] = &mForeignCells;
            mStores[MKTAG('D','L','A','N')] = &mForeignLands;

            mPathgrids.setCells(mCells);
        }

        void clearDynamic ()
        {
            for (std::map<int, StoreBase *>::iterator it = mStores.begin(); it != mStores.end(); ++it)
                it->second->clearDynamic();

            mNpcs.insert(mPlayerTemplate);
        }

        void movePlayerRecord ()
        {
            mPlayerTemplate = *mNpcs.find("player");
            mNpcs.eraseStatic(mPlayerTemplate.mId);
            mNpcs.insert(mPlayerTemplate);
        }

        void load(ESM::ESMReader &esm, Loading::Listener* listener);

        template <class T>
        const Store<T> &get() const {
            throw std::runtime_error("Storage for this type not exist");
        }

        template <class T>
        Store<T> &getModifiable() {
            throw std::runtime_error("Storage for this type not exist");
        }

        template <class T>
        const ForeignStore<T> &getForeign() const {
            throw std::runtime_error("Storage for this type not exist");
        }

        /// Insert a custom record (i.e. with a generated ID that will not clash will pre-existing records)
        template <class T>
        const T *insert(const T &x) {
            std::ostringstream id;
            id << "$dynamic" << mDynamicCount++;

            Store<T> &store = const_cast<Store<T> &>(get<T>());
            if (store.search(id.str()) != 0) {
                std::ostringstream msg;
                msg << "Try to override existing record '" << id.str() << "'";
                throw std::runtime_error(msg.str());
            }
            T record = x;

            record.mId = id.str();

            T *ptr = store.insert(record);
            for (iterator it = mStores.begin(); it != mStores.end(); ++it) {
                if (it->second == &store) {
                    mIds[ptr->mId] = it->first;
                }
            }
            return ptr;
        }

        /// Insert a record with set ID, and allow it to override a pre-existing static record.
        template <class T>
        const T *overrideRecord(const T &x) {
            Store<T> &store = const_cast<Store<T> &>(get<T>());

            T *ptr = store.insert(x);
            for (iterator it = mStores.begin(); it != mStores.end(); ++it) {
                if (it->second == &store) {
                    mIds[ptr->mId] = it->first;
                }
            }
            return ptr;
        }

        template <class T>
        const T *insertStatic(const T &x) {
            std::ostringstream id;
            id << "$dynamic" << mDynamicCount++;

            Store<T> &store = const_cast<Store<T> &>(get<T>());
            if (store.search(id.str()) != 0) {
                std::ostringstream msg;
                msg << "Try to override existing record '" << id.str() << "'";
                throw std::runtime_error(msg.str());
            }
            T record = x;

            T *ptr = store.insertStatic(record);
            for (iterator it = mStores.begin(); it != mStores.end(); ++it) {
                if (it->second == &store) {
                    mIds[ptr->mId] = it->first;
                }
            }
            return ptr;
        }

        // This method must be called once, after loading all master/plugin files. This can only be done
        //  from the outside, so it must be public.
        void setUp();

        int countSavedGameRecords() const;

        void write (ESM::ESMWriter& writer, Loading::Listener& progress) const;

        bool readRecord (ESM::ESMReader& reader, uint32_t type);
        ///< \return Known type?
    };

    template <>
    inline const ESM::Cell *ESMStore::insert<ESM::Cell>(const ESM::Cell &cell) {
        return mCells.insert(cell);
    }

    template <>
    inline const ESM::NPC *ESMStore::insert<ESM::NPC>(const ESM::NPC &npc) {
        std::ostringstream id;
        id << "$dynamic" << mDynamicCount++;

        if (Misc::StringUtils::ciEqual(npc.mId, "player")) {
            return mNpcs.insert(npc);
        } else if (mNpcs.search(id.str()) != 0) {
            std::ostringstream msg;
            msg << "Try to override existing record '" << id.str() << "'";
            throw std::runtime_error(msg.str());
        }
        ESM::NPC record = npc;

        record.mId = id.str();

        ESM::NPC *ptr = mNpcs.insert(record);
        mIds[ptr->mId] = ESM::REC_NPC_;
        return ptr;
    }

    template <>
    inline const Store<ESM::Activator> &ESMStore::get<ESM::Activator>() const {
        return mActivators;
    }

    template <>
    inline const Store<ESM::Potion> &ESMStore::get<ESM::Potion>() const {
        return mPotions;
    }

    template <>
    inline const Store<ESM::Apparatus> &ESMStore::get<ESM::Apparatus>() const {
        return mAppas;
    }

    template <>
    inline const Store<ESM::Armor> &ESMStore::get<ESM::Armor>() const {
        return mArmors;
    }

    template <>
    inline const Store<ESM::BodyPart> &ESMStore::get<ESM::BodyPart>() const {
        return mBodyParts;
    }

    template <>
    inline const Store<ESM::Book> &ESMStore::get<ESM::Book>() const {
        return mBooks;
    }

    template <>
    inline const Store<ESM::BirthSign> &ESMStore::get<ESM::BirthSign>() const {
        return mBirthSigns;
    }

    template <>
    inline const Store<ESM::Class> &ESMStore::get<ESM::Class>() const {
        return mClasses;
    }

    template <>
    inline const Store<ESM::Clothing> &ESMStore::get<ESM::Clothing>() const {
        return mClothes;
    }

    template <>
    inline const Store<ESM::Container> &ESMStore::get<ESM::Container>() const {
        return mContainers;
    }

    template <>
    inline const Store<ESM::Creature> &ESMStore::get<ESM::Creature>() const {
        return mCreatures;
    }

    template <>
    inline const Store<ESM::Dialogue> &ESMStore::get<ESM::Dialogue>() const {
        return mDialogs;
    }

    template <>
    inline const Store<ESM::Door> &ESMStore::get<ESM::Door>() const {
        return mDoors;
    }

    template <>
    inline const Store<ESM::Enchantment> &ESMStore::get<ESM::Enchantment>() const {
        return mEnchants;
    }

    template <>
    inline const Store<ESM::Faction> &ESMStore::get<ESM::Faction>() const {
        return mFactions;
    }

    template <>
    inline const Store<ESM::Global> &ESMStore::get<ESM::Global>() const {
        return mGlobals;
    }

    template <>
    inline const Store<ESM::Ingredient> &ESMStore::get<ESM::Ingredient>() const {
        return mIngreds;
    }

    template <>
    inline const Store<ESM::CreatureLevList> &ESMStore::get<ESM::CreatureLevList>() const {
        return mCreatureLists;
    }

    template <>
    inline const Store<ESM::ItemLevList> &ESMStore::get<ESM::ItemLevList>() const {
        return mItemLists;
    }

    template <>
    inline const Store<ESM::Light> &ESMStore::get<ESM::Light>() const {
        return mLights;
    }

    template <>
    inline const Store<ESM::Lockpick> &ESMStore::get<ESM::Lockpick>() const {
        return mLockpicks;
    }

    template <>
    inline const Store<ESM::Miscellaneous> &ESMStore::get<ESM::Miscellaneous>() const {
        return mMiscItems;
    }

    template <>
    inline const Store<ESM::NPC> &ESMStore::get<ESM::NPC>() const {
        return mNpcs;
    }

    template <>
    inline const Store<ESM::Probe> &ESMStore::get<ESM::Probe>() const {
        return mProbes;
    }

    template <>
    inline const Store<ESM::Race> &ESMStore::get<ESM::Race>() const {
        return mRaces;
    }

    template <>
    inline const Store<ESM::Region> &ESMStore::get<ESM::Region>() const {
        return mRegions;
    }

    template <>
    inline const Store<ESM::Repair> &ESMStore::get<ESM::Repair>() const {
        return mRepairs;
    }

    template <>
    inline const Store<ESM::SoundGenerator> &ESMStore::get<ESM::SoundGenerator>() const {
        return mSoundGens;
    }

    template <>
    inline const Store<ESM::Sound> &ESMStore::get<ESM::Sound>() const {
        return mSounds;
    }

    template <>
    inline const Store<ESM::Spell> &ESMStore::get<ESM::Spell>() const {
        return mSpells;
    }

    template <>
    inline const Store<ESM::StartScript> &ESMStore::get<ESM::StartScript>() const {
        return mStartScripts;
    }

    template <>
    inline const Store<ESM::Static> &ESMStore::get<ESM::Static>() const {
        return mStatics;
    }

    template <>
    inline const Store<ESM::Weapon> &ESMStore::get<ESM::Weapon>() const {
        return mWeapons;
    }

    template <>
    inline const Store<ForeignWorld> &ESMStore::get<ForeignWorld>() const {
        return mForeignWorlds;
    }

    template <>
    inline const Store<ForeignCell> &ESMStore::get<ForeignCell>() const {
        return mForeignCells;
    }

    template <>
    inline Store<ForeignLand> &ESMStore::getModifiable<ForeignLand>() {
        return mForeignLands;
    }

    template <>
    inline const Store<ForeignLand> &ESMStore::get<ForeignLand>() const {
        return mForeignLands;
    }

    template <>
    inline const Store<ESM::GameSetting> &ESMStore::get<ESM::GameSetting>() const {
        return mGameSettings;
    }

    template <>
    inline const Store<ESM::Script> &ESMStore::get<ESM::Script>() const {
        return mScripts;
    }

    template <>
    inline const Store<ESM::Cell> &ESMStore::get<ESM::Cell>() const {
        return mCells;
    }

    template <>
    inline const Store<ESM::Land> &ESMStore::get<ESM::Land>() const {
        return mLands;
    }

    template <>
    inline const Store<ESM::LandTexture> &ESMStore::get<ESM::LandTexture>() const {
        return mLandTextures;
    }

    template <>
    inline const Store<ESM::Pathgrid> &ESMStore::get<ESM::Pathgrid>() const {
        return mPathgrids;
    }

    template <>
    inline const Store<ESM::MagicEffect> &ESMStore::get<ESM::MagicEffect>() const {
        return mMagicEffects;
    }

    template <>
    inline const Store<ESM::Skill> &ESMStore::get<ESM::Skill>() const {
        return mSkills;
    }

    template <>
    inline const Store<ESM::Attribute> &ESMStore::get<ESM::Attribute>() const {
        return mAttributes;
    }

    template <>
    inline const ForeignStore<ESM4::Sound>& ESMStore::getForeign<ESM4::Sound>() const {
        return mForeignSounds;
    }

    template <>
    inline const ForeignStore<ESM4::Activator>& ESMStore::getForeign<ESM4::Activator>() const {
        return mForeignActivators;
    }

    template <>
    inline const ForeignStore<ESM4::Apparatus>& ESMStore::getForeign<ESM4::Apparatus>() const {
        return mForeignApparatuses;
    }

    template <>
    inline const ForeignStore<ESM4::Armor>& ESMStore::getForeign<ESM4::Armor>() const {
        return mForeignArmors;
    }

    template <>
    inline const ForeignStore<ESM4::Book>& ESMStore::getForeign<ESM4::Book>() const {
        return mForeignBooks;
    }

    template <>
    inline const ForeignStore<ESM4::Clothing>& ESMStore::getForeign<ESM4::Clothing>() const {
        return mForeignClothes;
    }

    template <>
    inline const ForeignStore<ESM4::Container>& ESMStore::getForeign<ESM4::Container>() const {
        return mForeignContainers;
    }

    template <>
    inline const ForeignStore<ESM4::Door>& ESMStore::getForeign<ESM4::Door>() const {
        return mForeignDoors;
    }

    template <>
    inline const ForeignStore<ESM4::Ingredient>& ESMStore::getForeign<ESM4::Ingredient>() const {
        return mForeignIngredients;
    }

    template <>
    inline const ForeignStore<ESM4::Light>& ESMStore::getForeign<ESM4::Light>() const {
        return mForeignLights;
    }

    template <>
    inline const ForeignStore<ESM4::MiscItem>& ESMStore::getForeign<ESM4::MiscItem>() const {
        return mForeignMiscItems;
    }

    template <>
    inline const ForeignStore<ESM4::Static>& ESMStore::getForeign<ESM4::Static>() const {
        return mForeignStatics;
    }

    template <>
    inline const ForeignStore<ESM4::Grass>& ESMStore::getForeign<ESM4::Grass>() const {
        return mForeignGrasses;
    }

    template <>
    inline const ForeignStore<ESM4::Tree>& ESMStore::getForeign<ESM4::Tree>() const {
        return mForeignTrees;
    }

    template <>
    inline const ForeignStore<ESM4::Flora>& ESMStore::getForeign<ESM4::Flora>() const {
        return mForeignFloras;
    }

    template <>
    inline const ForeignStore<ESM4::Furniture>& ESMStore::getForeign<ESM4::Furniture>() const {
        return mForeignFurnitures;
    }

    template <>
    inline const ForeignStore<ESM4::Weapon>& ESMStore::getForeign<ESM4::Weapon>() const {
        return mForeignWeapons;
    }

    template <>
    inline const ForeignStore<ESM4::Ammo>& ESMStore::getForeign<ESM4::Ammo>() const {
        return mForeignAmmos;
    }

    template <>
    inline const ForeignStore<ESM4::Npc>& ESMStore::getForeign<ESM4::Npc>() const {
        return mForeignNpcs;
    }

    template <>
    inline const ForeignStore<ESM4::Creature>& ESMStore::getForeign<ESM4::Creature>() const {
        return mForeignCreatures;
    }

    template <>
    inline const ForeignStore<ESM4::LeveledCreature>& ESMStore::getForeign<ESM4::LeveledCreature>() const {
        return mForeignLvlCreatures;
    }

    template <>
    inline const ForeignStore<ESM4::SoulGem>& ESMStore::getForeign<ESM4::SoulGem>() const {
        return mForeignSoulGems;
    }

    template <>
    inline const ForeignStore<ESM4::Key>& ESMStore::getForeign<ESM4::Key>() const {
        return mForeignKeys;
    }

    template <>
    inline const ForeignStore<ESM4::Potion>& ESMStore::getForeign<ESM4::Potion>() const {
        return mForeignPotions;
    }

    template <>
    inline const ForeignStore<ESM4::SigilStone>& ESMStore::getForeign<ESM4::SigilStone>() const {
        return mForeignSigilStones;
    }

    template <>
    inline const ForeignStore<ESM4::LeveledItem>& ESMStore::getForeign<ESM4::LeveledItem>() const {
        return mForeignLvlItems;
    }
}

#endif
