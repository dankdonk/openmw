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

// TODO: defer building constraints until all the bodies have been built
// TODO: each rigid body types should have their own frame conversion method

#include <cassert>
#include <stdexcept>

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

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

#if 0 // Commented out. Use instead: typedef bhkRefObject bhkSerializable
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

    //stream.skip(sizeof(char));          // Unknown Byte 1
    //stream.skip(sizeof(std::uint32_t)); // Unknown Int 3
    //stream.skip(sizeof(std::uint32_t)); // Unknown Int 4
    //stream.skip(sizeof(std::uint32_t)); // Unknown Int 5
    //stream.skip(sizeof(char));          // Unknown Byte 2
    stream.skip(sizeof(char)*2 + sizeof(std::uint32_t)*3);

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
// e.g. Skyrim/Data/meshes/traps/tripwire/traptripwire01.nif
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

// Some examples from TES4
//   architecture/ships/shipflag01.nif
//   armor/chainmail/f/cuirass_gnd.nif
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
            stream.skip(sizeof(float)*6 + sizeof(char));
        }
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
//
// Some examples from TES4:
//   architecture/chorrol/signfightersguild.nif
//   architecture/chorrol/signfireandsteel.nif
//   architecture/chorrol/signinnoakcrosier.nif
//   architecture/chorrol/signmageguild01.nif
//   architecture/chorrol/signnortherngoods.nif
//   architecture/chorrol/signrenoitbooks.nif
//   armor/chainmail/m/cuirass_gnd.nif
NiBtOgre::bhkLimitedHingeConstraint::bhkLimitedHingeConstraint(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkConstraint(index, stream, model)
{
    mLimitedHinge.read(stream);
}

void NiBtOgre::RagdollDescriptor::read(NiStream& stream)
{
    if (stream.nifVer() <= 0x14000005)
    {
        stream.readBtVector3(pivotA);
        stream.readBtVector3(planeA);
        stream.readBtVector3(twistA);
        stream.readBtVector3(pivotB);
        stream.readBtVector3(planeB);
        stream.readBtVector3(twistB);
    }
    else if (stream.nifVer() >= 0x14020007)
    {
        stream.readBtVector3(twistA);
        stream.readBtVector3(planeA);
        stream.readBtVector3(motorA);
        stream.readBtVector3(pivotA);
        stream.readBtVector3(twistB);
        stream.readBtVector3(planeB);
        stream.readBtVector3(motorB);
        stream.readBtVector3(pivotB);
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
            stream.skip(sizeof(float)*6 + sizeof(char));
        }
    }
}

// seen in nif ver 20.0.0.4, 20.0.0.5
//
//   architecture/ships/shipflag01.nif
//   characters/_male/skeleton.nif
//   characters/_male/skeletonbeast.nif
//   characters/_male/skeletonsesheogorath.nif
//   creatures/baliwog/skeleton.nif
//   creatures/clannfear/skeleton.nif
//   creatures/daedroth/skeleton.nif
//   creatures/dog/skeleton.nif
//   creatures/frostatronach/skeleton.nif
//   creatures/horse/skeleton.nif
//   creatures/imp/skeleton.nif
//   creatures/landdreugh/skeleton.nif
//   creatures/lich/skeleton.nif
//   creatures/lich/skeletonwarlock.nif
//   creatures/minotaur/skeleton.nif
//   creatures/mountainlion/skeleton.nif
//   creatures/murkdweller/skeleton.nif
//   creatures/ogre/skeleton.nif
//   creatures/rat/skeleton.nif
//   creatures/scamp/skeleton.nif
//   creatures/shambles/skeleton.nif
//   creatures/skeleton/skeleton.nif
//   creatures/spiderdaedra/skeleton.nif
//   creatures/spriggan/skeleton.nif
//   creatures/troll/skeleton.nif
//   creatures/zombie/skeleton.nif
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
//
// Seems to be for quivers only? (TES4)
//    weapons/Amber/arrow.nif
//    weapons/bone/arrow.nif
//    weapons/daedric/arrow.nif
//    weapons/darkseducer/arrow.nif
//    weapons/dwarven/arrow.nif
//    weapons/ebony/arrow.nif
//    weapons/ebony/arrowroseofsithis.nif
//    weapons/elven/arrow.nif
//    weapons/glass/arrow.nif
//    weapons/goldensaint/arrow.nif
//    weapons/gromite/gromitearrow.nif
//    weapons/gromite/gromitequiver.nif
//    weapons/iron/arrow.nif
//    weapons/madness/arrow.nif
//    weapons/se32ghostlydagger/se32ghostlyarrow.nif
//    weapons/silver/arrow.nif
//    weapons/steel/arrow.nif
//    weapons/steel/keyshapedarrow.nif
NiBtOgre::bhkPrismaticConstraint::bhkPrismaticConstraint(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkConstraint(index, stream, model)
{
    if (stream.nifVer() <= 0x14000005)
    {
        stream.read(mPivotA);
        mRotationMatrixA.resize(4);
        for (unsigned int i = 0; i < 4; ++i)
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

void NiBtOgre::bhkRagdollConstraint::linkBodies(BtOgreInst *inst, const bhkEntity *body) const
{
#if 0
if (node->name == "TargetchainRight02" /*|| node->name == "TargetchainRight02"*/)
//if (node->name == "TargetHeavyTarget")
    return;
#endif

    // FIXME: need to call bhkRigiBody to get the right position of the pivots

    btTransform localA;
    btTransform localB;
    Ogre::Vector3 scale = Ogre::Vector3(1.f/*inst->mScale*/); // FIXME

    localA.setIdentity();
    localB.setIdentity();
    localA.setOrigin(mRagdoll.pivotA * scale.x); // FIXME: assume uniform scaling for now
    localB.setOrigin(mRagdoll.pivotB * scale.x); // FIXME: assume uniform scaling for now

    assert(mEntities[0] == body && "the first entry is not self");
    assert(mEntities.size() == 2 && "unexpected number of entities");
    // FIXME: what to do if there are more than 2 rigid bodies?

    btGeneric6DofConstraint *constraint
        = new btGeneric6DofConstraint(*(inst->mRigidBodies[mEntities[0]->index()].get()),
                                      *(inst->mRigidBodies[mEntities[1]->index()].get()),
                                      localA,
                                      localB,
                                      /*useLinearReferenceFrameA*/false);

    // FIXME: need to tune these values
    constraint->setParam(BT_CONSTRAINT_STOP_ERP,0.8f,0);
    constraint->setParam(BT_CONSTRAINT_STOP_ERP,0.8f,1);
    constraint->setParam(BT_CONSTRAINT_STOP_ERP,0.8f,2);
    constraint->setParam(BT_CONSTRAINT_STOP_CFM,0.f,0);
    constraint->setParam(BT_CONSTRAINT_STOP_CFM,0.f,1);
    constraint->setParam(BT_CONSTRAINT_STOP_CFM,0.f,2);

    // defer this to later
    //mDynamicsWorld->addConstraint(constraint, /*disable collision between linked bodies*/true);












#if 0

    mShape->mJoints[rigidBody->recIndex].push_back(
            std::make_pair(rigidBody->constraints[i]->entities[0]->recIndex,
                           rigidBody->constraints[i]->entities[1]->recIndex));

    Nif::RagdollDescriptor ragdollDesc;
    const Nif::bhkRagdollConstraint *ragdoll
        = static_cast<const Nif::bhkRagdollConstraint*>(rigidBody->constraints[i].getPtr());

    // FIXME: try to get the NIF frame of reference so that pivotA and pivotB
    // can be "fixed" with correct frames of references
    const Nif::bhkConstraint *constraint = rigidBody->constraints[i].getPtr();
    // rbA is not needed since this we already have node, translation and rotation
    //const Nif::bhkRigidBody *rbA
        //= static_cast<const Nif::bhkRigidBody*>(constraint->entities[0].getPtr()); // 0 HACK
    // however it doesn't seem possible to get the node info from the rigid body
    // maybe the shape's mbox translation needs to be used? (but how to get the
    // second shape from the entity?)
    const Nif::bhkRigidBody *rbB
        = static_cast<const Nif::bhkRigidBody*>(constraint->entities[1].getPtr()); // 1 HACK

    Ogre::Matrix4 rbT = Ogre::Matrix4::IDENTITY;
    typedef std::map<size_t, btRigidBody::btRigidBodyConstructionInfo>::iterator ConstructionInfoIter;
    ConstructionInfoIter itCIA(mShape->mRigidBodyCI.find(rigidBody->recIndex));
    if (itCIA == mShape->mRigidBodyCI.end())
        continue; // FIXME: shouldn't happen, so probably best to throw here
    btQuaternion qa = itCIA->second.m_startWorldTransform.getRotation();

    if (rigidBody->recType == Nif::RC_bhkRigidBodyT)
    {
        rbT.makeTransform(translation*7, Ogre::Vector3(1.f), rotation);
    }

    if (rigidBody->shape->recType == Nif::RC_bhkCapsuleShape)
    {
            //ragdollDesc.pivotA = mShape->mShapeTrans * ragdoll->ragdoll.pivotA *7; // FIXME
            Ogre::Vector3 t = mShape->mShapeTrans; // t is the midpoint vector
            Ogre::Vector4 p = ragdoll->ragdoll.pivotA * 7;
            ragdollDesc.pivotA = Ogre::Vector4(p.x-t.x, p.y-t.y, p.z-t.z, p.w);
            ragdollDesc.planeA = ragdoll->ragdoll.planeA * 7;
            ragdollDesc.twistA = ragdoll->ragdoll.twistA * 7;
    }
    else
    {
#if 0
            Ogre::Vector4 p = ragdoll->ragdoll.pivotA * 7;
            Ogre::Vector3 v(p.x, p.y, p.z);
        v = Ogre::Quaternion(qa.w(), qa.x(), qa.y(), qa.z()) * v; // rotate as per m_startWorldTransform
            ragdollDesc.pivotA = Ogre::Vector4(v.x, v.y, v.z, p.w);
#endif


            //ragdollDesc.pivotA = rbT.inverse() * ragdoll->ragdoll.pivotA * 7;
            ragdollDesc.pivotA = ragdoll->ragdoll.pivotA * 7;
            ragdollDesc.planeA = ragdoll->ragdoll.planeA * 7;
            ragdollDesc.twistA = ragdoll->ragdoll.twistA * 7;
    }



        Ogre::Matrix4 rbTB = Ogre::Matrix4::IDENTITY;
        typedef std::map<size_t, btRigidBody::btRigidBodyConstructionInfo>::iterator ConstructionInfoIter;
        ConstructionInfoIter itCI(mShape->mRigidBodyCI.find(rbB->recIndex));
        if (itCI == mShape->mRigidBodyCI.end())
            continue; // FIXME: shouldn't happen, so probably best to throw here
        btQuaternion q = itCI->second.m_startWorldTransform.getRotation();
        //btTransform qt(q, btVector3(0.f, 0.f, 0.f)); // rotate only




    if (rbB->recType == Nif::RC_bhkRigidBodyT)
    {
        Ogre::Vector3 rbBtrans = Ogre::Vector3::ZERO;
        Ogre::Quaternion rbBrot = Ogre::Quaternion::IDENTITY;

            rbBrot = Ogre::Quaternion(rbB->rotation.w,
                    rbB->rotation.x, rbB->rotation.y, rbB->rotation.z);
            rbBtrans = Ogre::Vector3(rbB->translation.x,
                    rbB->translation.y, rbB->translation.z);
        rbTB.makeTransform(rbBtrans*7, Ogre::Vector3(1.f), rbBrot);
    }
    if (rbB->shape->recType == Nif::RC_bhkCapsuleShape)
    {
            Ogre::Vector3 trans = mShape->mShapeTransMap[rbB->recIndex]; // FIXME: just assume one exists!
            //ragdollDesc.pivotB = trans * ragdoll->ragdoll.pivotB *7; // FIXME
            Ogre::Vector3 t = trans;
            Ogre::Vector4 p = ragdoll->ragdoll.pivotB * 7;
            Ogre::Vector3 v(p.x, p.y, p.z);
        v = Ogre::Quaternion(q.w(), q.x(), q.y(), q.z()) * v; // rotate as per m_startWorldTransform
            ragdollDesc.pivotB = Ogre::Vector4(v.x-t.x, v.y-t.y, v.z-t.z, p.w);
            ragdollDesc.planeB = ragdoll->ragdoll.planeB * 7;
            ragdollDesc.twistB = ragdoll->ragdoll.twistB * 7;
    }
    else
    {
#if 0
            Ogre::Vector4 p = ragdoll->ragdoll.pivotB * 7;
            Ogre::Vector3 v(p.x, p.y, p.z);
        v = Ogre::Quaternion(q.w(), q.x(), q.y(), q.z()) * v; // rotate as per m_startWorldTransform
            ragdollDesc.pivotB = Ogre::Vector4(v.x, v.y, v.z, p.w);

#endif


            //ragdollDesc.pivotB = rbTB.inverse() * ragdoll->ragdoll.pivotB * 7;
            ragdollDesc.pivotB = ragdoll->ragdoll.pivotB * 7;
            ragdollDesc.planeB = ragdoll->ragdoll.planeB * 7;
            ragdollDesc.twistB = ragdoll->ragdoll.twistB * 7;
    }
        ragdollDesc.coneMaxAngle = ragdoll->ragdoll.coneMaxAngle;
        ragdollDesc.planeMinAngle = ragdoll->ragdoll.planeMinAngle;
        ragdollDesc.planeMaxAngle = ragdoll->ragdoll.planeMaxAngle;
        ragdollDesc.twistMinAngle = ragdoll->ragdoll.twistMinAngle;
        ragdollDesc.twistMaxAngle = ragdoll->ragdoll.twistMaxAngle;
        ragdollDesc.maxFriction = ragdoll->ragdoll.maxFriction;

        mShape->mNifRagdollDesc[std::make_pair(rigidBody->recIndex, rbB->recIndex)] = ragdollDesc; // cast away compiler warning
#endif
}

NiBtOgre::bhkShape::bhkShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkSerializable(index, stream, model)
{
}

// seen in nif ver 20.0.0.4, 20.0.0.5
//
// Only 4 examples in TES4:
//   creatures/willothewisp/skeleton.nif
//   creatures/willothewisp/skeleton02.nif
//   dungeons/misc/mdtapestryskinned01.nif
//   dungeons/misc/necrotapestryskinned01.nif
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
    for (unsigned int i = 0; i < 8; ++i)
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
    for (unsigned int i = 0; i < 4; ++i)
        stream.read(mUnknown4Bytes.at(i));

    stream.skip(sizeof(float)*4); // Unknown Floats 1

    stream.read(mRadius);
    stream.read(mScale);

    //stream.skip(sizeof(float)); // Unknown Float 3
    //stream.skip(sizeof(float)); // Unknown Float 4
    //stream.skip(sizeof(float)); // Unknown Float 5
    stream.skip(sizeof(float)*3);

    stream.read(mDataIndex);
}

std::unique_ptr<btCollisionShape> NiBtOgre::bhkCompressedMeshShape::buildShape(const btTransform& transform) const
{
    return std::unique_ptr<btCollisionShape>(nullptr); // FIXME: TODO needed for TES5 only
}

NiBtOgre::bhkConvexListShape::bhkConvexListShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkShape(index, stream, model)
{
    stream.readVector<bhkConvexShapeRef>(mSubShapes);
    stream.read(mMaterial);

    //mUnknownfloats.resize(6);
    //for (int i = 0; i < 6; ++i)
    //    stream.read(mUnknownFloats.at(i));
    stream.skip(sizeof(float)*6);

    stream.skip(sizeof(char));
    stream.skip(sizeof(float));
}

std::unique_ptr<btCollisionShape> NiBtOgre::bhkConvexListShape::buildShape(const btTransform& transform) const
{
    return std::unique_ptr<btCollisionShape>(nullptr); // FIXME: TODO needed for TO3
}

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkListShape::bhkListShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkShape(index, stream, model)
{
    stream.readVector<bhkShapeRef>(mSubShapes);
    stream.read(mMaterial);

    //mUnknownfloats.resize(6);
    //for (int i = 0; i < 6; ++i)
    //    stream.read(mUnknownFloats.at(i));
    stream.skip(sizeof(float)*6);

    stream.readVector<std::uint32_t>(mUnknownInts);
}

// Note that a bhkListShape can have bhkTransformShape children (in fact, quite often).
//
// coc "FortRoebeck02" - HandScythe01 - bhkTransformShape/bhkBoxShape
// coc "RockmilkCave03"
// - meshes/clutter/lowerclass/ClothBolt04.nif
//      bhkCapsuleShape, bhkConvexTransformShape/bhkBoxShape
// - meshes/clutter/lowerclass/lowerthatchbasket02.nif
//      bhkConvexVerticesShape, bhkConvexTransformShape/bhkBoxShape
std::unique_ptr<btCollisionShape> NiBtOgre::bhkListShape::buildShape(const btTransform& transform) const
{
    std::unique_ptr<btCollisionShape> collisionShape = std::unique_ptr<btCompoundShape>(new btCompoundShape());

    for (unsigned int i = 0; i < mSubShapes.size(); ++i)
    {
        if (mSubShapes[i] == -1)
            continue; // nothing to build

        bhkShape *subShape = mModel.getRef<bhkShape>(mSubShapes[i]);

        std::unique_ptr<btCollisionShape> subCollisionShape = subShape->buildShape(transform);
        assert(subCollisionShape.get() != nullptr && "bhkListShape: child buildShape failed");

        if (subCollisionShape->isCompound())
        {
            throw std::runtime_error ("bhkListShape: unexpected btCompoundShape");
            //std::cerr << "buildShape: child in ListShape is a List " << mModel.blockType(mSelfIndex) << std::endl;
            //continue;
        }

        btTransform subTransform;
        int userIndex = subCollisionShape->getUserIndex();

        if (userIndex == 2)        // shape and subshape have their own transforms
        {
            // e.g. bhkCapsuleShape: architecture/daedricstatues/daedricshrinehircine01.nif (cow "tamriel" -2 -5)
//#if 0 // FIXME: testing
            std::string subShapeType = mModel.blockType(mSubShapes[i]);

            if (subShapeType != "bhkTransformShape" && subShapeType != "bhkConvexTransformShape")
                throw std::runtime_error ("bhkListShape: transfrom shape was expected");
//#endif
            subTransform = transform * subShape->transform()
                * mModel.getRef<bhkShape>(static_cast<bhkTransformShape*>(subShape)->shapeIndex())->transform();
        }
        else if (userIndex == 1)   // shape has its own transform
        {
            subTransform = transform * subShape->transform();
        }
        else if (userIndex == 0)   // transform applied
        {
            subTransform.getIdentity();
        }
        else//if (userIndex == -1) // no transform applied
        {
            subTransform = transform;
        }

        static_cast<btCompoundShape*>(collisionShape.get())->addChildShape(subTransform, subCollisionShape.get());
    }

    collisionShape->setUserIndex(0); // indicate transform applied (to each of the child shapes, anyway)
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

// From BulletCollision/CollisionShapes/btStridingMeshInterface.cpp:
//
//   The btStridingMeshInterface is the interface class for high performance generic access to
//   triangle meshes, used in combination with btBvhTriangleMeshShape and some other collision
//   shapes.
//
//   Using index striding of 3*sizeof(integer) it can use triangle arrays, using index striding
//   of 1*sizeof(integer) it can handle triangle strips.
//
// From BulletCollision/CollisionShapes/btTriangleIndexVertexArray.h:
//
//   The btTriangleIndexVertexArray allows to access multiple triangle meshes, by indexing into
//   existing triangle/index arrays.
//
//   Additional meshes can be added using addIndexedMesh /No duplcate is made of the
//   vertex/index data, it only indexes into external vertex/index arrays.  So keep those
//   arrays around during the lifetime of this btTriangleIndexVertexArray.
//
// Notes:
//
// It seems Bullet only keeps pointers?  Which means vertices and indicies must be kept around.
//
// How does transforms affect this, especially for OL_CLUTTER which is probably havok enabled?
//
// However, if using btTriangleMesh m_4componentVertices or m_3componentVertices will keep the
// vertices and hence no need to keep the arrays around?
//
// e.g. architecture/arena/arenacolumn02.nif
//      clutter/books/wantedposter01.nif
//      clutter/books/wantedposter02.nif
std::unique_ptr<btCollisionShape> NiBtOgre::bhkNiTriStripsShape::buildShape(const btTransform& transform) const
{
    btTriangleMesh *mesh = new btTriangleMesh();

    for (size_t i = 0; i < mStripsData.size(); ++i)
    {
        if (mStripsData[i] == -1)
            continue; // nothing to build

        NiTriStripsData *triStripsData = mModel.getRef<NiTriStripsData>(mStripsData[i]);
        assert(triStripsData != nullptr && "mModel.getRef returned nullptr"); // FIXME: throw instead?

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
        mesh->addTriangle(transform * (triData->mVertices[triData->mTriangles[i].triangle[0]]*7), // NOTE: havok scale
                          transform * (triData->mVertices[triData->mTriangles[i].triangle[1]]*7),
                          transform * (triData->mVertices[triData->mTriangles[i].triangle[2]]*7));
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
        if (stream.nifVer() <= 0x14000005)
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
    return std::unique_ptr<btSphereShape>(new btSphereShape(mRadius*7)); // NOTE: havok scale
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

// Examples of bhkBoxShape with bhkRigidBodyT:
//   clutter/books/wantedposter02static.nif
//   plants/florasacredlotus01.nif
//   clutter/middleclass/middlecrate02.nif
std::unique_ptr<btCollisionShape> NiBtOgre::bhkBoxShape::buildShape(const btTransform& transform) const
{
    // TODO: check mMinimumSize first? Can it be zero?
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

    // Based on examples/Importers/ImportMJCFDemo/BulletMJCFImporter.cpp, also see:
    // http://lolengine.net/blog/2013/09/18/beautiful-maths-quaternion-from-vectors

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
    std::unique_ptr<btCollisionShape> collisionShape(new btCapsuleShapeZ(mRadius1, btScalar(0.5f)*mHeight));
    collisionShape->setUserIndex(1); // indicate that this shape has own transform
    return collisionShape;
}

// seen in nif ver 20.0.0.4, 20.0.0.5
NiBtOgre::bhkConvexVerticesShape::bhkConvexVerticesShape(uint32_t index, NiStream& stream, const NiModel& model)
    : bhkSphereRepShape(index, stream, model)
{
    mUnknown6Floats.resize(6);
    for (unsigned int i = 0; i < 6; ++i)
        stream.read(mUnknown6Floats.at(i));

    stream.read(mNumVertices);

    mVertices.resize(mNumVertices);
    for (unsigned int i = 0; i < mNumVertices; ++i)
        for (unsigned int j = 0; j < 4; ++j)
            stream.read(mVertices.at(i).m_floats[j]);

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

    convexHull->setUserIndex(0); // indicate that transform was applied
    return convexHull;
#else // use different ctor for minor optimisation; TODO test if it makes any difference
    std::unique_ptr<float[]> vertices(new float[mNumVertices*4]);
    for (unsigned int i = 0; i < mNumVertices; ++i)
    {
        btVector3 point = transform * (mVertices[i]*7); // NOTE: havok scale

        vertices[i*4]   = point.x();
        vertices[i*4+1] = point.y();
        vertices[i*4+2] = point.z();
        vertices[i*4+3] = point.w();
    }

    std::unique_ptr<btCollisionShape> collisionShape(new btConvexHullShape(vertices.get(),
                                                                           mNumVertices,
                                                                           sizeof(float)*4));
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
    return std::unique_ptr<btMultiSphereShape>(new btMultiSphereShape(mCenters.get(),
                                                                      mRadii.get(),
                                                                      (int)mNumSpheres));
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

    mTransform.setFromOpenGLMatrix(floats);
}

// Looks like most of the bhkTransformShapes in TES4 have bhkBoxShape or bhkCapsuleShape or bhkSphereShape,
// usually as part of bhkListShape.  Examples of stand-alone bhkTransformShape/bhkBoxShape are
//
//   dungeons/misc/triggers/trigzone02.nif (coc "vilverin")
//   dungeons/caves/clutter01/ancientcoin01.nif
//
// Other examples as part of a bhkListShape:
//
//   bhkSphereShape & bhkBoxShape: clutter/lowerclass/lowerjugtan01.nif (coc "ICMarketDistrictAFightingChance")
//   bhkCapsuleShape: architecture/daedricstatues/daedricshrinehircine01.nif (cow "tamriel" -2 -5)
//
// Examples of non-primitive shapes in FO3:
//
//   bhkMoppBvTreeShape/bhkNiTriStripsShape: architecture/urban/pedwalk/pedwalkstr01.nif
//
std::unique_ptr<btCollisionShape> NiBtOgre::bhkTransformShape::buildShape(const btTransform& transform) const
{
    if (mShapeIndex == -1)
        return std::unique_ptr<btCollisionShape>(nullptr);

// more testing
//#if 0
    std::string shapeType = mModel.blockType(mShapeIndex);
    if (shapeType == "bhkListShape" || shapeType == "bhkTransformShape" || shapeType == "bhkConvexTransformShape")
        throw std::runtime_error ("bhkTransformShape: unexpected shape type");
//#endif

    bhkShape *shape = static_cast<bhkShape*>(mModel.getRef<bhkShape>(mShapeIndex));
    std::unique_ptr<btCollisionShape> collisionShape = shape->buildShape(transform * mTransform);

    int userIndex = collisionShape->getUserIndex();

    if (userIndex == 1)                // shape has its own transform, e.g. btCapsuleShape
        collisionShape->setUserIndex(2);
    else if (userIndex == 0)           // transform applied , e.g. bhkMoppBvTreeShape/bhkNiTriStripsShape
        collisionShape->setUserIndex(0);
    else // userIndex == -1            // no transform applied, e.g. btBoxShape
        collisionShape->setUserIndex(1);

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
    for (unsigned int i = 0; i < 3; ++i)
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
    for (unsigned int i = 0; i < 7; ++i)
        stream.read(mUnknown7Shorts.at(i));

    for (unsigned int i = 0; i < 4; ++i)
        stream.read(mTranslation.m_floats[i]);

    float rot[4];
    for (unsigned int i = 0; i < 4; ++i)
        stream.read(rot[i]);
    mRotation = btQuaternion(rot[0], rot[1], rot[2], rot[3]);

    stream.read(mLinearVelocity);
    stream.read(mAngularVelocity);

    float value = 0;
    for (unsigned int i = 0; i < 3; ++i)
    {
        for (unsigned int j = 0; j < 4; ++j)
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
    for (unsigned int i = 0; i < numConstraints; ++i)
        stream.read(mConstraints.at(i));

    if (stream.userVer() <= 11)
        stream.read(mUnknownInt9);
    else if (stream.userVer() >= 12)
        stream.read(mUnknownInt91);
}

// WARNING: the parameter 'parent' is a NiNode (i.e. not bhkNiCollisionObject)
// 1. calculate the world transform of the NIF
// 2. create the btRidgidBody and store it in 'inst', keyed with the index of the parent NiNode
//    so that the associated Ogre::Entity can be used for ragdoll animation
// 3. the transform of the SceneNode to be applied with btRigidBody
// 4. create any associated constraints
// 5. if OL_STATIC register with Bullet for collisions
// FIXME: only do some of the steps if actual ragdoll?
//
// FIXME: some of these should allow raycasting for object identification?
void NiBtOgre::bhkRigidBody::build(BtOgreInst *inst, NiObject* parentNiNode)
{
    if (mShapeIndex == -1) // nothing to build
        return;

    Ogre::Vector3 pos;
    Ogre::Vector3 scale;
    Ogre::Quaternion rot;
    static_cast<NiAVObject*>(parentNiNode)->getWorldTransform().decomposition(pos, scale, rot);

    btTransform transform(btQuaternion(rot.x, rot.y, rot.z, rot.w), btVector3(pos.x, pos.y, pos.z));

    // apply rotation and translation only if the collision object's body is a bhkRigidBodyT type
    if (mModel.blockType(mSelfIndex) == "bhkRigidBodyT")
        transform = transform * btTransform(mRotation, mTranslation * 7); // NOTE: havok scale

    bhkShape *shape = mModel.getRef<bhkShape>(mShapeIndex);
    std::unique_ptr<btCollisionShape> btShape = shape->buildShape(transform); // FIXME: store a master copy?

    // FIXME
    if (!btShape.get())
        return;

    int userIndex = btShape->getUserIndex();
    if (userIndex == 2)                // shape has its own transform, e.g. transform capsule shape
    {
        // rigidbody.setWorldTransform(SceneNodeTrans * transform * shape->transform() * subShape->transform());
        //std::cout << "transform" << std::endl; // never happens?
    }
    else if (userIndex == 1)           // shape has its own transform, e.g. btCapsuleShape
    {
        // rigidbody.setWorldTransform(SceneNodeTrans * transform * shape->transform());
        //std::cout << "capsule" << std::endl; // never happens?
    }
    else if (userIndex == 0)           // transform applied
    {
        // rigidbody.setWorldTransform(SceneNodeTrans);
        //std::cout << "mesh" << std::endl;
    }
    else // userIndex == -1            // no transform applied, e.g. btBoxShape
    {
        // rigidbody.setWorldTransform(SceneNodeTrans * transform);
        //std::cout << "primitive" << std::endl;
    }

    //if (mLayer == 2) // OL_ANIM_STATIC
        //std::cout << "anim_static" << std::endl;
    //else if (mLayer == 8) // OL_BIPED
        //std::cout << "biped" << std::endl;
#if 0
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
    if (mModel.getModelName() == "meshes\\architecture\\imperialcity\\icsigncopious01.nif")
    {
        Ogre::Vector3 nodeTrans = inst->mBaseNode->_getDerivedPosition();
        Ogre::Quaternion nodeRot = inst->mBaseNode->_getDerivedOrientation();

        std::cout << mModel.getModelName() << std::endl;
    }
    // FIXME: end testing
#endif

    btRigidBody::btRigidBodyConstructionInfo rbCI(mMass, 0/*btMotionState**/, btShape.get());
    // FIXME: add friction, damping, etc, here

    inst->mRigidBodies[mSelfIndex] = std::unique_ptr<btRigidBody>(new btRigidBody(rbCI));

    // bhkRigidBody may have constraints
    for (size_t i = 0; i < mConstraints.size(); ++i)
    {
        // Defer the building of constraints till all the bhkEntities have been built.
        // e.g. meshes/architecture/arena/chaindollarena01.nif refers to one not yet built
        // Some (most?) constraints need the rigid bodies to supply the appropriate transforms
        // for converting NIF space to Bullet space.
        inst->mbhkConstraints.push_back(std::make_pair(mModel.getRef<bhkConstraint>(mConstraints[i]), this));
    }
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

// Called from bhkSPCollisionObject
// e.g. dungeons/misc/triggers/trigzone02.nif (coc "vilverin")
void NiBtOgre::bhkSimpleShapePhantom::build(BtOgreInst *inst, NiObject* parentNiNode)
{
}

// vim: fen fdm=marker fdl=0
