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
#include "nigeommorphercontroller.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "nimodel.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

// architecture/quests/se01waitingroomwalls.nif
// architecture/quests/se07ardensulalter.nif
// architecture/ships/mainmast02.nif
// architecture/statue/nightmotherstatue.nif
// architecture/statue/nightmotherstatuebase.nif
//
// plus lots of others, including most creatures
NiBtOgre::NiGeomMorpherController::NiGeomMorpherController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model)
{
    if (stream.nifVer() >= 0x0a000102) // from 10.0.1.2
        stream.read(mExtraFlags);

    if (stream.nifVer() == 0x0a01006a) // 10.1.0.106
        stream.skip(sizeof(char)); // Unknown 2

    stream.read(mDataIndex);
    stream.read(mAlwaysUpdate);

    if (stream.nifVer() >= 0x0a01006a) // from 10.1.0.106
    {
        std::uint32_t numInterpolators;
        stream.read(numInterpolators);
        if (stream.nifVer() >= 0x0a020000 && stream.nifVer() <= 0x14000005)
        {
            mInterpolators.resize(numInterpolators);
            for (unsigned int i = 0; i < numInterpolators; ++i)
                stream.read(mInterpolators.at(i));
        }

        if (stream.nifVer() >= 0x14010003) // from 20.1.0.3
        {
            mInterpolatorWeights.resize(numInterpolators);
            for (unsigned int i = 0; i < numInterpolators; ++i)
            {
                stream.read(mInterpolatorWeights[i].interpolatorIndex);
                stream.read(mInterpolatorWeights[i].weight);
            }
        }

        if (stream.nifVer() >= 0x14000004 && stream.nifVer() <= 0x14000005 && stream.userVer() >= 10)
        {
            std::uint32_t numUnknownInts;
            stream.read(numUnknownInts);
            mUnknownInts.resize(numUnknownInts);
            for (unsigned int i = 0; i < numUnknownInts; ++i)
                stream.read(mUnknownInts.at(i));
        }
    }
}
