/*
  Copyright (C) 2015-2017 cc9cii

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

*/
#include "niparticlemodifier.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "nistream.hpp"
#include "nitimecontroller.hpp" // static_cast NiParticleSystemController
#include "nimodel.hpp"

NiBtOgre::NiParticleModifier::NiParticleModifier(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    stream.read(mNextModifier);

    //stream.getPtr<NiNode>(mController, model.objects());
    std::int32_t index = -1;
    stream.read(index);
    mController = model.getRef<NiParticleSystemController>(index);
}

NiBtOgre::NiGravity::NiGravity(NiStream& stream, const NiModel& model)
    : NiParticleModifier(stream, model)
{
    stream.skip(sizeof(float));
    stream.read(mForce);
    stream.read(mType);
    stream.read(mPosition);
    stream.read(mDirection);
}

NiBtOgre::NiParticleColorModifier::NiParticleColorModifier(NiStream& stream, const NiModel& model)
    : NiParticleModifier(stream, model)
{
    stream.read(mColorData);
}

NiBtOgre::NiParticleGrowFade::NiParticleGrowFade(NiStream& stream, const NiModel& model)
    : NiParticleModifier(stream, model)
{
    stream.read(mGrowTime);
    stream.read(mFadeTime);
}

NiBtOgre::NiParticleRotation::NiParticleRotation(NiStream& stream, const NiModel& model)
    : NiParticleModifier(stream, model)
{
    stream.skip(sizeof(char));
    stream.skip(sizeof(float)*3);
    stream.skip(sizeof(float));
}

NiBtOgre::NiPlanarCollider::NiPlanarCollider(NiStream& stream, const NiModel& model)
    : NiParticleModifier(stream, model)
{
    stream.skip(sizeof(std::uint16_t));
    stream.skip(sizeof(float)*2);
    stream.skip(sizeof(std::uint16_t));
    stream.skip(sizeof(float)*14);
}
