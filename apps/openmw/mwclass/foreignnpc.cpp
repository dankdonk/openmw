#include "foreignnpc.hpp"

#include <extern/esm4/npc_.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignNpc::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Npc>()->mBase->mEditorId;
    }

    void ForeignNpc::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignNpc::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignNpc::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignNpc::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignNpc::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignNpc);

        registerClass (typeid (ESM4::Npc).name(), instance);
    }

    MWWorld::Ptr ForeignNpc::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();

        return MWWorld::Ptr(&cell.get<ESM4::Npc>().insert(*ref), &cell);
    }
}
