#include "foreignstatic.hpp"

#include <extern/esm4/stat.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignStatic::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Static>()->mBase->mEditorId;
    }

    void ForeignStatic::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Static> *ref = ptr.get<ESM4::Static>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignStatic::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignStatic::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Static> *ref = ptr.get<ESM4::Static>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignStatic::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignStatic::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignStatic);

        registerClass (typeid (ESM4::Static).name(), instance);
    }

    MWWorld::Ptr ForeignStatic::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Static> *ref = ptr.get<ESM4::Static>();

        return MWWorld::Ptr(&cell.get<ESM4::Static>().insert(*ref), &cell);
    }
}
