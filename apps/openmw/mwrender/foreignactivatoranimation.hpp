#ifndef GAME_RENDER_FOREIGNACTIVATORANIMATION_H
#define GAME_RENDER_FOREIGNACTIVATORANIMATION_H

#include "animation.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWRender
{
    class ForeignActivatorAnimation : public ObjectAnimation
    {
    public:
        ForeignActivatorAnimation(const MWWorld::Ptr& ptr, const std::string &model);
        virtual ~ForeignActivatorAnimation();

        //void addLight(const ESM::Light *light);
        //void removeParticles();
    };
}

#endif
