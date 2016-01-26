#ifndef OPENMW_COMPONENTS_NIF_NODE_HPP
#define OPENMW_COMPONENTS_NIF_NODE_HPP

#include <OgreMatrix4.h>

#include "base.hpp"
#include "data.hpp" // NiSkinData

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

    void read(NIFStream *nif);
    void post(NIFFile *nif);

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

    void makeRootBone(const NiSkinData::BoneTrafo *tr);
    void makeBone(short ind, const NiSkinData::BoneInfo &bi);

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

struct NodeGroup
{
    std::vector<NodePtr> nodes;

    void read(NIFStream *nif, unsigned int nifVer);
    void post(NIFFile *nif);
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

    void read(NIFStream *nif);
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

        void read(NIFStream *nif, unsigned int nifVer);
    };
    Camera cam;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
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
    std::vector<PropertyPtr> bsprops;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
    void getBSProperties(const Nif::BSLightingShaderProperty *&bsprop,
                         const Nif::NiAlphaProperty *&alphaprop,
                         const Nif::BSEffectShaderProperty *&effectprop,
                         const Nif::BSWaterShaderProperty *&waterprop) const;
};

struct NiAutoNormalParticles : public NiGeometry {};
struct NiRotatingParticles : public NiGeometry {};
struct NiTriShape : public NiGeometry {};
struct NiTriStrips : public NiGeometry {};

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
