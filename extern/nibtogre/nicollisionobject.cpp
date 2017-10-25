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
#include "nicollisionobject.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

#include "nistream.hpp"
#include "niavobject.hpp" // static_cast NiAVObject
#include "nimodel.hpp"

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiCollisionObject::NiCollisionObject(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    //stream.getPtr<NiAVObject>(mTarget, model.objects());
    std::int32_t index = -1;
    stream.read(index);
    mTarget = model.getRef<NiAVObject>(index);
}

NiBtOgre::NiCollisionData::NiCollisionData(NiStream& stream, const NiModel& model)
    : NiCollisionObject(stream, model)
{
    stream.read(mPropagationMode);
    stream.read(mCollisionMode);

    char useABV;
    stream.read(useABV);
    if (useABV != 0)
    {
        stream.read(mBoundingVolume.mCollisionType);
        switch (mBoundingVolume.mCollisionType)
        {
            case 0: // Sphere
                stream.read(mBoundingVolume.mSphere.center);
                stream.read(mBoundingVolume.mSphere.radius);
                break;
            case 1: // Box
                stream.read(mBoundingVolume.mBox.center);
                mBoundingVolume.mBox.axis.resize(3);
                for (int i = 0; i < 3; ++i)
                    stream.read(mBoundingVolume.mBox.axis.at(i));
                mBoundingVolume.mBox.extent.resize(3);
                for (int i = 0; i < 3; ++i)
                    stream.read(mBoundingVolume.mBox.extent.at(i));
                break;
            case 2: // Capsule
                stream.read(mBoundingVolume.mCapsule.center);
                stream.read(mBoundingVolume.mCapsule.origin);
                stream.read(mBoundingVolume.mCapsule.unknown1);
                stream.read(mBoundingVolume.mCapsule.unknown2);
                break;
            case 5: // HalfSpace
                stream.read(mBoundingVolume.mHalfspace.normal);
                stream.read(mBoundingVolume.mHalfspace.center);
                break;
            default:
                break;
        }
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkNiCollisionObject::bhkNiCollisionObject(NiStream& stream, const NiModel& model)
    : NiCollisionObject(stream, model)
{
    stream.read(mFlags);
    stream.read(mBodyIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkBlendCollisionObject::bhkBlendCollisionObject(NiStream& stream, const NiModel& model)
    : bhkCollisionObject(stream, model)
{
    stream.read(mUnknown1);
    stream.read(mUnknown2);
}
