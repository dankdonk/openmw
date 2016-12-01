#include "foreignclothing.hpp"

#include <extern/esm4/clot.hpp>

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
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignClothing::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignClothing::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Clothing> *ref = ptr.get<ESM4::Clothing>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignClothing::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
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
