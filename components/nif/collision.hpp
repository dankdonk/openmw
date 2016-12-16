#ifndef OPENMW_COMPONENTS_NIF_COLLISION_HPP
#define OPENMW_COMPONENTS_NIF_COLLISION_HPP

#include <OgreVector3.h>

#include "record.hpp"
#include "recordptr.hpp"

namespace Nif
{
    class bhkSerializable : public Record {};

    class bhkShape : public bhkSerializable {};

    struct hkTriangle
    {
        std::vector<short> triangle;
        unsigned short weldingInfo;
        Ogre::Vector3 normal;

        void read(NIFStream *nif, unsigned int nifVer);
    };

    struct OblivionSubShape
    {
        unsigned char layer; // http://niftools.sourceforge.net/doc/nif/OblivionLayer.html
        unsigned char collisionFilter;
        unsigned short unknown;
        unsigned int numVerts;
        unsigned int material; // http://niftools.sourceforge.net/doc/nif/HavokMaterial.html

        void read(NIFStream *nif);
    };

    class hkPackedNiTriStripsData : public bhkShape
    {
    public:
        std::vector<hkTriangle> triangles;
        std::vector<Ogre::Vector3> vertices;
        std::vector<OblivionSubShape> subShapes;

        void read(NIFStream *nif);
    };

    struct bhkPackedNiTriStripsShape : public  bhkShape
    {
        std::vector<OblivionSubShape> subShapes;
        unsigned int unknown1;
        unsigned int unknown2;
        float unknownF1;
        unsigned int unknown3;
        Ogre::Vector3 unknownVec;
        float unknownF2;
        float unknownF3;
        Ogre::Vector3 scale;
        float unknownF4;
        hkPackedNiTriStripsDataPtr data;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    struct OblivionColFilter
    {
        unsigned char layer;
        unsigned char colFilter;
        unsigned short unknown;
    };

    struct bhkNiTriStripsShape : public  bhkShape
    {
        // if (userVer < 12) // http://niftools.sourceforge.net/doc/nif/HavokMaterial.html
        // if (userVer >= 12) // http://niftools.sourceforge.net/doc/nif/SkyrimHavokMaterial.html
        unsigned int material;
        float unknownF1;
        unsigned int unknown1;
        std::vector<unsigned int> unknownInts;
        unsigned int unknown2;

        Ogre::Vector3 scale;
        unsigned int unknown3;

        std::vector<NiTriStripsDataPtr> stripsData;
        std::vector<OblivionColFilter> dataLayers;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class bhkListShape : public bhkShape
    {
    public:
        unsigned int numSubShapes;
        std::vector<bhkShapePtr> subShapes;
        unsigned int material; // if userVer >= 12, SkyrimHavokMaterial
        float unknownF1;
        std::vector<float> unknown;
        std::vector<unsigned int> unknownInts;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class bhkBoxShape : public bhkShape
    {
    public:
        unsigned int material; // if userVer >= 12, SkyrimHavokMaterial
        float radius;
        std::vector<unsigned char> unknown;
        Ogre::Vector3 dimensions_old; // deprecated
        btVector3 dimensions;
        float minSize;

        void read(NIFStream *nif);
    };

    class bhkConvexTransformShape : public bhkShape
    {
    public:
        bhkShapePtr shape;
        unsigned int material; // if userVer >= 12, SkyrimHavokMaterial
        float unknownF1;
        std::vector<unsigned char> unknown;
        Ogre::Vector3 dimensions;
        float transform[4][4];

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class bhkSphereShape : public bhkShape
    {
    public:
        unsigned int material; // if userVer >= 12, SkyrimHavokMaterial
        float radius;

        void read(NIFStream *nif);
    };

    struct SphereBV
    {
        Ogre::Vector3 center;
        float radius;
    };

    class bhkMultiSphereShape : public bhkShape
    {
    public:
        unsigned int material; // if userVer >= 12, SkyrimHavokMaterial
        float radius;
        float unknown1;
        float unknown2;
        std::vector<SphereBV> spheres;

        void read(NIFStream *nif);
    };

    class bhkTransformShape : public bhkShape
    {
    public:
        bhkShapePtr shape;
        unsigned int material; // if userVer >= 12, SkyrimHavokMaterial
        Ogre::Matrix4 transform;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class bhkCapsuleShape : public bhkShape
    {
    public:
        unsigned int material; // if userVer >= 12, SkyrimHavokMaterial
        float radius;
        Ogre::Vector3 firstPoint;
        float radius1;
        Ogre::Vector3 secondPoint;
        float radius2;

        void read(NIFStream *nif);
    };

    class bhkConvexVerticesShape : public bhkSphereShape
    {
    public:
        std::vector<float> unknown;
        std::vector<Ogre::Vector4> vertices_old; // deprecated
        std::vector<btVector3> vertices;
        std::vector<Ogre::Vector4> normals;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class bhkMoppBvTreeShape : public bhkShape
    {
    public:
        bhkShapePtr shape;
        unsigned int material;
        std::vector<unsigned char> unknown;
        float unknownF1;
        Ogre::Vector3 origin;
        float scale;
        std::vector<unsigned char> moppData;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class bhkCompressedMeshShape : public bhkShape
    {
    public:
        NodePtr target;
        unsigned int materialSkyrim;
        std::vector<unsigned char> unknown;
        float radius;
        float scale;
        bhkCompressedMeshShapeDataPtr data;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class bhkConstraint : public bhkSerializable
    {
    public:
        std::vector<bhkEntityPtr> entities;
        unsigned int priority;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
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

        void read(NIFStream *nif, unsigned int nifVer);
    };

    class bhkRagdollConstraint : public bhkConstraint
    {
    public:
        RagdollDescriptor ragdoll;

        void read(NIFStream *nif);
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

        void read(NIFStream *nif, unsigned int nifVer);
    };

    class bhkLimitedHingeConstraint : public bhkConstraint
    {
    public:
        LimitedHingeDescriptor limitedHinge;

        void read(NIFStream *nif);
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

        void read(NIFStream *nif, unsigned int nifVer);
    };

    class bhkHingeConstraint : public bhkConstraint
    {
    public:
        HingeDescriptor hinge;

        void read(NIFStream *nif);
    };

    class bhkPrismaticConstraint : public bhkConstraint
    {
    public:
        Ogre::Vector4 pivotA;
        std::vector<Ogre::Vector4> rotationMatrixA;
        Ogre::Vector4 pivotB;
        Ogre::Vector4 slidingB;
        Ogre::Vector4 planeB;
        Ogre::Vector4 slidingA;
        Ogre::Vector4 rotationA;
        Ogre::Vector4 planeA;
        Ogre::Vector4 rotationB;
        float minDistance;
        float maxDistance;
        float friction;

        void read(NIFStream *nif);
    };

    class bhkStiffSpringConstraint : public bhkConstraint
    {
    public:
        Ogre::Vector4 pivotA;
        Ogre::Vector4 pivotB;
        float length;

        void read(NIFStream *nif);
    };

    class bhkBreakableConstraint : public bhkConstraint
    {
    public:
        unsigned int unknownI1;
        std::vector<bhkEntityPtr> entities2;
        unsigned int priority2;
        unsigned int unknownI2;
        Ogre::Vector3 position;
        Ogre::Vector3 rotation;
        unsigned int unknownI3;
        float threshold;
        float unknownF1;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class bhkBallSocketConstraintChain : public bhkSerializable
    {
    public:
        std::vector<Ogre::Vector4> floats1;
        float unknownF1;
        float unknownF2;
        unsigned int unknownI1;
        unsigned int unknownI2;
        std::vector<NodePtr> links;
        std::vector<NodePtr> links2;
        unsigned int unknownI3;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class bhkMalleableConstraint : public bhkConstraint
    {
    public:
        unsigned int type;
        unsigned int unknown2;
        NodePtr link1;
        NodePtr link2;
        unsigned int unknown3;
        HingeDescriptor hinge;
        RagdollDescriptor ragdoll;
        LimitedHingeDescriptor limitedHinge;
        float tau;
        float damping;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class bhkEntity : public Record
    {
    public:

        bhkShapePtr shape;
        unsigned char layer; // http://niftools.sourceforge.net/doc/nif/OblivionLayer.html
        unsigned char collisionFilter;
        unsigned short unknownShort;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class bhkRigidBody : public bhkEntity
    {
    public:

        int unknownInt1;
        int unknownInt2;
        std::vector<int> unknown3Ints;
        unsigned char collisionResponse;
        unsigned char unknownByte;
        unsigned short callbackDelay;
        unsigned short unknown2;
        unsigned short unknown3;
        unsigned char layerCopy;
        unsigned char collisionFilterCopy;
        std::vector<unsigned short> unknown7Shorts;

        Ogre::Vector4 translation_old; // deprecated
        Ogre::Vector4 rotation_old; // deprecated
        btVector4 translation;
        btQuaternion rotation;
        Ogre::Vector4 velocityLinear;
        Ogre::Vector4 velocityAngular;
        Ogre::Real inertia[3][4];
        Ogre::Vector4 center;
        float mass;
        float dampingLinear;
        float dampingAngular;
        float friction;
        float gravityFactor1;
        float gravityFactor2;
        float rollingFrictionMultiplier;
        float restitution;
        float maxVelocityLinear;
        float maxVelocityAngular;
        float penetrationDepth;

        unsigned char motionSystem; // http://niftools.sourceforge.net/doc/nif/MotionSystem.html
        unsigned char deactivatorType; // http://niftools.sourceforge.net/doc/nif/DeactivatorType.html
        unsigned char solverDeactivation; // http://niftools.sourceforge.net/doc/nif/SolverDeactivation.html
        unsigned char motionQuality; // http://niftools.sourceforge.net/doc/nif/MotionQuality.html

        int unknownInt6;
        int unknownInt7;
        int unknownInt8;
        std::vector<bhkConstraintPtr> constraints;
        int unknownInt9;
        unsigned short unknownS9;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };
    class bhkRigidBodyT : public bhkRigidBody {};

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

    struct BoundingVolume
    {
        unsigned int collisionType;
        SphereBV sphere;
        BoxBV box;
        CapsuleBV capsule;
        HalfSpaceBV halfspace;
    };

    class NiCollisionObject : public Record
    {
    public:
        NodePtr target;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class NiCollisionData : public NiCollisionObject
    {
    public:
        unsigned int propagationMode;
        unsigned int collisionMode;
        BoundingVolume bv;

        void read(NIFStream *nif);
    };

    class bhkNiCollisionObject : public NiCollisionObject
    {
    public:
        unsigned short flags;
        bhkEntityPtr body; // should really be NiObject

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    class bhkCollisionObject : public bhkNiCollisionObject {};
    class bhkSPCollisionObject : public bhkNiCollisionObject {};

    class bhkBlendCollisionObject : public bhkNiCollisionObject
    {
    public:
        float unknown1;
        float unknown2;

        void read(NIFStream *nif);
    };

    struct bhkSimpleShapePhantom : public Record
    {
        bhkShapePtr shape;
        unsigned char oblivionLayer;
        unsigned char colFilter;

        void read(NIFStream *nif);
        void post(NIFFile *nif);
    };

    struct bhkCMSDMaterial
    {
        unsigned int skyrimMaterial;
        unsigned int unknown;
    };

    struct bhkCMSDTransform
    {
        Ogre::Vector4 translation;
        Ogre::Quaternion rotation;
    };

    struct bhkCMSDBigTris
    {
        unsigned short triangle1;
        unsigned short triangle2;
        unsigned short triangle3;
        unsigned int   unknown1;
        unsigned short unknown2;
    };

    struct bhkCMSDChunk
    {
        Ogre::Vector4 translation;
        unsigned int   materialIndex;
        unsigned short unknown1;
        unsigned short transformIndex;
        std::vector<unsigned short> vertices;
        std::vector<unsigned short> indicies;
        std::vector<unsigned short> strips;
        std::vector<unsigned short> indicies2;

        void read(NIFStream *nif, unsigned int nifVer);
    };

    class bhkCompressedMeshShapeData : public Record
    {
    public:
        unsigned int bitsPerIndex;
        unsigned int bitsPerWIndex;
        unsigned int maskWIndex;
        unsigned int maskIndex;
        float error;
        Ogre::Vector4 boundsMin;
        Ogre::Vector4 boundsMax;

        std::vector<bhkCMSDMaterial> chunkMaterials;
        std::vector<bhkCMSDTransform> chunkTransforms;
        std::vector<Ogre::Vector4> bigVerts;
        std::vector<bhkCMSDBigTris> bigTris;
        std::vector<bhkCMSDChunk> chunks;

        void read(NIFStream *nif);
    };
}
#endif // OPENMW_COMPONENTS_NIF_COLLISION_HPP
