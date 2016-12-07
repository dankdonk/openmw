#include "foreignclothing.hpp"

#include <extern/esm4/clot.hpp>

#include <components/misc/stringops.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignClothing::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Clothing>()->mBase->mEditorId;
    }

    void ForeignClothing::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Clothing> *ref = ptr.get<ESM4::Clothing>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignClothing::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignClothing::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Clothing> *ref = ptr.get<ESM4::Clothing>();
        assert(ref->mBase != NULL);

        // clothing and armor need "ground" models (with physics) unless being worn
        std::string model = ref->mBase->mModel;
        if (!model.empty())
        {
            size_t pos = Misc::StringUtils::lowerCase(model).find_last_of(".nif"); // pos points at 'f'
            if (pos != std::string::npos) // mModel does not end in ".nif"
                return "meshes\\" + model.substr(0, pos-3) + "_gnd.nif";
        }
        return "";
    }

    std::string ForeignClothing::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignClothing::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignClothing);

        registerClass (typeid (ESM4::Clothing).name(), instance);
    }

    MWWorld::Ptr ForeignClothing::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Clothing> *ref = ptr.get<ESM4::Clothing>();

        return MWWorld::Ptr(&cell.get<ESM4::Clothing>().insert(*ref), &cell);
    }
}
