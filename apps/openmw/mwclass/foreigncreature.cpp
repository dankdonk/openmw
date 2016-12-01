#include "foreigncreature.hpp"

#include <extern/esm4/crea.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignCreature::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Creature>()->mBase->mEditorId;
    }

    void ForeignCreature::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignCreature::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignCreature::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignCreature::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignCreature::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignCreature);

        registerClass (typeid (ESM4::Creature).name(), instance);
    }

    MWWorld::Ptr ForeignCreature::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();

        return MWWorld::Ptr(&cell.get<ESM4::Creature>().insert(*ref), &cell);
    }
}
