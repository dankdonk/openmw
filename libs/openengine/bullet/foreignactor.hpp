#ifndef OENGINE_BULLET_FOREIGNACTOR_H
#define OENGINE_BULLET_FOREIGNACTOR_H

#include <string>
#include <list>
#include <map>

#include <boost/shared_ptr.hpp>

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "BulletShapeLoader.h"
#include "BulletCollision/CollisionShapes/btScaledBvhTriangleMeshShape.h"

#include "physic.hpp"

class btRigidBody;
class btBroadphaseInterface;
class btDefaultCollisionConfiguration;
class btSequentialImpulseConstraintSolver;
class btCollisionDispatcher;
class btDiscreteDynamicsWorld;

namespace BtOgre
{
    class DebugDrawer;
}

namespace Ogre
{
    class SceneManager;
}

namespace MWWorld
{
    class World;
}

namespace OEngine {
namespace Physic
{

    class ForeignActor: public PhysicActor
    {
    public:
        ForeignActor(const std::string &name, const std::string &mesh, PhysicEngine *engine,
                const Ogre::Vector3 &position, const Ogre::Quaternion &rotation, float scale);

        ~ForeignActor();

    private:

        ForeignActor(const ForeignActor&);
        ForeignActor& operator=(const ForeignActor&);
    };
}}

#endif
