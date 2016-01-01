#ifndef OPENMW_COMPONENTS_NIF_NODE_HPP
#define OPENMW_COMPONENTS_NIF_NODE_HPP

#include <OgreMatrix4.h>

#include "controlled.hpp"
#include "extra.hpp"
#include "data.hpp"
#include "property.hpp"
#include "niftypes.hpp"
#include "controller.hpp"
#include "base.hpp"

namespace Nif
{

struct NiNode;

/** A Node is an object that's part of the main NIF tree. It has
    parent node (unless it's the root), and transformation (location
    and rotation) relative to it's parent.
 */
class Node : public Named
{
public:
    // Node flags. Interpretation depends somewhat on the type of node.
    int flags;
    Transformation trafo;
    Ogre::Vector3 velocity; // Unused? Might be a run-time game state
    PropertyList props;
    NiCollisionObjectPtr collision;

    // Bounding box info
    bool hasBounds;
    Ogre::Vector3 boundPos;
    Ogre::Matrix3 boundRot;
    Ogre::Vector3 boundXYZ; // Box size

    void read(NIFStream *nif)
    {
        Named::read(nif);

        flags = nif->getUShort();
        trafo = nif->getTrafo(); // scale (float) included

        if (nifVer <= 0x04020200) // up to 4.2.2.0
            velocity = nif->getVector3();

        props.read(nif);

        if (nifVer <= 0x04020200) // up to 4.2.2.0
        {
            if(nif->getBool(nifVer))
            {
                nif->getInt(); // always 1
                boundPos = nif->getVector3();
                boundRot = nif->getMatrix3();
                boundXYZ = nif->getVector3();
            }
        }

        parent = NULL;

        boneTrafo = NULL;
        boneIndex = -1;

        if (nifVer >= 0x0a000100) // from 10.0.1.0
            collision.read(nif);
    }

    void post(NIFFile *nif)
    {
        Named::post(nif);
        props.post(nif);
        collision.post(nif);
    }

    // Parent node, or NULL for the root node. As far as I'm aware, only
    // NiNodes (or types derived from NiNodes) can be parents.
    NiNode *parent;

    // Bone transformation. If set, node is a part of a skeleton.
    const NiSkinData::BoneTrafo *boneTrafo;

    // Bone weight info, from NiSkinData
    const NiSkinData::BoneInfo *boneInfo;

    // Bone index. If -1, this node is either not a bone, or if
    // boneTrafo is set it is the root bone in the skeleton.
    short boneIndex;

    void makeRootBone(const NiSkinData::BoneTrafo *tr)
    {
        boneTrafo = tr;
        boneIndex = -1;
    }

    void makeBone(short ind, const NiSkinData::BoneInfo &bi)
    {
        boneInfo = &bi;
        boneTrafo = &bi.trafo;
        boneIndex = ind;
    }

    void getProperties(const Nif::NiTexturingProperty *&texprop,
                       const Nif::NiMaterialProperty *&matprop,
                       const Nif::NiAlphaProperty *&alphaprop,
                       const Nif::NiVertexColorProperty *&vertprop,
                       const Nif::NiZBufferProperty *&zprop,
                       const Nif::NiSpecularProperty *&specprop,
                       const Nif::NiWireframeProperty *&wireprop,
                       const Nif::NiStencilProperty *&stencilprop) const;

    Ogre::Matrix4 getLocalTransform() const;
    Ogre::Matrix4 getWorldTransform() const;
};

struct NiNode : Node
{
    NodeList children;
    NodeList effects; // FIXME: should be a list of NiDynamicEffect

    enum Flags {
        Flag_Hidden = 0x0001,
        Flag_MeshCollision = 0x0002,
        Flag_BBoxCollision = 0x0004
    };
    enum BSAnimFlags {
        AnimFlag_AutoPlay = 0x0020
    };
    enum BSParticleFlags {
        ParticleFlag_AutoPlay = 0x0020,
        ParticleFlag_LocalSpace = 0x0080
    };
    enum ControllerFlags {
        ControllerFlag_Active = 0x8
    };

    void read(NIFStream *nif)
    {
        Node::read(nif);
        children.read(nif);
        effects.read(nif);

        // Discard tranformations for the root node, otherwise some meshes
        // occasionally get wrong orientation. Only for NiNode-s for now, but
        // can be expanded if needed.
        if (0 == recIndex)
        {
            static_cast<Nif::Node*>(this)->trafo = Nif::Transformation::getIdentity();
        }
    }

    void post(NIFFile *nif)
    {
        Node::post(nif);
        children.post(nif);
        effects.post(nif);

        for(size_t i = 0;i < children.length();i++)
        {
            // Why would a unique list of children contain empty refs?
            if(!children[i].empty())
                children[i]->parent = this;
        }
    }
};

struct NiCamera : public Node
{
    struct Camera
    {
        // Camera frustrum
        float left, right, top, bottom, nearDist, farDist;

        // Viewport
        float vleft, vright, vtop, vbottom;

        // Level of detail modifier
        float LOD;

        bool useOrtho;

        void read(NIFStream *nif, unsigned int nifVer)
        {
            if (nifVer >= 0x0a010000) // from 10.1.0.0
                nif->getUShort();
            left = nif->getFloat();
            right = nif->getFloat();
            top = nif->getFloat();
            bottom = nif->getFloat();
            nearDist = nif->getFloat();
            farDist = nif->getFloat();
            if (nifVer >= 0x0a010000) // from 10.1.0.0
                useOrtho = nif->getBool(nifVer);

            vleft = nif->getFloat();
            vright = nif->getFloat();
            vtop = nif->getFloat();
            vbottom = nif->getFloat();

            LOD = nif->getFloat();
        }
    };
    Camera cam;

    void read(NIFStream *nif)
    {
        Node::read(nif);

        cam.read(nif, nifVer);

        nif->getInt(); // -1
        nif->getInt(); // 0
        if (nifVer >= 0x04020100) // from 4.2.1.0
            nif->getInt();
    }
};

struct MaterialExtraData
{
    std::string name;
    int extraData;
};

/* Possible flags:
    0x40 - mesh has no vertex normals ?

    Only flags included in 0x47 (ie. 0x01, 0x02, 0x04 and 0x40) have
    been observed so far.
*/
class NiGeometry : public Node
{
public:
    ShapeDataPtr data; // subclass includes NiTriShapeData
    NiSkinInstancePtr skin;

    std::vector<MaterialExtraData> materials;
    std::string shader;
    int unknown;
    bool dirtyFlag;
    //NiPropertyPtr bs1;
    //NiPropertyPtr bs2;

    void read(NIFStream *nif)
    {
        Node::read(nif); // NiAVObject
        data.read(nif);  // refNiGeometryData
        skin.read(nif);

        if (nifVer >= 0x14020007) // from 20.2.0.7 (Skyrim)
        {
            unsigned int numMaterials = nif->getUInt();
            materials.resize(numMaterials);
            for (unsigned int i = 0; i < numMaterials; ++i)
            {
                materials[i].name = nif->getString();
                materials[i].extraData = nif->getInt();
            }
            nif->getInt(); // active material?
        }

        if (nifVer >= 0x0a000100 && nifVer <= 0x14010003 && nif->getBool(nifVer))
        {
            shader = nif->getString();
            unknown = nif->getInt();
        }

        if (nifVer >= 0x14020007) // from 20.2.0.7 (Skyrim)
        {
            dirtyFlag = nif->getBool(nifVer);
            nif->getInt(); // bs1.read(nif);
            nif->getInt(); // bs2.read(nif);
        }
    }

    void post(NIFFile *nif)
    {
        Node::post(nif);
        data.post(nif);
        skin.post(nif);
        // FIXME any other post stuff
    }
};

struct NiAutoNormalParticles : public NiGeometry {};
struct NiRotatingParticles : public NiGeometry {};
struct NiTriShape : public NiGeometry {};
class NiTriStrips : public NiGeometry {};

class NiParticleSystem : public NiGeometry
{
    unsigned short unknownS2;
    unsigned short unknownS3;
    unsigned int unknown1;
    bool worldSpace;
    std::vector<NiPSysModifierPtr> modifiers;

    void read(NIFStream *nif)
    {
        NiGeometry::read(nif);

        if (nifVer >= 0x0a010000) // from 10.1.0.0
        {
            worldSpace = nif->getBool(nifVer);
            unsigned int numModifiers = nif->getUInt();
            modifiers.resize(numModifiers);
            for (unsigned int i = 0; i < numModifiers; ++i)
                modifiers[i].read(nif);
        }
    }

    void post(NIFFile *nif)
    {
        NiGeometry::post(nif);
    }
};

struct OblivionSubShape
{
    unsigned char layer; // http://niftools.sourceforge.net/doc/nif/OblivionLayer.html
    unsigned char collisionFilter;
    unsigned short unknown;
    unsigned int numVerts;
    unsigned int material; // http://niftools.sourceforge.net/doc/nif/HavokMaterial.html
};

class bhkShape : public Record {};

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
    int refHkPackedNiTriStripsData;

    void read(NIFStream *nif)
    {
        unsigned short numShapes = nif->getUShort();
        subShapes.resize(numShapes);
        for (unsigned int i = 0; i < numShapes; ++i)
        {
            subShapes[i].layer = nif->getChar();
            subShapes[i].collisionFilter = nif->getChar();
            subShapes[i].unknown = nif->getUShort();
            subShapes[i].numVerts = nif->getUInt();
            subShapes[i].material = nif->getUInt();
        }
        unknown1 = nif->getUInt();
        unknown2 = nif->getUInt();
        unknownF1 = nif->getFloat();
        unknown3 = nif->getUInt();
        unknownVec = nif->getVector3();
        unknownF2 = nif->getFloat();
        unknownF3 = nif->getFloat();
        scale = nif->getVector3();
        unknownF4 = nif->getFloat();
        refHkPackedNiTriStripsData = nif->getInt();
    }

    void post(NIFFile *nif)
    {
        // FIXME
    }
};

struct OblivionColFilter
{
    unsigned char layer;
    unsigned char colFilter;
    unsigned short unknown;
};

struct bhkNiTriStripsShape : public  bhkShape
{
    unsigned int material; // http://niftools.sourceforge.net/doc/nif/HavokMaterial.html
    //unsigned int materialSkyrim; // http://niftools.sourceforge.net/doc/nif/SkyrimHavokMaterial.html
    float unknownF1;
    unsigned int unknown1;
    std::vector<unsigned int> unknownInts;
    unsigned int unknown2;

    Ogre::Vector3 scale;
    unsigned int unknown3;

    std::vector<NiTriStripsDataPtr> stripsData;
    std::vector<OblivionColFilter> dataLayers;

    void read(NIFStream *nif)
    {
        material = nif->getUInt();
        //materialSkyrim = nif->getUInt();  // not sure if this is version dependent
        unknownF1 = nif->getFloat();
        unknown1 = nif->getUInt();
        unknownInts.resize(4);
        for (unsigned int i = 0; i < 4; ++i)
            unknownInts[i] = nif->getUInt();
        unknown2 = nif->getUInt();

        scale = nif->getVector3();
        unknown3 = nif->getUInt();

        unsigned short numStrips = nif->getUInt();
        stripsData.resize(numStrips);
        for (unsigned int i = 0; i < numStrips; ++i)
        {
            stripsData[i].read(nif);
        }
        unsigned short numDataLayers = nif->getUInt();
        dataLayers.resize(numDataLayers);
        for (unsigned int i = 0; i < numDataLayers; ++i)
        {
            dataLayers[i].layer = nif->getChar();
            dataLayers[i].colFilter = nif->getChar();
            dataLayers[i].unknown = nif->getUShort();
        }
    }

    void post(NIFFile *nif)
    {
        // FIXME
    }
};

class bhkListShape : public bhkShape
{
public:
    unsigned int numSubShapes;
    std::vector<bhkShapePtr> subShapes;
    unsigned int material; // http://niftools.sourceforge.net/doc/nif/HavokMaterial.html
    //unsigned int materialSkyrim; // http://niftools.sourceforge.net/doc/nif/SkyrimHavokMaterial.html
    float unknownF1;
    std::vector<float> unknown;
    std::vector<unsigned int> unknownInts;

    void read(NIFStream *nif)
    {
        numSubShapes = nif->getUInt();
        subShapes.resize(numSubShapes);
        for (unsigned int i = 0; i < numSubShapes; ++i)
            subShapes[i].read(nif);

        material = nif->getUInt();
        //materialSkyrim = nif->getUInt();  // not sure if this is version dependent

        unknown.resize(6);
        for (int i = 0; i < 6; ++i)
            unknown[i] = nif->getFloat();

        unknownInts.resize(nif->getUInt());
        for (unsigned int i = 0; i < unknownInts.size(); ++i)
            unknownInts[i] = nif->getUInt();
    }

    void post(NIFFile *nif)
    {
        // FIXME
    }
};

class bhkBoxShape : public bhkShape
{
public:
    unsigned int material; // http://niftools.sourceforge.net/doc/nif/HavokMaterial.html
    //unsigned int materialSkyrim; // http://niftools.sourceforge.net/doc/nif/SkyrimHavokMaterial.html
    float radius;
    std::vector<unsigned char> unknown;
    Ogre::Vector3 dimensions;
    float minSize;

    void read(NIFStream *nif)
    {
        material = nif->getUInt();
        //materialSkyrim = nif->getUInt();  // not sure if this is version dependent
        radius = nif->getFloat();
        unknown.resize(8);
        for (int i = 0; i < 8; ++i)
            unknown[i] = nif->getChar();

        dimensions = nif->getVector3();
        minSize = nif->getFloat();
    }

    void post(NIFFile *nif)
    {
        // FIXME
    }
};

class bhkConvexTransformShape : public bhkShape
{
public:
    bhkShapePtr shape;
    unsigned int material; // http://niftools.sourceforge.net/doc/nif/HavokMaterial.html
    //unsigned int materialSkyrim; // http://niftools.sourceforge.net/doc/nif/SkyrimHavokMaterial.html
    float unknownF1;
    std::vector<unsigned char> unknown;
    Ogre::Vector3 dimensions;
    float transform[4][4];

    void read(NIFStream *nif)
    {
        shape.read(nif);
        material = nif->getUInt();
        //materialSkyrim = nif->getUInt();  // not sure if this is version dependent
        unknownF1 = nif->getFloat();
        unknown.resize(8);
        for (int i = 0; i < 8; ++i)
            unknown[i] = nif->getChar();

        for(size_t i = 0; i < 4; i++)
            for(size_t j = 0; j < 4; j++)
                transform[i][j] = nif->getFloat();
    }

    void post(NIFFile *nif)
    {
        shape.post(nif);

        // FIXME
    }
};

class bhkSphereShape : public bhkShape
{
public:
    unsigned int material; // http://niftools.sourceforge.net/doc/nif/HavokMaterial.html
    //unsigned int materialSkyrim; // http://niftools.sourceforge.net/doc/nif/SkyrimHavokMaterial.html
    float radius;

    void read(NIFStream *nif)
    {
        material = nif->getUInt();
        //materialSkyrim = nif->getUInt();  // not sure if this is version dependent
        radius = nif->getFloat();
    }
};

class bhkCapsuleShape : public bhkShape
{
public:
    unsigned int material;
    //unsigned int materialSkyrim;
    float radius;
    Ogre::Vector3 firstPoint;
    float radius1;
    Ogre::Vector3 secondPoint;
    float radius2;

    void read(NIFStream *nif)
    {
        material = nif->getUInt();
        //materialSkyrim = nif->getUInt();
        radius = nif->getFloat();

        std::vector<char> unknown;
        unknown.resize(8);
        for (int i = 0; i < 8; ++i)
            unknown[i] = nif->getChar();

        firstPoint= nif->getVector3();
        radius1= nif->getFloat();
        secondPoint= nif->getVector3();
        radius2= nif->getFloat();
    }
};

class bhkConvexVerticesShape : public bhkSphereShape
{
public:
    std::vector<float> unknown;
    std::vector<Ogre::Vector4> vertices;
    std::vector<Ogre::Vector4> normals;

    void read(NIFStream *nif)
    {
        bhkSphereShape::read(nif);

        unknown.resize(6);
        for (int i = 0; i < 6; ++i)
            unknown[i] = nif->getFloat();

        nif->getVector4s(vertices, nif->getUInt());
        nif->getVector4s(normals, nif->getUInt());
    }

    void post(NIFFile *nif)
    {
        //bhkSphereShape::post(nif);
        // FIXME
    }
};

class bhkMoppBvTreeShape : public bhkShape
{
public:
    bhkShapePtr shape;
    unsigned int material;
    //unsigned int materialSkyrim;
    std::vector<unsigned char> unknown;
    float unknownF1;
    Ogre::Vector3 origin;
    float scale;
    std::vector<unsigned char> moppData;

    void read(NIFStream *nif)
    {
        shape.read(nif);
        material = nif->getUInt();
        //materialSkyrim = nif->getUInt();  // not sure if this is version dependent
        unknown.resize(8);
        for (int i = 0; i < 8; ++i)
            unknown[i] = nif->getChar();

        unknownF1 = nif->getFloat();
        unsigned int size = nif->getUInt();
        origin = nif->getVector3();
        scale = nif->getFloat();

        moppData.resize(size);
        for (unsigned int i = 0; i < size; ++i)
        {
            moppData[i] = nif->getChar();
        }
    }

    void post(NIFFile *nif)
    {
        // FIXME
    }
};

class bhkConstraint : public Record
{
public:
    std::vector<bhkRigidBodyPtr> entities;
    unsigned int priority;

    void read(NIFStream *nif)
    {
        unsigned int numEntities = nif->getUInt();
        entities.resize(numEntities);
        for (unsigned int i = 0; i < numEntities; ++i)
        {
            entities[0].read(nif);
        }
    }
};

class bhkRigidBody : public Record
{
public:
    bhkShapePtr shape;
    unsigned char layer; // http://niftools.sourceforge.net/doc/nif/OblivionLayer.html
    unsigned char collisionFilter;
    unsigned short unknownShort;

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

    Ogre::Vector4 translation;
    Ogre::Vector4 rotation;
    Ogre::Vector4 velocityLinear;
    Ogre::Vector4 velocityAngular;
    Ogre::Real inertia[3][4];
    Ogre::Vector4 center;
    float mass;
    float dampingLinear;
    float dampingAngular;
    float friction;
    //float gravityFactor1;
    //float gravityFactor2;
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
    //unsigned short unknownS9;

    void read(NIFStream *nif)
    {
        shape.read(nif);
        layer = nif->getChar();
        collisionFilter = nif->getChar();
        unknownShort = nif->getUShort();

        unknownInt1 = nif->getInt();
        unknownInt2 = nif->getInt();
        unknown3Ints.resize(3);
        for(size_t i = 0; i < 3; i++)
            unknown3Ints[i] = nif->getInt();
        collisionResponse = nif->getChar();
        unknownByte = nif->getChar();
        callbackDelay = nif->getUShort();
        unknown2 = nif->getUShort();
        unknown3 = nif->getUShort();
        layerCopy = nif->getChar();
        collisionFilterCopy = nif->getChar();
        unknown7Shorts.resize(7);
        for(size_t i = 0; i < 7; i++)
            unknown7Shorts[i] = nif->getUShort();

        translation = nif->getVector4();
        rotation = nif->getVector4();
        velocityLinear = nif->getVector4();
        velocityAngular = nif->getVector4();
        for(size_t i = 0; i < 3; i++)
        {
            for(size_t j = 0; j < 4; j++)
                inertia[i][j] = Ogre::Real(nif->getFloat());
        }
        center = nif->getVector4();
        mass = nif->getFloat();
        dampingLinear = nif->getFloat();
        dampingAngular = nif->getFloat();
        //gravityFactor1 = nif->getFloat(); // FIXME: check why these need to be commented out
        //gravityFactor2 = nif->getFloat();
        friction = nif->getFloat();
        //rollingFrictionMultiplier = nif->getFloat();
        restitution = nif->getFloat();
        maxVelocityLinear = nif->getFloat();
        maxVelocityAngular = nif->getFloat();
        penetrationDepth = nif->getFloat();

        motionSystem = nif->getChar();
        deactivatorType = nif->getChar();
        solverDeactivation = nif->getChar();
        motionQuality = nif->getChar();

        unknownInt6 = nif->getInt();
        unknownInt7 = nif->getInt();
        unknownInt8 = nif->getInt();
        // another unsigned int for Skyrim here?
        unsigned int numConst = nif->getUInt();
        constraints.resize(numConst);
        for(size_t i = 0; i < numConst; i++)
            constraints[i].read(nif);
        unknownInt9 = nif->getInt();
        //unknownS9 = nif->getUShort();
    }

    void post(NIFFile *nif)
    {
        // FIXME
    }
};
typedef bhkRigidBody bhkRigidBodyT;

class NiCollisionObject : public Record
{
public:
    NodePtr target;

    void read(NIFStream *nif)
    {
        target.read(nif);
    }

    void post(NIFFile *nif)
    {
        target.post(nif);
    }
};

class bhkCollisionObject : public NiCollisionObject
{
public:
    unsigned short flags;
    NiCollisionObjectPtr body;

    void read(NIFStream *nif)
    {
        NiCollisionObject::read(nif);
        flags = nif->getUShort();
        body.read(nif);
    }
};

struct SphereBV
{
    Ogre::Vector3 center;
    float radius;
};

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

class NiCollisionData : public NiCollisionObject
{
public:
    unsigned int propagationMode;
    unsigned int collisionMode;
    BoundingVolume bv;

    void read(NIFStream *nif)
    {
        NiCollisionObject::read(nif);
        propagationMode = nif->getUInt();
        collisionMode = nif->getUInt();
        bool useABV = !!nif->getChar();
        if (useABV)
        {
            bv.collisionType = nif->getUInt();
            switch (bv.collisionType)
            {
                case 0:
                    bv.sphere.center = nif->getVector3();
                    bv.sphere.radius = nif->getFloat();
                    break;
                case 1:
                    bv.box.center = nif->getVector3();
                    bv.box.axis.resize(3);
                    for (int i = 0; i < 3; ++i)
                        bv.box.axis[i] = nif->getVector3();
                    bv.box.extent.resize(3);
                    for (int i = 0; i < 3; ++i)
                        bv.box.extent[i] = nif->getFloat();
                    break;
                case 2:
                    bv.capsule.center = nif->getVector3();
                    bv.capsule.origin = nif->getVector3();
                    bv.capsule.unknown1 = nif->getFloat();
                    bv.capsule.unknown2 = nif->getFloat();
                    break;
                case 5:
                    bv.halfspace.normal = nif->getVector3();
                    bv.halfspace.center = nif->getVector3();
                    break;
                default:
                    break;
            }
        }
    }
};

class NiControllerManager : public Controller
{
public:
    bool cumulative;
    std::vector<NiControllerSequencePtr> controllerSequences;
    //NiDefaultAvObjectPalettePtr objectPalette;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        cumulative = nif->getBool(nifVer);
        unsigned int numControllerSequences = nif->getUInt();
        controllerSequences.resize(numControllerSequences);
        for (unsigned int i = 0; i < numControllerSequences; ++i)
            controllerSequences[i].read(nif);
        nif->getUInt(); // FIXME
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
    }
};

struct ControllerLink
{
    std::string targetName;
    ControllerPtr controller;
    NiInterpolatorPtr interpolator;
    ControllerPtr controller2;
    unsigned char priority;
    NiStringPalettePtr stringPalette;
    std::string nodeName;
    int nodeNameOffset;
    std::string propertyType;
    int propertyTypeOffset;
    std::string controllerType;
    int controllerTypeOffset;
    std::string variable1;
    int variable1Offset;
    std::string variable2;
    int variable2Offset;

    void read(NIFStream *nif, unsigned int nifVer)
    {
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
        {
            targetName = nif->getString();
            controller.read(nif);
        }
        if (nifVer >= 0x0a01006a) // from 10.1.0.106
        {
            interpolator.read(nif);
            controller2.read(nif);
            if (nifVer == 0x0a01006a) // 10.1.0.106
            {
                nif->getUInt(); // FIXME
                nif->getUShort();
            }
            priority = nif->getChar();
        }
        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            stringPalette.read(nif);
        if (nifVer == 0x0a01006a) // 10.1.0.106
            nodeName = nif->getString();
        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            nodeNameOffset = nif->getInt();
        if (nifVer == 0x0a01006a) // 10.1.0.106
            propertyType = nif->getString();
        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            propertyTypeOffset = nif->getInt();
        if (nifVer == 0x0a01006a) // 10.1.0.106
            controllerType = nif->getString();
        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            controllerTypeOffset = nif->getInt();
        if (nifVer == 0x0a01006a) // 10.1.0.106
            variable1 = nif->getString();
        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            variable1Offset = nif->getInt();
        if (nifVer == 0x0a01006a) // 10.1.0.106
            variable2 = nif->getString();
        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            variable2Offset = nif->getInt();
    }
};

class NiSequence : public Record
{
public:
    std::string name;
    std::string textKeysName;
    NiTextKeyExtraDataPtr textKeys;
    std::vector<ControllerLink> controlledBlocks;

    void read(NIFStream *nif)
    {
        name = nif->getString();

        if (nifVer <= 0x0a010000) // up to 10.1.0.0
        {
            textKeysName = nif->getString();
            textKeys.read(nif);
        }
        unsigned int numControlledBlocks = nif->getUInt();
        if (nifVer >= 0x0a01006a) // from 10.1.0.106
            unsigned int unknown = nif->getUInt();
        controlledBlocks.resize(numControlledBlocks);
        for (unsigned int i = 0; i < numControlledBlocks; ++i)
            controlledBlocks[i].read(nif, nifVer);
    }
};

class NiControllerSequence : public NiSequence
{
public:
    float weight;
    NiTextKeyExtraDataPtr textKeys2;
    unsigned int cycleType;
    unsigned int unknown0;
    float frequency;
    float startTime;
    float unknown2;
    float stopTime;
    char unknownByte;
    NiControllerManagerPtr manager;
    std::string targetName;
    NiStringPalettePtr stringPalette;

    void read(NIFStream *nif)
    {
        NiSequence::read(nif);

        if (nifVer >= 0x0a01006a) // from 10.1.0.106
        {
            weight = nif->getFloat();
            textKeys2.read(nif);
            cycleType = nif->getUInt();

            if (nifVer == 0x0a01006a) // 10.1.0.106
                unknown0 = nif->getUInt();

            frequency = nif->getFloat();
            startTime = nif->getFloat();

            if (nifVer >= 0x0a020000 && nifVer <= 0x01040001)
                unknown2 = nif->getFloat();

            stopTime = nif->getFloat();

            if (nifVer == 0x0a01006a) // 10.1.0.106
                unknownByte = nif->getChar();

            manager.read(nif);
            targetName = nif->getString();

            if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
                stringPalette.read(nif);
        }
    }

    void post(NIFFile *nif)
    {
        NiSequence::post(nif);
    }
};

} // Namespace
#endif
