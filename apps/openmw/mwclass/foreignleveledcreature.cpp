#include "foreignleveledcreature.hpp"

#include <iostream> // FIXME

#include <extern/esm4/lvlc.hpp>
#include <extern/esm4/npc_.hpp>
#include <extern/esm4/crea.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/physicssystem.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwrender/objects.hpp"
#include "../mwrender/renderinginterface.hpp"

namespace MWClass
{
    std::string ForeignLeveledCreature::getId (const MWWorld::Ptr& ptr) const
    {
        return ptr.get<ESM4::LeveledCreature>()->mBase->mEditorId;
    }

    void ForeignLeveledCreature::insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const
    {
        MWWorld::LiveCellRef<ESM4::LeveledCreature> *ref = ptr.get<ESM4::LeveledCreature>();

        const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();
        ESM4::FormId creature;
        for (unsigned int i = 0; i <  ref->mBase->mLvlObject.size(); ++i)
        {
            if (ref->mBase->mLvlObject[i].item != 0)
                creature = ref->mBase->mLvlObject[i].item; // get the higest one
        }

        const MWWorld::ForeignStore<ESM4::Npc>& npcStore = store.getForeign<ESM4::Npc>();
        const MWWorld::ForeignStore<ESM4::Creature>& creatureStore = store.getForeign<ESM4::Creature>();
        const MWWorld::ForeignStore<ESM4::LeveledCreature>& lvlStore = store.getForeign<ESM4::LeveledCreature>();

        std::string lvlModel;
        if (store.find(creature) == MKTAG('C', 'L', 'V', 'L'))
        {
            const ESM4::LeveledCreature *lvl = lvlStore.search(creature);
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
    }

    void ForeignLeveledCreature::insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const
    {
        if(!model.empty())
            physics.addObject(ptr, model);
    }

    std::string ForeignLeveledCreature::getModel(const MWWorld::Ptr &ptr) const
    {
#if 0
        MWWorld::LiveCellRef<ESM4::LeveledCreature> *ref = ptr.get<ESM4::LeveledCreature>();
        assert(ref->mBase != NULL);

        const std::string &model = ref->mBase->mModel;
        if (!model.empty()) {
            return "meshes\\" + model;
        }
#endif
        return "";
    }

    std::string ForeignLeveledCreature::getName (const MWWorld::Ptr& ptr) const
    {
        return "";
    }

    void ForeignLeveledCreature::registerSelf()
    {
        boost::shared_ptr<Class> instance (new ForeignLeveledCreature);

        registerClass (typeid (ESM4::LeveledCreature).name(), instance);
    }

    MWWorld::Ptr ForeignLeveledCreature::copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const
    {
        MWWorld::LiveCellRef<ESM4::LeveledCreature> *ref = ptr.get<ESM4::LeveledCreature>();

        return MWWorld::Ptr(&cell.get<ESM4::LeveledCreature>().insert(*ref), &cell);
    }
}
