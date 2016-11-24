#include "foreignmiscitem.hpp"

#include <extern/esm4/misc.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignMiscItem::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::MiscItem>()->mBase->mEditorId;
    }

    void ForeignMiscItem::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignMiscItem::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignMiscItem::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignMiscItem::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignMiscItem::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignMiscItem);

        registerClass (typeid (ESM4::MiscItem).name(), instance);
    }

    MWWorld::Ptr ForeignMiscItem::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::MiscItem> *ref = ptr.get<ESM4::MiscItem>();

        return MWWorld::Ptr(&cell.get<ESM4::MiscItem>().insert(*ref), &cell);
    }
}
