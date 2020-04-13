#ifndef GAME_MWWORLD_INVENTORYSTORETES4_H
#define GAME_MWWORLD_INVENTORYSTORETES4_H

#include "inventorystore.hpp"

namespace MWWorld
{
    class InventoryStoreTES4 : public InventoryStore
    {
        public:
            // No separate shirt/pants/skirt/belt/pauldrons slots for TES4, etc.
            static const int Slot_TES4_Head      = 0;
            static const int Slot_TES4_Hair      = 1;
            static const int Slot_TES4_UpperBody = 2;
            static const int Slot_TES4_LowerBody = 3;
            static const int Slot_TES4_Hands     = 4;
            static const int Slot_TES4_Feet      = 5;
            static const int Slot_TES4_RightRing = 6;
            static const int Slot_TES4_LeftRing  = 7;
            static const int Slot_TES4_Amulet    = 8;
            static const int Slot_TES4_Weapon    = 9;
            static const int Slot_TES4_BackWeapon = 10;
            static const int Slot_TES4_SideWeapon = 11;
            static const int Slot_TES4_Quiver    = 12;
            static const int Slot_TES4_Shield    = 13;
            static const int Slot_TES4_Torch     = 14;
            static const int Slot_TES4_Tail      = 15;

            static const int TES4_Slots = 16;

            //static const int Slot_NoSlot = -1;

            InventoryStoreTES4();

            InventoryStoreTES4 (const InventoryStoreTES4& store);

            InventoryStoreTES4& operator= (const InventoryStoreTES4& store);

            virtual void autoEquip (const MWWorld::Ptr& actor);

            virtual int getNumSlots() const { return TES4_Slots; }

            virtual InventoryStore* clone() { return new InventoryStoreTES4(*this); }
    };
}

#endif
