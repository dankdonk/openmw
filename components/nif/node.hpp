#ifndef OPENMW_COMPONENTS_NIF_NODE_HPP
#define OPENMW_COMPONENTS_NIF_NODE_HPP

#include <OgreMatrix4.h>

//#include "controlled.hpp"
//#include "extra.hpp"
//#include "data.hpp"
#include "property.hpp"
//#include "niftypes.hpp"
//#include "controller.hpp"
#include "base.hpp"
#include "data.hpp" // NiSkinData
#include "collision.hpp"

namespace Nif
{

class NiNode;
class NiTexturingProperty;
class NiMaterialProperty;
class NiAlphaProperty;
class NiVertexColorProperty;
class NiZBufferProperty;
class NiSpecularProperty;
class NiWireframeProperty;
class NiStencilProperty;
class BSLightingShaderProperty;
class NiAlphaProperty;
class BSEffectShaderProperty;
class BSWaterShaderProperty;

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
    bool hasBounds = false; // NOTE: this needs to be set to false for NiTriStrips (see ManualBulletShapeLoader)
    Ogre::Vector3 boundPos;
    Ogre::Matrix3 boundRot;
    Ogre::Vector3 boundXYZ; // Box size

    void read(NIFStream *nif)
    {
        Named::read(nif);

        flags = nif->getUShort();
        if (nifVer >= 0x14020007 && (userVer >= 11 && userVer2 > 26)) // from 20.2.0.7
            nif->getUShort();

        trafo = nif->getTrafo(); // scale (float) included

        if (nifVer <= 0x04020200) // up to 4.2.2.0
            velocity = nif->getVector3();

        if (nifVer < 0x14020007 || userVer <= 11) // less than 20.2.0.7 (or user version <= 11)
            props.read(nif);

        if (nifVer <= 0x04020200) // up to 4.2.2.0
        {
            hasBounds = nif->getBool(nifVer);
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
            collision.read(nif);
    }

    void post(NIFFile *nif)
    {
        Named::post(nif);

        if (nifVer < 0x14020007 || userVer <= 11)
            props.post(nif);

        if (nifVer >= 0x0a000100)
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
                       const Nif::NiStencilProperty *&stencilprop,
                       const Nif::Property *&prop) const;

    Ogre::Matrix4 getLocalTransform() const;
    Ogre::Matrix4 getWorldTransform() const;
};

class NiNode : public Node
{
public:
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
        if (0 == recIndex && nifVer <= 0x04010000) // FIXME experiment
        {
            if (static_cast<Nif::Node*>(this)->trafo.rotation != Nif::Transformation::getIdentity().rotation)
                std::cout << "Non-identity rotation: " << this->name << ", ver " << std::hex << nifVer << std::endl;
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
            // Sometimes a unique list of children can contain empty refs e.g. NoM/NoM_ac_pool00.nif
            if(!children[i].empty())
                children[i]->parent = this;
        }
    }
};

struct MaterialExtraData
{
    std::string name;
    int extraData;
};

class NiGeometry : public Node
{
    /* Possible flags:
        0x40 - mesh has no vertex normals ?

        Only flags included in 0x47 (ie. 0x01, 0x02, 0x04 and 0x40) have
        been observed so far.
    */
public:
    ShapeDataPtr data; // subclass includes NiTriShapeData
    NiSkinInstancePtr skin;

    std::vector<MaterialExtraData> materials;
    std::string shader;
    int unknown;
    bool dirtyFlag;
    std::vector<PropertyPtr> bsprops;

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
            if (userVer == 12) // not present in FO3?
            {
                bsprops.resize(2);
                bsprops[0].read(nif);
                bsprops[1].read(nif);
            }
        }
    }

    void post(NIFFile *nif)
    {
        Node::post(nif);
        data.post(nif);
        skin.post(nif);
        if (nifVer >= 0x14020007) // from 20.2.0.7 (Skyrim)
        {
            if (userVer == 12) // not present in FO3?
            {
                bsprops[0].post(nif);
                bsprops[1].post(nif);
            }
        }
    }

    void getBSProperties(const Nif::BSLightingShaderProperty *&bsprop,
                         const Nif::NiAlphaProperty *&alphaprop,
                         const Nif::BSEffectShaderProperty *&effectprop,
                         const Nif::BSWaterShaderProperty *&waterprop) const;
};

struct NiTriShape : public NiGeometry {};
struct NiTriStrips : public NiGeometry {};

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

    void post(NIFFile *nif)
    {
        Node::post(nif);
    }
};

struct NiAutoNormalParticles : public NiGeometry {};

struct NiRotatingParticles : public NiGeometry {};





struct NodeGroup
{
    std::vector<NodePtr> nodes;

    void read(NIFStream *nif, unsigned int nifVer);
    void post(NIFFile *nif);
};

class BSMultiBound : public Record
{
public:
    BSMultiBoundDataPtr data;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSMultiBoundData : public Record
{
    void read(NIFStream *nif) {} // do nothing
};

class BSMultiBoundOBB : public BSMultiBoundData
{
public:
    Ogre::Vector3 center;
    Ogre::Vector3 size;
    Ogre::Matrix3 rotation;

    void read(NIFStream *nif);
};

class BSFadeNode : public NiNode
{
public:

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

struct BSLeafAnimNode : public NiNode {};

class BSTreeNode : public NiNode
{
public:
    std::vector<NiNodePtr> bones1;
    std::vector<NiNodePtr> bones2;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSValueNode : public NiNode
{
public:
    int value;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSOrderedNode : public NiNode
{
public:
    Ogre::Vector4 alphaSortBound;
    unsigned char isStaticBound;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSMultiBoundNode : public NiNode
{
public:
    BSMultiBoundPtr multiBound;
    unsigned int unknown;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSBlastNode : public NiNode
{
public:
    char unknown1;;
    short unknown2;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiSwitchNode : public NiNode
{
public:

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSDamageStage : public NiNode
{
public:
    char unknown1;;
    short unknown2;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

struct NiBillboardNode : public NiNode
{
    unsigned short billboardMode;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiParticleSystem : public NiGeometry
{
    unsigned short unknownS2;
    unsigned short unknownS3;
    unsigned int unknownI1;
    bool worldSpace;
    std::vector<NiPSysModifierPtr> modifiers;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSStripParticleSystem : public NiParticleSystem {};

class BSLODTriShape : public NiGeometry
{
public:
    unsigned int level0Size;
    unsigned int level1Size;
    unsigned int level2Size;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

} // Namespace
#endif
