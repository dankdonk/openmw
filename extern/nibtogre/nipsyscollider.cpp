/*
  Copyright (C) 2015-2017 cc9cii

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

*/
#include "nipsyscollider.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "nistream.hpp"
#include "niavobject.hpp" // static_cast NiNode
#include "nimodel.hpp"

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysCollider::NiPSysCollider(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    stream.read(mBounce);
    stream.read(mSpawnOnCollide);
    stream.read(mDieOnCollide);
    stream.read(mSpawnModifierIndex);

    //stream.getPtr<NiObject>(mParent, model.objects());
    std::int32_t index = -1;
    stream.read(index);
    mParent = model.getRef<NiObject>(index);

    stream.read(mNextColliderIndex);

    //stream.getPtr<NiObject>(mColliderObject, model.objects());
    index = -1; // note: variable reused
    stream.read(index);
    mColliderObject = model.getRef<NiNode>(index);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysPlanarCollider::NiPSysPlanarCollider(NiStream& stream, const NiModel& model)
    : NiPSysCollider(stream, model)
{
    stream.read(mWidth);
    stream.read(mHeight);
    stream.read(mXAxis);
    stream.read(mYAxis);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiPSysSphericalCollider::NiPSysSphericalCollider(NiStream& stream, const NiModel& model)
    : NiPSysCollider(stream, model)
{
    stream.read(mRadius);
}
