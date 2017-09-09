#include "physic.hpp"

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <BulletDynamics/ConstraintSolver/btTypedConstraint.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <OgreSceneManager.h>
#include <OgreEntity.h>

#include <components/nifbullet/bulletnifloader.hpp>
#include <components/misc/stringops.hpp>

#include "BtOgrePG.h"
#include "BtOgreGP.h"
#include "BtOgreExtras.h"

#include "foreignactor.hpp"

#ifndef M_PI_2
#define M_PI_2     btScalar(1.57079632679489661923) // FIXME: temp while testing ragdoll
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

        mDynamicsWorld->setGravity(btVector3(0,0,-72));

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
    // FIXME: maybe another compound shape needs to be created for raycasting?
    // e.g. to identify and pickup meshes\Clutter\UpperClass\UpperScales01.NIF
    RigidBody* PhysicEngine::createAndAdjustRagdollBody(const std::string &mesh, Ogre::SceneNode *node,
        const std::unordered_multimap<size_t, Ogre::Entity*>& ragdollEntitiesMap,
        float scale, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
        Ogre::Vector3* scaledBoxTranslation, Ogre::Quaternion* boxRotation, bool raycasting, bool placeable)
    {
        std::string sid = (boost::format("%07.3f") % scale).str();
        std::string outputstring = mesh + sid;

        //get the shape from the .nif
        mShapeLoader->load(outputstring,"General");
        BulletShapeManager::getSingletonPtr()->load(outputstring,"General");
        BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(outputstring,"General");

        // TODO: add option somewhere to enable collision for placeable meshes

        // FIXME: maybe another compound shape needs to be created for raycasting?
        // e.g. to identify and pickup meshes\Clutter\UpperClass\UpperScales01.NIF
        //if (shape->mIsRagdoll) // FIXME: this is an ugly hack
            return createRagdoll(shape, node, ragdollEntitiesMap, scale);
    }

    RigidBody* PhysicEngine::createAndAdjustRigidBody(const std::string &mesh, const std::string &name,
        float scale, const Ogre::Vector3 &position, const Ogre::Quaternion &rotation,
        Ogre::Vector3* scaledBoxTranslation, Ogre::Quaternion* boxRotation, bool raycasting, bool placeable)
    {
        std::string sid = (boost::format("%07.3f") % scale).str();
        std::string outputstring = mesh + sid;

        //get the shape from the .nif
        mShapeLoader->load(outputstring,"General");
        BulletShapeManager::getSingletonPtr()->load(outputstring,"General");
        BulletShapePtr shape = BulletShapeManager::getSingleton().getByName(outputstring,"General");

        // TODO: add option somewhere to enable collision for placeable meshes

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

    // Memory Leak
    // ===========
    //
    // FIXME: need to think about creation and destruction of various objects so that memory
    // leaks do not occur - maybe we need a mRagdollObjectMap
    //
    //
    // Scaling
    // =======
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
    //  "...discovered that the performance of the collision was largely affected by the scale
    //  (measuring units). Basically: if I simulated 20'000 spheres each with 0.005 diameter,
    //  the RAM requirement was very high, whereas the same exact problem with rescaled sizing,
    //  say 0.5 diameter for spheres, was optimal.
    //
    //  This was not related to dynamics, it was related only to the broadphase collision: the
    //  broadphase created way too many manifolds.
    //
    //  ...in the btPersistentManifold.cpp there is an hardcoded threshold of 0.02"
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
    //
    // Transforms
    // ==========
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
    RigidBody *PhysicEngine::createRagdoll(BulletShapePtr shape, Ogre::SceneNode *node,
            const std::unordered_multimap<size_t, Ogre::Entity*>& ragdollEntitiesMap, float scale)
    {
        //std::cout << "scene node " << node->getName() << std::endl;

        bool isDynamic = false;
        std::map<size_t, btRigidBody*>  rigidBodies; // keep track of bodies for setting up constraints

        // for each of the collision shapes in the ragdoll object
        std::map<size_t, btCollisionShape*>::iterator it(shape->mShapes.begin());
        for (; it != shape->mShapes.end(); ++it)
        {
            // find the corresponding btRigidBodyConstructionInfo
            // ----------------------------------------------------------------
            size_t recIndex = it->first; // bhkRegidBody's recIndex
            typedef std::map<size_t, btRigidBody::btRigidBodyConstructionInfo>::iterator ConstructionInfoIter;
            ConstructionInfoIter itCI(shape->mRigidBodyCI.find(recIndex));
            if (itCI == shape->mRigidBodyCI.end())
                continue; // FIXME: shouldn't happen, so probably best to throw here

            // create a child SceneNode so that each shape can be moved in relation to the base
            // ----------------------------------------------------------------
            // NOTE: m_startWorldTransform includes the world transfrom of NiNode (i.e. incl. its
            // parents up to the root node), any transform from bhkRigidBodyT and any local
            // transform of the shape.
            //
            // NOTE: Rather than following the parent/child structure of the NIF file, each of these
            // shapes are all relative from the same 'root'.  i.e. they all share the same parent
            // SceneNode (this is also how Bullet's btMotionState works, I think)
            //
            // FIXME: the Entities have the mesh built with their verticies already including
            // the NiNode transforms (unlike the collision shapes).  But somehow detaching and
            // re-attaching makes them 'lose' this?
            btVector3 v = itCI->second.m_startWorldTransform.getOrigin();
            btQuaternion q = itCI->second.m_startWorldTransform.getRotation();

            Ogre::SceneNode *childNode = node->createChildSceneNode(Ogre::Vector3::ZERO, Ogre::Quaternion::IDENTITY);
            //node->requestUpdate(childNode); // FIXME: is this needed?
#if 0
            Ogre::SceneNode *childNode = node->createChildSceneNode(Ogre::Vector3(v.x(), v.y(), v.z()),
                    Ogre::Quaternion(q.w(), q.x(), q.y(), q.z()));
#endif

            std::string meshName; // FIXME: temp testing

            // move each Ogre::Entity associated with a bhkRigidBody to the child SceneNode
            // which will be controlled by Bullet via RigidBodyState
            // ----------------------------------------------------------------
            isDynamic = (itCI->second.m_mass != 0.f);
            if (isDynamic) // do only for moving bkhRigidBody
            {
                // for each of the entities for a recindex (node)
                typedef std::unordered_multimap<size_t, Ogre::Entity*>::const_iterator BodyIndexMapIter;
                std::pair<BodyIndexMapIter, BodyIndexMapIter> range = ragdollEntitiesMap.equal_range(recIndex);
                for (BodyIndexMapIter itBody = range.first; itBody != range.second; ++itBody)
                {
                    Ogre::MovableObject *ent = static_cast<Ogre::MovableObject*>(itBody->second);

                    //meshName = itBody->second->getMesh()->getName(); // FIXME: temp testing

                    //if (itBody->second->getMesh()->getName() ==
                        //"meshes\\clutter\\fightersguild\\targetheavy01.nif@index=25@shape=targetchainright01:1")
                    //if (itCI->second.m_mass == 50.f && itCI->second.m_collisionShape->getName() == "Box")
                    {
                        // node 89
                        // X 0.3670 Y -0.3900 Z -0.5653
                        // Y -90.00 P 0.00 R -90.00
                        //
                        // bhkRigidBodyT recIndex 92 (= 0x5c)
                        // X 0.0161 Y -0.0532 Z 0.0449 W 0.0000
                        // Y -0.00 P 0.00 R -0.00
                        //
                        // transform shape->mRigidBodyCI[0x5c]
                        // -0.452738285  -0.00545939803  -0.0754421949
                        std::cout << itBody->second->getMesh()->getName() << std::endl;
                        //"meshes\\clutter\\fightersguild\\targetheavy01.nif@index=123@shape=targetheavytarget:0"
                        //"meshes\\clutter\\fightersguild\\targetheavy01.nif@index=123@shape=targetheavytarget:1"
                        //"meshes\\clutter\\fightersguild\\targetheavy01.nif@index=123@shape=targetheavytarget:2"
                        //"meshes\\clutter\\fightersguild\\targetheavy01.nif@index=123@shape=targetheavytarget:3"
                        //"meshes\\clutter\\fightersguild\\targetheavy01.nif@index=123@shape=targetheavytarget:4"
                        Ogre::Vector3 cv = childNode->_getDerivedPosition();
                        std::cout << "cv x " << cv.x << ", y " << cv.y << ", z " << cv.z << std::endl;
                        cv = node->getPosition();
                        std::cout << "parent x " << cv.x << ", y " << cv.y << ", z " << cv.z << std::endl;
                        //    cv x 1554.67, y 2494.1,  z 7527.72
                        //parent x 1554.68, y 2493.82, z 7527.77
                    }

                    // If below two lines are commented out, the chains of TargetHeavy01.NIF
                    // are rendered in the correct place. That suggests incorrect
                    // itCI->second.m_startWorldTransform, possibly calculation issue with the
                    // capsule shapes.
                    //
                    // After creating the shape:
                    //                       (q).w 1.00000000
                    // (v).x -0.0658496469   (q).x 0.000000000
                    // (v).y -0.108871818    (q).y 0.000000000
                    // (v).z -5.77761507     (q).z 0.000000000
                    //
                    // But then, with itCI->second.m_mass set at zero , all the physics objects
                    // are in the right place.  (see ManualBulletShapeLoader::handleBhkCollisionObject)
                    //
                    // Maybe Bullet is having an impact?
                    //ent->getParentSceneNode()->detachObject(ent);
                    //childNode->attachObject(ent); // Ogre calls _notifyAttached() and needUpdate() internally
                    std::cout << "entity name " << ent->getName() << std::endl;
                }
            }
            // ((*((MWWorld::LiveCellRef<ESM4::Static>*)((ptr).mRef))).mBase)->mModel = "Clutter\\FightersGuild\\TargetHeavy01.NIF"
            // ((*((MWWorld::LiveCellRef<ESM4::Static>*)((ptr).mRef))).mBase)->mEditorId = "TargetHeavy01"
            // ((((ptr).mRef)->mRef).mCellRef).mRefID = "00000CED"
            // ptr.mRef->mRef.mCellRef.mPos.pos[0] = 1554.67578
            // ptr.mRef->mRef.mCellRef.mPos.pos[1] = 2493.81860
            // ptr.mRef->mRef.mCellRef.mPos.pos[2] = 7527.77197
            // ptr.mRef->mRef.mCellRef.mPos.rot[0] = 0.000000000
            // ptr.mRef->mRef.mCellRef.mPos.rot[1] = -0.000000000
            // ptr.mRef->mRef.mCellRef.mPos.rot[2] = 1.57079637
            // ptr.mRef->mRef.mCellRef.mScale      = 0.629999995
#if 0
            else
            {
                std::unordered_multimap<size_t, Ogre::Entity*>::const_iterator itEnt = ragdollEntitiesMap.find(recIndex);

                if (itEnt != ragdollEntitiesMap.end() && itEnt->second && !itEnt->second->getMesh().isNull() && itEnt->second->getMesh()->getName() == "HeavyTargetStructure02:2")
                {
                    Ogre::Vector3 cv = childNode->_getDerivedPosition();
                    std::cout << "cv x " << cv.x << ", y " << cv.y << ", z " << cv.z << std::endl;
                }
            }
#endif


            Ogre::Vector3 pps = node->getScale(); // FIXME: experiment



            // create a controller for the child SceneNode (i.e. RigidBodyState)
            // ----------------------------------------------------------------
            Ogre::Vector3 cv = childNode->_getDerivedPosition();
            Ogre::Quaternion cq = childNode->_getDerivedOrientation();

#if 0

            // world transform of child node
            Ogre::Matrix4 childTrans;
            childTrans.makeTransform(cv, pps, cq);

            // local transform of the object
            btVector3 stv = itCI->second.m_startWorldTransform.getOrigin();
            btQuaternion stq = itCI->second.m_startWorldTransform.getRotation();
            Ogre::Matrix4 originalTrans;
            originalTrans.makeTransform(Ogre::Vector3(stv.x(), stv.y(), stv.z()), pps, Ogre::Quaternion(stq.w(), stq.x(), stq.y(), stq.z()));


            Ogre::Vector3 ov = originalTrans.getTrans();
            Ogre::Quaternion oq = originalTrans.extractQuaternion();


            //childTrans = childTrans * originalTrans;
            //trans = trans * itCI->second.m_startWorldTransform;

            cv = childTrans.getTrans();
            cq = childTrans.extractQuaternion();

            btTransform originalTransform(btQuaternion(oq.x, oq.y, oq.z, oq.w), btVector3(ov.x, ov.y, ov.z));

#endif




            // local transform of the object
            btQuaternion stq = itCI->second.m_startWorldTransform.getRotation();
            btVector3 stv = itCI->second.m_startWorldTransform.getOrigin();

                    std::cout << "recIndex " << recIndex << " startWorldTransform x "
                              << stv.x() << ", y " << stv.y() << ", z " << stv.z() << std::endl;

            // derived rotation
            Ogre::Quaternion dq = cq * Ogre::Quaternion(stq.w(), stq.x(), stq.y(), stq.z());
            // derived position
            Ogre::Vector3 dv =  cq * (pps * Ogre::Vector3(stv.x(), stv.y(), stv.z()));

            btTransform originalTransform(stq, btVector3(dv.x, dv.y, dv.z));

            dv += cv;

            // make transform
            btTransform trans(btQuaternion(dq.x, dq.y, dq.z, dq.w), btVector3(dv.x, dv.y, dv.z));






            //btTransform trans(btQuaternion(cq.x, cq.y, cq.z, cq.w), btVector3(cv.x, cv.y, cv.z));

            // node, start transform, offset transform
            //BtOgre::RigidBodyState *state = new BtOgre::RigidBodyState(childNode, itCI->second.m_startWorldTransform, originalTransform);
            //BtOgre::RigidBodyState *state = new BtOgre::RigidBodyState(childNode, trans, /*itCI->second.m_startWorldTransform*/originalTransform);
//#if 0
            BtOgre::RigidBodyState *state = new BtOgre::RigidBodyState(childNode, trans);
//#endif
            itCI->second.m_motionState = state; // NOTE: dtor of RigidBody deletes RigidBodyState
            //itCI->second.m_startWorldTransform = trans;
            // setup the rest of the construction info
            // ----------------------------------------------------------------
            // TargetHeavy01.NIF in ICMarketDistrictAFightingChanceBasement needs reduced scale from the node
            //
            // (called from scene.cpp InsertFunctor)
            //
            // FIXME: Should ForeignStatic reimplement:
            //
            // void Class::adjustScale(const MWWorld::Ptr& ptr,float& scale) const
            // {
            // }
            //
            // In comparison:
            //
            // void RenderingManager::scaleObject (const MWWorld::Ptr& ptr, const Ogre::Vector3& scale)
            // {
            //     ptr.getRefData().getBaseNode()->setScale(scale);
            // }
            //
            // But how will Ptr have access to the physics objects to scale them, anyway?
            Ogre::Vector3 ps = node->getScale(); // FIXME: experiment
            if (ps == Ogre::Vector3::ZERO) // FIXME: doesn't work if only one of them is zero
                ps = Ogre::Vector3(1.f, 1.f, 1.f);
            if (ps.x == 0 || ps.y == 0 || ps.z == 0)
                throw std::logic_error(std::string("zero scale "));
            //if (itCI->second.m_collisionShape->getName() == "CapsuleZ" /*&& itCI->second.m_mass == 50.f*/)
                //std::cout << "break" << std::endl;
            itCI->second.m_collisionShape->setLocalScaling(btVector3(ps.x, ps.y, ps.z));

            // FIXME: use the inertia matrix in bhkRigidBody instead?
            itCI->second.m_localInertia.setZero();
            if (isDynamic)
                itCI->second.m_collisionShape->calculateLocalInertia(itCI->second.m_mass,
                                                                     itCI->second.m_localInertia);

            // finally create the rigid body
            // ----------------------------------------------------------------
            RigidBody * body = new RigidBody(itCI->second, /*it->second->getName()*/meshName);
            //"meshes\\clutter\\fightersguild\\targetheavy01.nif@index=25@shape=targetchainright01:1")
            //(((((state)->mTransform).m_origin).mVec128).m128_f32)[0x00000000] 1586.31482
            //(((((state)->mTransform).m_origin).mVec128).m128_f32)[0x00000001] 2493.89404
            //(((((state)->mTransform).m_origin).mVec128).m128_f32)[0x00000002] 7564.13623

            // FIXME: use the damping values in bhkRigidBody instead?  (how do they relate to
            // Bullet's values, anyway?)
            body->setDamping(btScalar(0.05), btScalar(0.85)); // FIXME Was 0.05/0.85
            body->setDeactivationTime(btScalar(0.8));
            body->setSleepingThresholds(btScalar(1.6), btScalar(2.5));
            body->setActivationState(DISABLE_DEACTIVATION);
            body->setLinearFactor(btVector3(0.1,0.1,0.1)); // FIXME: temp testing
            body->setAngularFactor(btVector3(0.1,0.1,0.1)); // FIXME: temp testing

            // NOTE: NIF files (fortunately) refer to bhkRigidBody's already specified
            // a pointer to this RigdBody may be needed later for setting up constraints
            rigidBodies[recIndex] = body;

            // check if the child SceneNode already has a RigidBody (shouldn't happen)
            std::string name = childNode->getName();
            if (mCollisionObjectMap.find(name) != mCollisionObjectMap.end())
                std::cout << "break" << std::endl; // FIXME needs better error handlin

            // Each of the RigidBody's pointers are kept in mCollisionObjectMap
            //
            // MWWorld::Scene::unloadCell
            //   PhysicsSystem::removeObject      (for ptr.getRefData().getBaseNode() and its children)
            //     OEngine::Physic::PhysicEngine::removeCharacter
            //     OEngine::Physic::PhysicEngine::removeRigidBody
            //       mDynamicsWorld->removeRigidBody
            //     OEngine::Physic::PhysicEngine::deleteRigidBody
            //       mAnimatedShapes.erase(body);
            //       delete body;                <======== RigidBody and RigidBodyState are ok,
            //                                             delete the constraints here as well
            //                                           (shapes are deleted in BulletShape dtor)
            //
            mCollisionObjectMap[name] = body; // keep a pointer for deleting RigidBody later

            mDynamicsWorld->addRigidBody(body,
                    CollisionType_World, CollisionType_HeightMap|CollisionType_Actor|CollisionType_Projectile);

            // create the constraints
            // ----------------------------------------------------------------
            // itJoint->first               : recIndex of the RigidBody that has the Constraint
            // itJoint->second.at(i).first  : recIndex of self (should be the same as above)
            // itJoint->second.at(i).second : recIndex of the other RigidBody
            //
            // If a RigidBody has multiple Constraints, how to identify which one is which?
            // Or does that matter?
            //
            std::map<size_t, std::vector<std::pair<size_t, size_t> > >::const_iterator itJoint
                = shape->mJoints.find(recIndex);
            if (itJoint == shape->mJoints.end() || itJoint->second.empty())
                continue; // FIXME: probably should log an error

            size_t secondBody = 0;
            for (unsigned int i = 0; i < itJoint->second.size(); ++i)
            {
                // FIXME: assumed the first one of the pair is always the current RigidBody
                // should at least check itJoint->second.at(i).first == recIndex
                secondBody = itJoint->second.at(i).second;

                btTransform localA, localB;
                Ogre::Vector4 pivot;
                std::map<std::pair<size_t, size_t>, Nif::RagdollDescriptor>::const_iterator itJointDesc
                    = shape->mNifRagdollDesc.find(std::make_pair(recIndex, secondBody));
                if (itJointDesc == shape->mNifRagdollDesc.end())
                    continue; // FIXME: probably should log an error

                // NOTE: the sizes below are from the NIF files before any scaling (Havok or node) are applied.
                //
                // CathedralCryptLight02 chains have the pivot points spcified from the center of
                // the cylinder shapes.  e.g. bhkRagdollConstraint (refIndex 94) has pivotA Z value
                // of 0.420 and pivotB Z value of -0.420.  With the capsule's total height
                // 0.2700*2+0.0153*2=0.5706 or half height of 0.2853. The pivot points are
                // 0.420-0.2853=0.1347 outside the capsule shapes.
                //
                // However, TargetHeavy01 chains have the pivoit points at the center of the
                // capsule shapes. e.g. bhkRagdollConstraint (refIndex 38) pivotA has x=y=z=0. The
                // capsule size is 2*0.2858+(0.9542-0.4935)=1.0323 or half height of 0.51615. With
                // these values Bullet physics seems to go bezerk.
                //
                // pivotA is usually (always?) the lower one, so we can try to add
                localA.setIdentity();
                pivot = itJointDesc->second.pivotA;
                //if (pivot.x == 0 && pivot.y == 0 && pivot.z == 0)
                    //pivot.z = 3; // FIXME
                localA.setOrigin(/*scale**/btVector3(btScalar(pivot.x*ps.x), btScalar(pivot.y*ps.y), btScalar(pivot.z*ps.z)));
                //localA.setOrigin(/*scale**/btVector3(btScalar(pivot.x), btScalar(pivot.y), btScalar(pivot.z)));
                // http://stackoverflow.com/questions/28485134/what-is-the-purpose-of-seteulerzyx-in-bullet-physics
                // Don't really understand why this is needed, especilly why the Y axis?
                //localA.getBasis().setEulerZYX(0, M_PI_2, 0); // rotate Y axis 90 deg

                //if (recIndex == 9 || recIndex == 15 || recIndex == 31 || recIndex == 37 || recIndex == 92)
                    std::cout << "recIndex " << recIndex << " pivot A x "
                              << pivot.x << ", y " << pivot.y << ", z " << pivot.z << std::endl;



                localB.setIdentity();
                pivot = itJointDesc->second.pivotB;
                localB.setOrigin(/*scale**/btVector3(btScalar(pivot.x*ps.x), btScalar(pivot.y*ps.y), btScalar(pivot.z*ps.z)));
                //localB.setOrigin(/*scale**/btVector3(btScalar(pivot.x), btScalar(pivot.y), btScalar(pivot.z)));
                //localB.getBasis().setEulerZYX(0, M_PI_2, 0);
                //if (recIndex == 9 || recIndex == 15 || recIndex == 31 || recIndex == 37 || recIndex == 92)
                    std::cout << "recIndex " << recIndex << " pivot B x "
                              << pivot.x << ", y " << pivot.y << ", z " << pivot.z << std::endl;

            //BtOgre::RigidBodyState *state = new BtOgre::RigidBodyState(childNode, itCI->second.m_startWorldTransform, originalTransform);
#if 0
                btConeTwistConstraint *cons
                    = new btConeTwistConstraint(*body, *rigidBodies[secondBody], localA, localB);
                mRagdollConstraintMap.insert(std::make_pair(body, cons));

                // FIXME: hack (don't know how to apply the plane concept to btConeTwistConstraint)
                if (itJointDesc->second.planeA.x == 0)
                    cons->setLimit(itJointDesc->second.coneMaxAngle*2,
                                   itJointDesc->second.planeMaxAngle*2,
                                   itJointDesc->second.twistMaxAngle*2);
                else
                    cons->setLimit(itJointDesc->second.planeMaxAngle*2,
                                   itJointDesc->second.coneMaxAngle*2,
                                   itJointDesc->second.twistMaxAngle*2);

#endif
                btGeneric6DofConstraint *cons
                    = new btGeneric6DofConstraint(*body, *rigidBodies[secondBody], localA, localB, false);
                mRagdollConstraintMap.insert(std::make_pair(body, cons));

#if 0
                Ogre::Vector4 v = itJointDesc->second.planeB;
                btVector3 plane(v.x, v.y, v.z);
                v = itJointDesc->second.twistB;
                btVector3 twist(v.x, v.y, v.z);

                btVector3 cone = twist.cross(plane);

                cons->setAngularLowerLimit(-itJointDesc->second.coneMaxAngle*cone+
                                           itJointDesc->second.planeMinAngle*plane+
                                           itJointDesc->second.twistMinAngle*twist);
                cons->setAngularUpperLimit(itJointDesc->second.coneMaxAngle*cone+
                                           itJointDesc->second.planeMaxAngle*plane+
                                           itJointDesc->second.twistMaxAngle*twist);

#endif
//#if 0
                // FIXME: need to tune these values
                cons->setParam(BT_CONSTRAINT_STOP_ERP,0.8f,0);
                cons->setParam(BT_CONSTRAINT_STOP_ERP,0.8f,1);
                cons->setParam(BT_CONSTRAINT_STOP_ERP,0.8f,2);
                cons->setParam(BT_CONSTRAINT_STOP_CFM,0.f,0);
                cons->setParam(BT_CONSTRAINT_STOP_CFM,0.f,1);
                cons->setParam(BT_CONSTRAINT_STOP_CFM,0.f,2);
//#endif

                mDynamicsWorld->addConstraint(cons, /*disable collision between linked bodies*/true);
            }
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
                // remove all constraints associated with the RigidBody
                typedef std::unordered_multimap<RigidBody*, btTypedConstraint*>::const_iterator ConstraintMapIter;
                std::pair<ConstraintMapIter, ConstraintMapIter> range = mRagdollConstraintMap.equal_range(body);
                for (ConstraintMapIter itConst = range.first; itConst != range.second; ++itConst)
                    mDynamicsWorld->removeConstraint(itConst->second);

                // remove the RigidBody
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

                // delete the constraints first
                typedef std::unordered_multimap<RigidBody*, btTypedConstraint*>::const_iterator ConstraintMapIter;
                std::pair<ConstraintMapIter, ConstraintMapIter> range = mRagdollConstraintMap.equal_range(body);
                for (ConstraintMapIter itConst = range.first; itConst != range.second; ++itConst)
                {
                    delete itConst->second;
                    mRagdollConstraintMap.erase(body);
                }

                delete body;

                // NOTE: OEngine::Physic::BulletShape::~BulletShape deletes the ragdoll shapes
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
