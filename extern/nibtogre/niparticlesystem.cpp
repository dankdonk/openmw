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
#include "niparticlesystem.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "nimodel.hpp"
#include "btogreinst.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiParticleSystem::NiParticleSystem(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiParticles(index, stream, model, data), mWorldSpace(false)
{
    if (stream->userVer() >= 12)
    {
        stream->read(mUnknownS2);
        stream->read(mUnknownS3);
        stream->read(mUnknownI1);
    }

    if (stream->nifVer() >= 0x0a010000) // from 10.1.0.0
    {
        mWorldSpace = stream->getBool();
        std::uint32_t numModifiers;
        stream->read(numModifiers);
        mModifiers.resize(numModifiers);
        for (unsigned int i = 0; i < numModifiers; ++i)
            stream->read(mModifiers.at(i));
    }
}

void NiBtOgre::NiParticleSystem::build(NiObject *parent)
{
}
