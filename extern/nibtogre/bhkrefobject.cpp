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
#include "bhkrefobject.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

#include <OgreSceneNode.h>

#include <BulletCollision/CollisionShapes/btCollisionShape.h>
#include <BulletCollision/CollisionShapes/btBoxShape.h>
#include <BulletCollision/CollisionShapes/btSphereShape.h>
#include <BulletCollision/CollisionShapes/btCapsuleShape.h>
#include <BulletCollision/CollisionShapes/btMultiSphereShape.h>
#include <BulletCollision/CollisionShapes/btConvexHullShape.h>
#include <BulletCollision/CollisionShapes/btBvhTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btConvexTriangleMeshShape.h>
#include <BulletCollision/CollisionShapes/btCompoundShape.h>

#include "nistream.hpp"
#include "niavobject.hpp" // static_cast NiAVObject
#include "nimodel.hpp"
#include "btogreinst.hpp"
#include "nidata.hpp"

#if 0 // Commented out, instead use: typedef bhkRefObject bhkSerializable
NiBtOgre::bhkSerializable::bhkSerializable(uint32_t index, NiStream& stream, const NiModel& model)
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
NiBtOgre::bhkCompressedMeshShapeData::bhkCompressedMeshShapeData(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkRefObject(index, stream, model)
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
NiBtOgre::bhkBallSocketConstraintChain::bhkBallSocketConstraintChain(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkSerializable(index, stream, model)
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

    std::int32_t rIndex = -1;
    std::uint32_t numLinks;
    stream.read(numLinks);
    mLinks.resize(numLinks);
    for (unsigned int i = 0; i < numLinks; ++i)
    {
        //stream.getPtr<NiObject>(mLinks.at(i), model.objects());
        rIndex = -1;
        stream.read(rIndex);
        mLinks[i] = model.getRef<NiObject>(rIndex);
    }

    stream.read(numLinks); // note: numLinks reused
    mLinks2.resize(numLinks);
    for (unsigned int i = 0; i < numLinks; ++i)
    {
        //stream.getPtr<NiObject>(mLinks2.at(i), model.objects());
        rIndex = -1; // note: index reused
        stream.read(rIndex);
        mLinks2[i] = model.getRef<NiObject>(rIndex);
    }

    stream.read(mUnknownInt3);
}

NiBtOgre::bhkConstraint::bhkConstraint(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkSerializable(index, stream, model)
{
    std::int32_t rIndex = -1;
    std::uint32_t numEntities;
    stream.read(numEntities);
    mEntities.resize(numEntities);
    for (unsigned int i = 0; i < numEntities; ++i)
    {
        //stream.getPtr<bhkEntity>(mEntities.at(i), model.objects());
        rIndex = -1;
        stream.read(rIndex);
        mEntities[i] = model.getRef<bhkEntity>(rIndex);
    }

    stream.read(mPriority);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::bhkBreakableConstraint::bhkBreakableConstraint(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkConstraint(index, stream, model)
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

        std::int32_t rIndex = -1;
        std::uint32_t numEntities2;
        stream.read(numEntities2);
        mEntities2.resize(numEntities2);
        for (unsigned int i = 0; i < numEntities2; ++i)
        {
            //stream.getPtr<bhkEntity>(mEntities2.at(i), model.objects());
            index = -1;
            stream.read(rIndex);
            mEntities2[i] = model.getRef<bhkEntity>(rIndex);
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
NiBtOgre::bhkHingeConstraint::bhkHingeConstraint(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkConstraint(index, stream, model)
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
NiBtOgre::bhkLimitedHingeConstraint::bhkLimitedHingeConstraint(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkConstraint(index, stream, model)
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

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkMalleableConstraint::bhkMalleableConstraint(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkConstraint(index, stream, model)
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

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkPrismaticConstraint::bhkPrismaticConstraint(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkConstraint(index, stream, model)
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

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkRagdollConstraint::bhkRagdollConstraint(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkConstraint(index, stream, model)
{
    mRagdoll.read(stream);
}

NiBtOgre::bhkShape::bhkShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkSerializable(index, stream, model)
{
}

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkStiffSpringConstraint::bhkStiffSpringConstraint(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkConstraint(index, stream, model)
{
    stream.read(mPivotA);
    stream.read(mPivotB);
    stream.read(mLength);
}

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkMoppBvTreeShape::bhkMoppBvTreeShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkShape(index, stream, model)
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

std::unique_ptr<btCollisionShape> NiBtOgre::bhkMoppBvTreeShape::buildShape(const btTransform& transform) const
{
    // FIXME: TODO get some info before moving to the next shape in a link

    if (mShapeIndex == -1)
        return std::unique_ptr<btCollisionShape>(nullptr);

    return mModel.getRef<bhkShape>(mShapeIndex)->buildShape(transform);
}

// seen in nif version 20.2.0.7
NiBtOgre::bhkCompressedMeshShape::bhkCompressedMeshShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkShape(index, stream, model)
{
    //stream.getPtr<NiAVObject>(mTarget, model.objects());
    std::int32_t rIndex = -1;
    stream.read(rIndex);
    mTarget = model.getRef<NiAVObject>(rIndex);

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

std::unique_ptr<btCollisionShape> NiBtOgre::bhkCompressedMeshShape::buildShape(const btTransform& transform) const
{
    return std::unique_ptr<btCollisionShape>(nullptr); // FIXME: TODO needed for TES5 only
}

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkListShape::bhkListShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkShape(index, stream, model)
{
    stream.readVector<bhkShapeRef>(mSubShapes);
    stream.read(mMaterial);

    //munknownfloats.resize(6);
    //for (int i = 0; i < 6; ++i)
    //    stream.read(mUnknownFloats.at(i));
    stream.skip(sizeof(float)*6);

    stream.readVector<std::uint32_t>(mUnknownInts);
}

// Note that a bhkListShape can have bhkTransformShape children.  This means a btCompoundShape
// parent having btCompoundShape children. e.g. meshes/architecture/arena/chorrolarenainteriorground01.nif
std::unique_ptr<btCollisionShape> NiBtOgre::bhkListShape::buildShape(const btTransform& transform) const
{
    std::unique_ptr<btCollisionShape> collisionShape = std::unique_ptr<btCompoundShape>(new btCompoundShape());

    for (unsigned int i = 0; i < mSubShapes.size(); ++i)
    {
        if (mSubShapes[i] == -1)
            continue; // nothing to build

        bhkShape *subShape = mModel.getRef<bhkShape>(mSubShapes[i]);

// Implemention Option 1: Simple
#if 0
        std::unique_ptr<btCollisionShape> subCollisionShape = subShape->buildShape();
        assert(subCollisionShape.get() != nullptr && "bhkListShape: child buildShape failed");

        if (subCollisionShape->getName() == "Compound") // is there a better way than text comparison?
        {
            if (static_cast<btCompoundShape*>(subCollisionShape.get())->getNumChildShapes() > 1)
            {
                // FIXME: should throw here
                //throw std::runtime_error ("bhkListShape: unexpected btCompoundShape");
                std::cerr << "buildShape: child in ListShape is a List " << mModel.blockType(mSelfIndex) << std::endl;
                continue;
            }
        }

        static_cast<btCompoundShape*>(collisionShape.get())->addChildShape(btTransform::getIdentity(),
                                                                           subCollisionShape.get());
#else // Implemention Option 2: Complex

        // coc "FortRoebeck02" - HandScythe01 - bhkTransformShape/bhkBoxShape
        // coc "RockmilkCave03"
        // - meshes/clutter/lowerclass/ClothBolt04.nif
        //      bhkCapsuleShape, bhkConvexTransformShape/bhkBoxShape
        // - meshes/clutter/lowerclass/lowerthatchbasket02.nif
        //      bhkConvexVerticesShape, bhkConvexTransformShape/bhkBoxShape
        std::string subShapeType = mModel.blockType(mSubShapes[i]);

        if (subShapeType == "bhkTransformShape" || subShapeType == "bhkConvexTransformShape")
        {
            bhkTransformShape *shape = static_cast<bhkTransformShape*>(subShape);

            if (mModel.blockType(static_cast<bhkTransformShape*>(subShape)->shapeIndex()) == "bhkBoxShape")
            {
                std::unique_ptr<btCollisionShape> subCollisionShape =
                    mModel.getRef<bhkBoxShape>(shape->shapeIndex())->buildBoxShape();
                static_cast<btCompoundShape*>(collisionShape.get())->addChildShape(
                        transform * shape->transform(), subCollisionShape.get());
            }
            else if (mModel.blockType(static_cast<bhkTransformShape*>(subShape)->shapeIndex()) == "bhkCapsuleShape")
            {
                bhkCapsuleShape *capsuleShape
                    = static_cast<bhkCapsuleShape*>(mModel.getRef<bhkCapsuleShape>(shape->shapeIndex()));

                std::unique_ptr<btCollisionShape> subCollisionShape = capsuleShape->buildCapsuleShape();
                static_cast<btCompoundShape*>(collisionShape.get())->addChildShape(
                        transform * shape->transform() * capsuleShape->transform(), subCollisionShape.get());
            }
        }
        else if (subShapeType == "bhkCapsuleShape")
        {
            bhkCapsuleShape *shape = static_cast<bhkCapsuleShape*>(subShape);

            std::unique_ptr<btCapsuleShape> subCollisionShape = shape->buildCapsuleShape();
            static_cast<btCompoundShape*>(collisionShape.get())->addChildShape(transform * shape->transform(),
                                                                               subCollisionShape.get());
        }
        else
        {
            std::unique_ptr<btCollisionShape> subCollisionShape = subShape->buildShape(transform);
            // apply transform if buildShape has not applied it
            static_cast<btCompoundShape*>(collisionShape.get())->addChildShape(
                    (subCollisionShape->getUserIndex() != 0) ? transform : btTransform::getIdentity(),
                    subCollisionShape.get());
        }
#endif

    }

    return collisionShape;
}

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkNiTriStripsShape::bhkNiTriStripsShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkShape(index, stream, model)
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

// e.g. architecture/arena/arenacolumn02.nif
//      clutter/books/wantedposter01.nif
//      clutter/books/wantedposter02.nif
std::unique_ptr<btCollisionShape> NiBtOgre::bhkNiTriStripsShape::buildShape(const btTransform& transform) const
{
    btTriangleMesh *mesh = new btTriangleMesh();

    for (unsigned int i = 0; i < mStripsData.size(); ++i)
    {
        if (mStripsData[i] == -1)
            continue; // nothing to build

        NiTriStripsData *triStripsData = mModel.getRef<NiTriStripsData>(mStripsData[i]);
        assert(triStripsData != nullptr && "mModel.getRef returned nullptr"); // FIXME: throw instead?



        // From BulletCollision/CollisionShapes/btStridingMeshInterface.cpp:
        //
        //   The btStridingMeshInterface is the interface class for high performance generic
        //   access to triangle meshes, used in combination with btBvhTriangleMeshShape and
        //   some other collision shapes.
        //
        //   Using index striding of 3*sizeof(integer) it can use triangle arrays, using index
        //   striding of 1*sizeof(integer) it can handle triangle strips.
        //
        // From BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h:
        //
        //   The btTriangleIndexVertexArray allows to access multiple triangle meshes, by
        //   indexing into existing triangle/index arrays.
        //
        //   Additional meshes can be added using addIndexedMesh /No duplcate is made of the
        //   vertex/index data, it only indexes into external vertex/index arrays.  So keep
        //   those arrays around during the lifetime of this btTriangleIndexVertexArray.
        //
        // Notes:
        //
        // It seems Bullet only keeps pointers?  Which means vertices and indicies must be kept
        // around.
        //
        // How does transforms affect this, especially for OL_CLUTTER which is probably havok
        // enabled?
        //
        // However, if using btTriangleMesh m_4componentVertices or m_3componentVertices will
        // keep the vertices and hence no need to keep the arrays around?




        // TODO: possibly inefficient by creating too many triangles?
        const std::vector<Ogre::Vector3> &vertices = triStripsData->mVertices;
        const std::vector<uint16_t> &triangles = triStripsData->mTriangles;

        for(size_t j = 0; j < triStripsData->mTriangles.size(); j += 3)
        {
            Ogre::Vector3 b1 = vertices[triangles[j+0]];
            Ogre::Vector3 b2 = vertices[triangles[j+1]];
            Ogre::Vector3 b3 = vertices[triangles[j+2]];

            mesh->addTriangle(transform * btVector3(b1.x,b1.y,b1.z),
                              transform * btVector3(b2.x,b2.y,b2.z),
                              transform * btVector3(b3.x,b3.y,b3.z));
        }
    }

    std::unique_ptr<btCollisionShape> collisionShape
        = std::unique_ptr<btBvhTriangleMeshShape>(new btBvhTriangleMeshShape(mesh, true));

    collisionShape->setUserIndex(0); // indicate that transform was applied
    return collisionShape;
}

// seen in nif ver 20.0.0.4, 20.0.0.5
void NiBtOgre::OblivionSubShape::read(NiStream& stream)
{
    stream.read(layer);
    stream.read(colFilter);
    stream.read(unknownShort);
    stream.read(numVertices);
    stream.read(material);
}

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkPackedNiTriStripsShape::bhkPackedNiTriStripsShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkShape(index, stream, model)
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

// Looks like these are static hence ok to use btBvhTriangleMeshShape
//
// Notes from Bullet:
//
//   The btBvhTriangleMeshShape is a static-triangle mesh shape, it can only be used for
//   fixed/non-moving objects.
//
//   If you required moving concave triangle meshes, it is recommended to perform convex
//   decomposition using HACD, see Bullet/Demos/ConvexDecompositionDemo.
//
// e.g. architecture/imperialcity/basementset/icbasementcorner01.nif
//      architecture/imperialcity/basementset/icbasement3way.nif
//      architecture/imperialcity/iccolarc01.nif
// NOTE: ICColArc01.NIF doesn't have any collision defined for the roof parts
std::unique_ptr<btCollisionShape> NiBtOgre::bhkPackedNiTriStripsShape::buildShape(const btTransform& transform) const
{
    if (mDataIndex == -1)
        return std::unique_ptr<btCollisionShape>(nullptr); // nothing to build

    btTriangleMesh *mesh = new btTriangleMesh();
    const hkPackedNiTriStripsData* triData = mModel.getRef<hkPackedNiTriStripsData>(mDataIndex);
    assert(triData != nullptr && "mModel.getRef returned nullptr"); // FIXME: throw instead?

    // FIXME: preallocate

    for(size_t i = 0; i < triData->mTriangles.size(); ++i)
    {
        // NOTE: havok scale
        Ogre::Vector3 b1 = triData->mVertices[triData->mTriangles[i].triangle[0]]*7;
        Ogre::Vector3 b2 = triData->mVertices[triData->mTriangles[i].triangle[1]]*7;
        Ogre::Vector3 b3 = triData->mVertices[triData->mTriangles[i].triangle[2]]*7;

        mesh->addTriangle(transform * btVector3(b1.x,b1.y,b1.z),
                          transform * btVector3(b2.x,b2.y,b2.z),
                          transform * btVector3(b3.x,b3.y,b3.z));
    }

    // TODO: TES5 has triData->mSubShapes here

    std::unique_ptr<btCollisionShape> collisionShape
        = std::unique_ptr<btBvhTriangleMeshShape>(new btBvhTriangleMeshShape(mesh, true));

    collisionShape->setUserIndex(0); // indicate that transform was applied
    return collisionShape;
}

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::hkPackedNiTriStripsData::hkPackedNiTriStripsData(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkShape(index, stream, model)
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
    // FIXME: btVector3
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

std::unique_ptr<btCollisionShape> NiBtOgre::hkPackedNiTriStripsData::buildShape(const btTransform& transform) const
{
    throw std::runtime_error ("hkPackedNiTriStripsData: unexpected call to buildShape");
}

NiBtOgre::bhkSphereRepShape::bhkSphereRepShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkShape(index, stream, model)
{
    stream.read(mMaterial);
    stream.read(mRadius);
}

std::unique_ptr<btCollisionShape> NiBtOgre::bhkSphereRepShape::buildShape(const btTransform& transform) const
{
    std::unique_ptr<btCollisionShape> shape
        = std::unique_ptr<btSphereShape>(new btSphereShape(mRadius*7)); // NOTE: havok scale
    shape->setUserIndex(1); // didn't apply transform
    return shape;
}

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkBoxShape::bhkBoxShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkSphereRepShape(index, stream, model)
{
    mUnknown8Bytes.resize(8);
    for (int i = 0; i < 8; ++i)
        stream.read(mUnknown8Bytes.at(i));

    stream.read(mDimensions);
    stream.read(mMinimumSize);
}

// same as buildBoxShape, but sets m_userIndex to 1 to indicate that transform was not applied
std::unique_ptr<btCollisionShape> NiBtOgre::bhkBoxShape::buildShape(const btTransform& transform) const
{
    std::unique_ptr<btCollisionShape> shape
        = std::unique_ptr<btBoxShape>(new btBoxShape(mDimensions*7)); // NOTE: havok scale
    shape->setUserIndex(1); // didn't apply transform
    return shape;
}

std::unique_ptr<btBoxShape> NiBtOgre::bhkBoxShape::buildBoxShape() const
{
    // FIXME: check mMinimumSize first? Can it be zero?

    // old code replaced by Bullet data types
    //btVector3 dimensions = btVector3(mDimensions.x, mDimensions.y, mDimensions.z);

    // Examples with bhkRigidBodyT:
    //   clutter/books/wantedposter02static.nif
    //   plants/florasacredlotus01.nif
    //   clutter/middleclass/middlecrate02.nif
    return std::unique_ptr<btBoxShape>(new btBoxShape(mDimensions*7)); // NOTE: havok scale
}

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkCapsuleShape::bhkCapsuleShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkSphereRepShape(index, stream, model)
{
    mUnknown8Bytes.resize(8);
    for (unsigned int i = 0; i < 8; ++i)
        stream.read(mUnknown8Bytes.at(i));

    stream.read(mFirstPoint);
    stream.read(mRadius1);
    stream.read(mSecondPoint);
    stream.read(mRadius2);

    btVector3 firstPoint = mFirstPoint*7;   // NOTE: havok scale
    btVector3 secondPoint = mSecondPoint*7; // NOTE: havok scale

    btVector3 localTranslation = btScalar(0.5f) * (firstPoint + secondPoint); // midpoint

    btQuaternion localRotation;
    localRotation = btQuaternion::getIdentity();

    btVector3 diff = firstPoint - secondPoint;
    btScalar lenSqr = diff.length2();
    mHeight = 0.f;

    if (lenSqr > SIMD_EPSILON)
    {
        mHeight = btSqrt(lenSqr);
        btVector3 axis = diff / mHeight;

        btVector3 zAxis(0.f, 0.f, 1.f);
        localRotation = shortestArcQuat(zAxis, axis);
    }

    if (mRadius1 != mRadius2)
        std::cerr << "Capsule radius different." << std::endl; // FIXME: throw here?

    mTransform = btTransform(localRotation, localTranslation);
}

// examples of capsule shape:
// meshes\clutter\farm\yarn01.nif,    meshes\lights\candlefat01.nif,
// meshes\lights\candlefat02fake.nif, meshes\lights\candleskinny01fake.nif
//
// TODO: An alternative method is to wrap around using btCompoundShape
// See BulletMJCFImporter::convertLinkCollisionShapes
// in examples/Importers/ImportMJCFDemo/BulletMJCFImporter.cpp
std::unique_ptr<btCollisionShape> NiBtOgre::bhkCapsuleShape::buildShape(const btTransform& transform) const
{
// old and perhaps naive implementation
#if 0 //                                                                      {{{
    hasShapeTransform = true;
    Ogre::Matrix4 localShapeTransform(Ogre::Matrix4::IDENTITY);

    // the btcapsuleShape represents a capsule around the Y axis so it needs to be rotated.
    // make use of Ogre's convenient functions to do this.
    Ogre::Vector3 firstPoint = mFirstPoint*7;   // NOTE: havok scale
    Ogre::Vector3 secondPoint = mSecondPoint*7; // NOTE: havok scale
    Ogre::Vector3 axis = mSecondPoint - mFirstPoint;

    float radius = mRadius1; // FIXME: can mRadius2 be different to mRadius1 for a capsule?

    Ogre::Vector3 translation = firstPoint.midPoint(secondPoint);
    Ogre::Quaternion rotation = Ogre::Quaternion::IDENTITY;

    // FIXME: horrible hack - upright capsule shapes don't get the rotations correct for some reason
    if (firstPoint.x == secondPoint.x && firstPoint.y == secondPoint.y && firstPoint.z != secondPoint.z)
    {
        float height = std::abs(firstPoint.z - secondPoint.z); // NOTE: havok scale already factored in
        localShapeTransform.makeTransform(translation, Ogre::Vector3(1.f), rotation); // assume uniform scale
        shapeTransform = shapeTransform * localShapeTransform;
        return std::unique_ptr<btCapsuleShapeZ>(new btCapsuleShapeZ(radius*7, height)); // NOTE: havok scale
    }
    else
    {
        float height = firstPoint.distance(secondPoint);
        rotation = axis.getRotationTo(Ogre::Vector3::UNIT_Y); // should this be NEGATIVE_UNIT_Y?
        localShapeTransform.makeTransform(translation, Ogre::Vector3(1.f), rotation); // assume uniform scale
        shapeTransform = shapeTransform * localShapeTransform;
        return std::unique_ptr<btCapsuleShape>(new btCapsuleShape(radius*7, height)); // NOTE: havok scale
    }
#endif //                                                                      }}}

    // Based on examples/Importers/ImportMJCFDemo/BulletMJCFImporter.cpp, also see:
    // http://lolengine.net/blog/2013/09/18/beautiful-maths-quaternion-from-vectors
#if 0 //                                                                       {{{
    btVector3 firstPoint = mFirstPoint*7;   // NOTE: havok scale
    btVector3 secondPoint = mSecondPoint*7; // NOTE: havok scale

    btVector3 localTranslation = btScalar(0.5f) * (firstPoint + secondPoint); // midpoint

    btQuaternion localRotation;
    localRotation = btQuaternion::getIdentity();

    btVector3 diff = firstPoint - secondPoint;
    btScalar lenSqr = diff.length2();
    btScalar height = 0.f;

    if (lenSqr > SIMD_EPSILON)
    {
        height = btSqrt(lenSqr);
        btVector3 axis = diff / height;

        btVector3 zAxis(0.f, 0.f, 1.f);
        localRotation = shortestArcQuat(zAxis, axis);
    }

    if (mRadius1 != mRadius2)
        std::cerr << "Capsule radius different." << std::endl;

    btCapsuleShapeZ *shape = new btCapsuleShapeZ(mRadius1, btScalar(0.5f)*height);

    std::unique_ptr<btCollisionShape> collisionShape(new btCompoundShape());
    btTransform localTransform(localRotation, localTranslation);
    static_cast<btCompoundShape*>(collisionShape.get())->addChildShape(localTransform, shape);
    return collisionShape;
#endif //                                                                       }}}

    btCapsuleShapeZ *shape = new btCapsuleShapeZ(mRadius1, btScalar(0.5f)*mHeight);

    std::unique_ptr<btCollisionShape> collisionShape(new btCompoundShape());
    static_cast<btCompoundShape*>(collisionShape.get())->addChildShape(transform * mTransform, shape);
    collisionShape->setUserIndex(0); // indicate that transform was applied
    return collisionShape;
}

std::unique_ptr<btCapsuleShape> NiBtOgre::bhkCapsuleShape::buildCapsuleShape() const
{
    return std::unique_ptr<btCapsuleShapeZ>(new btCapsuleShapeZ(mRadius1, btScalar(0.5f)*mHeight));
}

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkConvexVerticesShape::bhkConvexVerticesShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkSphereRepShape(index, stream, model)
{
    mUnknown6Floats.resize(6);
    for (unsigned int i = 0; i < 6; ++i)
        stream.read(mUnknown6Floats.at(i));

    stream.read(mNumVertices);

#if 1 // old implementation
    mVertices.resize(mNumVertices);
    for (unsigned int i = 0; i < mNumVertices; ++i)
    {
        stream.read(mVertices.at(i).m_floats[0]);
        stream.read(mVertices.at(i).m_floats[1]);
        stream.read(mVertices.at(i).m_floats[2]);
        stream.read(mVertices.at(i).m_floats[3]);
    }
#else
    mVertices = std::unique_ptr<float[]>(new float[mNumVertices*4]);
    for (unsigned int i = 0; i < mNumVertices*4; ++i)
    {
        stream.read(mVertices[i]);
        mVertices[i] *= 7; // NOTE: havok scale
    }
#endif
    // if numVertices > 100 try below
    // http://www.bulletphysics.org/mediawiki-1.5.8/index.php/BtShapeHull_vertex_reduction_utility

    // TODO: skip instead? not using these
    std::uint32_t numNormals;
    stream.read(numNormals);
    mNormals.resize(numNormals);
    for (unsigned int i = 0; i < numNormals; ++i)
        stream.read(mNormals.at(i));
}

// e.g. with bhkRigidBody  architecture/imperialcity/icdoor04.nif
// e.g. with bhkRigidBodyT architecture/imperialcity/icsigncopious01.nif
std::unique_ptr<btCollisionShape> NiBtOgre::bhkConvexVerticesShape::buildShape(const btTransform& transform) const
{
#if 1 // old implementation
    std::unique_ptr<btCollisionShape> convexHull = std::unique_ptr<btConvexHullShape>(new btConvexHullShape());
    btConvexHullShape *shape = static_cast<btConvexHullShape*>(convexHull.get());

    for (unsigned int i = 0; i < mVertices.size(); ++i)
        shape->addPoint(transform * (mVertices[i]*7), false); // NOTE: havok scale
    shape->recalcLocalAabb();

    return convexHull;
#else
    // use different ctor for minor optimisation
    //return std::unique_ptr<btConvexHullShape>(new btConvexHullShape(mVertices.get(), mNumVertices, sizeof(float)*4));
    vertices = std::unique_ptr<float[]>(new float[mNumVertices*4]);
    for (unsigned int i = 0; i < mNumVertices; ++i)
    {
        btVector point = transform * btVector(mVertices[i*4], mVertices[i*4+1],mVertices[i*4+2]);

        vertices[i*4]   = point.getX();
        vertices[i*4+1] = point.getY();
        vertices[i*4+2] = point.getZ();
    }
    std::unique_ptr<btCollisionShape> collisionShape(
        new btConvexHullShape(vertices.get(), mNumVertices, sizeof(float)*4));
    collisionShape->setUserIndex(0); // indicate that transform was applied
    return collisionShape;
#endif
}

#if 0 // Commented out, instead use: typedef bhkSphereRepShape bhkSphereShape
// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhksphereShape::bhkSphereShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhksphererepShape(index, stream, model)
{
    stream.read(mMaterial);
    stream.read(mRadius);
}
#endif

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkMultiSphereShape::bhkMultiSphereShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkSphereRepShape(index, stream, model)
{
    stream.read(mUnknownFloat1);
    stream.read(mUnknownFloat2);

    stream.read(mNumSpheres);

// old code replaced by Bullet data types
#if 0 //                                                                       {{{
    mspheres.resize(mNumSpheres);
    for (unsigned int i = 0; i < mNumSpheres; ++i)
    {
        stream.read(mSpheres[i].center);
        stream.read(mSpheres[i].radius);
    }
#endif //                                                                       }}}

    mCenters = std::unique_ptr<btVector3[]>(new btVector3[mNumSpheres]);
    mRadii = std::unique_ptr<btScalar[]>(new btScalar[mNumSpheres]);

    for (unsigned int i = 0; i < mNumSpheres; ++i)
    {
        stream.read(mCenters[i]);
        stream.read(mRadii[i]);
        mCenters[i] *= 7; // NOTE: havok scale
        mRadii[i]   *= 7; // NOTE: havok scale
    }
}

std::unique_ptr<btCollisionShape> NiBtOgre::bhkMultiSphereShape::buildShape(const btTransform& transform) const
{
    std::unique_ptr<btCollisionShape> shape
        = std::unique_ptr<btMultiSphereShape>(new btMultiSphereShape(mCenters.get(), mRadii.get(), (int)mNumSpheres));
    shape->setUserIndex(1); // didn't apply transform
    return shape;
}

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkTransformShape::bhkTransformShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkShape(index, stream, model)
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
// FIXME: replace with bullet after testing
#if 0
    mTransformOld = Ogre::Matrix4(floats[0], floats[4], floats[8],  floats[12],
                                  floats[1], floats[5], floats[9],  floats[13],
                                  floats[2], floats[6], floats[10], floats[14],
                                  floats[3], floats[7], floats[11], floats[15]);
#endif
    mTransform.setFromOpenGLMatrix(floats);
}

// Looks like most (all?) of the bhkTransformShapes have bhkBoxShape or bhkCapsuleShape
//   bhkCapsuleShape: architecture/daedricstatues/daedricshrinehircine01.nif
//   cow "tamriel" -2 -5
std::unique_ptr<btCollisionShape> NiBtOgre::bhkTransformShape::buildShape(const btTransform& transform) const
{
    if (mShapeIndex == -1)
        return std::unique_ptr<btCollisionShape>(nullptr);

// old implementation
#if 0 //                                                                       {{{
    hasShapeTransform = true;

    // make this object's transform
    Ogre::Matrix4 localTransform(Ogre::Matrix4::IDENTITY);
    Ogre::Vector3 localTranslation = mTransform.getTrans();
    Ogre::Quaternion localRotation = mTransform.extractQuaternion();
    // NOTE: havok scale of 7
    localTransform.makeTransform(localTranslation*7, Ogre::Vector3(1.f), localRotation); // assume uniform scale

    Ogre::Matrix4 localShapeTransform(Ogre::Matrix4::IDENTITY); // NOTE: buildShape may modify
    bool hasLocalShapeTransform = false;                        // NOTE: buildShape may set to true

    std::unique_ptr<btCollisionShape> res = mModel.getRef<bhkShape>(mShapeIndex)->buildShape();

    if (hasLocalShapeTransform)
        localTransform = localTransform * localShapeTransform;

    shapeTransform = localTransform; // update the caller
    return res;
#endif //                                                                       }}}

// FIXME: testing only
#if 0 //                                                                       {{{
    // !string("Object ID" "00025100")
    // coc "SENSMazaddhasHouse"
    Ogre::Vector3 vOld = mTransformOld.getTrans();
    Ogre::Quaternion qOld = mTransformOld.extractQuaternion();
    btVector3 v = mTransform.getOrigin();
    btQuaternion q = mTransform.getRotation();

    if (vOld.x != v.x() || vOld.y != v.y() || vOld.z != v.z())
    {
        std::cout << "bhkTransformShape: bad vector" << std::endl;
    }

    if (qOld.x != q.x() || qOld.y != q.y() || qOld.z != q.z() || qOld.w != q.w())
    {
        std::cout << "bhkTransformShape: bad quaternion" << std::endl;
    }
#endif //                                                                       }}}
// FIXME: end of testing

// more testing
//#if 0
    std::string shapeType = mModel.blockType(mShapeIndex);
    if (shapeType == "bhkListShape" || shapeType == "bhkTransformShape" || shapeType == "bhkConvexTransformShape")
    {
        throw std::runtime_error ("bhkTransformShape: unexpected btCompoundShape");
    }
//#endif

    std::unique_ptr<btCollisionShape> collisionShape(new btCompoundShape());

    if (shapeType == "bhkCapsuleShape")
    {
        bhkCapsuleShape *capsuleShape = static_cast<bhkCapsuleShape*>(mModel.getRef<bhkCapsuleShape>(mShapeIndex));

        std::unique_ptr<btCapsuleShape> shape = capsuleShape->buildCapsuleShape();
        static_cast<btCompoundShape*>(collisionShape.get())->addChildShape(
                transform * mTransform * capsuleShape->transform(), shape.get());
    }
    else // bhkBoxShape
    { // FIXME: assert it is a bhkBoxShape
        std::unique_ptr<btCollisionShape> shape = mModel.getRef<bhkBoxShape>(mShapeIndex)->buildBoxShape();
        assert(shape.get() != nullptr && "bhkTransformShape: buildShape failed");

        static_cast<btCompoundShape*>(collisionShape.get())->addChildShape(transform * mTransform, shape.get());
    }
    return collisionShape;
}

// seen in nif ver 20.0.0.4, 20.0.0.5 ????
NiBtOgre::bhkEntity::bhkEntity(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkSerializable(index, stream, model)
{
    stream.read(mShapeIndex);
    stream.read(mLayer);
    stream.read(mColFilter);
    stream.read(mUnknownShort);
}

// oblivionlayer                                                                {{{
// sets mesh color in Oblivion Construction Set. Anything higher than 57 is also null.
// Number | Name               | Description
// -------+--------------------+---------------------------------------------------
//      0 | OL_UNIDENTIFIED    | Unidentified (white)
//      1 | OL_STATIC          | Static (red)
//      2 | OL_ANIM_STATIC     | AnimStatic (magenta)
//      3 | OL_TRANSPARENT     | Transparent (light pink)
//      4 | OL_CLUTTER         | Clutter (light blue)
//      5 | OL_WEAPON          | Weapon (orange)
//      6 | OL_PROJECTILE      | Projectile (light orange)
//      7 | OL_SPELL           | Spell (cyan)
//      8 | OL_BIPED           | Biped (green) Seems to apply to all creatures/NPCs
//      9 | OL_TREES           | Trees (light brown)
//     10 | OL_PROPS           | Props (magenta)
//     11 | OL_WATER           | Water (cyan)
//     12 | OL_TRIGGER         | Trigger (light grey)
//     13 | OL_TERRAIN         | Terrain (light yellow)
//     14 | OL_TRAP            | Trap (light grey)
//     15 | OL_NONCOLLIDABLE   | NonCollidable (white)
//     16 | OL_CLOUD_TRAP      | CloudTrap (greenish grey)
//     17 | OL_GROUND          | Ground (none)
//     18 | OL_PORTAL          | Portal (green)
//     19 | OL_STAIRS          | Stairs (white)
//     20 | OL_CHAR_CONTROLLER | CharController (yellow)
//     21 | OL_AVOID_BOX       | AvoidBox (dark yellow)
//     22 | OL_UNKNOWN1        | ? (white)
//     23 | OL_UNKNOWN2        | ? (white)
//     24 | OL_CAMERA_PICK     | CameraPick (white)
//     25 | OL_ITEM_PICK       | ItemPick (white)
//     26 | OL_LINE_OF_SIGHT   | LineOfSight (white)
//     27 | OL_PATH_PICK       | PathPick (white)
//     28 | OL_CUSTOM_PICK_1   | CustomPick1 (white)
//     29 | OL_CUSTOM_PICK_2   | CustomPick2 (white)
//     30 | OL_SPELL_EXPLOSION | SpellExplosion (white)
//     31 | OL_DROPPING_PICK   | DroppingPick (white)
//     32 | OL_OTHER           | Other (white)
//     33 | OL_HEAD            | Head
//     34 | OL_BODY            | Body
//     35 | OL_SPINE1          | Spine1
//     36 | OL_SPINE2          | Spine2
//     37 | OL_L_UPPER_ARM     | LUpperArm
//     38 | OL_L_FOREARM       | LForeArm
//     39 | OL_L_HAND          | LHand
//     40 | OL_L_THIGH         | LThigh
//     41 | OL_L_CALF          | LCalf
//     42 | OL_L_FOOT          | LFoot
//     43 | OL_R_UPPER_ARM     | RUpperArm
//     44 | OL_R_FOREARM       | RForeArm
//     45 | OL_R_HAND          | RHand
//     46 | OL_R_THIGH         | RThigh
//     47 | OL_R_CALF          | RCalf
//     48 | OL_R_FOOT          | RFoot
//     49 | OL_TAIL            | Tail
//     50 | OL_SIDE_WEAPON     | SideWeapon
//     51 | OL_SHIELD          | Shield
//     52 | OL_QUIVER          | Quiver
//     53 | OL_BACK_WEAPON     | BackWeapon
//     54 | OL_BACK_WEAPON2    | BackWeapon (?)
//     55 | OL_PONYTAIL        | PonyTail
//     56 | OL_WING            | Wing
//     57 | OL_NULL            | Null                                           }}}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkRigidBody::bhkRigidBody(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkEntity(index, stream, model)
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

    stream.read(mTranslation); // FIXME: btVector3
    stream.readQuaternionXYZW(mRotation); // FIXME: btQuaternion
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

    if (mModel.blockType(mSelfIndex) == "bhkRigidBodyT")
    {
        // NOTE: havok scale applied to mTranslation
        mBodyTransform = btTransform(btQuaternion(mRotation.x, mRotation.y, mRotation.z, mRotation.w),
                                     btVector3(mTranslation.x*7, mTranslation.y*7, mTranslation.z*7));
    }
    else
        mBodyTransform.setIdentity();
}

// WARNING: the parameter 'parent' is a NiNode (i.e. not bhkNiCollisionObject)
// 1. calculate the world transform of the NIF
// 2. adjust with the transform of the SceneNode
// 3. create the bkRidgidBody and store it in 'inst', keyed with the index of the parent NiNode
//    so that the associated Ogre::Entity can be used for ragdoll animation
// 4. create any associated constraints
// 5. if OL_STATIC register with Bullet for collisions
// FIXME: only do some of the steps if actual ragdoll?
//
// FIXME: some of these should allow raycasting for object identification?
void NiBtOgre::bhkRigidBody::build(BtOgreInst *inst, NiObject* parentNiNode)
//void NiBtOgre::bhkRigidBody::buildEntity(BtOgreInst *inst, NiAVObject* parentNiNode)
{
    if (mShapeIndex == -1) // nothing to build
        return;

    Ogre::Vector3 pos;
    Ogre::Vector3 scale;
    Ogre::Quaternion rot;
    static_cast<NiAVObject*>(parentNiNode)->getWorldTransform().decomposition(pos, scale, rot);

    btTransform transform;

    // mSelfIndex refers to either a bhkRigidBody or bhkRigidBodyT
    // apply rotation and translation only if the collision object's body is a bhkRigidBodyT type
//#if 0
    if (mModel.blockType(mSelfIndex) == "bhkRigidBodyT")
    {
        // NOTE: havok scale applied to mTranslation
        btTransform bodyTransform(btQuaternion(mRotation.x, mRotation.y, mRotation.z, mRotation.w),
                                  btVector3(mTranslation.x*7, mTranslation.y*7, mTranslation.z*7));
        transform = bodyTransform * btTransform(btQuaternion(rot.x, rot.y, rot.z, rot.w),
                                                btVector3(pos.x, pos.y, pos.z));
    }
    else
        transform = btTransform(btQuaternion(rot.x, rot.y, rot.z, rot.w), btVector3(pos.x, pos.y, pos.z));
//#endif

// old implementation
#if 0 //                                                                       {{{
    // for primitive shapes, e.g. btBoxShape, btSphereShape and btMultiSphereShape the
    // RigidBody/RigidBodyT's world transform is used in createAndAdjustRigidBody
    //
    // for bhkCapsuleShape

    Ogre::Matrix4 shapeTransform(Ogre::Matrix4::IDENTITY); // NOTE: buildShape may modify
    bool hasShapeTransform = false;                        // NOTE: buildShape may set to true
    bhkShape *shape = mModel.getRef<bhkShape>(mShapeIndex);

    std::unique_ptr<btCollisionShape> tmp = shape->buildShape(); // FIXME

    //if (mModel.blockType(mShapeIndex) == "bhkCapsuleShape") // less efficient?
    if (hasShapeTransform)
    {
        // FIXME: apply shape's transform and havok scale
    }
#endif //                                                                       }}}

    bhkShape *shape = mModel.getRef<bhkShape>(mShapeIndex);
    std::unique_ptr<btCollisionShape> tmp = shape->buildShape(transform); // FIXME: store a master copy?

    // OL_ANIM_STATIC and OL_BIPED might be ragdoll?
    if (mLayer == 1) // OL_STATIC
    {
        // FIXME: check that mass is zero
        Ogre::Vector3 nodeTrans = inst->mBaseNode->_getDerivedPosition();
        Ogre::Quaternion nodeRot = inst->mBaseNode->_getDerivedOrientation();
    }
    else if (mLayer == 2 /* OL_ANIM_STATIC */ || mLayer == 8 /* OL_BIPED */)
    {
        // put in a map in inst keyed by parentNiNode's index?
    }

    // TODO: populate inst and/or register rigid body to Bullet dynamics world?
    // How to deal with ragdoll and havok objects?

    // FIXME: testing only
    // bhkRigidBodyT, bhkConvexVerticesShape
    if (mModel.getName() == "meshes\\architecture\\imperialcity\\icsigncopious01.nif")
    {
        Ogre::Vector3 nodeTrans = inst->mBaseNode->_getDerivedPosition();
        Ogre::Quaternion nodeRot = inst->mBaseNode->_getDerivedOrientation();

        std::cout << mModel.getName() << std::endl;
    }
    // FIXME: end testing
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkSimpleShapePhantom::bhkSimpleShapePhantom(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkSerializable(index, stream, model)
{
    stream.read(mShapeIndex);
    stream.read(mLayer);
    stream.read(mColFilter);
    stream.read(mUnknownShort);

    stream.skip(sizeof(float)*23); // 7 + 3*5 + 1
}

// vim: fen fdm=marker fdl=0
