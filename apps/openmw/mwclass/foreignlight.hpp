#ifndef GAME_MWCLASS_FOREIGNLIGHT_H
#define GAME_MWCLASS_FOREIGNLIGHT_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class ForeignLight : public MWWorld::Class
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

            virtual std::pair<std::vector<int>, bool> getEquipmentSlots (const MWWorld::Ptr& ptr) const;

            static void registerSelf();

            virtual std::pair<int, std::string> canBeEquipped(const MWWorld::Ptr &ptr, const MWWorld::Ptr &npc) const;

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;
    };
}

#endif
