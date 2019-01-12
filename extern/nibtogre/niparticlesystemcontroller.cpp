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
#include "niparticlesystemcontroller.hpp"

#include <cassert>
#include <stdexcept>
#include <iostream> // FIXME: debugging only

#include "nistream.hpp"
#include "nimodel.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

NiBtOgre::NiParticleSystemController::NiParticleSystemController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiTimeController(index, stream, model, data)
{
    stream.read(mSpeed);
    stream.read(mSpeedRandom);
    stream.read(mVerticalDirection);
    stream.read(mVerticalAngle);
    stream.read(mHorizontalDirection);
    stream.read(mHorizontalAngle);
    stream.skip(sizeof(float)*3);// normal?
    stream.skip(sizeof(float)*4);// color?
    stream.read(mSize);
    stream.read(mEmitStartTime);
    stream.read(mEmitStopTime);
    stream.skip(sizeof(char)); // Unknown Byte, from 4.0.0.2
    stream.read(mEmitRate);
    stream.read(mLifetime);
    stream.read(mLifetimeRandom);

    stream.read(mEmitFlags);
    stream.read(mStartRandom);

    //stream.getPtr<NiObject>(mEmitter, model.objects());
    std::int32_t rIndex = -1;
    stream.read(rIndex);
    mEmitter = model.getRef<NiObject>(rIndex);

    stream.skip(16); // FIXME: provide more detail on what's being skipped

    stream.read(mNumParticles);
    stream.read(mNumValid);

    mParticles.resize(mNumParticles);
    for (unsigned int i = 0; i < mNumParticles; ++i)
    {
        stream.read(mParticles[i].velocity);
        stream.skip(sizeof(float)*3); // Unknown Vector
        stream.read(mParticles[i].lifetime);
        stream.read(mParticles[i].lifespan);
        stream.read(mParticles[i].timestamp);
        stream.skip(sizeof(std::int16_t)); // Unknown Short
        stream.read(mParticles[i].vertexID);
    }

    stream.skip(sizeof(std::int32_t)); // Unknown Link
    stream.read(mParticleExtraIndex);
    stream.skip(sizeof(std::int32_t)); // Unknown Link 2
    stream.skip(sizeof(char)); // Trailer
}
