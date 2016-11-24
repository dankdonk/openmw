#include "foreignbook.hpp"

#include <extern/esm4/book.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignBook::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Book>()->mBase->mEditorId;
    }

    void ForeignBook::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignBook::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignBook::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignBook::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignBook::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignBook);

        registerClass (typeid (ESM4::Book).name(), instance);
    }

    MWWorld::Ptr ForeignBook::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Book> *ref = ptr.get<ESM4::Book>();

        return MWWorld::Ptr(&cell.get<ESM4::Book>().insert(*ref), &cell);
    }
}
