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
#include "nikeyframecontroller.hpp"

#include <cassert>
#include <stdexcept>
#include <iostream> // FIXME

#include "nistream.hpp"
#include "nimodel.hpp"
#include "ninode.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

NiBtOgre::NiKeyframeController::NiKeyframeController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    if (stream->nifVer() <= 0x0a010000) // up to 10.1.0.0
        stream->read(mDataRef);

#if 1
    //if (NiTimeController::mTargetRef != -1) // FIXME creatures\cliffracer\skeleton.nif
    if (model.blockType(NiTimeController::mTargetRef) == "NiNode")
        data.addSkelLeafIndex(NiTimeController::mTargetRef); // FIXME: do this for old NIF versions only?
#else
    // FIXME: is there a better way than doing a string comparison each time?
    if (model.blockType(NiTimeController::mTargetRef) == "NiNode")
    {
        data.addSkelLeafIndex(NiTimeController::mTargetRef);

        NiNode *node = model.getRef<NiNode>(NiTimeController::mTargetRef);
        const std::vector<NiAVObjectRef>& children = node->getChildren();

        if (NiTimeController::mTargetRef == 182)
            std::cout << "stop" << std::endl;

        for (unsigned int i = 0; i < children.size(); ++i)
        {
            if (children[i] == -1)
                continue;

            if (model.blockType(children[i]) == "NiNode")
                data.addSkelLeafIndex(children[i]);
        }
    }
#endif
}

NiBtOgre::NiTimeControllerRef NiBtOgre::NiKeyframeController::build(std::vector<Ogre::Controller<float> > & controllers, Ogre::Mesh *mesh)
{
    return -1;
}

void NiBtOgre::NiKeyframeController::build(NiAVObject* target, const NiTransformInterpolator& interpolator, std::vector<Ogre::Controller<float> >& controllers)
{
}

#if 0
NiBtOgre::BSKeyframeController::BSKeyframeController(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiSingleInterpController(index, stream, model, data)
{
    stream->read(mData2Ref);
}
#endif
