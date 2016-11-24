#include "foreignlight.hpp"

#include <extern/esm4/ligh.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignLight::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Light>()->mBase->mEditorId;
    }

    void ForeignLight::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignLight::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignLight::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignLight::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignLight::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignLight);

        registerClass (typeid (ESM4::Light).name(), instance);
    }

    MWWorld::Ptr ForeignLight::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Light> *ref = ptr.get<ESM4::Light>();

        return MWWorld::Ptr(&cell.get<ESM4::Light>().insert(*ref), &cell);
    }
}
