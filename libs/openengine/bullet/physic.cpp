#include "physic.hpp"

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

#include "foreignactor.hpp"

#ifndef M_PI_2
#define M_PI_2     btScalar(1.57079632679489661923)
#endif

namespace
{

// Create a copy of the given collision shape (responsibility of user to delete the returned shape).
btCollisionShape *duplicateCollisionShape(btCollisionShape *shape)
{
    if(shape->isCompound())
    {
        btCompoundShape *comp = static_cast<btCompoundShape*>(shape);
        btCompoundShape *newShape = new btCompoundShape;

        int numShapes = comp->getNumChildShapes();
        for(int i = 0;i < numShapes;i++)
        {
            btCollisionShape *child = duplicateCollisionShape(comp->getChildShape(i));
            btTransform trans = comp->getChildTransform(i);
            newShape->addChildShape(trans, child);
        }

        return newShape;
    }

    if(btBvhTriangleMeshShape *trishape = dynamic_cast<btBvhTriangleMeshShape*>(shape))
    {
        btTriangleMesh* oldMesh = static_cast<btTriangleMesh*>(trishape->getMeshInterface());
        btTriangleMesh* newMesh = new btTriangleMesh(*oldMesh);
        NifBullet::TriangleMeshShape *newShape = new NifBullet::TriangleMeshShape(newMesh, true);

        return newShape;
    }

    throw std::logic_error(std::string("Unhandled Bullet shape duplication: ")+shape->getName());
}

void deleteShape(btCollisionShape* shape)
{
    if(shape!=NULL)
    {
        if(shape->isCompound())
        {
            btCompoundShape* ms = static_cast<btCompoundShape*>(shape);
            int a = ms->getNumChildShapes();
            for(int i=0; i <a;i++)
            {
                deleteShape(ms->getChildShape(i));
            }
        }
        delete shape;
    }
}

}

namespace OEngine {
namespace Physic
{

    PhysicActor::PhysicActor(const std::string &name, const std::string &mesh, PhysicEngine *engine, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation, float scale)
      : mCanWaterWalk(false), mWalkingOnWater(false)
      , mBody(0), mScale(scale), mForce(0.0f), mOnGround(false)
      , mInternalCollisionMode(true)
      , mExternalCollisionMode(true)
      , mMesh(mesh), mName(name), mEngine(engine)
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

    PhysicActor::~PhysicActor()
    {
        if(mBody)
        {
            mEngine->mDynamicsWorld->removeRigidBody(mBody);
            delete mBody;
        }
    }

    void PhysicActor::enableCollisionMode(bool collision)
    {
        mInternalCollisionMode = collision;
    }

    void PhysicActor::enableCollisionBody(bool collision)
    {
        if (mExternalCollisionMode != collision)
        {
            mExternalCollisionMode = collision;
            updateCollisionMask();
        }
    }

    void PhysicActor::updateCollisionMask()
    {
        mEngine->mDynamicsWorld->removeRigidBody(mBody);
        int collisionMask = CollisionType_World | CollisionType_HeightMap;
        if (mExternalCollisionMode)
            collisionMask |= CollisionType_Actor | CollisionType_Projectile;
        if (mCanWaterWalk)
            collisionMask |= CollisionType_Water;
        mEngine->mDynamicsWorld->addRigidBody(mBody, CollisionType_Actor, collisionMask);
    }

    const Ogre::Vector3& PhysicActor::getPosition() const
    {
        return mPosition;
    }

    void PhysicActor::setPosition(const Ogre::Vector3 &position)
    {
        assert(mBody);

        mPosition = position;

        btTransform tr = mBody->getWorldTransform();
        Ogre::Quaternion meshrot = mMeshOrientation;
        Ogre::Vector3 transrot = meshrot * (mMeshTranslation * mScale);
        Ogre::Vector3 newPosition = transrot + position;

        tr.setOrigin(BtOgre::Convert::toBullet(newPosition));
        mBody->setWorldTransform(tr);
    }

    void PhysicActor::setRotation (const Ogre::Quaternion& rotation)
    {
        btTransform tr = mBody->getWorldTransform();
        tr.setRotation(BtOgre::Convert::toBullet(mMeshOrientation * rotation));
        mBody->setWorldTransform(tr);
    }

    void PhysicActor::setScale(float scale)
    {
        mScale = scale;
        mShape->setLocalScaling(btVector3(scale,scale,scale));
        setPosition(mPosition);
    }

    Ogre::Vector3 PhysicActor::getHalfExtents() const
    {
        return mHalfExtents * mScale;
    }

    void PhysicActor::setInertialForce(const Ogre::Vector3 &force)
    {
        mForce = force;
    }

    void PhysicActor::setOnGround(bool grounded)
    {
        mOnGround = grounded;
    }

    bool PhysicActor::isWalkingOnWater() const
    {
        return mWalkingOnWater;
    }

    void PhysicActor::setWalkingOnWater(bool walkingOnWater)
    {
        mWalkingOnWater = walkingOnWater;
    }

    void PhysicActor::setCanWaterWalk(bool waterWalk)
    {
        if (waterWalk != mCanWaterWalk)
        {
            mCanWaterWalk = waterWalk;
            updateCollisionMask();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////


    RigidBody::RigidBody(btRigidBody::btRigidBodyConstructionInfo& CI,std::string name)
        : btRigidBody(CI)
        , mName(name)
        , mPlaceable(false)
    {
    }

    RigidBody::~RigidBody()
    {
        delete getMotionState();
    }



    ///////////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////////



    PhysicEngine::PhysicEngine(BulletShapeLoader* shapeLoader) :
        mSceneMgr(NULL)
      , mDebugActive(0)
    {
        // Set up the collision configuration and dispatcher
        collisionConfiguration = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfiguration);

        // The actual physics solver
        solver = new btSequentialImpulseConstraintSolver;

        broadphase = new btDbvtBroadphase();

        // The world.
        mDynamicsWorld = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration);

        // Don't update AABBs of all objects every frame. Most objects in MW are static, so we don't need this.
        // Should a "static" object ever be moved, we have to update its AABB manually using DynamicsWorld::updateSingleAabb.
        mDynamicsWorld->setForceUpdateAllAabbs(false);

        mDynamicsWorld->setGravity(btVector3(0,0,-10));

        if(BulletShapeManager::getSingletonPtr() == NULL)
        {
            new BulletShapeManager();
        }
        //TODO:singleton?
        mShapeLoader = shapeLoader;

        isDebugCreated = false;
        mDebugDrawer = NULL;
    }

    void PhysicEngine::createDebugRendering()
    {
        if(!isDebugCreated)
        {
            Ogre::SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
            mDebugDrawer = new BtOgre::DebugDrawer(node, mDynamicsWorld);
            mDynamicsWorld->setDebugDrawer(mDebugDrawer);
            isDebugCreated = true;
            mDynamicsWorld->debugDrawWorld();
        }
    }

    void PhysicEngine::setDebugRenderingMode(bool isDebug)
    {
        if(!isDebugCreated)
        {
            createDebugRendering();
        }
        mDebugDrawer->setDebugMode(isDebug);
        mDebugActive = isDebug;
    }

    bool  PhysicEngine::toggleDebugRendering()
    {
        setDebugRenderingMode(!mDebugActive);
        return mDebugActive;
    }

    void PhysicEngine::setSceneManager(Ogre::SceneManager* sceneMgr)
    {
        mSceneMgr = sceneMgr;
    }

    PhysicEngine::~PhysicEngine()
    {
        for (std::map<RigidBody*, AnimatedShapeInstance>::iterator it = mAnimatedShapes.begin(); it != mAnimatedShapes.end(); ++it)
            deleteShape(it->second.mCompound);
        for (std::map<RigidBody*, AnimatedShapeInstance>::iterator it = mAnimatedRaycastingShapes.begin(); it != mAnimatedRaycastingShapes.end(); ++it)
            deleteShape(it->second.mCompound);

        HeightFieldContainer::iterator hf_it = mHeightFieldMap.begin();
        for (; hf_it != mHeightFieldMap.end(); ++hf_it)
        {
            mDynamicsWorld->removeRigidBody(hf_it->second.mBody);
            delete hf_it->second.mShape;
            delete hf_it->second.mBody;
        }

        RigidBodyContainer::iterator rb_it = mCollisionObjectMap.begin();
        for (; rb_it != mCollisionObjectMap.end(); ++rb_it)
        {
            if (rb_it->second != NULL)
            {
                mDynamicsWorld->removeRigidBody(rb_it->second);

                delete rb_it->second;
                rb_it->second = NULL;
            }
        }
        rb_it = mRaycastingObjectMap.begin();
        for (; rb_it != mRaycastingObjectMap.end(); ++rb_it)
        {
            if (rb_it->second != NULL)
            {
                mDynamicsWorld->removeRigidBody(rb_it->second);

                delete rb_it->second;
                rb_it->second = NULL;
            }
        }

        PhysicActorContainer::iterator pa_it = mActorMap.begin();
        for (; pa_it != mActorMap.end(); ++pa_it)
        {
            if (pa_it->second != NULL)
            {
                delete pa_it->second;
                pa_it->second = NULL;
            }
        }

        delete mDebugDrawer;

        delete mDynamicsWorld;
        delete solver;
        delete collisionConfiguration;
        delete dispatcher;
        delete broadphase;
        delete mShapeLoader;

        // Moved the cleanup to mwworld/physicssystem
        //delete BulletShapeManager::getSingletonPtr();
    }

    void PhysicEngine::addHeightField(const float* heights,
        int x, int y, float yoffset,
        float triSize, float sqrtVerts)
    {
        const std::string name = "HeightField_"
            + boost::lexical_cast<std::string>(x) + "_"
            + boost::lexical_cast<std::string>(y);

        // find the minimum and maximum heights (needed for bullet)
        float minh = heights[0];
        float maxh = heights[0];
        for (int i=0; i<sqrtVerts*sqrtVerts; ++i)
        {
            float h = heights[i];

            if (h>maxh) maxh = h;
            if (h<minh) minh = h;
        }

        btHeightfieldTerrainShape* hfShape = new btHeightfieldTerrainShape(
            static_cast<int>(sqrtVerts), static_cast<int>(sqrtVerts), heights, 1,
            minh, maxh, 2,
            PHY_FLOAT,true);

        hfShape->setUseDiamondSubdivision(true);

        btVector3 scl(triSize, triSize, 1);
        hfShape->setLocalScaling(scl);

        btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo(0,0,hfShape);
        RigidBody* body = new RigidBody(CI,name);
        body->getWorldTransform().setOrigin(btVector3( (x+0.5f)*triSize*(sqrtVerts-1), (y+0.5f)*triSize*(sqrtVerts-1), (maxh+minh)/2.f));

        HeightField hf;
        hf.mBody = body;
        hf.mShape = hfShape;

        mHeightFieldMap [name] = hf;

        mDynamicsWorld->addRigidBody(body,CollisionType_HeightMap,
                                    CollisionType_Actor|CollisionType_Raycasting|CollisionType_Projectile);
    }

    void PhysicEngine::removeHeightField(int x, int y)
    {
        const std::string name = "HeightField_"
            + boost::lexical_cast<std::string>(x) + "_"
            + boost::lexical_cast<std::string>(y);

        HeightFieldContainer::iterator it = mHeightFieldMap.find(name);
        if (it == mHeightFieldMap.end())
            return;

        const HeightField& hf = it->second;

        mDynamicsWorld->removeRigidBody(hf.mBody);
        delete hf.mShape;
        delete hf.mBody;

        mHeightFieldMap.erase(it);
    }

    void PhysicEngine::adjustRigidBody(RigidBody* body, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
        const Ogre::Vector3 &scaledBoxTranslation, const Ogre::Quaternion &boxRotation)
    {
        btTransform tr;
        Ogre::Quaternion boxrot = rotation * boxRotation;
        Ogre::Vector3 transrot = boxrot * scaledBoxTranslation;
        Ogre::Vector3 newPosition = transrot + position;

        tr.setOrigin(btVector3(newPosition.x, newPosition.y, newPosition.z));
        tr.setRotation(btQuaternion(boxrot.x,boxrot.y,boxrot.z,boxrot.w));
        body->setWorldTransform(tr);
    }
    void PhysicEngine::boxAdjustExternal(const std::string &mesh, RigidBody* body,
        float scale, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation)
    {
        std::string sid = (boost::format("%07.3f") % scale).str();
        std::string outputstring = mesh + sid;

        //get the shape from the .nif
        mShapeLoader->load(outputstring,"General");
        BulletShapeManager::getSingletonPtr()->load(outputstring,"General");
        BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(outputstring,"General");

        adjustRigidBody(body, position, rotation, shape->mBoxTranslation * scale, shape->mBoxRotation);
    }

    // When this method is called, e.g. from  insertObject, the caller does not know whether
    // the object is a simple bullet shape (either primitives or compound) or a ragdoll
    // (e.g. Meshes\Architecture\Cathedral\Crypt\CathedralCryptLight02.NIF)
    //
    // MWClass::ForeignStatic::insertObject
    //   MWWorld::PhysicsSystem::addObject
    //     OEngine::Physic::PhysicEngine::createAndAdjustRigidBody   <------- (we are here)
    //       NifBullet::ManualBulletShapeLoader::load
    //         OEngine::Physic::BulletShapeManager::create
    //           Ogre::ResourceManager::createResource
    //                           :               :
    //                           :               :
    //                           : (callback)    :
    //                           :               v
    //                           :            OEngine::Physic::BulletShapeManager::createImpl
    //                           :               (creates a new BulletShape)
    //                           v
    //             NifBullet::ManualBulletShapeLoader::loadResource
    //                (loads the BulletShape created above)
    //
    //
    // We need a way to detect a ragdoll after these series of calls below:
    //
    //   mShapeLoader->load(outputstring,"General");
    //   BulletShapeManager::getSingletonPtr()->load(outputstring,"General");
    //   BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(outputstring,"General");
    //
    // But before calling:
    //
    //   RigidBody* body = new RigidBody(CI,name);
    //
    // Because for a ragdoll we'll have a number of rigid bodies and constraints (joints), not
    // just one.
    //
    // A quick hack is to place a flag in the BulletShape returned from the BulletShapeManager
    // to indicate what type it is.
    //
    // [Update]
    //
    // Actually the problem of detecting a ragdoll type occurs much earlier, while parsing the
    // NIF file.  The shapes and constraints specififed in the NIF need to be stored
    // differently to the "normal" shapes, so either BulletShape needs to be modified to suit
    // all types or a new class need to be used (maybe a subclass of BulletShape).
    //
    // The trouble is, the file has to be at least partially loaded/parsed before we know if it
    // has to be a ragdoll type.
    //
    // Ogre::ResourceManager calls OEngine::physic::BulletShapeManager::createImpl() to create a
    // new instance of BulletShape. This of course happens before the NIF file is loaded.  We
    // seem to have a some kind of circular dependency here.
    //
    // Because the resource is managed by Ogre, it is probably not a good idea to delete and
    // assign a new object on the fly.
    //
    // For now simply "bloat" BulletShape to include ragdoll related information.
    //
    RigidBody* PhysicEngine::createAndAdjustRigidBody(const std::string &mesh, Ogre::SceneNode *node,
        float scale, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
        Ogre::Vector3* scaledBoxTranslation, Ogre::Quaternion* boxRotation, bool raycasting, bool placeable)
    {
        std::string sid = (boost::format("%07.3f") % scale).str();
        std::string outputstring = mesh + sid;
        std::string name = node->getName();

        //get the shape from the .nif
        mShapeLoader->load(outputstring,"General");
        BulletShapeManager::getSingletonPtr()->load(outputstring,"General");
        BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(outputstring,"General");

        // TODO: add option somewhere to enable collision for placeable meshes

        // FIXME: maybe another compound shape needs to be created for raycasting?
        // e.g. to identify and pickup meshes\Clutter\UpperClass\UpperScales01.NIF
        if (shape->mIsRagdoll) // FIXME: this is an ugly hack
            return createRagdoll(shape, node, scale);

        if (placeable && !raycasting && shape->mCollisionShape)
            return NULL;

        if (!shape->mCollisionShape && !raycasting)
            return NULL;
        if (!shape->mRaycastingShape && raycasting)
            return NULL;

        btCollisionShape* collisionShape = raycasting ? shape->mRaycastingShape : shape->mCollisionShape;

        // If this is an animated compound shape, we must duplicate it so we can animate
        // multiple instances independently.
        if (!raycasting && !shape->mAnimatedShapes.empty())
            collisionShape = duplicateCollisionShape(collisionShape);
        if (raycasting && !shape->mAnimatedRaycastingShapes.empty())
            collisionShape = duplicateCollisionShape(collisionShape);

        collisionShape->setLocalScaling( btVector3(scale,scale,scale));

        //create the real body
        btRigidBody::btRigidBodyConstructionInfo CI = btRigidBody::btRigidBodyConstructionInfo
                (0,0, collisionShape);
        RigidBody* body = new RigidBody(CI,name);
        body->mPlaceable = placeable;

        if (!raycasting && !shape->mAnimatedShapes.empty())
        {
            AnimatedShapeInstance instance;
            instance.mAnimatedShapes = shape->mAnimatedShapes;
            instance.mCompound = collisionShape;
            mAnimatedShapes[body] = instance;
        }
        if (raycasting && !shape->mAnimatedRaycastingShapes.empty())
        {
            AnimatedShapeInstance instance;
            instance.mAnimatedShapes = shape->mAnimatedRaycastingShapes;
            instance.mCompound = collisionShape;
            mAnimatedRaycastingShapes[body] = instance;
        }

        if(scaledBoxTranslation != 0)
            *scaledBoxTranslation = shape->mBoxTranslation * scale;
        if(boxRotation != 0)
            *boxRotation = shape->mBoxRotation;

        adjustRigidBody(body, position, rotation, shape->mBoxTranslation * scale, shape->mBoxRotation);

        if (!raycasting)
        {
            assert (mCollisionObjectMap.find(name) == mCollisionObjectMap.end());
            mCollisionObjectMap[name] = body;
            mDynamicsWorld->addRigidBody(body,CollisionType_World,CollisionType_Actor|CollisionType_HeightMap);
        }
        else
        {
            assert (mRaycastingObjectMap.find(name) == mRaycastingObjectMap.end());
            mRaycastingObjectMap[name] = body;
            mDynamicsWorld->addRigidBody(body,CollisionType_Raycasting,CollisionType_Raycasting|CollisionType_Projectile);
            body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
        }

        return body;
    }

    // FIXME: need to think about creation and destruction of various objects so that memory
    // leaks do not occur - maybe we need a mRagdollObjectMap
    // FIXME: how to get a SceneNode for each RigidBody?
    //
    // FIXME: need to scale correctly for Bullet to work properly (e.g. not move as if in slow
    // motion).  Since Bullet considers 1 unit to be 1 meter, TES game units need to be divided
    // by 70.  Havok scale is already down by 7, so they need to be divided by 10.
    //
    // One of the smallest moving object would be a link in the chain for
    // CathedralCryptLight02. It has a radius of 0.27 Havok units and total length of
    // 2*0.27+2*0.0154=0.5708.  Dividing by 10, it would end up with a diameter of 0.054 m and
    // length of 0.057 m (or 5.4cm x 5.7cm).  This would be on the borderline of Havok's
    // accuracy and accoriding to this post, reduces performance:
    //
    // http://bulletphysics.org/Bullet/phpBB3/viewtopic.php?f=9&t=5037&p=18424&hilit=scale#p18424
    //
    // Using Havok's scale may be a reasonable compromise between realistic physics and system
    // performance.  TODO: verify by profiling
    //
    // With regards to scaling strategy, the wiki recommends:
    //
    // . Scale collision shapes about origin by X
    // . Scale all positions by X
    // . Scale all linear (but not angular) velocities by X
    // . Scale linear [Sleep Threshold] by X
    // . Scale gravity by X
    // . Scale all impulses you supply by X
    // . Scale all torques by X^2
    // . Scale all inertias by X if not computed by Bullet
    // . Damping is a ratio so this does not need to be changed.
    // . Angular velocity should not need to be changed either.
    //
    // Experiments with BenchmarkDemmo's RagDoll class modified to use the data from
    // CathedralCryptLight02, it appears that for Bullet:
    //
    //  . btCollisionShape's are created without transforms in the examples.  In relation to
    //    NIF, bhkTransformShape or bhkConvexTransformShape provide local transforms (e.g.
    //    rotating a box shape).
    //
    //  . btRigdBody's are created with btMotionState using the world transform which means
    //    when setWorldTransform is called the world transform needs to be translated to the
    //    child scene node's (i.e. somehow the parent scene node's transfrom needs to be taken
    //    away before calling Node::setPosition and Node::setOrientation)
    //
    //  . btConeTwistConstraint can be mapped to bhkRagdollConstraint, but it is unclear how
    //    planeA can be mapped to one of the swingSpans (or how to use planeB/twistB/maxFriction)
    //
    RigidBody *PhysicEngine::createRagdoll(BulletShapePtr shape, Ogre::SceneNode *node, float scale)
    {
#if 0
        std::map<int, std::vector<btTypedConstraint*> >::iterator it(mJoints.begin());
        for (; it != mJoints.end(); ++it)
        {
            for (unsigned int i = 0; i < it->second.size(); ++i)
                delete it->second[i];
        }
#endif
        //Ogre::Vector3 nodeV = node->getPosition();
        //Ogre::Quaternion nodeQ = node->getOrientation();

        std::map<int, btCollisionShape*>::iterator it(shape->mShapes.begin());
        for (; it != shape->mShapes.end(); ++it)
        {
            std::map<int, btRigidBody::btRigidBodyConstructionInfo>::iterator itCI(shape->mRigidBodyCI.find(it->first));
            if (itCI == shape->mRigidBodyCI.end())
                continue;

            // m_startWorldTransform includes the world transfrom of NiNode (i.e. incl. its
            // parents up to the root node), any from bhkRigidBodyT and any local transform of the shape.
            //
            // Rather than following the parent/child structure of the NIF file, each of these
            // shapes are all relative from the root.  i.e. they all share the same parent
            // scene node
            btVector3 v = itCI->second.m_startWorldTransform.getOrigin();
            btQuaternion q = itCI->second.m_startWorldTransform.getRotation();
            //std::cout << "shape " << it->second->getName() << ", x " << v.x() << ", y " << v.y() << ", z " << v.z() << std::endl;

            Ogre::SceneNode *childNode = node->createChildSceneNode(Ogre::Vector3(v.x(), v.y(), v.z()),
                    Ogre::Quaternion(q.w(), q.x(), q.y(), q.z()));

            // below are not needed if the child scene nodes are created with the position &
            // rotation
            //Ogre::SceneNode *childNode = node->createChildSceneNode();
            //childNode->translate(Ogre::Vector3(v.x(), v.y(), v.z()), Ogre::SceneNode::TS_PARENT);
            //childNode->rotate(Ogre::Quaternion(q.w(), q.x(), q.y(), q.z()), Ogre::SceneNode::TS_PARENT);
            // alternative experiment
            //childNode->setPosition(Ogre::Vector3(v.x(), v.y(), v.z()));
            //childNode->setOrientation(Ogre::Quaternion(q.w(), q.x(), q.y(), q.z()));

            Ogre::Vector3 cv = childNode->_getDerivedPosition();
            //std::cout << "derived shape " << /*it->second->getName()*/childNode->getName() << ", x " << cv.x << ", y " << cv.y << ", z " << cv.z << std::endl;
            Ogre::Quaternion cq = childNode->_getDerivedOrientation();
            btTransform trans(btQuaternion(cq.x, cq.y, cq.z, cq.w), btVector3(cv.x, cv.y, cv.z));

            //BtOgre::RigidBodyState *state = new BtOgre::RigidBodyState(childNode, itCI->second.m_startWorldTransform);
            // FIXME: the starting transforms seem to require the derived positions?
            BtOgre::RigidBodyState *state = new BtOgre::RigidBodyState(childNode, trans);
            itCI->second.m_motionState = state; // NOTE: dtor of RigidBody deletes RigidBodyState


            bool isDynamic = (itCI->second.m_mass != 0.f);
            //isDynamic = false;

            itCI->second.m_localInertia.setZero();
            if (isDynamic)
                itCI->second.m_collisionShape->calculateLocalInertia(itCI->second.m_mass, itCI->second.m_localInertia);
#if 0
            std::cout << "inertia x " << itCI->second.m_localInertia.x()
                << " y " << itCI->second.m_localInertia.y()
                << " z " << itCI->second.m_localInertia.z() << std::endl;
            btCapsuleShape* cs = dynamic_cast<btCapsuleShape*>(itCI->second.m_collisionShape);
            if (cs)
            {
                std::cout << "half height " << cs->getHalfHeight() << std::endl;
                std::cout << "up axis " << cs->getUpAxis() << std::endl;
                std::cout << "radius " << cs->getRadius() << std::endl;
            }
#endif


            RigidBody * body = new RigidBody(itCI->second, it->second->getName());

            body->setDamping(btScalar(0.05), btScalar(0.85));
            body->setDeactivationTime(btScalar(0.8));
            body->setSleepingThresholds(btScalar(1.6), btScalar(2.5));
            body->setActivationState(DISABLE_DEACTIVATION);


            //body->setGravity(btVector3(0, 0, -10));
            //btVector3 gravity = body->getGravity();
            //std::cout << "gravity " << it->second->getName() << ", x " << gravity.x() << ", y " << gravity.y() << ", z " << gravity.z() << std::endl;
            //body->applyGravity();
            shape->mBodies[it->first] = body;
            //std::cout << "shape " << it->second->getName() << " mass " << body->:w

            mDynamicsWorld->addRigidBody(body, CollisionType_World, /*CollisionType_World|*/CollisionType_Actor|CollisionType_Projectile);
            // FIXME: we have a memory leak here!!!

            if (shape->mJoints[it->first].empty())
                continue;

            btTransform localA, localB;
            Ogre::Vector4 pivot;

            localA.setIdentity();
            pivot = shape->mNifRagdollDesc[it->first].pivotA;
            localA.setOrigin(/*scale**/7*btVector3(btScalar(pivot.x), btScalar(pivot.y), btScalar(pivot.z)));
            localA.getBasis().setEulerZYX(0, 0, M_PI_2); // http://stackoverflow.com/questions/28485134/what-is-the-purpose-of-seteulerzyx-in-bullet-physics
            std::cout << "A pivot " << /*it->second->getName()*/childNode->getName() << ", x " << pivot.x << ", y " << pivot.y << ", z " << pivot.z << std::endl;

            int secondBody = shape->mJoints[it->first].back().second;

            std::map<int, btRigidBody::btRigidBodyConstructionInfo>::iterator itCI2(shape->mRigidBodyCI.find(secondBody));
            if (itCI2 == shape->mRigidBodyCI.end())
                continue; // FIXME: probably should log an error

            localB.setIdentity();
            pivot = shape->mNifRagdollDesc[it->first].pivotB;
            localB.setOrigin(/*scale**/7*btVector3(btScalar(pivot.x), btScalar(pivot.y), btScalar(pivot.z)));
            localB.getBasis().setEulerZYX(0, 0, M_PI_2); // rotate X axis 90 deg
            std::cout << "B pivot " << /*it->second->getName()*/childNode->getName() << ", x " << pivot.x << ", y " << pivot.y << ", z " << pivot.z << std::endl;

            btConeTwistConstraint *cons
                = new btConeTwistConstraint(*body, *shape->mBodies[secondBody], localA, localB);

            // FIXME: hack
            if (shape->mNifRagdollDesc[it->first].planeA.x == 0)
                cons->setLimit(shape->mNifRagdollDesc[it->first].coneMaxAngle*2,
                           shape->mNifRagdollDesc[it->first].planeMaxAngle*2,
                           shape->mNifRagdollDesc[it->first].twistMaxAngle*2);
            else
                cons->setLimit(shape->mNifRagdollDesc[it->first].planeMaxAngle*2,
                           shape->mNifRagdollDesc[it->first].coneMaxAngle*2,
                           shape->mNifRagdollDesc[it->first].twistMaxAngle*2);
            //std::cout << "cone angle " << shape->mNifRagdollDesc[it->first].coneMaxAngle*2 << std::endl;
            //m_joints[bhkRagdollConstraint49] = coneC;

#if 0
            btGeneric6DofConstraint *cons = new btGeneric6DofConstraint(*body, *shape->mBodies[secondBody],
                                         itCI->second.m_startWorldTransform,
                                         itCI2->second.m_startWorldTransform,
                                         true /*useLinearReferenceFrameA*/);

            //void setLinearLowerLimit(const btVector3& linearLower)
            //void setLinearUpperLimit(const btVector3& linearUpper)
            //void setAngularLowerLimit(const btVector3& angularLower)
            //void setAngularUpperLimit(const btVector3& angularUpper)

#endif
#if 0
            btPoint2PointConstraint *cons = new btPoint2PointConstraint(*body, *shape->mBodies[secondBody], body->getCenterOfMassPosition(), shape->mBodies[secondBody]->getCenterOfMassPosition());
#endif
            cons->setParam(BT_CONSTRAINT_STOP_ERP,0.8,0);
            cons->setParam(BT_CONSTRAINT_STOP_ERP,0.8,1);
            cons->setParam(BT_CONSTRAINT_STOP_ERP,0.8,2);
            cons->setParam(BT_CONSTRAINT_STOP_CFM,0.5,0);
            cons->setParam(BT_CONSTRAINT_STOP_CFM,0.5,1);
            cons->setParam(BT_CONSTRAINT_STOP_CFM,0.5,2);


            mDynamicsWorld->addConstraint(cons, /*disable collision between linke bodies*/true);
        }

        return nullptr;
    }

    void PhysicEngine::removeRigidBody(const std::string &name)
    {
        RigidBodyContainer::iterator it = mCollisionObjectMap.find(name);
        if (it != mCollisionObjectMap.end() )
        {
            RigidBody* body = it->second;
            if(body != NULL)
            {
                mDynamicsWorld->removeRigidBody(body);
            }
        }
        it = mRaycastingObjectMap.find(name);
        if (it != mRaycastingObjectMap.end() )
        {
            RigidBody* body = it->second;
            if(body != NULL)
            {
                mDynamicsWorld->removeRigidBody(body);
            }
        }
    }

    void PhysicEngine::deleteRigidBody(const std::string &name)
    {
        RigidBodyContainer::iterator it = mCollisionObjectMap.find(name);
        if (it != mCollisionObjectMap.end() )
        {
            RigidBody* body = it->second;

            if(body != NULL)
            {
                if (mAnimatedShapes.find(body) != mAnimatedShapes.end())
                    deleteShape(mAnimatedShapes[body].mCompound);
                mAnimatedShapes.erase(body);

                delete body;
            }
            mCollisionObjectMap.erase(it);
        }
        it = mRaycastingObjectMap.find(name);
        if (it != mRaycastingObjectMap.end() )
        {
            RigidBody* body = it->second;

            if(body != NULL)
            {
                if (mAnimatedRaycastingShapes.find(body) != mAnimatedRaycastingShapes.end())
                    deleteShape(mAnimatedRaycastingShapes[body].mCompound);
                mAnimatedRaycastingShapes.erase(body);

                delete body;
            }
            mRaycastingObjectMap.erase(it);
        }
        // FIXME
        //
        //it = mRagdollObjectMap.find(name);
        //if (it != mRagdollObjectMap.end() )
        //{
        //}
    }

    RigidBody* PhysicEngine::getRigidBody(const std::string &name, bool raycasting)
    {
        RigidBodyContainer* map = raycasting ? &mRaycastingObjectMap : &mCollisionObjectMap;
        RigidBodyContainer::iterator it = map->find(name);
        if (it != map->end() )
        {
            RigidBody* body = (*map)[name];
            return body;
        }
        else
        {
            return NULL;
        }
    }

    class ContactTestResultCallback : public btCollisionWorld::ContactResultCallback
    {
    public:
        std::vector<std::string> mResult;

        // added in bullet 2.81
        // this is just a quick hack, as there does not seem to be a BULLET_VERSION macro?
#if defined(BT_COLLISION_OBJECT_WRAPPER_H)
        virtual	btScalar addSingleResult(btManifoldPoint& cp,
                                            const btCollisionObjectWrapper* colObj0Wrap,int partId0,int index0,
                                            const btCollisionObjectWrapper* colObj1Wrap,int partId1,int index1)
        {
            const RigidBody* body = dynamic_cast<const RigidBody*>(colObj0Wrap->m_collisionObject);
            if (body && !(colObj0Wrap->m_collisionObject->getBroadphaseHandle()->m_collisionFilterGroup
                          & CollisionType_Raycasting))
                mResult.push_back(body->mName);

            return 0.f;
        }
#else
        virtual btScalar addSingleResult(btManifoldPoint& cp, const btCollisionObject* col0, int partId0, int index0,
                                         const btCollisionObject* col1, int partId1, int index1)
        {
            const RigidBody* body = dynamic_cast<const RigidBody*>(col0);
            if (body && !(col0->getBroadphaseHandle()->m_collisionFilterGroup
                          & CollisionType_Raycasting))
                mResult.push_back(body->mName);

            return 0.f;
        }
#endif
    };

    class DeepestNotMeContactTestResultCallback : public btCollisionWorld::ContactResultCallback
    {
        const std::string &mFilter;
        // Store the real origin, since the shape's origin is its center
        btVector3 mOrigin;

    public:
        const RigidBody *mObject;
        btVector3 mContactPoint;
        btScalar mLeastDistSqr;

        DeepestNotMeContactTestResultCallback(const std::string &filter, const btVector3 &origin)
          : mFilter(filter), mOrigin(origin), mObject(0), mContactPoint(0,0,0),
            mLeastDistSqr(std::numeric_limits<float>::max())
        { }

#if defined(BT_COLLISION_OBJECT_WRAPPER_H)
        virtual btScalar addSingleResult(btManifoldPoint& cp,
                                         const btCollisionObjectWrapper* col0Wrap,int partId0,int index0,
                                         const btCollisionObjectWrapper* col1Wrap,int partId1,int index1)
        {
            const RigidBody* body = dynamic_cast<const RigidBody*>(col1Wrap->m_collisionObject);
            if(body && body->mName != mFilter)
            {
                btScalar distsqr = mOrigin.distance2(cp.getPositionWorldOnA());
                if(!mObject || distsqr < mLeastDistSqr)
                {
                    mObject = body;
                    mLeastDistSqr = distsqr;
                    mContactPoint = cp.getPositionWorldOnA();
                }
            }

            return 0.f;
        }
#else
        virtual btScalar addSingleResult(btManifoldPoint& cp,
                                         const btCollisionObject* col0, int partId0, int index0,
                                         const btCollisionObject* col1, int partId1, int index1)
        {
            const RigidBody* body = dynamic_cast<const RigidBody*>(col1);
            if(body && body->mName != mFilter)
            {
                btScalar distsqr = mOrigin.distance2(cp.getPositionWorldOnA());
                if(!mObject || distsqr < mLeastDistSqr)
                {
                    mObject = body;
                    mLeastDistSqr = distsqr;
                    mContactPoint = cp.getPositionWorldOnA();
                }
            }

            return 0.f;
        }
#endif
    };


    std::vector<std::string> PhysicEngine::getCollisions(const std::string& name, int collisionGroup, int collisionMask)
    {
        RigidBody* body = getRigidBody(name);
        if (!body) // fall back to raycasting body if there is no collision body
            body = getRigidBody(name, true);
        ContactTestResultCallback callback;
        callback.m_collisionFilterGroup = collisionGroup;
        callback.m_collisionFilterMask = collisionMask;
        mDynamicsWorld->contactTest(body, callback);
        return callback.mResult;
    }


    std::pair<const RigidBody*,btVector3> PhysicEngine::getFilteredContact(const std::string &filter,
                                                                           const btVector3 &origin,
                                                                           btCollisionObject *object)
    {
        DeepestNotMeContactTestResultCallback callback(filter, origin);
        callback.m_collisionFilterGroup = CollisionType_Actor;
        callback.m_collisionFilterMask = CollisionType_World | CollisionType_HeightMap | CollisionType_Actor;
        mDynamicsWorld->contactTest(object, callback);
        return std::make_pair(callback.mObject, callback.mContactPoint);
    }

    void PhysicEngine::stepSimulation(double deltaT)
    {
        // This seems to be needed for character controller objects
        int subStep = mDynamicsWorld->stepSimulation(static_cast<btScalar>(deltaT), 10, 1 / 60.0f);
        if (subStep == 0)
            mDynamicsWorld->applyGravity();
            //std::cout << "no gravity" << std::endl;
        if(isDebugCreated)
        {
            mDebugDrawer->step();
        }
    }

    void PhysicEngine::addCharacter(const std::string &name, const std::string &mesh,
        const Ogre::Vector3 &position, float scale, const Ogre::Quaternion &rotation)
    {
        // Remove character with given name, so we don't make memory
        // leak when character would be added twice
        removeCharacter(name);

        PhysicActor* newActor = new PhysicActor(name, mesh, this, position, rotation, scale);

        mActorMap[name] = newActor;
    }

    void PhysicEngine::addForeignCharacter(const std::string &name, const std::string &mesh,
        const Ogre::Vector3 &position, float scale, const Ogre::Quaternion &rotation)
    {
        // Remove character with given name, so we don't make memory
        // leak when character would be added twice
        removeCharacter(name);

        PhysicActor* newActor = new ForeignActor(name, mesh, this, position, rotation, scale);

        mActorMap[name] = newActor;
    }

    void PhysicEngine::removeCharacter(const std::string &name)
    {
        PhysicActorContainer::iterator it = mActorMap.find(name);
        if (it != mActorMap.end() )
        {
            PhysicActor* act = it->second;
            if(act != NULL)
            {
                delete act;
            }
            mActorMap.erase(it);
        }
    }

    PhysicActor* PhysicEngine::getCharacter(const std::string &name)
    {
        PhysicActorContainer::iterator it = mActorMap.find(name);
        if (it != mActorMap.end() )
        {
            PhysicActor* act = mActorMap[name];
            return act;
        }
        else
        {
            return 0;
        }
    }

    std::pair<std::string,float> PhysicEngine::rayTest(const btVector3 &from, const btVector3 &to, bool raycastingObjectOnly, bool ignoreHeightMap, Ogre::Vector3* normal)
    {
        std::string name = "";
        float d = -1;

        btCollisionWorld::ClosestRayResultCallback resultCallback1(from, to);
        resultCallback1.m_collisionFilterGroup = 0xff;
        if(raycastingObjectOnly)
            resultCallback1.m_collisionFilterMask = CollisionType_Raycasting|CollisionType_Actor;
        else
            resultCallback1.m_collisionFilterMask = CollisionType_World;

        if(!ignoreHeightMap)
            resultCallback1.m_collisionFilterMask = resultCallback1.m_collisionFilterMask | CollisionType_HeightMap;
        mDynamicsWorld->rayTest(from, to, resultCallback1);
        if (resultCallback1.hasHit())
        {
            name = static_cast<const RigidBody&>(*resultCallback1.m_collisionObject).mName;
            d = resultCallback1.m_closestHitFraction;
            if (normal)
                *normal = Ogre::Vector3(resultCallback1.m_hitNormalWorld.x(),
                                        resultCallback1.m_hitNormalWorld.y(),
                                        resultCallback1.m_hitNormalWorld.z());
        }

        return std::pair<std::string,float>(name,d);
    }

    // callback that ignores player in results
    struct	OurClosestConvexResultCallback : public btCollisionWorld::ClosestConvexResultCallback
    {
    public:
        OurClosestConvexResultCallback(const btVector3&	convexFromWorld,const btVector3&	convexToWorld)
            : btCollisionWorld::ClosestConvexResultCallback(convexFromWorld, convexToWorld) {}

        virtual	btScalar	addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace)
        {
            if (const RigidBody* body = dynamic_cast<const RigidBody*>(convexResult.m_hitCollisionObject))
                if (body->mName == "player")
                    return 0;
            return btCollisionWorld::ClosestConvexResultCallback::addSingleResult(convexResult, normalInWorldSpace);
        }
    };

    std::pair<bool, float> PhysicEngine::sphereCast (float radius, btVector3& from, btVector3& to)
    {
        OurClosestConvexResultCallback callback(from, to);
        callback.m_collisionFilterGroup = 0xff;
        callback.m_collisionFilterMask = OEngine::Physic::CollisionType_World|OEngine::Physic::CollisionType_HeightMap;

        btSphereShape shape(radius);
        const btQuaternion btrot(0.0f, 0.0f, 0.0f);

        btTransform from_ (btrot, from);
        btTransform to_ (btrot, to);

        mDynamicsWorld->convexSweepTest(&shape, from_, to_, callback);

        if (callback.hasHit())
            return std::make_pair(true, callback.m_closestHitFraction);
        else
            return std::make_pair(false, 1.0f);
    }

    std::vector< std::pair<float, std::string> > PhysicEngine::rayTest2(const btVector3& from, const btVector3& to, int filterGroup)
    {
        MyRayResultCallback resultCallback1;
        resultCallback1.m_collisionFilterGroup = filterGroup;
        resultCallback1.m_collisionFilterMask = CollisionType_Raycasting|CollisionType_Actor|CollisionType_HeightMap;
        mDynamicsWorld->rayTest(from, to, resultCallback1);
        std::vector< std::pair<float, const btCollisionObject*> > results = resultCallback1.results;

        std::vector< std::pair<float, std::string> > results2;

        for (std::vector< std::pair<float, const btCollisionObject*> >::iterator it=results.begin();
            it != results.end(); ++it)
        {
            results2.push_back( std::make_pair( (*it).first, static_cast<const RigidBody&>(*(*it).second).mName ) );
        }

        std::sort(results2.begin(), results2.end(), MyRayResultCallback::cmp);

        return results2;
    }

    void PhysicEngine::getObjectAABB(const std::string &mesh, float scale, btVector3 &min, btVector3 &max)
    {
        std::string sid = (boost::format("%07.3f") % scale).str();
        std::string outputstring = mesh + sid;

        mShapeLoader->load(outputstring, "General");
        BulletShapeManager::getSingletonPtr()->load(outputstring, "General");
        BulletShapePtr shape =
            BulletShapeManager::getSingleton().getByName(outputstring, "General");

        btTransform trans;
        trans.setIdentity();

        if (shape->mRaycastingShape)
            shape->mRaycastingShape->getAabb(trans, min, max);
        else if (shape->mCollisionShape)
            shape->mCollisionShape->getAabb(trans, min, max);
        else
        {
            min = btVector3(0,0,0);
            max = btVector3(0,0,0);
        }
    }

    int PhysicEngine::toggleDebugRendering(Ogre::SceneManager *sceneMgr)
    {
        if(!sceneMgr)
            return 0;

        std::map<Ogre::SceneManager *, BtOgre::DebugDrawer *>::iterator iter =
            mDebugDrawers.find(sceneMgr);
        if(iter != mDebugDrawers.end()) // found scene manager
        {
            if((*iter).second)
            {
                // set a new drawer each time (maybe with a different scene manager)
                mDynamicsWorld->setDebugDrawer(mDebugDrawers[sceneMgr]);
                if(!mDebugDrawers[sceneMgr]->getDebugMode())
                    mDebugDrawers[sceneMgr]->setDebugMode(1 /*mDebugDrawFlags*/);
                else
                    mDebugDrawers[sceneMgr]->setDebugMode(0);
                mDynamicsWorld->debugDrawWorld();

                return mDebugDrawers[sceneMgr]->getDebugMode();
            }
        }
        return 0;
    }

    void PhysicEngine::stepDebug(Ogre::SceneManager *sceneMgr)
    {
        if(!sceneMgr)
            return;

        std::map<Ogre::SceneManager *, BtOgre::DebugDrawer *>::iterator iter =
            mDebugDrawers.find(sceneMgr);
        if(iter != mDebugDrawers.end()) // found scene manager
        {
            if((*iter).second)
                (*iter).second->step();
            else
                return;
        }
    }

    void PhysicEngine::createDebugDraw(Ogre::SceneManager *sceneMgr)
    {
        if(mDebugDrawers.find(sceneMgr) == mDebugDrawers.end())
        {
            mDebugSceneNodes[sceneMgr] = sceneMgr->getRootSceneNode()->createChildSceneNode();
            mDebugDrawers[sceneMgr] = new BtOgre::DebugDrawer(mDebugSceneNodes[sceneMgr], mDynamicsWorld);
            mDebugDrawers[sceneMgr]->setDebugMode(0);
        }
    }

    void PhysicEngine::removeDebugDraw(Ogre::SceneManager *sceneMgr)
    {
        std::map<Ogre::SceneManager *, BtOgre::DebugDrawer *>::iterator iter =
            mDebugDrawers.find(sceneMgr);
        if(iter != mDebugDrawers.end())
        {
            delete (*iter).second;
            mDebugDrawers.erase(iter);
        }

        std::map<Ogre::SceneManager *, Ogre::SceneNode *>::iterator it =
            mDebugSceneNodes.find(sceneMgr);
        if(it != mDebugSceneNodes.end())
        {
            std::string sceneNodeName = (*it).second->getName();
            if(sceneMgr->hasSceneNode(sceneNodeName))
                sceneMgr->destroySceneNode(sceneNodeName);
        }
    }

}
}
