#include "foreignsoulgem.hpp"

#include <extern/esm4/slgm.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignSoulGem::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::SoulGem>()->mBase->mEditorId;
    }

    void ForeignSoulGem::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignSoulGem::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignSoulGem::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignSoulGem::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignSoulGem::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignSoulGem);

        registerClass (typeid (ESM4::SoulGem).name(), instance);
    }

    MWWorld::Ptr ForeignSoulGem::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();

        return MWWorld::Ptr(&cell.get<ESM4::SoulGem>().insert(*ref), &cell);
    }
}
