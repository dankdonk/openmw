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
#include "niflipcontroller.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "nimodel.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

// architecture/City/Dementia/dementiastreetlightwallwhite.nif
// architecture/sestatues/dementiastatue.nif
// creatures/endgame/battle.nif
// creatures/flameatronach/flameatronach.nif
// creatures/wraith/death.kf
// creatures/wraith/skeleton.nif
// creatures/wraith/skeleton_lord.nif
// creatures/wraith/skeleton_luminous.nif
// creatures/wraith/skeleton_luminouslord.nif
// dungeons/caves/clutter01/bluefire01.nif
// dungeons/caves/clutter01/warlocktorch03.nif
// dungeons/misc/fx/fxwhiteflamemedium.nif
// dungeons/misc/fx/fxwhitetorchlarge.nif
// effects/sefxwhiteflamemedium.nif
// fire/firearcanemedium01.nif
// fire/firecandleflame.nif
// fire/fireopenlarge.nif
// fire/fireopenlargesmoke.nif
// fire/fireopenmedium.nif
// fire/fireopenmediumsmoke.nif
// fire/fireopensmall.nif
// fire/fireopensmallsmoke.nif
// fire/firetorchlarge.nif
// fire/firetorchlargesmoke.nif
// fire/firetorchsmall.nif
// magiceffects/fireball.nif
// magiceffects/fireball.nif
// magiceffects/shockshield.nif
// magiceffects/shockshield.nif
// magiceffects/summonundead.nif
// menus/enemy health bar/health_bar01.nif
// menus/enemy health bar/health_bar01_old.nif
// menus/enemy health bar/health_bar_old.nif
// menus/spell effect timer/timer.nif
// oblivion/gate/oblivionarchgate01.nif
// oblivion/gate/obliviongate_forming.nif
// oblivion/gate/obliviongate_simple.nif
// oblivion/gate/oblivionwargateani02.nif
NiBtOgre::NiFlipController::NiFlipController(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream.read(mTexureSlot);

    if (stream.nifVer() >= 0x04000000  && stream.nifVer() <= 0x0a010000)
        stream.skip(sizeof(std::uint32_t)); // Unknown Int 2

    if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream.read(mDelta);

    stream.readVector<NiSourceTextureRef>(mSources); // from 4.0.0.0
}
