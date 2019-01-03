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
#include "nicollisionobject.hpp"

#include <cassert>
#include <stdexcept>

#include <btBulletDynamicsCommon.h>

#include "nistream.hpp"
#include "niavobject.hpp" // static_cast NiAVObject
#include "nimodel.hpp"
#include "bhkrefobject.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiCollisionObject::NiCollisionObject(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiObject(index, stream, model, data)
{
    //stream.getPtr<NiAVObject>(mTarget, model.objects());
    //std::int32_t rIndex = -1;
    //stream.read(rIndex);
    //mTarget = model.getRef<NiAVObject>(rIndex);
    stream.read(mTargetIndex);
}

NiBtOgre::NiCollisionData::NiCollisionData(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiCollisionObject(index, stream, model, data)
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
NiBtOgre::bhkNiCollisionObject::bhkNiCollisionObject(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : NiCollisionObject(index, stream, model, data)
{
    stream.read(mFlags);
    stream.read(mBodyIndex);
}

// build the 'body' to the 'target'
//
// Old implementation was at ManualBulletShapeLoader::handleBhkCollisionObject()
// It is probably intended to be per rigid body, possibly TES3 NIF had no separate
// collision shapes and hence considered the whole thing as one rigid body?
void NiBtOgre::bhkNiCollisionObject::build(BtOgreInst *inst, ModelData *data, NiObject *parentNiNode)
{
    // collision objects have 'target' which should have the parent world transform
    // FIXME: assert during testing only
    assert(mTargetIndex == parentNiNode->index() && "CollisionObject: parent is not the target");

#if 0
    // mBodyIndex refers to either a bhkRigidBody or bhkRigidBodyT
    // apply rotation and translation only if the collision object's body is a bhkRigidBodyT type
    if (mModel.blockType(mBodyIndex) == "bhkRigidBodyT")
    {
        rotation = Ogre::Quaternion(rigidBody->rotation.w,
                rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);
        translation = Ogre::Vector3(rigidBody->translation.x,
                rigidBody->translation.y, rigidBody->translation.z);
    }
#endif

    mModel.getRef<NiObject>(mBodyIndex)->build(inst, data, parentNiNode); // NOTE: parent passed
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkBlendCollisionObject::bhkBlendCollisionObject(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data)
    : bhkCollisionObject(index, stream, model, data)
{
    stream.read(mUnknown1);
    stream.read(mUnknown2);
}
