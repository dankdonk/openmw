#include "levellednpc.hpp"

#include <extern/esm4/lvln.hpp>

#include "../mwmechanics/levelledlist.hpp"

#include "../mwworld/customdata.hpp"

namespace
{
    struct LeveledNpcCustomData : public MWWorld::CustomData
    {
        // actorId of the creature we spawned
        int mSpawnActorId;
        bool mSpawn; // Should a new creature be spawned?

        virtual MWWorld::CustomData *clone() const;
    };

    MWWorld::CustomData *LeveledNpcCustomData::clone() const
    {
        return new LeveledNpcCustomData (*this);
    }
}

namespace MWClass
{
    std::string LevelledNpc::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::LevelledNpc>()->mBase->mEditorId;
    }

    std::string LevelledNpc::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void LevelledNpc::respawn(const MWWorld::Ptr &ptr) const
    {
        ensureCustomData(ptr);

        LeveledNpcCustomData& customData = dynamic_cast<LeveledNpcCustomData&> (*ptr.getRefData().getCustomData());
        customData.mSpawn = true;
    }

    void LevelledNpc::registerSelf()
    {
        boost::shared_ptr<Class> instance (new LevelledNpc);

        registerClass (typeid (ESM4::LevelledNpc).name(), instance);
    }

    void LevelledNpc::insertObjectRendering(const MWWorld::Ptr &ptr, const std::string& model, MWRender::RenderingInterface &renderingInterface) const
    {
        ensureCustomData(ptr);

        LeveledNpcCustomData& customData = dynamic_cast<LeveledNpcCustomData&> (*ptr.getRefData().getCustomData());
        if (!customData.mSpawn)
            return;

        MWWorld::LiveCellRef<ESM4::LevelledNpc> *ref = ptr.get<ESM4::LevelledNpc>();

        // FIXME: save inventory details (and baseconfig?) to update later

        // should return the formid of an Npc which ManualRef then uses to find the Npc
        std::string id = MWMechanics::getTES4LevelledNpc(ref->mBase);

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

            // FIXME: update inventory details, etc. (probably have to use customData)
            //ForeignNpcCustomData& data = dynamic_cast<ForeignNpcCustomData>ref.getPtr().getRefData().getCustomData();

            ref.getPtr().getCellRef().setPosition(ptr.getCellRef().getPosition());

// FIXME: use insertActor here? if so spawn details in customData will not be used?
#if 0
            renderingInterface.getActors().insertNPC(ref.getPtr());
#else
            MWWorld::Ptr placed = MWBase::Environment::get().getWorld()->safePlaceObject(ref.getPtr(), ptr.getCell() , ptr.getCellRef().getPosition());
#endif
            customData.mSpawnActorId = placed.getClass().getCreatureStats(placed).getActorId();
            customData.mSpawn = false;
        }
        else
            customData.mSpawn = false;
    }

    void LevelledNpc::ensureCustomData(const MWWorld::Ptr &ptr) const
    {
        if (!ptr.getRefData().getCustomData())
        {
            std::auto_ptr<LeveledNpcCustomData> data (new LeveledNpcCustomData);
            data->mSpawnActorId = -1;
            data->mSpawn = true;

            ptr.getRefData().setCustomData(data.release());
        }
    }

    void LevelledNpc::readAdditionalState (const MWWorld::Ptr& ptr, const ESM::ObjectState& state)
        const
    {
#if 0
        const ESM::CreatureLevListState& state2 = dynamic_cast<const ESM::CreatureLevListState&> (state);

        ensureCustomData(ptr);
        LeveledNpcCustomData& customData = dynamic_cast<LeveledNpcCustomData&> (*ptr.getRefData().getCustomData());
        customData.mSpawnActorId = state2.mSpawnActorId;
        customData.mSpawn = state2.mSpawn;
#endif
    }

    void LevelledNpc::writeAdditionalState (const MWWorld::Ptr& ptr, ESM::ObjectState& state)
        const
    {
#if 0
        ESM::CreatureLevListState& state2 = dynamic_cast<ESM::CreatureLevListState&> (state);

        ensureCustomData(ptr);
        LeveledNpcCustomData& customData = dynamic_cast<LeveledNpcCustomData&> (*ptr.getRefData().getCustomData());
        state2.mSpawnActorId = customData.mSpawnActorId;
        state2.mSpawn = customData.mSpawn;
#endif
    }
}
