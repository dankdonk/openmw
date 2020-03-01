#include "manualref.hpp"

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
        ESM4::FormId id =ESM4::stringToFormId(name);
        switch (store.find(id))
        {
        case MKTAG('A','A','P','P'): create(store.getForeign<ESM4::Apparatus>(), id, mRef, mPtr); break;
        case MKTAG('O','A','R','M'): create(store.getForeign<ESM4::Armor>(), id, mRef, mPtr); break;
        case MKTAG('K','B','O','O'): create(store.getForeign<ESM4::Book>(), id, mRef, mPtr); break;
        case MKTAG('T','C','L','O'): create(store.getForeign<ESM4::Clothing>(), id, mRef, mPtr); break;
        case MKTAG('R','I','N','G'): create(store.getForeign<ESM4::Ingredient>(), id, mRef, mPtr); break;
        case MKTAG('H','L','I','G'): create(store.getForeign<ESM4::Light>(), id, mRef, mPtr); break;
        case MKTAG('C','M','I','S'): create(store.getForeign<ESM4::MiscItem>(), id, mRef, mPtr); break;
        case MKTAG('P','W','E','A'): create(store.getForeign<ESM4::Weapon>(), id, mRef, mPtr); break;
        case MKTAG('O','A','M','M'): create(store.getForeign<ESM4::Ammo>(), id, mRef, mPtr); break;
        case MKTAG('M','S','L','G'): create(store.getForeign<ESM4::SoulGem>(), id, mRef, mPtr); break;
        case MKTAG('M','K','E','Y'): create(store.getForeign<ESM4::Key>(), id, mRef, mPtr); break;
        case MKTAG('H','A','L','C'): create(store.getForeign<ESM4::Potion>(), id, mRef, mPtr); break;
        case MKTAG('T','S','G','S'): create(store.getForeign<ESM4::SigilStone>(), id, mRef, mPtr); break;
        case MKTAG('I','L','V','L'): create(store.getForeign<ESM4::LeveledItem>(), id, mRef, mPtr); break;
        case MKTAG('E','N','O','T'): create(store.getForeign<ESM4::Note>(), id, mRef, mPtr); break;
        //case MKTAG('L','S','C','R'): create(store.getForeign<ESM4::Scroll>(), id, mRef, mPtr); break;
        default:
            throw std::logic_error("failed to create manual cell ref for " + name + " (unknown ID)");
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
