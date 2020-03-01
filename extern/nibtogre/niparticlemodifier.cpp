/*
  Copyright (C) 2015-2020 cc9cii

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
#include "niparticlemodifier.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "niparticlesystemcontroller.hpp" // static_cast NiParticleSystemController
#include "nimodel.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

NiBtOgre::NiParticleModifier::NiParticleModifier(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
    stream->read(mNextModifier);

    //stream->getPtr<NiParticleSystemController>(mController, model.objects());
    std::int32_t rIndex = -1;
    stream->read(rIndex);
    mController = model.getRef<NiParticleSystemController>(rIndex);
}

NiBtOgre::NiGravity::NiGravity(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiParticleModifier(index, stream, model, data)
{
    stream->skip(sizeof(float));
    stream->read(mForce);
    stream->read(mType);
    stream->read(mPosition);
    stream->read(mDirection);
}

NiBtOgre::NiParticleColorModifier::NiParticleColorModifier(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiParticleModifier(index, stream, model, data)
{
    stream->read(mColorData);
}

NiBtOgre::NiParticleGrowFade::NiParticleGrowFade(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiParticleModifier(index, stream, model, data)
{
    stream->read(mGrowTime);
    stream->read(mFadeTime);
}

NiBtOgre::NiParticleRotation::NiParticleRotation(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiParticleModifier(index, stream, model, data)
{
    stream->skip(sizeof(char));
    stream->skip(sizeof(float)*3);
    stream->skip(sizeof(float));
}

NiBtOgre::NiPlanarCollider::NiPlanarCollider(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiParticleModifier(index, stream, model, data)
{
    stream->skip(sizeof(std::uint16_t));
    stream->skip(sizeof(float)*2);
    stream->skip(sizeof(std::uint16_t));
    stream->skip(sizeof(float)*14);
}
