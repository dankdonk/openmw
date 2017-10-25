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
#include "bhkrefobject.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

#include "nistream.hpp"
#include "niavobject.hpp" // static_cast NiAVObject
#include "nimodel.hpp"

#if 0
NiBtOgre::bhkSerializable::bhkSerializable(NiStream& stream, const NiModel& model)
{
}
#endif

void NiBtOgre::bhkCompressedMeshShapeData::bhkCMSDChunk::read(NiStream& stream)
{
    stream.read(translation);
    stream.read(materialIndex);
    stream.read(unknown1);
    stream.read(transformIndex);

    stream.readVector<std::uint16_t>(vertices);
    stream.readVector<std::uint16_t>(indicies);
    stream.readVector<std::uint16_t>(strips);
    stream.readVector<std::uint16_t>(indicies2);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::bhkCompressedMeshShapeData::bhkCompressedMeshShapeData(NiStream& stream, const NiModel& model)
    : bhkRefObject(stream, model)
{
    stream.read(mBitsPerIndex);
    stream.read(mBitsPerWIndex);
    stream.read(mMaskWIndex);
    stream.read(mMaskIndex);
    stream.read(mError);
    stream.read(mBoundsMin);
    stream.read(mBoundsMax);

    stream.skip(sizeof(char));          // Unknown Byte 1
    stream.skip(sizeof(std::uint32_t)); // Unknown Int 3
    stream.skip(sizeof(std::uint32_t)); // Unknown Int 4
    stream.skip(sizeof(std::uint32_t)); // Unknown Int 5
    stream.skip(sizeof(char));          // Unknown Byte 2

    std::uint32_t numMaterials;
    stream.read(numMaterials);
    mChunkMaterials.resize(numMaterials);
    for (unsigned int i = 0; i < numMaterials; ++i)
    {
        stream.read(mChunkMaterials[i].skyrimMaterial);
        stream.read(mChunkMaterials[i].unknown);
    }

    stream.skip(sizeof(std::uint32_t)); // Unknown Int 6

    std::uint32_t numTransforms;
    stream.read(numTransforms);
    mChunkTransforms.resize(numTransforms);
    for (unsigned int i = 0; i < numTransforms; ++i)
    {
        stream.read(mChunkTransforms[i].translation);
        stream.readQuaternionXYZW(mChunkTransforms[i].rotation);
    }

    std::uint32_t numBigVerts;
    stream.read(numBigVerts);
    mBigVerts.resize(numBigVerts);
    for (unsigned int i = 0; i < numBigVerts; ++i)
        stream.read(mBigVerts.at(i));

    std::uint32_t numBigTris;
    stream.read(numBigTris);
    mBigTris.resize(numBigTris);
    for (unsigned int i = 0; i < numBigTris; ++i)
    {
        stream.read(mBigTris[i].triangle1);
        stream.read(mBigTris[i].triangle2);
        stream.read(mBigTris[i].triangle3);
        stream.read(mBigTris[i].unknown1);
        stream.read(mBigTris[i].unknown2);
    }

    std::uint32_t numChunks;
    stream.read(numChunks);
    mChunks.resize(numChunks);
    for (unsigned int i = 0; i < numChunks; ++i)
        mChunks[i].read(stream);

    stream.skip(sizeof(std::uint32_t)); // Unknown Int 12
}

// Seen in NIF version 20.2.0.7
NiBtOgre::bhkBallSocketConstraintChain::bhkBallSocketConstraintChain(NiStream& stream, const NiModel& model)
    : bhkSerializable(stream, model)
{
    std::uint32_t numFloats;
    stream.read(numFloats);
    mFloats1.resize(numFloats);
    for (unsigned int i = 0; i < numFloats; ++i)
        stream.read(mFloats1.at(i));

    stream.read(mUnknownFloat1);
    stream.read(mUnknownFloat2);
    stream.read(mUnknownInt1);
    stream.read(mUnknownInt2);

    std::int32_t index = -1;
    std::uint32_t numLinks;
    stream.read(numLinks);
    mLinks.resize(numLinks);
    for (unsigned int i = 0; i < numLinks; ++i)
    {
        //stream.getPtr<NiObject>(mLinks.at(i), model.objects());
        index = -1;
        stream.read(index);
        mLinks[i] = model.getRef<NiObject>(index);
    }

    stream.read(numLinks); // note: numLinks reused
    mLinks2.resize(numLinks);
    for (unsigned int i = 0; i < numLinks; ++i)
    {
        //stream.getPtr<NiObject>(mLinks2.at(i), model.objects());
        index = -1; // note: index reused
        stream.read(index);
        mLinks2[i] = model.getRef<NiObject>(index);
    }

    stream.read(mUnknownInt3);
}

NiBtOgre::bhkConstraint::bhkConstraint(NiStream& stream, const NiModel& model)
    : bhkSerializable(stream, model)
{
    std::int32_t index = -1;
    std::uint32_t numEntities;
    stream.read(numEntities);
    mEntities.resize(numEntities);
    for (unsigned int i = 0; i < numEntities; ++i)
    {
        //stream.getPtr<bhkEntity>(mEntities.at(i), model.objects());
        index = -1;
        stream.read(index);
        mEntities[i] = model.getRef<bhkEntity>(index);
    }

    stream.read(mPriority);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::bhkBreakableConstraint::bhkBreakableConstraint(NiStream& stream, const NiModel& model)
    : bhkConstraint(stream, model)
{
    if (stream.userVer() <= 11)
    {
        //for (unsigned int i = 0; i < 41; ++i)
            //stream.skip(sizeof(std::int32_t));
        stream.skip(sizeof(std::int32_t)*41);

        stream.skip(sizeof(std::int16_t));
    }
    else if (stream.userVer() == 12)
    {
        stream.read(mUnknownInt1);

        std::int32_t index = -1;
        std::uint32_t numEntities2;
        stream.read(numEntities2);
        mEntities2.resize(numEntities2);
        for (unsigned int i = 0; i < numEntities2; ++i)
        {
            //stream.getPtr<bhkEntity>(mEntities2.at(i), model.objects());
            index = -1;
            stream.read(index);
            mEntities2[i] = model.getRef<bhkEntity>(index);
        }

        stream.read(mPriority2);

        stream.read(mUnknownInt2);
        stream.read(mPosition);
        stream.read(mRotation);
        stream.read(mUnknownInt3);
        stream.read(mThreshold);
        if (mUnknownInt1 >= 1)
            stream.read(mUnknownFloat1);
        stream.skip(sizeof(char));
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
void NiBtOgre::HingeDescriptor::read(NiStream& stream)
{
    if (stream.nifVer() <= 0x14000005)
    {
        stream.read(pivotA);
        stream.read(perp2AxleA1);
        stream.read(perp2AxleA2);
        stream.read(pivotB);
        stream.read(axleB);
    }
    else if (stream.nifVer() >= 0x14020007)
    {
        stream.read(axleA);
        stream.read(perp2AxleA1);
        stream.read(perp2AxleA2);
        stream.read(pivotA);
        stream.read(axleB);
        stream.read(perp2AxleB1);
        stream.read(perp2AxleB2);
        stream.read(pivotB);
    }
}
// Seen in NIF version 20.2.0.7
NiBtOgre::bhkHingeConstraint::bhkHingeConstraint(NiStream& stream, const NiModel& model)
    : bhkConstraint(stream, model)
{
    mHinge.read(stream);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
void NiBtOgre::LimitedHingeDescriptor::read(NiStream& stream)
{
    if (stream.nifVer() <= 0x14000005)
    {
        stream.read(pivotA);
        stream.read(axleA);
        stream.read(perp2AxleA1);
        stream.read(perp2AxleA2);
        stream.read(pivotB);
        stream.read(axleB);
        stream.read(perp2AxleB2);
    }
    else if (stream.nifVer() >= 0x14020007)
    {
        stream.read(axleA);
        stream.read(perp2AxleA1);
        stream.read(perp2AxleA2);
        stream.read(pivotA);
        stream.read(axleB);
        stream.read(perp2AxleB2);
        stream.read(perp2AxleB1);
        stream.read(pivotB);
    }

    stream.read(minAngle);
    stream.read(maxAngle);
    stream.read(maxFriction);

    if (stream.nifVer() >= 0x14020007)
    {
        enableMotor = stream.getBool();
        if (enableMotor)
        {
            //stream.skip(sizeof(float)); // unknown float 1
            //stream.skip(sizeof(float)); // unknown float 2
            //stream.skip(sizeof(float)); // unknown float 3
            //stream.skip(sizeof(float)); // unknown float 4
            //stream.skip(sizeof(float)); // unknown float 5
            //stream.skip(sizeof(float)); // unknown float 6
            //stream.skip(sizeof(char);   // unknown byte 1
            stream.skip(sizeof(float)*6+sizeof(char));
        }
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkLimitedHingeConstraint::bhkLimitedHingeConstraint(NiStream& stream, const NiModel& model)
    : bhkConstraint(stream, model)
{
    mLimitedHinge.read(stream);
}

void NiBtOgre::RagdollDescriptor::read(NiStream& stream)
{
    if (stream.nifVer() <= 0x14000005)
    {
        stream.read(pivotA);
        stream.read(planeA);
        stream.read(twistA);
        stream.read(pivotB);
        stream.read(planeB);
        stream.read(twistB);
    }
    else if (stream.nifVer() >= 0x14020007)
    {
        stream.read(twistA);
        stream.read(planeA);
        stream.read(motorA);
        stream.read(pivotA);
        stream.read(twistB);
        stream.read(planeB);
        stream.read(motorB);
        stream.read(pivotB);
    }

    stream.read(coneMaxAngle);
    stream.read(planeMinAngle);
    stream.read(planeMaxAngle);
    stream.read(twistMinAngle);
    stream.read(twistMaxAngle);
    stream.read(maxFriction);

    if (stream.nifVer() >= 0x14020007)
    {
        enableMotor = stream.getBool();
        if (enableMotor)
        {
            //stream.skip(sizeof(float)); // unknown float 1
            //stream.skip(sizeof(float)); // unknown float 2
            //stream.skip(sizeof(float)); // unknown float 3
            //stream.skip(sizeof(float)); // unknown float 4
            //stream.skip(sizeof(float)); // unknown float 5
            //stream.skip(sizeof(float)); // unknown float 6
            //stream.skip(sizeof(char));  // unknown byte 1
            stream.skip(sizeof(float)*6+sizeof(char));
        }
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkMalleableConstraint::bhkMalleableConstraint(NiStream& stream, const NiModel& model)
    : bhkConstraint(stream, model)
{
    stream.read(mType);
    stream.read(mUnknownInt2);
    stream.read(mUnknownLink1Index);
    stream.read(mUnknownLink2Index);
    stream.read(mUnknownInt3);

    if (mType == 1)
        mHinge.read(stream);
    else if (mType == 2)
        mLimitedHinge.read(stream);
    else if (mType == 7)
        mRagdoll.read(stream);

    if (stream.nifVer() <= 0x14000005) // up to 20.0.0.5
        stream.read(mTau);

    stream.read(mDamping);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkPrismaticConstraint::bhkPrismaticConstraint(NiStream& stream, const NiModel& model)
    : bhkConstraint(stream, model)
{
    if (stream.nifVer() <= 0x14000005)
    {
        stream.read(mPivotA);
        mRotationMatrixA.resize(4);
        for (int i = 0; i < 4; ++i)
            stream.read(mRotationMatrixA.at(i));
        stream.read(mPivotB);
        stream.read(mSlidingB);
        stream.read(mPlaneB);
    }
    else if (stream.nifVer() >= 0x14020007)
    {
        stream.read(mSlidingA);
        stream.read(mRotationA);
        stream.read(mPlaneA);
        stream.read(mPivotA);
        stream.read(mSlidingB);
        stream.read(mRotationB);
        stream.read(mPlaneB);
        stream.read(mPivotB);
    }

    stream.read(mMinDistance);
    stream.read(mMaxDistance);
    stream.read(mFriction);

    if (stream.nifVer() >= 0x14020007)
        stream.skip(sizeof(char));  // unknown byte 1
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkRagdollConstraint::bhkRagdollConstraint(NiStream& stream, const NiModel& model)
    : bhkConstraint(stream, model)
{
    mRagdoll.read(stream);
}

#if 0
NiBtOgre::bhkShape::bhkShape(NiStream& stream, const NiModel& model)
{
}
#endif

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkStiffSpringConstraint::bhkStiffSpringConstraint(NiStream& stream, const NiModel& model)
    : bhkConstraint(stream, model)
{
    stream.read(mPivotA);
    stream.read(mPivotB);
    stream.read(mLength);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkMoppBvTreeShape::bhkMoppBvTreeShape(NiStream& stream, const NiModel& model)
    : bhkShape(stream, model)
{
    stream.read(mShapeIndex);
    stream.read(mMaterial);

    mUnknown8Bytes.resize(8);
    for (int i = 0; i < 8; ++i)
        stream.read(mUnknown8Bytes.at(i));

    stream.read(mUnknownFloat);

    std::uint32_t moppDataSize;
    stream.read(moppDataSize);
    stream.read(mOrigin);
    stream.read(mScale);

    mMOPPData.resize(moppDataSize);
    for (unsigned int i = 0; i < moppDataSize; ++i)
        stream.read(mMOPPData.at(i));

    if (stream.nifVer() >= 0x14020005 && stream.userVer() >= 12) // from 20.2.0.7
        stream.skip(sizeof(char));  // unknown byte 1
}

// Seen in NIF version 20.2.0.7
NiBtOgre::bhkCompressedMeshShape::bhkCompressedMeshShape(NiStream& stream, const NiModel& model)
    : bhkShape(stream, model)
{
    //stream.getPtr<NiAVObject>(mTarget, model.objects());
    std::int32_t index = -1;
    stream.read(index);
    mTarget = model.getRef<NiAVObject>(index);

    stream.read(mSkyrimMaterial);

    stream.skip(sizeof(float)); // Unknown Float 1

    mUnknown4Bytes.resize(4);
    for (int i = 0; i < 4; ++i)
        stream.read(mUnknown4Bytes.at(i));

    stream.skip(sizeof(float)*4); // Unknown Floats 1

    stream.read(mRadius);
    stream.read(mScale);

    stream.skip(sizeof(float)); // Unknown Float 3
    stream.skip(sizeof(float)); // Unknown Float 4
    stream.skip(sizeof(float)); // Unknown Float 5

    stream.read(mDataIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkListShape::bhkListShape(NiStream& stream, const NiModel& model)
    : bhkShape(stream, model)
{
    stream.readVector<bhkShapeRef>(mSubShapes);
    stream.read(mMaterial);

    //mUnknownFloats.resize(6);
    //for (int i = 0; i < 6; ++i)
    //    stream.read(mUnknownFloats.at(i));
    stream.skip(sizeof(float)*6);

    stream.readVector<std::uint32_t>(mUnknownInts);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkNiTriStripsShape::bhkNiTriStripsShape(NiStream& stream, const NiModel& model)
    : bhkShape(stream, model)
{
    stream.read(mMaterial);
    stream.read(mUnknownFloat1);
    stream.read(mUnknownInt1);
    mUnknownInts1.resize(4);
    for (unsigned int i = 0; i < 4; ++i)
        stream.read(mUnknownInts1.at(i));
    stream.read(mUnknownInt2);

    stream.read(mScale);
    stream.read(mUnknownInt3);

    std::uint32_t numStripsData;
    stream.read(numStripsData);
    mStripsData.resize(numStripsData);
    for (unsigned int i = 0; i < numStripsData; ++i)
    {
        stream.read(mStripsData.at(i));
    }
    std::uint32_t numDataLayers;
    stream.read(numDataLayers);
    mDataLayers.resize(numDataLayers);
    for (unsigned int i = 0; i < numDataLayers; ++i)
    {
        stream.read(mDataLayers[i].layer);
        stream.read(mDataLayers[i].colFilter);
        stream.read(mDataLayers[i].unknownShort);
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
void NiBtOgre::OblivionSubShape::read(NiStream& stream)
{
    stream.read(layer);
    stream.read(colFilter);
    stream.read(unknownShort);
    stream.read(numVertices);
    stream.read(material);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkPackedNiTriStripsShape::bhkPackedNiTriStripsShape(NiStream& stream, const NiModel& model)
    : bhkShape(stream, model)
{
    if (stream.nifVer() <= 0x14000005) // up to 20.0.0.5 only
    {
        std::uint16_t numSubShapes;
        stream.read(numSubShapes);
        mSubShapes.resize(numSubShapes);
        for (unsigned int i = 0; i < numSubShapes; ++i)
            mSubShapes[i].read(stream);
    }

    stream.read(mUnknownInt1);
    stream.read(mUnknownInt2);
    stream.read(mUnknownFloat1);
    stream.read(mUnknownInt3);
    stream.read(mScaleCopy);
    stream.read(mUnknownFloat2);
    stream.read(mUnknownFloat3);
    stream.read(mScale);
    stream.read(mUnknownFloat4);

    stream.read(mDataIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::hkPackedNiTriStripsData::hkPackedNiTriStripsData(NiStream& stream, const NiModel& model)
    : bhkShape(stream, model)
{
    std::uint32_t numTriangles;
    stream.read(numTriangles);
    mTriangles.resize(numTriangles);
    for (unsigned int i = 0; i < numTriangles; i++)
    {
        mTriangles[i].triangle.resize(3);
        stream.read(mTriangles[i].triangle.at(0));
        stream.read(mTriangles[i].triangle.at(1));
        stream.read(mTriangles[i].triangle.at(2));
        stream.read(mTriangles[i].weldingInfo);
        stream.read(mTriangles[i].normal);
    }

    std::uint32_t numVertices;
    stream.read(numVertices);
    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7
        stream.skip(sizeof(char)); // unknown byte 1
    mVertices.resize(numVertices);
    for (unsigned int i = 0; i < numVertices; i++)
        stream.read(mVertices.at(i));

    if (stream.nifVer() >= 0x14020007) // from 20.2.0.7
    {
        unsigned short numSubShapes;
        stream.read(numSubShapes);
        mSubShapes.resize(numSubShapes);
        for (unsigned int i = 0; i < numSubShapes; i++)
            mSubShapes[i].read(stream);
    }
}
NiBtOgre::bhkSphereRepShape::bhkSphereRepShape(NiStream& stream, const NiModel& model)
    : bhkShape(stream, model)
{
    stream.read(mMaterial);
    stream.read(mRadius);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkBoxShape::bhkBoxShape(NiStream& stream, const NiModel& model)
    : bhkSphereRepShape(stream, model)
{
    mUnknown8Bytes.resize(8);
    for (int i = 0; i < 8; ++i)
        stream.read(mUnknown8Bytes.at(i));

    stream.read(mDimensions);
    stream.read(mMinimumSize);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkCapsuleShape::bhkCapsuleShape(NiStream& stream, const NiModel& model)
    : bhkSphereRepShape(stream, model)
{
    mUnknown8Bytes.resize(8);
    for (unsigned int i = 0; i < 8; ++i)
        stream.read(mUnknown8Bytes.at(i));

    stream.read(mFirstPoint);
    stream.read(mRadius1);
    stream.read(mSecondPoint);
    stream.read(mRadius2);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkConvexVerticesShape::bhkConvexVerticesShape(NiStream& stream, const NiModel& model)
    : bhkSphereRepShape(stream, model)
{
    mUnknown6Floats.resize(6);
    for (unsigned int i = 0; i < 6; ++i)
        stream.read(mUnknown6Floats.at(i));

    std::uint32_t numVertices;
    stream.read(numVertices);
    mVertices.resize(numVertices);
    for (unsigned int i = 0; i < numVertices; ++i)
        stream.read(mVertices.at(i));

    std::uint32_t numNormals;
    stream.read(numNormals);
    mNormals.resize(numNormals);
    for (unsigned int i = 0; i < numNormals; ++i)
        stream.read(mNormals.at(i));
}

#if 0
// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkSphereShape::bhkSphereShape(NiStream& stream, const NiModel& model)
    : bhkSphereRepShape(stream, model)
{
    stream.read(mMaterial);
    stream.read(mRadius);
}
#endif

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkMultiSphereShape::bhkMultiSphereShape(NiStream& stream, const NiModel& model)
    : bhkSphereRepShape(stream, model)
{
    stream.read(mUnknownFloat1);
    stream.read(mUnknownFloat2);

    std::uint32_t numSpheres;
    stream.read(numSpheres);
    mSpheres.resize(numSpheres);
    for (unsigned int i = 0; i < numSpheres; ++i)
    {
        stream.read(mSpheres[i].center);
        stream.read(mSpheres[i].radius);
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkTransformShape::bhkTransformShape(NiStream& stream, const NiModel& model)
    : bhkShape(stream, model)
{
    stream.read(mShapeIndex);
    stream.read(mMaterial);
    stream.read(mUnknownFloat1);
    mUnknown8Bytes.resize(8);
    for (int i = 0; i < 8; ++i)
        stream.read(mUnknown8Bytes.at(i));

    float floats[16];
    for (int i = 0; i < 16; ++i)
        stream.read(floats[i]);
    mTransform = Ogre::Matrix4(floats[0], floats[4], floats[8],  floats[12],
                               floats[1], floats[5], floats[9],  floats[13],
                               floats[2], floats[6], floats[10], floats[14],
                               floats[3], floats[7], floats[11], floats[15]);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5 ????
NiBtOgre::bhkEntity::bhkEntity(NiStream& stream, const NiModel& model)
    : bhkSerializable(stream, model)
{
    stream.read(mShapeIndex);
    stream.read(mLayer);
    stream.read(mColFilter);
    stream.read(mUnknownShort);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkRigidBody::bhkRigidBody(NiStream& stream, const NiModel& model)
    : bhkEntity(stream, model)
{
    stream.read(mUnknownInt1);
    stream.read(mUnknownInt2);

    mUnknown3Ints.resize(3);
    for (size_t i = 0; i < 3; ++i)
        stream.read(mUnknown3Ints.at(i));

    stream.read(mCollisionResponse);
    stream.read(mUnknownByte);
    stream.read(mProcessContactCallbackDelay);

    mUnknown2Shorts.resize(2);
    stream.read(mUnknown2Shorts.at(0));
    stream.read(mUnknown2Shorts.at(1));

    stream.read(mLayerCopy);
    stream.read(mColFilterCopy);

    mUnknown7Shorts.resize(7);
    for (size_t i = 0; i < 7; ++i)
        stream.read(mUnknown7Shorts.at(i));

    stream.read(mTranslation);
    stream.readQuaternionXYZW(mRotation);
    stream.read(mLinearVelocity);
    stream.read(mAngularVelocity);

    float value = 0;
    for (size_t i = 0; i < 3; ++i)
    {
        for (size_t j = 0; j < 4; ++j)
        {
            stream.read(value);
            mInertia[i][j] = Ogre::Real(value);
        }
    }

    stream.read(mCenter);
    stream.read(mMass);
    stream.read(mLinearDamping);
    stream.read(mAngularDamping);

    if (stream.userVer() >= 12)
    {
        stream.read(mGravityFactor1);
        stream.read(mGravityFactor2);
    }

    stream.read(mFriction);

    if (stream.userVer() >= 12)
        stream.read(mRollingFrictionMultiplier);

    stream.read(mRestitution);
    stream.read(mMaxLinearVelocity);
    stream.read(mMaxAngularVelocity);
    stream.read(mPenetrationDepth);

    stream.read(mMotionSystem);
    stream.read(mDeactivatorType);
    stream.read(mSolverDeactivation);
    stream.read(mQualityType);

    stream.read(mUnknownInt6);
    stream.read(mUnknownInt7);
    stream.read(mUnknownInt8);
    if (stream.userVer() >= 12)
        stream.read(mUnknownInt81);

    std::uint32_t numConstraints;
    stream.read(numConstraints);
    mConstraints.resize(numConstraints);
    for (size_t i = 0; i < numConstraints; ++i)
        stream.read(mConstraints.at(i));

    if (stream.userVer() <= 11)
        stream.read(mUnknownInt9);
    else if (stream.userVer() >= 12)
        stream.read(mUnknownInt91);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkSimpleShapePhantom::bhkSimpleShapePhantom(NiStream& stream, const NiModel& model)
    : bhkSerializable(stream, model)
{
    stream.read(mShapeIndex);
    stream.read(mLayer);
    stream.read(mColFilter);
    stream.read(mUnknownShort);

    stream.skip(sizeof(float)*23); // 7 + 3*5 +1
}
