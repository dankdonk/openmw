
#include "inventorystorefo3.hpp"

#include <iterator>
#include <algorithm>

//#include <components/esm/loadench.hpp>
//#include <components/esm/inventorystate.hpp>
//
//#include "../mwbase/environment.hpp"
//#include "../mwbase/world.hpp"
//#include "../mwbase/mechanicsmanager.hpp"
//#include "../mwbase/windowmanager.hpp"
//
//#include "../mwgui/inventorywindow.hpp"
//
//#include "../mwmechanics/npcstats.hpp"
//#include "../mwmechanics/spellcasting.hpp"
//
//
//#include "esmstore.hpp"
#include "class.hpp"

MWWorld::InventoryStoreFO3::InventoryStoreFO3()
    : InventoryStore ()
{
    //initSlots (mSlots);
}

MWWorld::InventoryStoreFO3::InventoryStoreFO3 (const InventoryStoreFO3& store)
    : InventoryStore (store)
{
    //copySlots (store);
}

MWWorld::InventoryStoreFO3& MWWorld::InventoryStoreFO3::operator= (const InventoryStoreFO3& store)
{
    InventoryStore::operator= (store);
    //mSlots.clear();
    //copySlots (store);
    return *this;
}

void MWWorld::InventoryStoreFO3::autoEquip (const MWWorld::Ptr& actor)
{
    std::vector<ContainerStoreIterator> slots_;
    initSlots (slots_); // push_back iterators for each possible equippable slots e.g. Helmet

    // disable excessive model update to listners during auto-equip
    mUpdatesEnabled = false;

    // iter points to the actor's inventory (all types, e.g. potion, armor, lockpicks)
    for (ContainerStoreIterator iter (begin()); iter!=end(); ++iter)
    {
        Ptr test = *iter; // temp inventory item for checking suitability

        // itemSlots.first specifies the slots for the equipment, the int values are defined in the
        // header, e.g.:
        //     static const int Slot_Helmet = 0;
        std::pair<std::vector<int>, bool> itemsSlots = iter->getClass().getEquipmentSlots (*iter);

        // iter2 points to a slot number for this equipment
        std::vector<int>::const_iterator iter2 (itemsSlots.first.begin());
        for (; iter2 != itemsSlots.first.end(); ++iter2)
        {
            if (slots_.at (*iter2)!=end()) // equipment slot for the new equipment already occupied
            {
                Ptr old = *slots_.at (*iter2);

                // check if the new equipment is more valuable than existing, etc
                //
                // not sure how to compare items that have more than one slot?
                // One possibility might be to sort the inventory in value order and equip the
                // items while checking for slot clashes.
                //
                // This item equipping algorithm should be encapsulated as it is likely to have
                // a large bearing on the gameplay (and whether we emulate vanilla closely).
                //
                // NOTE: Baurus's BladesGauntlets have value of 0, so simply comparing value
                // won't work.  Use Armor rating as a priority, then use value only if AR
                // values are equal.
                //
                // TODO: NPC scripts may disallow some items to be equipped (TODO: confirm this)
                //
                // FIXME: still to check for item health, etc
                if(old.getTypeName() == typeid(ESM4::Weapon).name()
                        || test.getTypeName() == typeid(ESM4::Weapon).name())
                {
                    // FIXME: probably should check weapon damage, health, value, etc
                    if (old.getClass().getValue(old) > test.getClass().getValue(test))
                    {
                        //std::cout << test.getClass().getName(test) << " is cheaper than "
                            //<< old.getClass().getName(old) << std::endl;
                        continue;
                    }
                }
                else if (old.getClass().getArmorRating(old) > test.getClass().getArmorRating(test))
                {
                    continue;
                }
                else if (old.getClass().getArmorRating (old) == test.getClass().getArmorRating(test))
                {
                    if (old.getClass().getValue(old) >= test.getClass().getValue(test))
                    {
                        //std::cout << test.getClass().getName(test) << " is cheaper than "
                            //<< old.getClass().getName(old) << std::endl;
                        continue;
                    }
                }
            }

            // canBeEquipped() - each class, e.g. ESM4::Clothing has its own logic
            //   0 if player cannot equip item
            //   1 if can equip
            //   2 if it's twohanded weapon
            //   3 if twohanded weapon conflicts with that
            if (test.getClass().canBeEquipped (test, actor).first == 0)
                continue; // check the next itemSlot in the for loop

            if (!itemsSlots.second) // can't stack when equipped
            {
                // unstack item pointed to by iterator if required
                if (iter->getRefData().getCount() > 1)
                {
                    unstack(*iter, actor);
                }
            }

            slots_[*iter2] = iter;
            //break;  // for FO3 an item can occupy more than one slot
        }
    }

    bool changed = false;

    for (std::size_t i = 0; i < slots_.size(); ++i)
    {
        if (slots_[i] != mSlots[i])
        {
            changed = true;
            break;
        }
    }
    mUpdatesEnabled = true;

    if (changed)
    {
        mSlots.swap (slots_);
        //fireEquipmentChangedEvent(actor); // FIXME
        //updateMagicEffects(actor);
        flagAsModified();
    }
}
