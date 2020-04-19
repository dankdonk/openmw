#include "subspace.hpp"

#include <extern/esm4/sbsp.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string Subspace::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Subspace>()->mBase->mEditorId;
    }

    void Subspace::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Subspace> *ref = ptr.get<ESM4::Subspace>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void Subspace::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string Subspace::getModel(const MWWorld::Ptr &ptr) const
    {
#if 0
        MWWorld::LiveCellRef<ESM4::Subspace> *ref = ptr.get<ESM4::Subspace>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
#endif
        return "";
    }

    std::string Subspace::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void Subspace::registerSelf()
    {
        boost::shared_ptr<Class> instance (new Subspace);

        registerClass (typeid (ESM4::Subspace).name(), instance);
    }

    MWWorld::Ptr Subspace::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Subspace> *ref = ptr.get<ESM4::Subspace>();

        return MWWorld::Ptr(&cell.get<ESM4::Subspace>().insert(*ref), &cell);
    }
}
