#include "foreigningredient.hpp"

#include <extern/esm4/ingr.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignIngredient::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Ingredient>()->mBase->mEditorId;
    }

    void ForeignIngredient::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Ingredient> *ref = ptr.get<ESM4::Ingredient>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignIngredient::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignIngredient::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Ingredient> *ref = ptr.get<ESM4::Ingredient>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignIngredient::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignIngredient::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignIngredient);

        registerClass (typeid (ESM4::Ingredient).name(), instance);
    }

    MWWorld::Ptr ForeignIngredient::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Ingredient> *ref = ptr.get<ESM4::Ingredient>();

        return MWWorld::Ptr(&cell.get<ESM4::Ingredient>().insert(*ref), &cell);
    }
}
