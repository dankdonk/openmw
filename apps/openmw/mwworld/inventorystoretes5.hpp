#ifndef GAME_MWWORLD_INVENTORYSTORETES5_H
#define GAME_MWWORLD_INVENTORYSTORETES5_H

#include "inventorystore.hpp"

namespace MWWorld
{
    class InventoryStoreTES5 : public InventoryStore
    {
        public:
            static const int Slot_TES5_Head        = 0;
            static const int Slot_TES5_Hair        = 1;
            static const int Slot_TES5_Body        = 2;
            static const int Slot_TES5_Hands       = 3;
            static const int Slot_TES5_Forearms    = 4;
            static const int Slot_TES5_Amulet      = 5;
            static const int Slot_TES5_Ring        = 6;
            static const int Slot_TES5_Feet        = 7;
            static const int Slot_TES5_Calves      = 8;
            static const int Slot_TES5_Shield      = 9;
            static const int Slot_TES5_Tail        = 10;
            static const int Slot_TES5_LongHair    = 11;
            static const int Slot_TES5_Circlet     = 12;
            static const int Slot_TES5_Ears        = 13;
            static const int Slot_TES5_BodyAddOn3  = 14;
            static const int Slot_TES5_BodyAddOn4  = 15;
            static const int Slot_TES5_BodyAddOn5  = 16;
            static const int Slot_TES5_BodyAddOn6  = 17;
            static const int Slot_TES5_BodyAddOn7  = 18;
            static const int Slot_TES5_BodyAddOn8  = 19;
            static const int SLOT_TES5_DecapHead   = 20;
            static const int SLOT_TES5_Decapitate  = 21;
            static const int Slot_TES5_BodyAddOn9  = 22;
            static const int Slot_TES5_BodyAddOn10 = 23;
            static const int Slot_TES5_BodyAddOn11 = 24;
            static const int Slot_TES5_BodyAddOn12 = 25;
            static const int Slot_TES5_BodyAddOn13 = 26;
            static const int Slot_TES5_BodyAddOn14 = 27;
            static const int Slot_TES5_BodyAddOn15 = 28;
            static const int Slot_TES5_BodyAddOn16 = 29;
            static const int Slot_TES5_BodyAddOn17 = 30;
            static const int SLOT_TES5_FX01        = 31;

            static const int TES5_Slots = 32;

            //static const int Slot_NoSlot = -1;

            InventoryStoreTES5();

            InventoryStoreTES5 (const InventoryStoreTES5& store);

            InventoryStoreTES5& operator= (const InventoryStoreTES5& store);

            virtual void autoEquip (const MWWorld::Ptr& actor);

            virtual int getNumSlots() const { return TES5_Slots; }

            virtual InventoryStore* clone() { return new InventoryStoreTES5(*this); }
    };
}

#endif
