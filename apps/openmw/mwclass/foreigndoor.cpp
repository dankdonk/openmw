#include "foreigndoor.hpp"

#include <extern/esm4/door.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignDoor::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Door>()->mBase->mEditorId;
    }

    void ForeignDoor::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref = ptr.get<ESM4::Door>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignDoor::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignDoor::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref = ptr.get<ESM4::Door>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignDoor::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignDoor::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignDoor);

        registerClass (typeid (ESM4::Door).name(), instance);
    }

    MWWorld::Ptr ForeignDoor::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Door> *ref = ptr.get<ESM4::Door>();

        return MWWorld::Ptr(&cell.get<ESM4::Door>().insert(*ref), &cell);
    }
}
