#include "foreignarmor.hpp"

#include <extern/esm4/armo.hpp>

#include "../mwgui/tooltips.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include <components/misc/stringops.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignArmor::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Armor>()->mBase->mEditorId;
    }

    void ForeignArmor::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        if (!model.empty()) {
            // TES4 seems to lack _gnd models for shields
            std::string lowerModel = Misc::StringUtils::lowerCase(model);
            size_t pos = lowerModel.find("shield_gnd.");
            if (pos != std::string::npos)
                renderingInterface.getObjects().insertModel(ptr, lowerModel.replace(pos+5,4,"")); // FIXME
            else
            {
                pos = lowerModel.find("hatchild_gnd.");
                if (pos != std::string::npos)
                    renderingInterface.getObjects().insertModel(ptr, lowerModel.replace(pos+7,4,"")); // FIXME
                else
                    renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
            }
        }
    }

    void ForeignArmor::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
        {
            // TES4 seems to lack _gnd models for shields
            std::string lowerModel = Misc::StringUtils::lowerCase(model);
            size_t pos = lowerModel.find("shield_gnd.");
            if (pos != std::string::npos)
                physics.addObject(ptr, lowerModel.replace(pos+5,4,""));
            else
            {
                pos = lowerModel.find("hatchild_gnd.");
                if (pos != std::string::npos)
                    physics.addObject(ptr, lowerModel.replace(pos+7,4,""));
                else
                    physics.addObject(ptr, model);
            }
        }
    }

    std::string ForeignArmor::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();
        assert(ref->mBase != NULL);

        // clothing and armor need "ground" models (with physics) unless being worn
        std::string model = ref->mBase->mModelMale; // FIXME: what about female?
        if (!model.empty())
        {
            size_t pos = Misc::StringUtils::lowerCase(model).find_last_of("."); // pos points at '.'
            if (pos == std::string::npos || model.substr(pos+1) != "nif") // mModel does not end in ".nif"
                return "meshes\\" + model.substr(0, pos) + "_gnd.nif";
        }
        return "";
    }

    std::string ForeignArmor::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref =
            ptr.get<ESM4::Armor>();

        return ref->mBase->mFullName;
    }

    bool ForeignArmor::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref =
            ptr.get<ESM4::Armor>();

        return (ref->mBase->mFullName != "");
    }

    // FIXME
    MWGui::ToolTipInfo ForeignArmor::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref =
            ptr.get<ESM4::Armor>();

        MWGui::ToolTipInfo info;

        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

        int count = ptr.getRefData().getCount();

        info.icon = ref->mBase->mIconMale;  // FIXME: there is also mIconFemale

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        text += MWGui::ToolTips::getValueString(getValue(ptr), "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script"); // FIXME: need to lookup FormId
        }

        info.text = text;

        return info;
    }

    std::pair<std::vector<int>, bool> ForeignArmor::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        std::vector<int> slots_;

        const int size = 7;

        static const int sMapping[size][2] =
        {
            { ESM4::Armor::TES4_Hair,      MWWorld::InventoryStore::Slot_ForeignHair },
            { ESM4::Armor::TES4_UpperBody, MWWorld::InventoryStore::Slot_ForeignUpperBody },
            { ESM4::Armor::TES4_LowerBody, MWWorld::InventoryStore::Slot_ForeignLowerBody },
            { ESM4::Armor::TES4_Hand,      MWWorld::InventoryStore::Slot_ForeignHand },
            { ESM4::Armor::TES4_Foot,      MWWorld::InventoryStore::Slot_ForeignFoot },
            { ESM4::Armor::TES4_Shield,    MWWorld::InventoryStore::Slot_ForeignShield },
            { ESM4::Armor::TES4_Tail,      MWWorld::InventoryStore::Slot_ForeignTail },
        };

        for (int i=0; i<size; ++i)
            if ((sMapping[i][0] & ref->mBase->mArmorFlags) != 0)
                slots_.push_back (int (sMapping[i][1]));

        return std::make_pair (slots_, false);
    }

    int ForeignArmor::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref =
            ptr.get<ESM4::Armor>();

        int value = ref->mBase->mData.value;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

        return value;
    }

    void ForeignArmor::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignArmor);

        registerClass (typeid (ESM4::Armor).name(), instance);
    }

    // Armor and Weapon have "health" - a character will not wear the item if the health is zero.
    std::pair<int, std::string> ForeignArmor::canBeEquipped(const MWWorld::Ptr &ptr, const MWWorld::Ptr &npc) const
    {
        MWWorld::InventoryStore& invStore = npc.getClass().getInventoryStore(npc);

        if (ptr.getCellRef().getCharge() == 0)
            return std::make_pair(0, "#{sInventoryMessage1}");

        // slots that this item can be equipped in
        std::pair<std::vector<int>, bool> slots_ = ptr.getClass().getEquipmentSlots(ptr);

        if (slots_.first.empty())
            return std::make_pair(0, "");

//      if (npc.getClass().isNpc())
//      {
//          std::string npcRace = npc.get<ESM::NPC>()->mBase->mRace;

//          // Beast races cannot equip shoes / boots, or full helms (head part vs hair part)
//          const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(npcRace);
//          if(race->mData.mFlags & ESM::Race::Beast)
//          {
//              std::vector<ESM::PartReference> parts = ptr.get<ESM::Armor>()->mBase->mParts.mParts;

//              for(std::vector<ESM::PartReference>::iterator itr = parts.begin(); itr != parts.end(); ++itr)
//              {
//                  if((*itr).mPart == ESM::PRT_Head)
//                      return std::make_pair(0, "#{sNotifyMessage13}");
//                  if((*itr).mPart == ESM::PRT_LFoot || (*itr).mPart == ESM::PRT_RFoot)
//                      return std::make_pair(0, "#{sNotifyMessage14}");
//              }
//          }
//      }

        for (std::vector<int>::const_iterator slot=slots_.first.begin();
            slot!=slots_.first.end(); ++slot)
        {
            // If equipping a shield, check if there's a twohanded weapon conflicting with it
            if(*slot == MWWorld::InventoryStore::Slot_CarriedLeft)
            {
                MWWorld::ContainerStoreIterator weapon = invStore.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);

                if(weapon == invStore.end())
                    return std::make_pair(1,"");

//              if(weapon->getTypeName() == typeid(ESM::Weapon).name() &&
//                      (weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::LongBladeTwoHand ||
//              weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoClose ||
//              weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::BluntTwoWide ||
//              weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::SpearTwoWide ||
//              weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::AxeTwoHand ||
//              weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanBow ||
//              weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow))
//              {
//                  return std::make_pair(3,"");
//              }
                return std::make_pair(1,"");
            }
        }
        return std::make_pair(1,"");
    }

    MWWorld::Ptr ForeignArmor::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        return MWWorld::Ptr(&cell.get<ESM4::Armor>().insert(*ref), &cell);
    }
}
