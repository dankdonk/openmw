/*
  Copyright (C) 2017-2019 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

  Much of the information on the NIF file structures are based on the NifSkope
  documenation.  See http://niftools.sourceforge.net/wiki/NifSkope for details.

*/
#ifndef NIBTOGRE_NIPARTICLESYSTEMCONTROLLER_H
#define NIBTOGRE_NIPARTICLESYSTEMCONTROLLER_H

#include <string>
#include <cstdint>

#include <OgreVector3.h>

#include "niobject.hpp"
#include "nitimecontroller.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiTimeController
//     NiParticleSystemController
//         NiBSPArrayController <------------------------- /* NiParticleSystemController */
namespace NiBtOgre
{
    class NiParticleSystemController : public NiTimeController
    {
    public:
        struct Particle
        {
            Ogre::Vector3 velocity;
            float lifetime;
            float lifespan;
            float timestamp;
            std::uint16_t vertexID;
        };

        float mSpeed;
        float mSpeedRandom;

        float mVerticalDirection;
        float mVerticalAngle;
        float mHorizontalDirection;
        float mHorizontalAngle;

        float mSize;
        float mEmitStartTime;
        float mEmitStopTime;

        float mEmitRate;
        float mLifetime;
        float mLifetimeRandom;

        std::uint16_t mEmitFlags;

        Ogre::Vector3 mStartRandom;

        NiObject *mEmitter; // Ptr

        std::uint16_t mNumParticles;
        std::uint16_t mNumValid;
        std::vector<Particle> mParticles;

        NiParticleModifierRef mParticleExtraRef;

        NiParticleSystemController(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data);
    };

    typedef NiParticleSystemController NiBSPArrayController;
}

#endif // NIBTOGRE_NIPARTICLESYSTEMCONTROLLER_H
