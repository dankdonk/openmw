#include "foreignapparatus.hpp"

#include <extern/esm4/appa.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignApparatus::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Apparatus>()->mBase->mEditorId;
    }

    void ForeignApparatus::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignApparatus::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignApparatus::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignApparatus::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignApparatus::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignApparatus);

        registerClass (typeid (ESM4::Apparatus).name(), instance);
    }

    MWWorld::Ptr ForeignApparatus::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Apparatus> *ref = ptr.get<ESM4::Apparatus>();

        return MWWorld::Ptr(&cell.get<ESM4::Apparatus>().insert(*ref), &cell);
    }
}
