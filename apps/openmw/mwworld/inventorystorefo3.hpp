#ifndef GAME_MWWORLD_INVENTORYSTOREFO3_H
#define GAME_MWWORLD_INVENTORYSTOREFO3_H

#include "inventorystore.hpp"

namespace MWWorld
{
    class InventoryStoreFO3 : public InventoryStore
    {
        public:
            static const int Slot_FO3_Head        = 0;
            static const int Slot_FO3_Hair        = 1;
            static const int Slot_FO3_UpperBody   = 2;
            static const int Slot_FO3_LeftHand    = 3;
            static const int Slot_FO3_RightHand   = 4;
            static const int Slot_FO3_Weapon      = 5;
            static const int Slot_FO3_PipBoy      = 6;
            static const int Slot_FO3_Backpack    = 7;
            static const int Slot_FO3_Necklace    = 8;
            static const int Slot_FO3_Headband    = 9;
            static const int Slot_FO3_Hat         = 10;
            static const int Slot_FO3_EyeGlasses  = 11;
            static const int Slot_FO3_NoseRing    = 12;
            static const int Slot_FO3_Earrings    = 13;
            static const int Slot_FO3_Mask        = 14;
            static const int Slot_FO3_Choker      = 15;
            static const int Slot_FO3_MouthObject = 16;
            static const int Slot_FO3_BodyAddOn1  = 17;
            static const int Slot_FO3_BodyAddOn2  = 18;
            static const int Slot_FO3_BodyAddOn3  = 19;

            static const int FO3_Slots = 20;

            //static const int Slot_NoSlot = -1;

            InventoryStoreFO3();

            InventoryStoreFO3 (const InventoryStoreFO3& store);

            InventoryStoreFO3& operator= (const InventoryStoreFO3& store);

            virtual void autoEquip (const MWWorld::Ptr& actor);

            virtual int getNumSlots() const { return FO3_Slots; }

            virtual InventoryStore* clone() { return new InventoryStoreFO3(*this); }
    };
}

#endif
