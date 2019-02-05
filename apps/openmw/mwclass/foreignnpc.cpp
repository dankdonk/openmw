#include "foreignnpc.hpp"

#include <extern/esm4/npc_.hpp>
#include <extern/esm4/formid.hpp> // FIXME mainly for debugging

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/npcstats.hpp"
#include "../mwmechanics/movement.hpp"

#include "../mwrender/actors.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwgui/tooltips.hpp"

namespace
{
    struct ForeignNpcCustomData : public MWWorld::CustomData
    {
        MWMechanics::NpcStats mNpcStats;
        MWMechanics::Movement mMovement;
        MWWorld::InventoryStore mInventoryStore;

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *ForeignNpcCustomData::clone() const
    {
        return new ForeignNpcCustomData (*this);
    }
}

namespace MWClass
{
    std::string ForeignNpc::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Npc>()->mBase->mEditorId;
    }

    void ForeignNpc::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        renderingInterface.getActors().insertNPC(ptr);
#if 0
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
#endif
    }

    void ForeignNpc::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        physics.addActor(ptr, model);
        MWBase::Environment::get().getMechanicsManager()->add(ptr);
#if 0
        if(!model.empty())
            physics.addObject(ptr, model);
#endif
    }

    std::string ForeignNpc::getModel(const MWWorld::Ptr &ptr) const
    {
#if 0
        MWWorld::LiveCellRef<ESM::NPC> *ref =
            ptr.get<ESM4::Npc>();
        assert(ref->mBase != NULL);

        std::string model = "meshes\\skeleton.nif";
        return model;
#endif
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            //std::cout << "ForeignNpc: " << model << std::endl;
            return "meshes\\" + model;
        }
        return "";
    }

    std::string ForeignNpc::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    MWMechanics::CreatureStats& ForeignNpc::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mNpcStats;
    }

    MWMechanics::NpcStats& ForeignNpc::getNpcStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mNpcStats;
    }

    MWWorld::ContainerStore& ForeignNpc::getContainerStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    MWWorld::InventoryStore& ForeignNpc::getInventoryStore (const MWWorld::Ptr& ptr)
        const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    bool ForeignNpc::hasToolTip (const MWWorld::Ptr& ptr) const
    {
        //FIXME
        //return !ptr.getClass().getCreatureStats(ptr).getAiSequence().isInCombat() || getCreatureStats(ptr).isDead();
        return true;
    }

    MWGui::ToolTipInfo ForeignNpc::getToolTipInfo (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();

        bool fullHelp = MWBase::Environment::get().getWindowManager()->getFullHelp();
        MWGui::ToolTipInfo info;

        info.caption = getName(ptr);
        if(fullHelp && getNpcStats(ptr).isWerewolf())
        {
            info.caption += " (";
            info.caption += ref->mBase->mEditorId;
            info.caption += ")";
        }

        //if(fullHelp)
            //info.text = MWGui::ToolTips::getMiscString(ref->mBase->mScript, "Script");

        return info;
    }

    void ForeignNpc::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignNpc);

        registerClass (typeid (ESM4::Npc).name(), instance);
    }

    MWWorld::Ptr ForeignNpc::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();

        return MWWorld::Ptr(&cell.get<ESM4::Npc>().insert(*ref), &cell);
    }

    int ForeignNpc::getSkill(const MWWorld::Ptr& ptr, int skill) const
    {
        // FIXME
        //return ptr.getClass().getNpcStats(ptr).getSkill(skill).getModified();
        return 15;
    }

    int ForeignNpc::getBloodTexture(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();

        // FIXME
#if 0
        if (ref->mBase->mFlags & ESM::NPC::Skeleton)
            return 1;
        if (ref->mBase->mFlags & ESM::NPC::Metal)
            return 2;
#endif
        return 0;
    }

    void ForeignNpc::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<ForeignNpcCustomData> data(new ForeignNpcCustomData);

            MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();

            // creature stats
            int gold = 0;
            if ((ref->mBase->mBaseConfig.flags & /*ACBS_Autocalcstats*/0x000010) != 0)
            {
                gold = ref->mBase->mBaseConfig.barterGold;

                data->mNpcStats.getSkill (ESM::Skill::Block).setBase (ref->mBase->mData.skills.block);
                data->mNpcStats.getSkill (ESM::Skill::Armorer).setBase (ref->mBase->mData.skills.armorer);
                data->mNpcStats.getSkill (ESM::Skill::MediumArmor).setBase (0);
                data->mNpcStats.getSkill (ESM::Skill::HeavyArmor).setBase (ref->mBase->mData.skills.heavyArmor);
                data->mNpcStats.getSkill (ESM::Skill::BluntWeapon).setBase (ref->mBase->mData.skills.blunt);
                data->mNpcStats.getSkill (ESM::Skill::LongBlade).setBase (ref->mBase->mData.skills.blade);
                data->mNpcStats.getSkill (ESM::Skill::Axe).setBase (ref->mBase->mData.skills.blunt);
                data->mNpcStats.getSkill (ESM::Skill::Spear).setBase (0);
                data->mNpcStats.getSkill (ESM::Skill::Athletics).setBase (ref->mBase->mData.skills.athletics);
                data->mNpcStats.getSkill (ESM::Skill::Enchant).setBase (0);
                data->mNpcStats.getSkill (ESM::Skill::Destruction).setBase (ref->mBase->mData.skills.destruction);
                data->mNpcStats.getSkill (ESM::Skill::Alteration).setBase (ref->mBase->mData.skills.alteration);
                data->mNpcStats.getSkill (ESM::Skill::Illusion).setBase (ref->mBase->mData.skills.illusion);
                data->mNpcStats.getSkill (ESM::Skill::Conjuration).setBase (ref->mBase->mData.skills.conjuration);
                data->mNpcStats.getSkill (ESM::Skill::Mysticism).setBase (ref->mBase->mData.skills.mysticism);
                data->mNpcStats.getSkill (ESM::Skill::Restoration).setBase (ref->mBase->mData.skills.restoration);
                data->mNpcStats.getSkill (ESM::Skill::Alchemy).setBase (ref->mBase->mData.skills.alchemy);
                data->mNpcStats.getSkill (ESM::Skill::Unarmored).setBase (0);
                data->mNpcStats.getSkill (ESM::Skill::Security).setBase (ref->mBase->mData.skills.security);
                data->mNpcStats.getSkill (ESM::Skill::Sneak).setBase (ref->mBase->mData.skills.sneak);
                data->mNpcStats.getSkill (ESM::Skill::Acrobatics).setBase (ref->mBase->mData.skills.acrobatics);
                data->mNpcStats.getSkill (ESM::Skill::LightArmor).setBase (ref->mBase->mData.skills.lightArmor);
                data->mNpcStats.getSkill (ESM::Skill::ShortBlade).setBase (ref->mBase->mData.skills.blade);
                data->mNpcStats.getSkill (ESM::Skill::Marksman).setBase (ref->mBase->mData.skills.marksman);
                data->mNpcStats.getSkill (ESM::Skill::Mercantile).setBase (ref->mBase->mData.skills.mercantile);
                data->mNpcStats.getSkill (ESM::Skill::Speechcraft).setBase (ref->mBase->mData.skills.speechcraft);
                data->mNpcStats.getSkill (ESM::Skill::HandToHand).setBase (ref->mBase->mData.skills.handToHand);


                data->mNpcStats.setAttribute(ESM::Attribute::Strength, ref->mBase->mData.attribs.strength);
                data->mNpcStats.setAttribute(ESM::Attribute::Intelligence, ref->mBase->mData.attribs.intelligence);
                data->mNpcStats.setAttribute(ESM::Attribute::Willpower, ref->mBase->mData.attribs.willpower);
                data->mNpcStats.setAttribute(ESM::Attribute::Agility, ref->mBase->mData.attribs.agility);
                data->mNpcStats.setAttribute(ESM::Attribute::Speed, ref->mBase->mData.attribs.speed);
                data->mNpcStats.setAttribute(ESM::Attribute::Endurance, ref->mBase->mData.attribs.endurance);
                data->mNpcStats.setAttribute(ESM::Attribute::Personality, ref->mBase->mData.attribs.personality);
                data->mNpcStats.setAttribute(ESM::Attribute::Luck, ref->mBase->mData.attribs.luck);

                data->mNpcStats.setHealth (ref->mBase->mData.health); // FIXME: uint32 to float
                data->mNpcStats.setMagicka (ref->mBase->mBaseConfig.baseSpell);
                data->mNpcStats.setFatigue (ref->mBase->mBaseConfig.fatigue);

                data->mNpcStats.setLevel(ref->mBase->mBaseConfig.level);
                data->mNpcStats.setBaseDisposition(0); // see http://www.uesp.net/wiki/Oblivion:Disposition
                // Fame is probably a global (GLOB)
                data->mNpcStats.setReputation(0); // see http://www.uesp.net/wiki/Oblivion:Fame

                data->mNpcStats.setNeedRecalcDynamicStats(false);
            }
            else // FIXME autocalc
            {
                gold = ref->mBase->mBaseConfig.barterGold;

                data->mNpcStats.setHealth (ref->mBase->mData.health); // FIXME: uint32 to float
                data->mNpcStats.setMagicka (ref->mBase->mBaseConfig.baseSpell);
                data->mNpcStats.setFatigue (ref->mBase->mBaseConfig.fatigue);

                for (int i=0; i<3; ++i)
                    data->mNpcStats.setDynamic (i, 10);

                data->mNpcStats.setLevel(ref->mBase->mBaseConfig.level);
                data->mNpcStats.setBaseDisposition(0);
                data->mNpcStats.setReputation(0);

                //autoCalculateAttributes(ref->mBase, data->mNpcStats);
                //autoCalculateSkills(ref->mBase, data->mNpcStats, ptr);

                data->mNpcStats.setNeedRecalcDynamicStats(true);
            }
#if 0
            // race powers
            const ESM::Race *race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);
            for (std::vector<std::string>::const_iterator iter (race->mPowers.mList.begin());
                iter!=race->mPowers.mList.end(); ++iter)
            {
                if (const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(*iter))
                    data->mNpcStats.getSpells().add (spell);
                else
                    std::cerr << "Warning: ignoring nonexistent race power '" << *iter << "' on NPC '" << ref->mBase->mId << "'" << std::endl;
            }

#endif
#if 0
            if (!ref->mBase->mFaction.empty())
            {
                static const int iAutoRepFacMod = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("iAutoRepFacMod")->getInt();
                static const int iAutoRepLevMod = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>()
                        .find("iAutoRepLevMod")->getInt();
                int rank = ref->mBase->getFactionRank();

                data->mNpcStats.setReputation(iAutoRepFacMod * (rank+1) + iAutoRepLevMod * (data->mNpcStats.getLevel()-1));
            }
#endif
#if 0
            data->mNpcStats.getAiSequence().fill(ref->mBase->mAiPackage);

            data->mNpcStats.setAiSetting (MWMechanics::CreatureStats::AI_Hello, ref->mBase->mAiData.mHello);
            data->mNpcStats.setAiSetting (MWMechanics::CreatureStats::AI_Fight, ref->mBase->mAiData.mFight);
            data->mNpcStats.setAiSetting (MWMechanics::CreatureStats::AI_Flee, ref->mBase->mAiData.mFlee);
            data->mNpcStats.setAiSetting (MWMechanics::CreatureStats::AI_Alarm, ref->mBase->mAiData.mAlarm);

            // spells
            for (std::vector<std::string>::const_iterator iter (ref->mBase->mSpells.mList.begin());
                iter!=ref->mBase->mSpells.mList.end(); ++iter)
            {
                if (const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(*iter))
                    data->mNpcStats.getSpells().add (spell);
                else
                {
                    /// \todo add option to make this a fatal error message pop-up, but default to warning for vanilla compatibility
                    std::cerr << "Warning: ignoring nonexistent spell '" << *iter << "' on NPC '" << ref->mBase->mId << "'" << std::endl;
                }
            }
#endif

            // inventory
            // setting ownership is used to make the NPC auto-equip his initial equipment only, and not bartered items
            ESM::InventoryList inventory;
            for (unsigned int i = 0; i < ref->mBase->mInventory.size(); ++i)
            {
                ESM::ContItem item;
                item.mCount = ref->mBase->mInventory.at(i).count;
                item.mItem.assign(ESM4::formIdToString(ref->mBase->mInventory.at(i).item)); // FIXME

                inventory.mList.push_back(item);
            }

            data->mInventoryStore.fill(inventory, getId(ptr));

            data->mNpcStats.setGoldPool(gold);

            // store
            ptr.getRefData().setCustomData (data.release());

            getInventoryStore(ptr).autoEquip(ptr);
        }
    }

    MWMechanics::Movement& ForeignNpc::getMovementSettings (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignNpcCustomData&> (*ptr.getRefData().getCustomData()).mMovement;
    }

    Ogre::Vector3 ForeignNpc::getMovementVector (const MWWorld::Ptr& ptr) const
    {
        MWMechanics::Movement &movement = getMovementSettings(ptr);
        Ogre::Vector3 vec(movement.mPosition);
        movement.mPosition[0] = 0.0f;
        movement.mPosition[1] = 0.0f;
        movement.mPosition[2] = 0.0f;
        return vec;
    }

    Ogre::Vector3 ForeignNpc::getRotationVector (const MWWorld::Ptr& ptr) const
    {
        MWMechanics::Movement &movement = getMovementSettings(ptr);
        Ogre::Vector3 vec(movement.mRotation);
        movement.mRotation[0] = 0.0f;
        movement.mRotation[1] = 0.0f;
        movement.mRotation[2] = 0.0f;
        return vec;
    }

    float ForeignNpc::getCapacity (const MWWorld::Ptr& ptr) const
    {
        return 1.f;
#if 0
        const MWMechanics::CreatureStats& stats = getCreatureStats (ptr);
        static const float fEncumbranceStrMult = MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>().find("fEncumbranceStrMult")->getFloat();
        return stats.getAttribute(0).getModified()*fEncumbranceStrMult;
#endif
    }

    float ForeignNpc::getArmorRating (const MWWorld::Ptr& ptr) const
    {
        return 1.f;
#if 0
        const MWBase::World *world = MWBase::Environment::get().getWorld();
        const MWWorld::Store<ESM::GameSetting> &store = world->getStore().get<ESM::GameSetting>();

        MWMechanics::NpcStats &stats = getNpcStats(ptr);
        MWWorld::InventoryStore &invStore = getInventoryStore(ptr);

        float fUnarmoredBase1 = store.find("fUnarmoredBase1")->getFloat();
        float fUnarmoredBase2 = store.find("fUnarmoredBase2")->getFloat();
        int unarmoredSkill = stats.getSkill(ESM::Skill::Unarmored).getModified();

        int ratings[MWWorld::InventoryStore::Slots];
        for(int i = 0;i < MWWorld::InventoryStore::Slots;i++)
        {
            MWWorld::ContainerStoreIterator it = invStore.getSlot(i);
            if (it == invStore.end() || it->getTypeName() != typeid(ESM::Armor).name())
            {
                // unarmored
                ratings[i] = static_cast<int>((fUnarmoredBase1 * unarmoredSkill) * (fUnarmoredBase2 * unarmoredSkill));
            }
            else
            {
                ratings[i] = it->getClass().getEffectiveArmorRating(*it, ptr);
            }
        }

        float shield = stats.getMagicEffects().get(ESM::MagicEffect::Shield).getMagnitude();

        return ratings[MWWorld::InventoryStore::Slot_Cuirass] * 0.3f
                + (ratings[MWWorld::InventoryStore::Slot_CarriedLeft] + ratings[MWWorld::InventoryStore::Slot_Helmet]
                    + ratings[MWWorld::InventoryStore::Slot_Greaves] + ratings[MWWorld::InventoryStore::Slot_Boots]
                    + ratings[MWWorld::InventoryStore::Slot_LeftPauldron] + ratings[MWWorld::InventoryStore::Slot_RightPauldron]
                    ) * 0.1f
                + (ratings[MWWorld::InventoryStore::Slot_LeftGauntlet] + ratings[MWWorld::InventoryStore::Slot_RightGauntlet])
                    * 0.05f
                + shield;
#endif
    }

    float ForeignNpc::getEncumbrance (const MWWorld::Ptr& ptr) const
    {
        const MWMechanics::NpcStats &stats = getNpcStats(ptr);

        // According to UESP, inventory weight is ignored in werewolf form. Does that include
        // feather and burden effects?
        float weight = 0.0f;
        if(!stats.isWerewolf())
        {
            weight  = getContainerStore(ptr).getWeight();
            weight -= stats.getMagicEffects().get(ESM::MagicEffect::Feather).getMagnitude();
            weight += stats.getMagicEffects().get(ESM::MagicEffect::Burden).getMagnitude();
            if(weight < 0.0f)
                weight = 0.0f;
        }

        return weight;
    }

    int ForeignNpc::getBaseFightRating (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();
        //return ref->mBase->mAiData.mFight;
        return 5;
    }

    bool ForeignNpc::isBipedal(const MWWorld::Ptr &ptr) const
    {
        return true;
    }

    std::string ForeignNpc::getPrimaryFaction (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();
        //return ref->mBase->mFaction;
        return "foreign faction";
    }

    int ForeignNpc::getPrimaryFactionRank (const MWWorld::Ptr& ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Npc> *ref = ptr.get<ESM4::Npc>();
        //return ref->mBase->getFactionRank();
        return 10;
    }
}
