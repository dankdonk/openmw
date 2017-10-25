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
#ifndef NIBTOGRE_BHKREFOBJECT_H
#define NIBTOGRE_BHKREFOBJECT_H

#include <vector>
#include <cstdint>

#include <OgreVector3.h>
#include <OgreVector4.h>
#include <OgreQuaternion.h>
#include <OgreMatrix4.h>

#include "niobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// bhkRefObject <------------------------------ /* typedef NiObject */
//     bhkCompressedMeshShapeData
//     bhkSerializable <----------------------- /* typedef NiRefObject */
//         bhkBallSocketConstraintChain
//         bhkConstraint
//             bhkBreakableConstraint
//             bhkHingeConstraint
//             bhkLimitedHingeConstraint
//             bhkMalleableConstraint
//             bhkPrismaticConstraint
//             bhkRagdollConstraint
//             bhkStiffSpringConstraint
//         bhkShape <-------------------------- /* typedef bhkSerializable */
//             bhkBvTreeShape <---------------- /* not implemented */
//                 bhkMoppBvTreeShape
//             bhkCompressedMeshShape
//             bhkShapeCollection <------------ /* not implemented */
//                 bhkListShape
//                 bhkNiTriStripsShape
//                 bhkPackedNiTriStripsShape
//                 hkPackedNiTriStripsData
//             bhkSphereRepShape
//                 bhkConvexShape <------------ /* not implemented */
//                     bhkBoxShape
//                     bhkCapsuleShape
//                     bhkConvexVerticesShape
//                     bhkSphereShape
//                 bhkMultiSphereShape
//             bhkTransformShape
//                 bhkConvexTransformShape <--- /* typedef bhkTransformShape */
//         bhkWorldObject <-------------------- /* not implemented */
//             bhkEntity
//                 bhkRigidBody
//                     bhkRigidBodyT <--------- /* typedef bhkRigidBody */
//             bhkPhantom <-------------------- /* not implemented */
//                 bhkShapePhantom <----------- /* not implemented */
//                     bhkSimpleShapePhantom
// hkbStateMachineEventPropertyArray <--------- /* TODO */
namespace NiBtOgre
{
    class NiStream;
    class Header;

    typedef NiObject bhkRefObject;
    typedef bhkRefObject bhkSerializable;
#if 0
    struct bhkSerializable : public NiObject
    {
        bhkSerializable(NiStream& stream, const NiModel& model);
    };
#endif

    // Seen in NIF version 20.2.0.7
    struct bhkCompressedMeshShapeData : public bhkRefObject
    {
        struct bhkCMSDMaterial
        {
            std::uint32_t skyrimMaterial;
            std::uint32_t unknown;
        };

        struct bhkCMSDTransform
        {
            Ogre::Vector4 translation;
            Ogre::Quaternion rotation;
        };

        struct bhkCMSDBigTris
        {
            std::uint16_t triangle1;
            std::uint16_t triangle2;
            std::uint16_t triangle3;
            std::uint32_t unknown1;
            std::uint16_t unknown2;
        };

        struct bhkCMSDChunk
        {
            Ogre::Vector4 translation;
            std::uint32_t materialIndex;
            std::uint16_t unknown1;
            std::uint16_t transformIndex;
            std::vector<std::uint16_t> vertices;
            std::vector<std::uint16_t> indicies;
            std::vector<std::uint16_t> strips;
            std::vector<std::uint16_t> indicies2;

            void read(NiStream& stream);
        };

        std::uint32_t mBitsPerIndex;
        std::uint32_t mBitsPerWIndex;
        std::uint32_t mMaskWIndex;
        std::uint32_t mMaskIndex;
        float         mError;
        Ogre::Vector4 mBoundsMin;
        Ogre::Vector4 mBoundsMax;

        std::vector<bhkCMSDMaterial>  mChunkMaterials;
        std::vector<bhkCMSDTransform> mChunkTransforms;
        std::vector<Ogre::Vector4>    mBigVerts;
        std::vector<bhkCMSDBigTris>   mBigTris;
        std::vector<bhkCMSDChunk>     mChunks;

        bhkCompressedMeshShapeData(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    struct bhkBallSocketConstraintChain : public bhkSerializable
    {
        std::vector<Ogre::Vector4> mFloats1;
        float mUnknownFloat1;
        float mUnknownFloat2;
        std::uint32_t mUnknownInt1;
        std::uint32_t mUnknownInt2;
        std::vector<NiObject*> mLinks;  // Ptr
        std::vector<NiObject*> mLinks2; // Ptr
        std::uint32_t mUnknownInt3;

        bhkBallSocketConstraintChain(NiStream& stream, const NiModel& model);
    };

    struct bhkEntity;

    struct bhkConstraint : public bhkSerializable
    {
        std::vector<bhkEntity*> mEntities; // Ptr
        std::uint32_t mPriority;

        bhkConstraint(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    struct bhkBreakableConstraint : public bhkConstraint
    {
        std::int16_t mUnknownShort1;
        std::uint32_t mUnknownInt1;
        std::vector<bhkEntity*> mEntities2; // Ptr
        std::uint32_t mPriority2;
        std::uint32_t mUnknownInt2;
        Ogre::Vector3 mPosition;
        Ogre::Vector3 mRotation;
        std::uint32_t mUnknownInt3;
        float mThreshold;
        float mUnknownFloat1;

        bhkBreakableConstraint(NiStream& stream, const NiModel& model);
    };

    struct HingeDescriptor
    {
        Ogre::Vector4 pivotA;
        Ogre::Vector4 perp2AxleA1;
        Ogre::Vector4 perp2AxleA2;
        Ogre::Vector4 pivotB;
        Ogre::Vector4 axleB;
        Ogre::Vector4 axleA;
        Ogre::Vector4 perp2AxleB1;
        Ogre::Vector4 perp2AxleB2;

        void read(NiStream& stream);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkHingeConstraint : public bhkConstraint
    {
        HingeDescriptor mHinge;

        bhkHingeConstraint(NiStream& stream, const NiModel& model);
    };

    struct LimitedHingeDescriptor
    {
        Ogre::Vector4 pivotA;
        Ogre::Vector4 axleA;
        Ogre::Vector4 perp2AxleA1;
        Ogre::Vector4 perp2AxleA2;
        Ogre::Vector4 pivotB;
        Ogre::Vector4 axleB;
        Ogre::Vector4 perp2AxleB2;
        Ogre::Vector4 perp2AxleB1;

        float minAngle;
        float maxAngle;
        float maxFriction;

        bool enableMotor;

        void read(NiStream& stream);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkLimitedHingeConstraint : public bhkConstraint
    {
        LimitedHingeDescriptor mLimitedHinge;

        bhkLimitedHingeConstraint(NiStream& stream, const NiModel& model);
    };

    struct RagdollDescriptor
    {
        Ogre::Vector4 pivotA;
        Ogre::Vector4 planeA;
        Ogre::Vector4 twistA;
        Ogre::Vector4 pivotB;
        Ogre::Vector4 planeB;
        Ogre::Vector4 twistB;
        Ogre::Vector4 motorA;
        Ogre::Vector4 motorB;
        float coneMaxAngle;
        float planeMinAngle;
        float planeMaxAngle;
        float twistMinAngle;
        float twistMaxAngle;
        float maxFriction;

        bool enableMotor;

        void read(NiStream& stream);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkMalleableConstraint : public bhkConstraint
    {
        std::uint32_t mType;
        std::uint32_t mUnknownInt2;
        NiObjectRef   mUnknownLink1Index;
        NiObjectRef   mUnknownLink2Index;
        std::uint32_t mUnknownInt3;
        HingeDescriptor mHinge;
        RagdollDescriptor mRagdoll;
        LimitedHingeDescriptor mLimitedHinge;
        float mTau;
        float mDamping;

        bhkMalleableConstraint(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkPrismaticConstraint : public bhkConstraint
    {
        Ogre::Vector4 mPivotA;
        std::vector<Ogre::Vector4> mRotationMatrixA;
        Ogre::Vector4 mPivotB;
        Ogre::Vector4 mSlidingB;
        Ogre::Vector4 mPlaneB;
        Ogre::Vector4 mSlidingA;
        Ogre::Vector4 mRotationA;
        Ogre::Vector4 mPlaneA;
        Ogre::Vector4 mRotationB;
        float mMinDistance;
        float mMaxDistance;
        float mFriction;

        bhkPrismaticConstraint(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkRagdollConstraint : public bhkConstraint
    {
        RagdollDescriptor mRagdoll;

        bhkRagdollConstraint(NiStream& stream, const NiModel& model);
    };

    typedef bhkSerializable bhkShape;
#if 0
    struct bhkShape : public bhkSerializable
    {
        bhkShape(NiStream& stream, const NiModel& model);
    };
#endif

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkStiffSpringConstraint : public bhkConstraint
    {
        Ogre::Vector4 mPivotA;
        Ogre::Vector4 mPivotB;
        float         mLength;

        bhkStiffSpringConstraint(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkMoppBvTreeShape : public bhkShape
    {
        bhkShapeRef mShapeIndex;
        std::uint32_t mMaterial;
        std::vector<unsigned char> mUnknown8Bytes;
        float mUnknownFloat;
        Ogre::Vector3 mOrigin;
        float mScale;
        std::vector<unsigned char> mMOPPData;

        bhkMoppBvTreeShape(NiStream& stream, const NiModel& model);
    };

    class NiAVObject;

    // Seen in NIF version 20.2.0.7
    struct bhkCompressedMeshShape : public bhkShape
    {
        NiAVObject *mTarget; // Ptr
        std::uint32_t mSkyrimMaterial;
        std::vector<unsigned char> mUnknown4Bytes;
        float mRadius;
        float mScale;
        bhkCompressedMeshShapeDataRef mDataIndex;

        bhkCompressedMeshShape(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkListShape : public bhkShape
    {
        std::vector<bhkShapeRef> mSubShapes;
        std::uint32_t mMaterial; // if userVer >= 12, SkyrimHavokMaterial
        //std::vector<float> mUnknownFloats;
        std::vector<std::uint32_t> mUnknownInts;

        bhkListShape(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkNiTriStripsShape : public  bhkShape
    {
        struct OblivionColFilter
        {
            unsigned char layer;
            unsigned char colFilter;
            std::uint16_t unknownShort;
        };

        // if (userVer < 12) // http://niftools.sourceforge.net/doc/nif/HavokMaterial.html
        // if (userVer >= 12) // http://niftools.sourceforge.net/doc/nif/SkyrimHavokMaterial.html
        std::uint32_t mMaterial;
        float mUnknownFloat1;
        std::uint32_t mUnknownInt1;
        std::vector<std::uint32_t> mUnknownInts1;
        std::uint32_t mUnknownInt2;

        Ogre::Vector3 mScale;
        std::uint32_t mUnknownInt3;

        std::vector<NiTriStripsDataRef> mStripsData;
        std::vector<OblivionColFilter> mDataLayers;

        bhkNiTriStripsShape(NiStream& stream, const NiModel& model);
    };

    struct OblivionSubShape
    {
        unsigned char layer; // http://niftools.sourceforge.net/doc/nif/OblivionLayer.html
        unsigned char colFilter;
        std::uint16_t unknownShort;
        std::uint32_t numVertices;
        std::uint32_t material; // http://niftools.sourceforge.net/doc/nif/HavokMaterial.html

        void read(NiStream& stream);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkPackedNiTriStripsShape : public  bhkShape
    {
        std::vector<OblivionSubShape> mSubShapes;
        std::uint32_t mUnknownInt1;
        std::uint32_t mUnknownInt2;
        float         mUnknownFloat1;
        std::uint32_t mUnknownInt3;
        Ogre::Vector3 mScaleCopy;
        float         mUnknownFloat2;
        float         mUnknownFloat3;
        Ogre::Vector3 mScale;
        float         mUnknownFloat4;
        hkPackedNiTriStripsDataRef mDataIndex;

        bhkPackedNiTriStripsShape(NiStream& stream, const NiModel& model);
    };


    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct hkPackedNiTriStripsData : public bhkShape
    {
        struct hkTriangle
        {
            std::vector<short> triangle;
            std::uint16_t weldingInfo;
            Ogre::Vector3 normal;
        };

        std::vector<hkTriangle> mTriangles;
        std::vector<Ogre::Vector3> mVertices;
        std::vector<OblivionSubShape> mSubShapes;

        hkPackedNiTriStripsData(NiStream& stream, const NiModel& model);
    };

    struct bhkSphereRepShape : public bhkShape
    {
        std::uint32_t mMaterial; // if userVer >= 12, SkyrimHavokMaterial
        float mRadius;

        bhkSphereRepShape(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkBoxShape : public bhkSphereRepShape
    {
        std::vector<unsigned char> mUnknown8Bytes;
        Ogre::Vector3 mDimensions;
        float mMinimumSize;

        bhkBoxShape(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkCapsuleShape : public bhkSphereRepShape
    {
        std::vector<unsigned char> mUnknown8Bytes;
        Ogre::Vector3 mFirstPoint;
        float mRadius1;
        Ogre::Vector3 mSecondPoint;
        float mRadius2;

        bhkCapsuleShape(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkConvexVerticesShape : public bhkSphereRepShape
    {
        std::vector<float> mUnknown6Floats;
        std::vector<Ogre::Vector4> mVertices;
        std::vector<Ogre::Vector4> mNormals;

        bhkConvexVerticesShape(NiStream& stream, const NiModel& model);
    };

    typedef bhkSphereRepShape bhkSphereShape; // Seen in NIF ver 20.0.0.4, 20.0.0.5
#if 0
    struct bhkSphereShape : public bhkSphereRepShape
    {
        bhkSphereShape(NiStream& stream, const NiModel& model);
    };
#endif

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkMultiSphereShape : public bhkSphereRepShape
    {
        struct SphereBV
        {
            Ogre::Vector3 center;
            float radius;
        };

        float mUnknownFloat1;
        float mUnknownFloat2;
        std::vector<SphereBV> mSpheres;

        bhkMultiSphereShape(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkTransformShape : public bhkShape
    {
        bhkShapeRef mShapeIndex;
        std::uint32_t mMaterial; // if userVer >= 12, SkyrimHavokMaterial
        float mUnknownFloat1;
        std::vector<unsigned char> mUnknown8Bytes;
        Ogre::Matrix4 mTransform;

        bhkTransformShape(NiStream& stream, const NiModel& model);
    };

    typedef bhkTransformShape bhkConvexTransformShape; // Seen in NIF ver 20.0.0.4, 20.0.0.5

    struct bhkEntity : public bhkSerializable // bhkWorldObject
    {
        bhkShapeRef mShapeIndex;
        unsigned char mLayer; // http://niftools.sourceforge.net/doc/nif/OblivionLayer.html
        unsigned char mColFilter;
        std::uint16_t mUnknownShort;

        bhkEntity(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkRigidBody : public bhkEntity
    {
        std::int32_t mUnknownInt1;
        std::int32_t mUnknownInt2;
        std::vector<std::int32_t> mUnknown3Ints;
        unsigned char mCollisionResponse;
        unsigned char mUnknownByte;
        std::uint16_t mProcessContactCallbackDelay;
        std::vector<std::uint16_t> mUnknown2Shorts;
        unsigned char mLayerCopy;
        unsigned char mColFilterCopy;
        std::vector<std::uint16_t> mUnknown7Shorts;

        Ogre::Vector4 mTranslation;
        Ogre::Quaternion mRotation;
        Ogre::Vector4 mLinearVelocity;
        Ogre::Vector4 mAngularVelocity;
        Ogre::Real mInertia[3][4];
        Ogre::Vector4 mCenter;
        float mMass;
        float mLinearDamping;
        float mAngularDamping;
        float mGravityFactor1;
        float mGravityFactor2;
        float mFriction;
        float mRollingFrictionMultiplier;
        float mRestitution;
        float mMaxLinearVelocity;
        float mMaxAngularVelocity;
        float mPenetrationDepth;

        unsigned char mMotionSystem; // http://niftools.sourceforge.net/doc/nif/MotionSystem.html
        unsigned char mDeactivatorType; // http://niftools.sourceforge.net/doc/nif/DeactivatorType.html
        unsigned char mSolverDeactivation; // http://niftools.sourceforge.net/doc/nif/SolverDeactivation.html
        unsigned char mQualityType; // http://niftools.sourceforge.net/doc/nif/MotionQuality.html

        std::uint32_t mUnknownInt6;
        std::uint32_t mUnknownInt7;
        std::uint32_t mUnknownInt8;
        std::uint32_t mUnknownInt81;
        std::vector<bhkSerializableRef> mConstraints;
        std::uint32_t mUnknownInt9;
        std::uint16_t mUnknownInt91;

        bhkRigidBody(NiStream& stream, const NiModel& model);
    };

    typedef bhkRigidBody bhkRigidBodyT; // Seen in NIF ver 20.0.0.4, 20.0.0.5

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    struct bhkSimpleShapePhantom : public bhkSerializable // bhkWorldObject
    {
        bhkShapeRef mShapeIndex;
        unsigned char mLayer; // http://niftools.sourceforge.net/doc/nif/OblivionLayer.html
        unsigned char mColFilter;
        std::uint16_t mUnknownShort;

        // FIXME

        bhkSimpleShapePhantom(NiStream& stream, const NiModel& model);
    };
}

#endif // NIBTOGRE_BHKREFOBJECT_H
