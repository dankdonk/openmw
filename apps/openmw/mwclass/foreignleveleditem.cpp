#include "foreignleveleditem.hpp"

#include <extern/esm4/lvli.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignLeveledItem::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::LeveledItem>()->mBase->mEditorId;
    }

    void ForeignLeveledItem::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::LeveledItem> *ref = ptr.get<ESM4::LeveledItem>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignLeveledItem::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignLeveledItem::getModel(const MWWorld::Ptr &ptr) const
    {
#if 0
        MWWorld::LiveCellRef<ESM4::LeveledItem> *ref = ptr.get<ESM4::LeveledItem>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
#endif
        return "";
    }

    std::string ForeignLeveledItem::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignLeveledItem::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignLeveledItem);

        registerClass (typeid (ESM4::LeveledItem).name(), instance);
    }

    MWWorld::Ptr ForeignLeveledItem::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::LeveledItem> *ref = ptr.get<ESM4::LeveledItem>();

        return MWWorld::Ptr(&cell.get<ESM4::LeveledItem>().insert(*ref), &cell);
    }
}
