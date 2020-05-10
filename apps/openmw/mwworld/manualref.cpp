#include "manualref.hpp"

#include <stdexcept>

#include <extern/esm4/formid.hpp>

#include "esmstore.hpp"
#include "cellstore.hpp"

namespace
{
    template<typename T>
    void create(const MWWorld::Store<T>& list, const std::string& name, boost::any& refValue, MWWorld::Ptr& ptrValue)
    {
        const T* base = list.find(name);

        ESM::CellRef cellRef;
        cellRef.mRefNum.unset();
        cellRef.mRefID = name;
        cellRef.mScale = 1;
        cellRef.mFactionRank = 0;
        cellRef.mChargeInt = -1;
        cellRef.mGoldValue = 1;
        cellRef.mEnchantmentCharge = -1;
        cellRef.mTeleport = false;
        cellRef.mLockLevel = 0;
        cellRef.mReferenceBlocked = 0;

        MWWorld::LiveCellRef<T> ref(cellRef, base);

        refValue = ref;
        ptrValue = MWWorld::Ptr(&boost::any_cast<MWWorld::LiveCellRef<T>&>(refValue), 0);
    }

    template<typename T>
    void create(const MWWorld::ForeignStore<T>& list, ESM4::FormId id, boost::any& refValue, MWWorld::Ptr& ptrValue)
    {
        const T* base = list.search(id);

        ESM::CellRef cellRef;
        cellRef.mRefNum.unset();
        cellRef.mRefID = ESM4::formIdToString(id); // FIXME
        cellRef.mScale = 1;
        cellRef.mFactionRank = 0;
        cellRef.mChargeInt = -1;
        cellRef.mGoldValue = 1;
        cellRef.mEnchantmentCharge = -1;
        cellRef.mTeleport = false;
        cellRef.mLockLevel = 0;
        cellRef.mReferenceBlocked = 0;

        MWWorld::LiveCellRef<T> ref(cellRef, base);

        refValue = ref;
        ptrValue = MWWorld::Ptr(&boost::any_cast<MWWorld::LiveCellRef<T>&>(refValue), 0);
    }
}

MWWorld::ManualRef::ManualRef(const MWWorld::ESMStore& store, const std::string& name, const int count)
{
    if (ESM4::isFormId(name))
    {
        // TODO: levelled item/creature/npc ManualRef won't be created since the actual id of
        //       the item/actor will be supplied? maybe used for random spawning?
        ESM4::FormId id = ESM4::stringToFormId(name);
        switch (store.getRecordType(id))
        {
            case ESM4::REC_APPA: create(store.getForeign<ESM4::Apparatus>(), id, mRef, mPtr); break;
            case ESM4::REC_ARMO: create(store.getForeign<ESM4::Armor>(), id, mRef, mPtr); break;
            case ESM4::REC_BOOK: create(store.getForeign<ESM4::Book>(), id, mRef, mPtr); break;
            case ESM4::REC_CLOT: create(store.getForeign<ESM4::Clothing>(), id, mRef, mPtr); break;
            case ESM4::REC_INGR: create(store.getForeign<ESM4::Ingredient>(), id, mRef, mPtr); break;
            case ESM4::REC_LIGH: create(store.getForeign<ESM4::Light>(), id, mRef, mPtr); break;
            case ESM4::REC_MISC: create(store.getForeign<ESM4::MiscItem>(), id, mRef, mPtr); break;
            case ESM4::REC_WEAP: create(store.getForeign<ESM4::Weapon>(), id, mRef, mPtr); break;
            case ESM4::REC_AMMO: create(store.getForeign<ESM4::Ammo>(), id, mRef, mPtr); break;
            case ESM4::REC_NPC_: create(store.getForeign<ESM4::Npc>(), id, mRef, mPtr); break;
            case ESM4::REC_CREA: create(store.getForeign<ESM4::Creature>(), id, mRef, mPtr); break;
            case ESM4::REC_LVLC: create(store.getForeign<ESM4::LevelledCreature>(), id, mRef, mPtr); break;
            case ESM4::REC_SLGM: create(store.getForeign<ESM4::SoulGem>(), id, mRef, mPtr); break;
            case ESM4::REC_KEYM: create(store.getForeign<ESM4::Key>(), id, mRef, mPtr); break;
            case ESM4::REC_ALCH: create(store.getForeign<ESM4::Potion>(), id, mRef, mPtr); break;
            case ESM4::REC_SGST: create(store.getForeign<ESM4::SigilStone>(), id, mRef, mPtr); break;
            case ESM4::REC_LVLI: create(store.getForeign<ESM4::LevelledItem>(), id, mRef, mPtr); break; // TES4 only
            case ESM4::REC_LVLN: create(store.getForeign<ESM4::LevelledNpc>(), id, mRef, mPtr); break;
            case ESM4::REC_NOTE: create(store.getForeign<ESM4::Note>(), id, mRef, mPtr); break;
            case ESM4::REC_SCRL: create(store.getForeign<ESM4::Scroll>(), id, mRef, mPtr); break;
            case ESM4::REC_CMNY: break;// create(store.getForeign<ESM4::CasinoMoney>(), id, mRef, mPtr); break;
            // FIXME: IDLM, MSTT, TERM, etc, etc - are they needed?
            default:
                throw std::logic_error("failed to create a manual cell ref for " + name + " (unknown ID)");
        }
    }
    else
    {
        std::string lowerName = Misc::StringUtils::lowerCase(name);
        switch (store.find(lowerName))
        {
            case ESM::REC_ACTI: create(store.get<ESM::Activator>(), lowerName, mRef, mPtr); break;
            case ESM::REC_ALCH: create(store.get<ESM::Potion>(), lowerName, mRef, mPtr); break;
            case ESM::REC_APPA: create(store.get<ESM::Apparatus>(), lowerName, mRef, mPtr); break;
            case ESM::REC_ARMO: create(store.get<ESM::Armor>(), lowerName, mRef, mPtr); break;
            case ESM::REC_BOOK: create(store.get<ESM::Book>(), lowerName, mRef, mPtr); break;
            case ESM::REC_CLOT: create(store.get<ESM::Clothing>(), lowerName, mRef, mPtr); break;
            case ESM::REC_CONT: create(store.get<ESM::Container>(), lowerName, mRef, mPtr); break;
            case ESM::REC_CREA: create(store.get<ESM::Creature>(), lowerName, mRef, mPtr); break;
            case ESM::REC_DOOR: create(store.get<ESM::Door>(), lowerName, mRef, mPtr); break;
            case ESM::REC_INGR: create(store.get<ESM::Ingredient>(), lowerName, mRef, mPtr); break;
            case ESM::REC_LEVC: create(store.get<ESM::CreatureLevList>(), lowerName, mRef, mPtr); break;
            case ESM::REC_LEVI: create(store.get<ESM::ItemLevList>(), lowerName, mRef, mPtr); break;
            case ESM::REC_LIGH: create(store.get<ESM::Light>(), lowerName, mRef, mPtr); break;
            case ESM::REC_LOCK: create(store.get<ESM::Lockpick>(), lowerName, mRef, mPtr); break;
            case ESM::REC_MISC: create(store.get<ESM::Miscellaneous>(), lowerName, mRef, mPtr); break;
            case ESM::REC_NPC_: create(store.get<ESM::NPC>(), lowerName, mRef, mPtr); break;
            case ESM::REC_PROB: create(store.get<ESM::Probe>(), lowerName, mRef, mPtr); break;
            case ESM::REC_REPA: create(store.get<ESM::Repair>(), lowerName, mRef, mPtr); break;
            case ESM::REC_STAT: create(store.get<ESM::Static>(), lowerName, mRef, mPtr); break;
            case ESM::REC_WEAP: create(store.get<ESM::Weapon>(), lowerName, mRef, mPtr); break;

            case 0:
                throw std::logic_error("failed to create manual cell ref for " + lowerName + " (unknown ID)");

            default:
                throw std::logic_error("failed to create manual cell ref for " + lowerName + " (unknown type)");
        }
    }

    mPtr.getRefData().setCount(count);
}

// TODO: thought about creating a separate constructor for ManualRef but doing so may create a
//       lot of duplicate code everywhere (not 100% certain, though)
#if 0
MWWorld::ManualRef::ManualRef(const MWWorld::ESMStore& store, ESM4::FormId id, const int count)
{
    // TODO: levelled item/creature/npc ManualRef won't be created since the actual id of
    //       the item/actor will be supplied? maybe used for random spawning?
    switch (store.getRecordType(id))
    {
        case ESM4::REC_APPA: create(store.getForeign<ESM4::Apparatus>(), id, mRef, mPtr); break;
        case ESM4::REC_ARMO: create(store.getForeign<ESM4::Armor>(), id, mRef, mPtr); break;
        case ESM4::REC_BOOK: create(store.getForeign<ESM4::Book>(), id, mRef, mPtr); break;
        case ESM4::REC_CLOT: create(store.getForeign<ESM4::Clothing>(), id, mRef, mPtr); break;
        case ESM4::REC_INGR: create(store.getForeign<ESM4::Ingredient>(), id, mRef, mPtr); break;
        case ESM4::REC_LIGH: create(store.getForeign<ESM4::Light>(), id, mRef, mPtr); break;
        case ESM4::REC_MISC: create(store.getForeign<ESM4::MiscItem>(), id, mRef, mPtr); break;
        case ESM4::REC_WEAP: create(store.getForeign<ESM4::Weapon>(), id, mRef, mPtr); break;
        case ESM4::REC_AMMO: create(store.getForeign<ESM4::Ammo>(), id, mRef, mPtr); break;
        case ESM4::REC_NPC_: create(store.getForeign<ESM4::Npc>(), id, mRef, mPtr); break;
        case ESM4::REC_CREA: create(store.getForeign<ESM4::Creature>(), id, mRef, mPtr); break;
      //case ESM4::REC_LVLC: create(store.getForeign<ESM4::LevelledCreature>(), id, mRef, mPtr); break;
        case ESM4::REC_SLGM: create(store.getForeign<ESM4::SoulGem>(), id, mRef, mPtr); break;
        case ESM4::REC_KEYM: create(store.getForeign<ESM4::Key>(), id, mRef, mPtr); break;
        case ESM4::REC_ALCH: create(store.getForeign<ESM4::Potion>(), id, mRef, mPtr); break;
        case ESM4::REC_SGST: create(store.getForeign<ESM4::SigilStone>(), id, mRef, mPtr); break;
      //case ESM4::REC_LVLI: create(store.getForeign<ESM4::LevelledItem>(), id, mRef, mPtr); break;
      //case ESM4::REC_LVLN: create(store.getForeign<ESM4::LevelledNpc>(), id, mRef, mPtr); break;
        case ESM4::REC_NOTE: create(store.getForeign<ESM4::Note>(), id, mRef, mPtr); break;
        case ESM4::REC_SCRL: create(store.getForeign<ESM4::Scroll>(), id, mRef, mPtr); break;
        // FIXME: IDLM, MSTT, TERM, etc, etc - are they needed?
        default:
            throw std::logic_error("failed to create a manual cell ref for " + ESM4::formIdToString(id) + " (unknown ID)");
    }
}
#endif
