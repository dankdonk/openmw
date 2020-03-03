#include "foreignclothing.hpp"

#include <extern/esm4/clot.hpp>

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
    std::string ForeignClothing::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Clothing>()->mBase->mEditorId;
    }

    void ForeignClothing::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Clothing> *ref = ptr.get<ESM4::Clothing>();

        if (!model.empty()) {
            // TES4 seems to lack _gnd models for shields
            std::string lowerModel = Misc::StringUtils::lowerCase(model);
            size_t pos = lowerModel.find("ring");
            if (pos != std::string::npos)
            {
                size_t pos = lowerModel.find("_gnd.");
                if (pos != std::string::npos)
                    renderingInterface.getObjects().insertModel(ptr, lowerModel.replace(pos,4,"")/*, !ref->mBase->mPersistent*/); // FIXME
            }
            else
                renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignClothing::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
        {
            // TES4 seems to lack _gnd models for shields
            std::string lowerModel = Misc::StringUtils::lowerCase(model);
            size_t pos = lowerModel.find("ring");
            if (pos != std::string::npos)
            {
                size_t pos = lowerModel.find("_gnd.");
                if (pos != std::string::npos)
                    physics.addObject(ptr, lowerModel.replace(pos,4,""));
            }
            else
                physics.addObject(ptr, model);
        }
    }

    std::string ForeignClothing::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Clothing> *ref = ptr.get<ESM4::Clothing>();
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

    std::string ForeignClothing::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    bool ForeignClothing::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Clothing> *ref =
            ptr.get<ESM4::Clothing>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignClothing::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Clothing> *ref =
            ptr.get<ESM4::Clothing>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->mBase->mIconMale; // FIXME: there is also mIconFemale

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.value, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script"); // FIXME: need to lookup FormId
        }

        info.enchant = "";//ref->mBase->mEnchantment;  // FIXME: need to lookup FormId
        if (!info.enchant.empty())
            info.remainingEnchantCharge = static_cast<int>(ptr.getCellRef().getEnchantmentCharge());

        info.text = text;

        return info;
    }

    std::pair<std::vector<int>, bool> ForeignClothing::getEquipmentSlots (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Clothing> *ref = ptr.get<ESM4::Clothing>();

        std::vector<int> slots_;

        const int size = 10;

        static const int sMapping[size][2] =
        {
            { ESM4::Armor::TES4_Head,      MWWorld::InventoryStore::Slot_ForeignHead },
            { ESM4::Armor::TES4_Hair,      MWWorld::InventoryStore::Slot_ForeignHair },
            { ESM4::Armor::TES4_UpperBody, MWWorld::InventoryStore::Slot_ForeignUpperBody },
            { ESM4::Armor::TES4_LowerBody, MWWorld::InventoryStore::Slot_ForeignLowerBody },
            { ESM4::Armor::TES4_Hand,      MWWorld::InventoryStore::Slot_ForeignHand },
            { ESM4::Armor::TES4_Foot,      MWWorld::InventoryStore::Slot_ForeignFoot },
            { ESM4::Armor::TES4_RightRing, MWWorld::InventoryStore::Slot_ForeignRightRing },
            { ESM4::Armor::TES4_LeftRing,  MWWorld::InventoryStore::Slot_ForeignLeftRing },
            { ESM4::Armor::TES4_Amulet,    MWWorld::InventoryStore::Slot_ForeignAmulet },
            { ESM4::Armor::TES4_Tail,      MWWorld::InventoryStore::Slot_ForeignTail }, // ??
        };

        for (int i=0; i<size; ++i)
            if ((sMapping[i][0] & ref->mBase->mClothingFlags) != 0)
                slots_.push_back (int (sMapping[i][1]));

        return std::make_pair (slots_, false);
    }

    int ForeignClothing::getValue (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Clothing> *ref =
            ptr.get<ESM4::Clothing>();

        int value = ref->mBase->mData.value;
        if (ptr.getCellRef().getGoldValue() > 1 && ptr.getRefData().getCount() == 1)
            value = ptr.getCellRef().getGoldValue();

        return value;
    }

    void ForeignClothing::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignClothing);

        registerClass (typeid (ESM4::Clothing).name(), instance);
    }

    std::pair<int, std::string> ForeignClothing::canBeEquipped(const MWWorld::Ptr &ptr, const MWWorld::Ptr &npc) const
    {
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
//              std::vector<ESM::PartReference> parts = ptr.get<ESM::Clothing>()->mBase->mParts.mParts;

//              for(std::vector<ESM::PartReference>::iterator itr = parts.begin(); itr != parts.end(); ++itr)
//              {
//                  if((*itr).mPart == ESM::PRT_Head)
//                      return std::make_pair(0, "#{sNotifyMessage13}");
//                  if((*itr).mPart == ESM::PRT_LFoot || (*itr).mPart == ESM::PRT_RFoot)
//                      return std::make_pair(0, "#{sNotifyMessage15}");
//              }
//          }
//      }

        return std::make_pair (1, "");
    }

    MWWorld::Ptr ForeignClothing::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Clothing> *ref = ptr.get<ESM4::Clothing>();

        return MWWorld::Ptr(&cell.get<ESM4::Clothing>().insert(*ref), &cell);
    }
}
