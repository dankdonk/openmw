#ifndef GAME_MWCLASS_FOREIGNARMOR_H
#define GAME_MWCLASS_FOREIGNARMOR_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class ForeignArmor : public MWWorld::Class
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

            virtual bool hasToolTip (const MWWorld::Ptr& ptr) const;
            ///< @return true if this object has a tooltip when focused (default implementation: false)

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::Ptr& ptr) const;
            ///< @return the content of the tool tip to be displayed. raises exception if the object has no tooltip.

            virtual std::pair<std::vector<int>, bool> getEquipmentSlots (const MWWorld::Ptr& ptr) const;

            virtual int getValue (const MWWorld::Ptr& ptr) const;
            ///< Return trade value of the object. Throws an exception, if the object can't be traded.

            virtual float getArmorRating (const MWWorld::Ptr& ptr) const;

            static void registerSelf();

            virtual std::pair<int, std::string> canBeEquipped(const MWWorld::Ptr &ptr, const MWWorld::Ptr &npc) const;

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;
    };
}

#endif
