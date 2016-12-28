#include "foreignactor.hpp"

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <OgreSceneManager.h>

#include <components/nifbullet/bulletnifloader.hpp>
#include <components/misc/stringops.hpp>

#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "BtOgreExtras.h"

namespace OEngine {
namespace Physic
{

    ForeignActor::ForeignActor(const std::string &name, const std::string &mesh,
            PhysicEngine *engine, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation, float scale)
    : PhysicActor(name, mesh, engine, position, rotation, scale)
    {
        if (!NifBullet::getBoundingBox(mMesh, mHalfExtents, mMeshTranslation, mMeshOrientation))
        {
            mHalfExtents = Ogre::Vector3(0.f);
            mMeshTranslation = Ogre::Vector3(0.f);
            mMeshOrientation = Ogre::Quaternion::IDENTITY;
        }

        // Use capsule shape only if base is square (nonuniform scaling apparently doesn't work on it)
        if (std::abs(mHalfExtents.x-mHalfExtents.y)<mHalfExtents.x*0.05 && mHalfExtents.z >= mHalfExtents.x)
        {
            // Could also be btCapsuleShapeZ, but the movement solver seems to have issues with it (jumping on slopes doesn't work)
            mShape.reset(new btCylinderShapeZ(BtOgre::Convert::toBullet(mHalfExtents)));
        }
        else
            mShape.reset(new btBoxShape(BtOgre::Convert::toBullet(mHalfExtents)));

        mShape->setLocalScaling(btVector3(scale,scale,scale));

        btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo
                (0,0, mShape.get());
        mBody = new RigidBody(CI, name);
        mBody->mPlaceable = false;
        mBody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
        mBody->setActivationState(DISABLE_DEACTIVATION);

        setPosition(position);
        setRotation(rotation);

        updateCollisionMask();
    }

    ForeignActor::~ForeignActor()
    {
        if(mBody)
        {
            mEngine->mDynamicsWorld->removeRigidBody(mBody);
            delete mBody;
        }
    }
}
}
