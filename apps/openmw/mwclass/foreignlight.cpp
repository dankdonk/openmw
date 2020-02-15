#include "foreignlight.hpp"

#include <iostream> // FIXME: testing only

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

        // some LIGH records have models (and most likely will have "AttachLight" node/bone) -
        // for those insertLight should specify to attach
        //
        // some ACTI records (e.g. Lights\TorchTall01.NIF) also have "AttachLight" node/bone -
        // thse may also have associated scripts for damage
        //
        // some ACTI, MISC or STAT records may be torches - not sure what kind of light to attach
        if (!model.empty()) {
            //std::cout << "Foreign light has model " << model << std::endl;
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
        else
            renderingInterface.getObjects().insertLight(ptr);
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
