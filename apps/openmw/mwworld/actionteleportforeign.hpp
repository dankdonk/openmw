#ifndef GAME_MWWORLD_ACTIONTELEPORTFOREIGN_H
#define GAME_MWWORLD_ACTIONTELEPORTFOREIGN_H

#include <string>

//#include <extern/esm4/common.hpp>

#include <components/esm/defs.hpp>

#include "action.hpp"

namespace ESM4
{
    typedef uint32_t FormId;
}

namespace MWWorld
{
    class ActionTeleportForeign : public Action
    {
            std::string mCellName;
            ESM4::FormId mCellId;
            ESM::Position mPosition;
            bool mTeleportFollowers;

            /// Teleports this actor and also teleports anyone following that actor.
            virtual void executeImp (const Ptr& actor);

            /// Teleports only the given actor (internal use).
            void teleport(const Ptr &actor);

        public:

            ActionTeleportForeign (const std::string& cellName,
                    ESM4::FormId cellId, const ESM::Position& position, bool teleportFollowers);
            ///< If cellName is empty, an exterior cell is assumed.
            /// @param teleportFollowers Whether to teleport any following actors of the target actor as well.
    };
}

#endif
