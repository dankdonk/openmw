#include "foreignweapon.hpp"

#include <extern/esm4/weap.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignWeapon::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Weapon>()->mBase->mEditorId;
    }

    void ForeignWeapon::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignWeapon::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignWeapon::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignWeapon::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignWeapon::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignWeapon);

        registerClass (typeid (ESM4::Weapon).name(), instance);
    }

    MWWorld::Ptr ForeignWeapon::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Weapon> *ref = ptr.get<ESM4::Weapon>();

        return MWWorld::Ptr(&cell.get<ESM4::Weapon>().insert(*ref), &cell);
    }
}
