#include "foreignarmor.hpp"

#include <extern/esm4/armo.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignArmor::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Armor>()->mBase->mEditorId;
    }

    void ForeignArmor::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignArmor::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignArmor::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignArmor::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignArmor::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignArmor);

        registerClass (typeid (ESM4::Armor).name(), instance);
    }

    MWWorld::Ptr ForeignArmor::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        return MWWorld::Ptr(&cell.get<ESM4::Armor>().insert(*ref), &cell);
    }
}
