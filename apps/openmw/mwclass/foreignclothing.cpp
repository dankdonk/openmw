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
        std::string model = ref->mBase->mModel;
        if (!model.empty())
        {
            size_t pos = Misc::StringUtils::lowerCase(model).find_last_of(".nif"); // pos points at 'f'
            if (pos != std::string::npos) // mModel does not end in ".nif"
                return "meshes\\" + model.substr(0, pos-3) + "_gnd.nif";
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

    MWWorld::Ptr ForeignClothing::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Clothing> *ref = ptr.get<ESM4::Clothing>();

        return MWWorld::Ptr(&cell.get<ESM4::Clothing>().insert(*ref), &cell);
    }
}
