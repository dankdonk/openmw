#include "foreignleveledcreature.hpp"

#include <extern/esm4/lvlc.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignLeveledCreature::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::LeveledCreature>()->mBase->mEditorId;
    }

    void ForeignLeveledCreature::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::LeveledCreature> *ref = ptr.get<ESM4::LeveledCreature>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignLeveledCreature::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignLeveledCreature::getModel(const MWWorld::Ptr &ptr) const
    {
#if 0
        MWWorld::LiveCellRef<ESM4::LeveledCreature> *ref = ptr.get<ESM4::LeveledCreature>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
#endif
        return "";
    }

    std::string ForeignLeveledCreature::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignLeveledCreature::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignLeveledCreature);

        registerClass (typeid (ESM4::LeveledCreature).name(), instance);
    }

    MWWorld::Ptr ForeignLeveledCreature::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::LeveledCreature> *ref = ptr.get<ESM4::LeveledCreature>();

        return MWWorld::Ptr(&cell.get<ESM4::LeveledCreature>().insert(*ref), &cell);
    }
}
