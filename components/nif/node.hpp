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
// NiAVObject
public:
    // Node flags. Interpretation depends somewhat on the type of node.
    int flags;
    Transformation trafo;
    Ogre::Vector3 velocity; // Unused? Might be a run-time game state
    PropertyList props;
    unsigned int collision;

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
            hasBounds = !!nif->getInt();
            if(hasBounds)
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
            collision = nif->getUInt(); // reference to a collision object
    }

    void post(NIFFile *nif)
    {
        Named::post(nif);
        props.post(nif);
        //collision.post(nif);
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
    NodeList effects;

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

struct NiTriShape : Node
{
    /* Possible flags:
        0x40 - mesh has no vertex normals ?

        Only flags included in 0x47 (ie. 0x01, 0x02, 0x04 and 0x40) have
        been observed so far.
    */

    NiTriShapeDataPtr data;
    NiSkinInstancePtr skin;

    std::string shader;
    int unknown;

    void read(NIFStream *nif)
    {
        Node::read(nif);
        data.read(nif);
        skin.read(nif);

        if (nifVer >= 0x0a000100) // from 10.0.1.0
        {
            bool hasShader = !!nif->getInt();
            if (hasShader)
            {
                shader = nif->getString();
                unknown = nif->getInt();
            }
        }
    }

    void post(NIFFile *nif)
    {
        Node::post(nif);
        data.post(nif);
        skin.post(nif);
    }
};

struct NiCamera : Node
{
    struct Camera
    {
        // Camera frustrum
        float left, right, top, bottom, nearDist, farDist;

        // Viewport
        float vleft, vright, vtop, vbottom;

        // Level of detail modifier
        float LOD;

        void read(NIFStream *nif)
        {
            left = nif->getFloat();
            right = nif->getFloat();
            top = nif->getFloat();
            bottom = nif->getFloat();
            nearDist = nif->getFloat();
            farDist = nif->getFloat();

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

        cam.read(nif);

        nif->getInt(); // -1
        nif->getInt(); // 0
    }
};

struct NiAutoNormalParticles : Node
{
    NiAutoNormalParticlesDataPtr data;

    void read(NIFStream *nif)
    {
        Node::read(nif);
        data.read(nif);
        nif->getInt(); // -1
    }

    void post(NIFFile *nif)
    {
        Node::post(nif);
        data.post(nif);
    }
};

struct NiRotatingParticles : Node
{
    NiRotatingParticlesDataPtr data;

    void read(NIFStream *nif)
    {
        Node::read(nif);
        data.read(nif);
        nif->getInt(); // -1
    }

    void post(NIFFile *nif)
    {
        Node::post(nif);
        data.post(nif);
    }
};

struct NiTriStrips : Node
{
    NiTriShapeDataPtr data;
    NiSkinInstancePtr skin;

    std::string shader;
    int unknown;

    void read(NIFStream *nif)
    {
        Node::read(nif); // NiAVObject
        data.read(nif);  // RefNiGeometryData
        skin.read(nif);

        if (nifVer >= 0x0a000100) // from 10.0.1.0
        {
            bool hasShader = !!nif->getInt();
            if (hasShader)
            {
                shader = nif->getString();
                unknown = nif->getInt();
            }
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

struct OblivionSubShape
{
    unsigned char layer; // http://niftools.sourceforge.net/doc/nif/OblivionLayer.html
    unsigned char collisionFilter;
    unsigned short unknown;
    unsigned int numVerts;
    unsigned int material; // http://niftools.sourceforge.net/doc/nif/HavokMaterial.html
};

struct bhkPackedNiTriStripsShape : Node
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

struct bhkMoppBvTreeShape : Node
{
    unsigned int refBhkShape;
    unsigned int material; // http://niftools.sourceforge.net/doc/nif/HavokMaterial.html
    unsigned int materialSkyrim;
    std::vector<unsigned char> unknown;
    float unknownF1;
    Ogre::Vector3 origin;
    float scale;
    std::vector<unsigned char> moppData;

    void read(NIFStream *nif)
    {
        refBhkShape = nif->getUInt();
        material = nif->getUInt();
        materialSkyrim = nif->getUInt();
        unknown.resize(8);
        for (int i = 0; i < 8; ++i)
            unknown[i] = nif->getChar();

        unsigned int size = nif->getUInt();
        unknownF1 = nif->getFloat();
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

struct bhkRigidBodyT : Node
{
    int refBhkShape;
    unsigned char layer; // http://niftools.sourceforge.net/doc/nif/OblivionLayer.html
    unsigned char collisionFilter;
    unsigned short unknownS1;

    int unknownI1;
    int unknownI2;
    std::vector<int> unknownV1; // size 3
    unsigned char collisionResponse;
    unsigned char unknown1;
    unsigned char unknown2;
    unsigned char unknown3;
    unsigned char layerCopy;
    unsigned char collisionFilterCopy;
    std::vector<unsigned short> unknownV2; // size 7

    Ogre::Vector4 translation;
    Ogre::Vector4 rotation;
    Ogre::Vector4 velocityLinear;
    Ogre::Vector4 velocityAngular;
    Ogre::Real inertia[3][4];
    Ogre::Vector4 center;
    float mass;
    float dampingLinear;
    float dampingAngular;
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

    int unknownI6;
    int unknownI7;
    int unknownI8;
    std::vector<unsigned int> constraints;
    int unknownI9;
    unsigned short unknownS9;

    void read(NIFStream *nif)
    {
        refBhkShape = nif->getInt();
        layer = nif->getChar();
        collisionFilter = nif->getChar();
        unknownS1 = nif->getUShort();

        int unknownI1 = nif->getInt();
        int unknownI2 = nif->getInt();
        unknownV1.resize(3);
        for(size_t i = 0; i < 3; i++)
            unknownV1[i] = nif->getInt();
        collisionResponse = nif->getChar();
        unknown1 = nif->getChar();
        unknown2 = nif->getChar();
        unknown3 = nif->getChar();
        layerCopy = nif->getChar();
        collisionFilterCopy = nif->getChar();
        unknownV2.resize(7);
        for(size_t i = 0; i < 7; i++)
            unknownV2[i] = nif->getUShort();

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
        gravityFactor1 = nif->getFloat();
        gravityFactor2 = nif->getFloat();
        rollingFrictionMultiplier = nif->getFloat();
        restitution = nif->getFloat();
        maxVelocityLinear = nif->getFloat();
        maxVelocityAngular = nif->getFloat();
        penetrationDepth = nif->getFloat();

        motionSystem = nif->getChar();
        deactivatorType = nif->getChar();
        solverDeactivation = nif->getChar();
        motionQuality = nif->getChar();

        unknownI6 = nif->getInt();
        unknownI7 = nif->getInt();
        unknownI8 = nif->getInt();
        unsigned int numConst = nif->getUInt();
        constraints.resize(numConst);
        for(size_t i = 0; i < numConst; i++)
            constraints[i] = nif->getUInt();
        unknownI9 = nif->getInt();
        unknownS9 = nif->getUShort();
    }

    void post(NIFFile *nif)
    {
        // FIXME
    }
};

struct bhkCollisionObject : Node
{
    int ptrNiAVObject;
    unsigned short flags;
    int refNiObject;

    void read(NIFStream *nif)
    {
        ptrNiAVObject = nif->getInt();
        flags = nif->getUShort();
        refNiObject = nif->getUInt();
    }

    void post(NIFFile *nif)
    {
        // FIXME
    }
};

} // Namespace
#endif
