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
#ifndef NIBTOGRE_NICOLLISIONOBJECT_H
#define NIBTOGRE_NICOLLISIONOBJECT_H

#include <vector>
#include <cstdint>

#include <OgreVector4.h>

#include "niobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiCollisionObject
//     bhkNiCollisionObject
//         bhkCollisionObject <---------- /* bhkNiCollisionObject */
//             bhkBlendCollisionObject
//         bhkPCollisionObject <--------- /* bhkNiCollisionObject */
//             bhkSPCollisionObject <---- /* bhkNiCollisionObject */
namespace NiBtOgre
{
    class NiAVObject;

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    class NiCollisionObject : public NiObject
    {
    protected:
        // architecture/castle/castlelight01.nif (TES4) shows that some of the Ptr refer to
        // objects not yet loaded.  Change to Ref instead.
        //NiAVObject *mTarget; // Ptr
        NiAVObjectRef mTargetRef;

    public:
        NiCollisionObject(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data);
        virtual ~NiCollisionObject() {}
    };

    // NiMesh, NiSkinningMeshModifier, bhkMultiSphereShape, BoundingVolume
    struct SphereBV
    {
        Ogre::Vector3 center;
        float radius;
    };

    struct BoundingVolume
    {
        struct BoxBV
        {
            Ogre::Vector3 center;
            std::vector<Ogre::Vector3> axis;
            std::vector<float> extent;
        };

        struct CapsuleBV
        {
            Ogre::Vector3 center;
            Ogre::Vector3 origin;
            float unknown1;
            float unknown2;
        };

        struct HalfSpaceBV
        {
            Ogre::Vector3 normal;
            Ogre::Vector3 center;
        };

        std::int32_t mCollisionType;
        SphereBV     mSphere;
        BoxBV        mBox;
        CapsuleBV    mCapsule;
        HalfSpaceBV  mHalfspace;
    };

    // Collision box
    class NiCollisionData : public NiCollisionObject
    {
        std::uint32_t  mPropagationMode;
        std::uint32_t  mCollisionMode;
        BoundingVolume mBoundingVolume;

    public:
        NiCollisionData(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data);
        virtual ~NiCollisionData() {}
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    class bhkNiCollisionObject : public NiCollisionObject
    {
        // Notes from NifSkope: Set to 1 for most objects, and to 41 for animated objects (OL_ANIM_STATIC).
        //                      Bits: 0=Active 2=Notify 3=Set Local 6=Reset.
        std::uint16_t mFlags;
        NiObjectRef   mBodyRef;

    public:
        bhkNiCollisionObject(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data);
        virtual ~bhkNiCollisionObject() {}

        // parentNiNode is used to calculate the world transform
        virtual void build(BtOgreInst *inst, BuildData *data, NiObject *parentNiNode = nullptr);
        int32_t getBodyRef() const { return mBodyRef; }
    };

    typedef bhkNiCollisionObject bhkCollisionObject; // Seen in NIF version 20.0.0.4, 20.0.0.5

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    class bhkBlendCollisionObject : public bhkCollisionObject
    {
        float mUnknown1;
        float mUnknown2;

    public:
        bhkBlendCollisionObject(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data);
        virtual ~bhkBlendCollisionObject() {}
    };

    typedef bhkNiCollisionObject bhkPCollisionObject;
    typedef bhkNiCollisionObject bhkSPCollisionObject; // Seen in NIF version 20.0.0.4, 20.0.0.5
}

#endif // NIBTOGRE_NICOLLISIONOBJECT_H
