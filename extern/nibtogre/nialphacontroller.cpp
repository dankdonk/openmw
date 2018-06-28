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
#include "nialphacontroller.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "nimodel.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

// architecture/quests/se07ardensulalter.nif
// architecture/quests/se09breathactbottle.nif
// creatures/endgame/battle.nif
// creatures/ghost/death.kf
// creatures/ghost/heademissive.nif
// creatures/ghost/innerbodyemissive.nif
// creatures/ghost/lefthandemissive.nif
// creatures/ghost/outerbodyemissive.nif
// creatures/ghost/righthandemissive.nif
// creatures/ghost/shrink.nif
// creatures/ghost/skeleton.nif
// dungeons/misc/fx/fxlightbeam01.nif
// dungeons/misc/fx/fxlightbeam02.nif
// dungeons/misc/fx/fxlightbeamlong01.nif
// dungeons/misc/fx/fxlightbeampink01.nif
// effects/se09flamesblocking.nif
// effects/se09poolfxsmisc.nif
// effects/se09poollid.nif
// effects/se11sheopooffx.nif
// effects/se11staffofsheofx.nif
// effects/se13fxjygspeakdead.nif
// effects/se13jygafx01.nif
// effects/seflamesofagnoncental.nif
// effects/seflamesofagnondcrown.nif
// effects/seflamesofagnondem.nif
// effects/seflamesofagnondemloop.nif
// effects/seflamesofagnondloopsm.nif
// effects/seflamesofagnonmaincrown.nif
// effects/seflamesofagnonmani.nif
// effects/seflamesofagnonmaniloop.nif
// effects/seflamesofagnonmcrown.nif
// effects/seflamesofagnonmloopsm.nif
// magiceffects/fireball.nif
// magiceffects/shockshield.nif
// obelisk/obeliskenergybox01.nif
// obelisk/obeliskwavebox01.nif
// obelisk/se03resonator01.nif
// obelisk/se08obeliske01.nif
// obelisk/se08towerenergy01.nif
// oblivion/environment/fxoblivionlightbeam01.nif
// oblivion/environment/fxoblivionlightbeamlong01.nif
// oblivion/environment/fxoblivionlightbeamlong01_far.nif
// oblivion/gate/oblivionarchgate01.nif
// oblivion/gate/obliviongate_forming.nif
// oblivion/gate/oblivionwargateani02.nif
// oblivion/plants/spiddalcloudplant.nif
NiBtOgre::NiAlphaController::NiAlphaController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiSingleInterpController(index, stream, model)
{
    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDataIndex);
}
