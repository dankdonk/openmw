#ifndef GAME_MWCLASS_FOREIGNCREATURE_H
#define GAME_MWCLASS_FOREIGNCREATURE_H

#include "../mwworld/class.hpp"

namespace MWWorld
{
    class InventoryStoreTES4;
}

namespace MWClass
{
    class ForeignCreature : public MWWorld::Class
    {
            virtual MWWorld::Ptr copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const;

        public:

            /// Return ID of \a ptr
            virtual std::string getId (const MWWorld::Ptr& ptr) const;

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;
            ///< Add reference into a cell for rendering

            virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const;

            virtual std::string getName (const MWWorld::Ptr& ptr) const;
            ///< \return name (the one that is to be presented to the user; not the internal one);
            /// can return an empty string.

            static void registerSelf();

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;

            void ensureCustomData (const MWWorld::Ptr& ptr) const;
            MWWorld::InventoryStore& getInventoryStore (const MWWorld::Ptr& ptr) const;
            MWWorld::InventoryStoreTES4& getInventoryStoreTES4 (const MWWorld::Ptr& ptr) const;
            MWMechanics::CreatureStats& getCreatureStats (const MWWorld::Ptr& ptr) const;
            MWMechanics::Movement& getMovementSettings (const MWWorld::Ptr& ptr) const;
    };
}

#endif
