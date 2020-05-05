#ifndef GAME_MWCLASS_TALKINGACTIVATOR_H
#define GAME_MWCLASS_TALKINGACTIVATOR_H

#include "../mwworld/class.hpp"

namespace MWClass
{
    class TalkingActivator : public MWWorld::Class
    {
            virtual MWWorld::Ptr copyToCellImpl(const MWWorld::Ptr &ptr, MWWorld::CellStore &cell) const;

        public:

            virtual std::string getId (const MWWorld::Ptr& ptr) const;

            virtual void insertObjectRendering (const MWWorld::Ptr& ptr, const std::string& model, MWRender::RenderingInterface& renderingInterface) const;

            virtual void insertObject(const MWWorld::Ptr& ptr, const std::string& model, MWWorld::PhysicsSystem& physics) const;

            virtual std::string getName (const MWWorld::Ptr& ptr) const;

            virtual bool hasToolTip (const MWWorld::Ptr& ptr) const;

            virtual MWGui::ToolTipInfo getToolTipInfo (const MWWorld::Ptr& ptr) const;

            static void registerSelf();

            virtual std::string getModel(const MWWorld::Ptr &ptr) const;
    };
}

#endif
