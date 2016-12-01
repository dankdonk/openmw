#include "foreignsigilstone.hpp"

#include <extern/esm4/sgst.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignSigilStone::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::SigilStone>()->mBase->mEditorId;
    }

    void ForeignSigilStone::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignSigilStone::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignSigilStone::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignSigilStone::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignSigilStone::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignSigilStone);

        registerClass (typeid (ESM4::SigilStone).name(), instance);
    }

    MWWorld::Ptr ForeignSigilStone::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::SigilStone> *ref = ptr.get<ESM4::SigilStone>();

        return MWWorld::Ptr(&cell.get<ESM4::SigilStone>().insert(*ref), &cell);
    }
}
