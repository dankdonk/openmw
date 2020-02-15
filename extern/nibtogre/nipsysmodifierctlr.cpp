/*
  Copyright (C) 2015-2019 cc9cii

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
#include "nipsysmodifierctlr.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "nimodel.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

NiBtOgre::NiPSysModifierCtlr::NiPSysModifierCtlr(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream.readLongString(mModifierNameRef);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysEmitterCtlr::NiPSysEmitterCtlr(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data)
    : NiPSysModifierCtlr(index, stream, model, data)
{
#if 0 // commented out since this object is not seen in TES3
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataRef);
    if (stream.nifVer() >= 0x0a020000) // from 10.2.0.0
#endif
        stream.read(mVisibilityInterpolatorRef);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
//
// creatures/ghost/skeleton.nif
// dungeons/Root/Interior/Misc/explodingrootpod.nif
// dungeons/Root/Interior/Misc/explodingrootpod.nif
// dungeons/Root/Interior/Misc/treasurestump.nif
// dungeons/Root/Interior/Misc/treasurestump.nif
// dungeons/RuinInteriors/traps/trapgascloud01.nif
// dungeons/RuinInteriors/traps/trapgascloud01.nif
// and other examples
NiBtOgre::NiPSysModifierActiveCtlr::NiPSysModifierActiveCtlr(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data)
    : NiPSysModifierCtlr(index, stream, model, data)
{
#if 0 // commented out since this object is not seen in TES3
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataRef);
#endif
}

NiBtOgre::NiPSysModifierFloatCtlr::NiPSysModifierFloatCtlr(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data)
    : NiPSysModifierCtlr(index, stream, model, data)
{
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataRef);
}
