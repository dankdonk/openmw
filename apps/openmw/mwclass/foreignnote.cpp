#include "foreignnote.hpp"

#include <extern/esm4/note.hpp>

#include "../mwgui/tooltips.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignNote::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Note>()->mBase->mEditorId;
    }

    void ForeignNote::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignNote::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignNote::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignNote::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignNote::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignNote);

        registerClass (typeid (ESM4::Note).name(), instance);
    }

    MWWorld::Ptr ForeignNote::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>();

        return MWWorld::Ptr(&cell.get<ESM4::Note>().insert(*ref), &cell);
    }

    bool ForeignNote::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignNote::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Note> *ref = ptr.get<ESM4::Note>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        //info.icon = ref->mBase->mIcon;

        std::string text;

        //text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        //text += MWGui::ToolTips::getValueString(ref->mBase->mData.value, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script"); // FIXME: need to lookup FormId
        }

        //info.enchant = ref->mBase->mEnchant; // FIXME: need to look up FormId

        info.text = text;

        return info;
    }
}
