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
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignArmor::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignArmor::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();
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

    MWWorld::Ptr ForeignArmor::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        return MWWorld::Ptr(&cell.get<ESM4::Armor>().insert(*ref), &cell);
    }
}
