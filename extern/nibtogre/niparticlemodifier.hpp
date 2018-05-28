/*
  Copyright (C) 2015-2018 cc9cii

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
#ifndef NIBTOGRE_NIPARTICLEMODIFIER_H
#define NIBTOGRE_NIPARTICLEMODIFIER_H

#include <cstdint>

#include <OgreVector3.h>

#include "niobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiParticleModifier
//     NiGravity
//     NiParticleColorModifier
//     NiParticleGrowFade
//     NiParticleRotation
//     NiPlanarCollider
namespace NiBtOgre
{
    class NiStream;
    class Header;

    class NiParticleSystemController;

    class NiParticleModifier : public NiObject
    {
    public:
        NiParticleModifierRef mNextModifier;
        NiParticleSystemController *mController; // Ptr

        NiParticleModifier(uint32_t index, NiStream& stream, const NiModel& model);
    };

    class NiGravity : public NiParticleModifier
    {
    public:
        float mForce;
        std::uint32_t mType;
        Ogre::Vector3 mPosition;
        Ogre::Vector3 mDirection;

        NiGravity(uint32_t index, NiStream& stream, const NiModel& model);
    };

    class NiParticleColorModifier : public NiParticleModifier
    {
    public:
        NiColorDataRef mColorData;

        NiParticleColorModifier(uint32_t index, NiStream& stream, const NiModel& model);
    };

    class NiParticleGrowFade : public NiParticleModifier
    {
    public:
        float mGrowTime;
        float mFadeTime;

        NiParticleGrowFade(uint32_t index, NiStream& stream, const NiModel& model);
    };

    class NiParticleRotation : public NiParticleModifier
    {
        // FIXME: all skipped for now
    public:
        NiParticleRotation(uint32_t index, NiStream& stream, const NiModel& model);
    };

    class NiPlanarCollider : public NiParticleModifier
    {
        // FIXME: all skipped for now
    public:
        NiPlanarCollider(uint32_t index, NiStream& stream, const NiModel& model);
    };
}

#endif // NIBTOGRE_NIPARTICLEMODIFIER_H
