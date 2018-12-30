#include "foreignbook.hpp"

#include <extern/esm4/book.hpp>

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
    std::string ForeignBook::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Book>()->mBase->mEditorId;
    }

    void ForeignBook::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignBook::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignBook::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignBook::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignBook::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignBook);

        registerClass (typeid (ESM4::Book).name(), instance);
    }

    MWWorld::Ptr ForeignBook::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();

        return MWWorld::Ptr(&cell.get<ESM4::Book>().insert(*ref), &cell);
    }

    bool ForeignBook::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref =
            ptr.get<ESM4::Book>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignBook::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref =
            ptr.get<ESM4::Book>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        text += MWGui::ToolTips::getValueString(ref->mBase->mData.value, "#{sValue}");

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script"); // FIXME: need to lookup FormId
        }

        //info.enchant = ref->mBase->mEnchant; // FIXME: need to look up FormId

        info.text = text;

        return info;
    }
}
