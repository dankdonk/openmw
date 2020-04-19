#include "levelledcreature.hpp"

#include <extern/esm4/lvlc.hpp>

#include "../mwmechanics/levelledlist.hpp"

#include "../mwworld/customdata.hpp"

namespace
{
    struct LevelledCreatureCustomData : public MWWorld::CustomData
    {
        // actorId of the creature we spawned
        int mSpawnActorId;
        bool mSpawn; // Should a new creature be spawned?

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *LevelledCreatureCustomData::clone() const
    {
        return new LevelledCreatureCustomData (*this);
    }
}

namespace MWClass
{
    std::string LevelledCreature::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::LevelledCreature>()->mBase->mEditorId;
    }

    std::string LevelledCreature::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void LevelledCreature::respawn(const MWWorld::Ptr &ptr) const
    {
        ensureCustomData(ptr);

        LevelledCreatureCustomData& customData = dynamic_cast<LevelledCreatureCustomData&> (*ptr.getRefData().getCustomData());
        customData.mSpawn = true;
    }

    void LevelledCreature::registerSelf()
    {
        boost::shared_ptr<Class> instance (new LevelledCreature);

        registerClass (typeid (ESM4::LevelledCreature).name(), instance);
    }

    void LevelledCreature::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
#if 0
        MWWorld::LiveCellRef<ESM4::LevelledCreature> *ref = ptr.get<ESM4::LevelledCreature>();

        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        ESM4::FormId creature;
        for (unsigned int i = 0; i <  ref->mBase->mLvlObject.size(); ++i)
        {
            if (ref->mBase->mLvlObject[i].item != 0)
                creature = ref->mBase->mLvlObject[i].item; // get the higest one
        }

        const MWWorld::ForeignStore<ESM4::Npc>& npcStore = store.getForeign<ESM4::Npc>();
        const MWWorld::ForeignStore<ESM4::Creature>& creatureStore = store.getForeign<ESM4::Creature>();
        const MWWorld::ForeignStore<ESM4::LevelledCreature>& lvlStore = store.getForeign<ESM4::LevelledCreature>();

        std::string lvlModel;
        if (store.find(creature) == MKTAG('C', 'L', 'V', 'L'))
        {
            const ESM4::LevelledCreature *lvl = lvlStore.search(creature);
            for (unsigned int i = 0; i < lvl->mLvlObject.size(); ++i)
            {
                switch (store.find(lvl->mLvlObject[i].item))
                {
                    case MKTAG('_','N','P','C'):
                    {
                        const ESM4::Npc *npc_ = npcStore.search(lvl->mLvlObject[i].item);
                        lvlModel = npc_->mModel;
                        //std::cout << "lvl creature " << i << " " << npc_->mEditorId << std::endl;
                        break;
                    }
                    case MKTAG('A','C','R','E'):
                    {
                        const ESM4::Creature *crea = creatureStore.search(lvl->mLvlObject[i].item);
                        lvlModel = crea->mModel;
                        //std::cout << "lvl creature " << i << " " << crea->mEditorId << std::endl;
                        break;
                    }
                    default:
                        std::cout << "lvl creature " << i <<  " ?" << std::endl;
                        break;
                }
            }
        }
        else
            std::cout << "lvl creature lvl object is not a lvlc" << std::endl;

        if (!lvlModel.empty()) {
            renderingInterface.getObjects().insertModel(ptr, lvlModel/*, !ref->mBase->mPersistent*/); // FIXME
        }
#else
        ensureCustomData(ptr);

        LevelledCreatureCustomData& customData = dynamic_cast<LevelledCreatureCustomData&> (*ptr.getRefData().getCustomData());
        if (!customData.mSpawn)
            return;

        MWWorld::LiveCellRef<ESM4::LevelledCreature> *ref = ptr.get<ESM4::LevelledCreature>();

        // should return the formid of a Creature which ManualRef then uses to find the Creature
        std::string id = MWMechanics::getTES4LevelledCreature(ref->mBase);

        if (!id.empty())
        {
            // Delete the previous actor
            if (customData.mSpawnActorId != -1)
            {
                MWWorld::Ptr npc = MWBase::Environment::get().getWorld()->searchPtrViaActorId(customData.mSpawnActorId);
                if (!npc.isEmpty())
                    MWBase::Environment::get().getWorld()->deleteObject(npc);
                customData.mSpawnActorId = -1;
            }

            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
            MWWorld::ManualRef ref(store, id);
            ref.getPtr().getCellRef().setPosition(ptr.getCellRef().getPosition());

            MWWorld::Ptr placed = MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(), ptr.getCell() , ptr.getCellRef().getPosition());

            customData.mSpawnActorId = placed.getClass().getCreatureStats(placed).getActorId();
            customData.mSpawn = false;
        }
        else
            customData.mSpawn = false;
#endif
    }

    void LevelledCreature::ensureCustomData(const MWWorld::Ptr &ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<LevelledCreatureCustomData> data (new LevelledCreatureCustomData);
            data->mSpawnActorId = -1;
            data->mSpawn = true;

            ptr.getRefData().setCustomData(data.release());
        }
    }

    void LevelledCreature::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
        const
    {
#if 0
        const ESM::CreatureLevListState& state2 = dynamic_cast<const ESM::CreatureLevListState&> (state);

        ensureCustomData(ptr);
        LeveledCreatureCustomData& customData = dynamic_cast<LeveledCreatureCustomData&> (*ptr.getRefData().getCustomData());
        customData.mSpawnActorId = state2.mSpawnActorId;
        customData.mSpawn = state2.mSpawn;
#endif
    }

    void LevelledCreature::writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state)
        const
    {
#if 0
        ESM::CreatureLevListState& state2 = dynamic_cast<ESM::CreatureLevListState&> (state);

        ensureCustomData(ptr);
        LeveledCreatureCustomData& customData = dynamic_cast<LeveledCreatureCustomData&> (*ptr.getRefData().getCustomData());
        state2.mSpawnActorId = customData.mSpawnActorId;
        state2.mSpawn = customData.mSpawn;
#endif
    }
}
