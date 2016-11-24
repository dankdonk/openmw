#include "foreignactivator.hpp"

#include <extern/esm4/acti.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignActivator::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Activator>()->mBase->mEditorId;
    }

    void ForeignActivator::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Activator> *ref = ptr.get<ESM4::Activator>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignActivator::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignActivator::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Activator> *ref = ptr.get<ESM4::Activator>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignActivator::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignActivator::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignActivator);

        registerClass (typeid (ESM4::Activator).name(), instance);
    }

    MWWorld::Ptr ForeignActivator::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Activator> *ref = ptr.get<ESM4::Activator>();

        return MWWorld::Ptr(&cell.get<ESM4::Activator>().insert(*ref), &cell);
    }
}
