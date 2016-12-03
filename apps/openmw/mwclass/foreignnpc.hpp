#ifndef GAME_MWCLASS_FOREIGNNPC_H
#define GAME_MWCLASS_FOREIGNNPC_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class ForeignNpc : public MWWorld::Class
    {
            void ensureCustomData (const MWWorld::Ptr& ptr) const;
            virtual MWWorld::Ptr copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const;

        public:

            /// Return ID of \a ptr
            virtual std::string getId (const MWWorld::Ptr& ptr) const;

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const;

            virtual MWWorld::InventoryStore& getInventoryStore (const MWWorld::Ptr& ptr) const;
            ///< Return inventory store

            virtual bool hasInventoryStore(const MWWorld::Ptr &ptr) const { return true; }

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            virtual MWMechanics::CreatureStats& getCreatureStats (const MWWorld::Ptr& ptr) const;
            ///< Return creature stats

            virtual MWMechanics::NpcStats& getNpcStats (const MWWorld::Ptr& ptr) const;
            ///< Return NPC stats

            static void registerSelf();

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;
    };
}

#endif
