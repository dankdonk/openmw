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
#ifndef NIBTOGRE_NIKEYFRAMECONTROLLER_H
#define NIBTOGRE_NIKEYFRAMECONTROLLER_H

#include "nitimecontroller.hpp"
#include "niobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiTimeController
//     NiInterpController <------------------------------- /* NiTimeController */
//         NiSingleInterpController
//             NiKeyframeController
//                 BSKeyframeController
//                 NiTransformController <---------------- /* NiKeyframeController */
namespace NiBtOgre
{
    class NiKeyframeController : public NiSingleInterpController
    {
    public:
        NiKeyframeDataRef mDataRef;

        NiKeyframeController(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data);

        void build(NiAVObject* target, const NiTransformInterpolator& interpolator,
                std::vector<Ogre::Controller<float> >& controllers);

        NiTimeControllerRef build(std::vector<Ogre::Controller<float> >& controllers, Ogre::Mesh *mesh = nullptr);
    };

    // NiTransformController replaces the NiKeyframeController.
    typedef NiKeyframeController NiTransformController; // Seen in NIF ver 20.0.0.4, 20.0.0.5

#if 0
    // creatures/minotaur/minotaurold.nif
    class BSKeyframeController : public NiKeyframeController
    {
    public:
        NiKeyframeDataRef mData2Ref;

        BSKeyframeController(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data);
    };
#endif
}

#endif // NIBTOGRE_NIKEYFRAMECONTROLLER_H
