#include "soulgem.hpp"

#include <extern/esm4/slgm.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string SoulGem::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::SoulGem>()->mBase->mEditorId;
    }

    void SoulGem::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void SoulGem::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string SoulGem::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string SoulGem::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void SoulGem::registerSelf()
    {
        boost::shared_ptr<Class> instance (new SoulGem);

        registerClass (typeid (ESM4::SoulGem).name(), instance);
    }

    MWWorld::Ptr SoulGem::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::SoulGem> *ref = ptr.get<ESM4::SoulGem>();

        return MWWorld::Ptr(&cell.get<ESM4::SoulGem>().insert(*ref), &cell);
    }
}
