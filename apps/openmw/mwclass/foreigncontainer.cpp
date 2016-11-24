#include "foreigncontainer.hpp"

#include <extern/esm4/cont.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignContainer::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Container>()->mBase->mEditorId;
    }

    void ForeignContainer::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Container> *ref = ptr.get<ESM4::Container>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignContainer::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignContainer::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Container> *ref = ptr.get<ESM4::Container>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignContainer::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignContainer::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignContainer);

        registerClass (typeid (ESM4::Container).name(), instance);
    }

    MWWorld::Ptr ForeignContainer::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Container> *ref = ptr.get<ESM4::Container>();

        return MWWorld::Ptr(&cell.get<ESM4::Container>().insert(*ref), &cell);
    }
}
