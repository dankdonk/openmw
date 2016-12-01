#include "foreignpotion.hpp"

#include <extern/esm4/alch.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignPotion::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Potion>()->mBase->mEditorId;
    }

    void ForeignPotion::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignPotion::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignPotion::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignPotion::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignPotion::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignPotion);

        registerClass (typeid (ESM4::Potion).name(), instance);
    }

    MWWorld::Ptr ForeignPotion::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        return MWWorld::Ptr(&cell.get<ESM4::Potion>().insert(*ref), &cell);
    }
}
