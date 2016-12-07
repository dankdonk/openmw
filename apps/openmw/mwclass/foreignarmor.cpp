#include "foreignarmor.hpp"

#include <extern/esm4/armo.hpp>

#include <components/misc/stringops.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignArmor::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Armor>()->mBase->mEditorId;
    }

    void ForeignArmor::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignArmor::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignArmor::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();
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

    std::string ForeignArmor::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignArmor::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignArmor);

        registerClass (typeid (ESM4::Armor).name(), instance);
    }

    MWWorld::Ptr ForeignArmor::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Armor> *ref = ptr.get<ESM4::Armor>();

        return MWWorld::Ptr(&cell.get<ESM4::Armor>().insert(*ref), &cell);
    }
}
