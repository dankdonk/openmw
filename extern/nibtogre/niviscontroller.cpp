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
#include "niviscontroller.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "nimodel.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

// architecture/anvil/lorgrenskeleton01.nif
// architecture/quests/se01waitingroomwalls.nif
// architecture/quests/se07ardensulalter.nif
// architecture/quests/se09breathactbottle.nif
//
// plus lots of other examples
NiBtOgre::NiVisController::NiVisController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataIndex);
}
