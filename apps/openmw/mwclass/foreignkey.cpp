#include "foreignkey.hpp"

#include <extern/esm4/keym.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignKey::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Key>()->mBase->mEditorId;
    }

    void ForeignKey::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignKey::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignKey::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignKey::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignKey::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignKey);

        registerClass (typeid (ESM4::Key).name(), instance);
    }

    MWWorld::Ptr ForeignKey::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Key> *ref = ptr.get<ESM4::Key>();

        return MWWorld::Ptr(&cell.get<ESM4::Key>().insert(*ref), &cell);
    }
}
