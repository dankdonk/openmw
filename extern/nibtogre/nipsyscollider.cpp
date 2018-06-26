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
#include "nipsyscollider.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "ninode.hpp" // static_cast NiNode
#include "nimodel.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysCollider::NiPSysCollider(uint32_t index, NiStream& stream, const NiModel& model)
    : NiObject(index, stream, model)
{
    stream.read(mBounce);
    stream.read(mSpawnOnCollide);
    stream.read(mDieOnCollide);
    stream.read(mSpawnModifierIndex);

    //stream.getPtr<NiObject>(mParent, model.objects());
    std::int32_t rIndex = -1;
    stream.read(rIndex);
    mParent = model.getRef<NiObject>(rIndex);

    stream.read(mNextColliderIndex);

    //stream.getPtr<NiNode>(mColliderObject, model.objects());
    rIndex = -1; // note: variable reused
    stream.read(rIndex);
    mColliderObject = model.getRef<NiNode>(rIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysPlanarCollider::NiPSysPlanarCollider(uint32_t index, NiStream& stream, const NiModel& model)
    : NiPSysCollider(index, stream, model)
{
    stream.read(mWidth);
    stream.read(mHeight);
    stream.read(mXAxis);
    stream.read(mYAxis);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiPSysSphericalCollider::NiPSysSphericalCollider(uint32_t index, NiStream& stream, const NiModel& model)
    : NiPSysCollider(index, stream, model)
{
    stream.read(mRadius);
}
