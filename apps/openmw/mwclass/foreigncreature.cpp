#include "foreigncreature.hpp"

#include <extern/esm4/crea.hpp>
#include <extern/esm4/formid.hpp> // FIXME mainly for debugging

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/customdata.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwrender/actors.hpp"
#include "../mwrender/renderinginterface.hpp"

#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/movement.hpp"

namespace
{
    struct ForeignCreatureCustomData : public MWWorld::CustomData
    {
        MWMechanics::CreatureStats mCreatureStats;
        MWMechanics::Movement mMovement;
        MWWorld::InventoryStore mInventoryStore;

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *ForeignCreatureCustomData::clone() const
    {
        return new ForeignCreatureCustomData (*this);
    }
}

namespace MWClass
{
    std::string ForeignCreature::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::Creature>()->mBase->mEditorId;
    }

    void ForeignCreature::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();

        MWRender::Actors& actors = renderingInterface.getActors();
        actors.insertCreature(ptr, model, false/*(ref->mBase->mFlags & ESM::Creature::Weapon) != 0*/);
#if 0
        MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();

        if (!model.empty()) {
            renderingInterface.getObjects().insertModel(ptr, model/*, !ref->mBase->mPersistent*/); // FIXME
        }
#endif
    }

    void ForeignCreature::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
        {
            physics.addActor(ptr, model);
            //if (getCreatureStats(ptr).isDead())
                //MWBase::Environment::get().getWorld()->enableActorCollision(ptr, false);
        }
        MWBase::Environment::get().getMechanicsManager()->add(ptr);
#if 0
        if(!model.empty())
            physics.addObject(ptr, model);
#endif
    }

    std::string ForeignCreature::getModel(const MWWorld::Ptr &ptr) const
    {
        MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();
        assert (ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
#if 0
        MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
        return "";
#endif
    }

    std::string ForeignCreature::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignCreature::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignCreature);

        registerClass (typeid (ESM4::Creature).name(), instance);
    }

    MWWorld::Ptr ForeignCreature::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();

        return MWWorld::Ptr(&cell.get<ESM4::Creature>().insert(*ref), &cell);
    }

    void ForeignCreature::ensureCustomData (const MWWorld::Ptr& ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<ForeignCreatureCustomData> data(new ForeignCreatureCustomData);

            MWWorld::LiveCellRef<ESM4::Creature> *ref = ptr.get<ESM4::Creature>();

            // creature stats
            int gold = 0;
            if ((ref->mBase->mBaseConfig.flags & /*ACBS_Autocalcstats*/0x000010) != 0)
            {
                gold = ref->mBase->mBaseConfig.barterGold;

                data->mCreatureStats.setAttribute(ESM::Attribute::Strength, ref->mBase->mData.attribs.strength);
                data->mCreatureStats.setAttribute(ESM::Attribute::Intelligence, ref->mBase->mData.attribs.intelligence);
                data->mCreatureStats.setAttribute(ESM::Attribute::Willpower, ref->mBase->mData.attribs.willpower);
                data->mCreatureStats.setAttribute(ESM::Attribute::Agility, ref->mBase->mData.attribs.agility);
                data->mCreatureStats.setAttribute(ESM::Attribute::Speed, ref->mBase->mData.attribs.speed);
                data->mCreatureStats.setAttribute(ESM::Attribute::Endurance, ref->mBase->mData.attribs.endurance);
                data->mCreatureStats.setAttribute(ESM::Attribute::Personality, ref->mBase->mData.attribs.personality);
                data->mCreatureStats.setAttribute(ESM::Attribute::Luck, ref->mBase->mData.attribs.luck);

                data->mCreatureStats.setHealth (/*ref->mBase->mData.health*/ 50); // FIXME: uint32 to float
                data->mCreatureStats.setMagicka (ref->mBase->mBaseConfig.baseSpell);
                data->mCreatureStats.setFatigue (ref->mBase->mBaseConfig.fatigue);

                data->mCreatureStats.setLevel(ref->mBase->mBaseConfig.level);

                data->mCreatureStats.setNeedRecalcDynamicStats(false);
            }
            else // FIXME autocalc
            {
                gold = ref->mBase->mBaseConfig.barterGold;

                data->mCreatureStats.setHealth (/*ref->mBase->mData.health*/ 50); // FIXME: uint32 to float
                data->mCreatureStats.setMagicka (ref->mBase->mBaseConfig.baseSpell);
                data->mCreatureStats.setFatigue (ref->mBase->mBaseConfig.fatigue);

                for (int i=0; i<3; ++i)
                    data->mCreatureStats.setDynamic (i, 10);

                data->mCreatureStats.setLevel(ref->mBase->mBaseConfig.level);

                //autoCalculateAttributes(ref->mBase, data->mCreatureStats);
                //autoCalculateSkills(ref->mBase, data->mCreatureStats, ptr);

                data->mCreatureStats.setNeedRecalcDynamicStats(true);
            }
#if 0
            // race powers
            const ESM::Race *race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(ref->mBase->mRace);
            for (std::vector<std::string>::const_iterator iter (race->mPowers.mList.begin());
                iter!=race->mPowers.mList.end(); ++iter)
            {
                if (const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(*iter))
                    data->mCreatureStats.getSpells().add (spell);
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

                data->mCreatureStats.setReputation(iAutoRepFacMod * (rank+1) + iAutoRepLevMod * (data->mCreatureStats.getLevel()-1));
            }
#endif
#if 0
            data->mCreatureStats.getAiSequence().fill(ref->mBase->mAiPackage);

            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Hello, ref->mBase->mAiData.mHello);
            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Fight, ref->mBase->mAiData.mFight);
            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Flee, ref->mBase->mAiData.mFlee);
            data->mCreatureStats.setAiSetting (MWMechanics::CreatureStats::AI_Alarm, ref->mBase->mAiData.mAlarm);

            // spells
            for (std::vector<std::string>::const_iterator iter (ref->mBase->mSpells.mList.begin());
                iter!=ref->mBase->mSpells.mList.end(); ++iter)
            {
                if (const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().search(*iter))
                    data->mCreatureStats.getSpells().add (spell);
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

            data->mCreatureStats.setGoldPool(gold);

            // store
            ptr.getRefData().setCustomData (data.release());

            getInventoryStore(ptr).autoEquipTES4(ptr);
        }
    }

    MWWorld::InventoryStore& ForeignCreature::getInventoryStore (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignCreatureCustomData&> (*ptr.getRefData().getCustomData()).mInventoryStore;
    }

    MWMechanics::CreatureStats& ForeignCreature::getCreatureStats (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignCreatureCustomData&> (*ptr.getRefData().getCustomData()).mCreatureStats;
    }

    MWMechanics::Movement& ForeignCreature::getMovementSettings (const MWWorld::Ptr& ptr) const
    {
        ensureCustomData (ptr);

        return dynamic_cast<ForeignCreatureCustomData&> (*ptr.getRefData().getCustomData()).mMovement;
    }
}
