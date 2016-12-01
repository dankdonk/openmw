#include "foreignsubspace.hpp"

#include <extern/esm4/sbsp.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignSubspace::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Subspace>()->mBase->mEditorId;
    }

    void ForeignSubspace::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Subspace> *ref = ptr.get<ESM4::Subspace>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignSubspace::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignSubspace::getModel(const MWWorld::Ptr &ptr) const
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

    std::string ForeignSubspace::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignSubspace::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignSubspace);

        registerClass (typeid (ESM4::Subspace).name(), instance);
    }

    MWWorld::Ptr ForeignSubspace::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Subspace> *ref = ptr.get<ESM4::Subspace>();

        return MWWorld::Ptr(&cell.get<ESM4::Subspace>().insert(*ref), &cell);
    }
}
