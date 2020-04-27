#include "foreignammo.hpp"

#include <extern/esm4/ammo.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace MWClass
{
    std::string ForeignAmmo::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Ammo>()->mBase->mEditorId;
    }

    void ForeignAmmo::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Ammo> *ref = ptr.get<ESM4::Ammo>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignAmmo::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignAmmo::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammo> *ref = ptr.get<ESM4::Ammo>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignAmmo::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammo> *ref = ptr.get<ESM4::Ammo>();

        return ref->mBase->mFullName;
    }

    void ForeignAmmo::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignAmmo);

        registerClass (typeid (ESM4::Ammo).name(), instance);
    }

    bool ForeignAmmo::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammo> *ref = ptr.get<ESM4::Ammo>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignAmmo::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ammo> *ref = ptr.get<ESM4::Ammo>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        //info.icon = ref->mBase->mIcon;

        std::string text;

        text += MWGui::ToolTips::getValueString(ref->mBase->mData.value, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    MWWorld::Ptr ForeignAmmo::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Ammo> *ref = ptr.get<ESM4::Ammo>();

        return MWWorld::Ptr(&cell.get<ESM4::Ammo>().insert(*ref), &cell);
    }
}
