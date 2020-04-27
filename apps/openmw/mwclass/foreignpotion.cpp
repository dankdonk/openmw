#include "foreignpotion.hpp"

#include <extern/esm4/alch.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/action.hpp"
#include "../mwworld/actionapply.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace MWClass
{
    std::string ForeignPotion::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Potion>()->mBase->mEditorId;
    }

    void ForeignPotion::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
    }

    void ForeignPotion::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignPotion::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignPotion::getName (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        return ref->mBase->mFullName;
    }

    boost::shared_ptr<MWWorld::Action> ForeignPotion::activate (const MWWorld::Ptr& ptr,
        const MWWorld::Ptr& actor) const
    {
        return defaultItemActivate(ptr, actor);
    }

    void ForeignPotion::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignPotion);

        registerClass (typeid (ESM4::Potion).name(), instance);
    }

    bool ForeignPotion::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        return (ref->mBase->mFullName != "");
    }

    MWGui::ToolTipInfo ForeignPotion::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        MWGui::ToolTipInfo info;
        info.caption = ref->mBase->mFullName + MWGui::ToolTips::getCountString(ptr.getRefData().getCount());
        //info.icon = ref->mBase->mIcon;

        std::string text;

        text += "\n#{sWeight}: " + MWGui::ToolTips::toString(ref->mBase->mData.weight);
        text += MWGui::ToolTips::getValueString(ref->mBase->mItem.value, "#{sValue}");

        //info.effects = MWGui::Widgets::MWEffectList::effectListFromESM(&ref->mBase->mEffects);

        // hide effects the player doesnt know about
        MWWorld::Ptr player = MWBase::Environment::get().getWorld ()->getPlayerPtr();
        MWMechanics::NpcStats& npcStats = player.getClass().getNpcStats (player);
#if 0
        int alchemySkill = npcStats.getSkill (ESM::Skill::Alchemy).getBase();
        int i=0;
        static const float fWortChanceValue =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fWortChanceValue")->getFloat();
        for (MWGui::Widgets::SpellEffectList::iterator it = info.effects.begin(); it != info.effects.end(); ++it)
        {
            it->mKnown = (i <= 1 && alchemySkill >= fWortChanceValue)
                 || (i <= 3 && alchemySkill >= fWortChanceValue*2);
            ++i;
        }
#endif
        info.isPotion = true;

        if (MWBase::Environment::get().getWindowManager()->getFullHelp()) {
            text += MWGui::ToolTips::getCellRefString(ptr.getCellRef());
            //text += MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");
        }

        info.text = text;

        return info;
    }

    boost::shared_ptr<MWWorld::Action> ForeignPotion::use (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        boost::shared_ptr<MWWorld::Action> action (
            new MWWorld::ActionApply (ptr, ref->mBase->mEditorId));

        action->setSound ("Drink"); // FIXME

        return action;
    }

    MWWorld::Ptr ForeignPotion::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Potion> *ref = ptr.get<ESM4::Potion>();

        return MWWorld::Ptr(&cell.get<ESM4::Potion>().insert(*ref), &cell);
    }
}
