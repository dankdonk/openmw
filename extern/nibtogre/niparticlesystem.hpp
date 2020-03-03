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
#ifndef NIBTOGRE_NIPARTICLESYSTEM_H
#define NIBTOGRE_NIPARTICLESYSTEM_H

#include "nigeometry.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiObjectNET
//     NiAVObject
//         NiGeometry
//             NiParticles <---------------- /* typedef NiGeometry */
//                 NiAutoNormalParticles <-- /* typedef NiParticles */
//                 NiParticleSystem
//                     BSStripParticleSystem /* typedef NiParticleSystem */
//                 NiRotatingParticles <---- /* typedef NiParticles */
namespace NiBtOgre
{
    class NiStream;
    class Header;

    typedef NiGeometry NiParticles;
    typedef NiParticles NiAutoNormalParticles;

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiParticleSystem : public NiParticles
    {
        std::uint16_t mUnknownS2;
        std::uint16_t mUnknownS3;
        std::uint32_t mUnknownI1;
        bool mWorldSpace;                          // from 10.1.0.0
        std::vector<NiPSysModifierRef> mModifiers; // from 10.1.0.0

        NiParticleSystem(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data);

        virtual void build(NiObject *parentNiNode = nullptr);
    };

    typedef NiParticleSystem BSStripParticleSystem; // Seen in NIF version 20.2.0.7
    typedef NiParticles NiRotatingParticles;
}

#endif // NIBTOGRE_NIPARTICLESYSTEM_H
