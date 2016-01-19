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

class NiNode;

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
        if (nifVer >= 0x14020007 && (userVer >= 11 || userVer2 > 26)) // from 20.2.0.7
            nif->getUShort();

        trafo = nif->getTrafo(); // scale (float) included

#if 0
        int y = Ogre::Math::ATan2(-trafo.rotation[2][0], trafo.rotation[0][0]).valueDegrees();
        if (/*recIndex == 0 &&*/ y != 0)
            std::cout << this->name << " Y " << y << std::endl;
        int r = Ogre::Math::ATan2(-trafo.rotation[1][2], trafo.rotation[1][1]).valueDegrees();
        if (/*recIndex == 0 &&*/ r != 0)
            std::cout << this->name << " R " << r << std::endl;
        if (name == "ANCastleWallCurve02" || name == "AnCastleWallEnd01")
        {
            std::cout << trafo.rotation[0][0] << ", " << trafo.rotation[0][1] << ", " << trafo.rotation[0][2] << std::endl;
            std::cout << trafo.rotation[1][0] << ", " << trafo.rotation[1][1] << ", " << trafo.rotation[1][2] << std::endl;
            std::cout << trafo.rotation[2][0] << ", " << trafo.rotation[2][1] << ", " << trafo.rotation[2][2] << std::endl;
            if (recIndex == 0)
            {
                std::cout << "root" << std::endl;
                std::cout << "Y " << Ogre::Math::ATan2(-trafo.rotation[2][0], trafo.rotation[0][0]) << std::endl;
                std::cout << "R " << Ogre::Math::ATan2(-trafo.rotation[1][2], trafo.rotation[1][1]) << std::endl;
            }
        }
#endif

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
                       const Nif::NiStencilProperty *&stencilprop) const;

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
            // Why would a unique list of children contain empty refs?
            if(!children[i].empty())
                children[i]->parent = this;
        }
    }
};

class BSMultiBound : public Record
{
public:
    BSMultiBoundDataPtr data;

    void read(NIFStream *nif)
    {
        data.read(nif);
    }

    void post(NIFFile *nif)
    {
        data.post(nif);
    }
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

    void read(NIFStream *nif)
    {
        center = nif->getVector3();
        size = nif->getVector3();
        rotation = nif->getMatrix3();
    }
};

struct BSFadeNode : public NiNode {};
struct BSLeafAnimNode : public NiNode {};

class BSTreeNode : public NiNode
{
public:
    std::vector<NiNodePtr> bones1;
    std::vector<NiNodePtr> bones2;

    void read(NIFStream *nif)
    {
        NiNode::read(nif);

        unsigned int numBones = nif->getUInt();
        bones1.resize(numBones);
        for (unsigned int i = 0; i < numBones; ++i)
            bones1[i].read(nif);

        numBones = nif->getUInt();
        bones2.resize(numBones);
        for (unsigned int i = 0; i < numBones; ++i)
            bones2[i].read(nif);
    }

    void post(NIFFile *nif)
    {
        NiNode::post(nif);

        for (unsigned int i = 0; i < bones1.size(); ++i)
            bones1[i].post(nif);

        for (unsigned int i = 0; i < bones2.size(); ++i)
            bones2[i].post(nif);
    }
};

class BSValueNode : public NiNode
{
public:
    int value;

    void read(NIFStream *nif)
    {
        NiNode::read(nif);

        value = nif->getInt();

        nif->getChar(); // unknown byte
    }

    void post(NIFFile *nif)
    {
        NiNode::post(nif);
    }
};

class BSOrderedNode : public NiNode
{
public:
    Ogre::Vector4 alphaSortBound;
    unsigned char isStaticBound;

    void read(NIFStream *nif)
    {
        NiNode::read(nif);

        alphaSortBound = nif->getVector4();
        isStaticBound = nif->getChar();
    }

    void post(NIFFile *nif)
    {
        NiNode::post(nif);
    }
};

class BSMultiBoundNode : public NiNode
{
public:
    BSMultiBoundPtr multiBound;
    unsigned int unknown;

    void read(NIFStream *nif)
    {
        NiNode::read(nif);

        multiBound.read(nif);
        if (nifVer >= 0x14020007) // from 20.2.0.7
            unknown = nif->getUInt();
    }

    void post(NIFFile *nif)
    {
        NiNode::post(nif);

        multiBound.post(nif);
    }
};

class BSBlastNode : public NiNode
{
public:
    char unknown1;;
    short unknown2;

    void read(NIFStream *nif)
    {
        NiNode::read(nif);

        unknown1 = nif->getChar();
        unknown2 = nif->getShort();
    }

    void post(NIFFile *nif)
    {
        NiNode::post(nif);
    }
};

class NiSwitchNode : public NiNode
{
public:

    void read(NIFStream *nif)
    {
        NiNode::read(nif);

        if (nifVer >= 0x0a010000) // from 10.1.0.0
            nif->getUShort();

        nif->getInt();
    }

    void post(NIFFile *nif)
    {
        NiNode::post(nif);
    }
};

struct NiBillboardNode : public NiNode
{
    unsigned short billboardMode;

    void read(NIFStream *nif)
    {
        NiNode::read(nif);

        if (nifVer >= 0x0a010000) // from 10.1.0.0
            billboardMode = nif->getUShort();
    }

    void post(NIFFile *nif)
    {
        NiNode::post(nif);
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

    void post(NIFFile *nif)
    {
        Node::post(nif);
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
            bsprops.resize(2);
            bsprops[0].read(nif);
            bsprops[1].read(nif);
        }
    }

    void post(NIFFile *nif)
    {
        Node::post(nif);
        data.post(nif);
        skin.post(nif);
        if (nifVer >= 0x14020007) // from 20.2.0.7 (Skyrim)
        {
            bsprops[0].post(nif);
            bsprops[1].post(nif);
        }
    }

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

    void read(NIFStream *nif)
    {
        NiGeometry::read(nif);

        if (userVer >= 12)
        {
            unknownS2 = nif->getUShort();
            unknownS3 = nif->getUShort();
            unknownI1 = nif->getInt();
        }

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

        if (nifVer >= 0x0a010000) // from 10.1.0.0
            for (unsigned int i = 0; i < modifiers.size(); ++i)
                modifiers[i].post(nif);
    }
};

class BSStripParticleSystem : public NiParticleSystem {};

class BSLODTriShape : public NiGeometry
{
public:
    unsigned int level0Size;
    unsigned int level1Size;
    unsigned int level2Size;

    void read(NIFStream *nif)
    {
        NiGeometry::read(nif);

        level0Size = nif->getUInt();
        level1Size = nif->getUInt();
        level2Size = nif->getUInt();
    }

    void post(NIFFile *nif)
    {
        NiGeometry::post(nif);
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

        for (unsigned int i = 0; i < controllerSequences.size(); ++i)
            controllerSequences[i].post(nif);
        //objectPalette.post(nif);
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

    void read(NIFStream *nif, unsigned int nifVer, std::vector<std::string> *strings)
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
            priority = nif->getChar(); // TODO userVer >= 10
        }

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            stringPalette.read(nif);

        if (nifVer >= 0x14010003) // 20.1.0.3
            nodeName =  nif->getSkyrimString(nifVer, strings);
        else if (nifVer == 0x0a01006a) // 10.1.0.106
            nodeName = nif->getString();

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            nodeNameOffset = nif->getInt();

        if (nifVer >= 0x14010003) // 20.1.0.3
            propertyType =  nif->getSkyrimString(nifVer, strings);
        else if (nifVer == 0x0a01006a) // 10.1.0.106
            propertyType = nif->getString();

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            propertyTypeOffset = nif->getInt();

        if (nifVer >= 0x14010003) // 20.1.0.3
            controllerType =  nif->getSkyrimString(nifVer, strings);
        else if (nifVer == 0x0a01006a) // 10.1.0.106
            controllerType = nif->getString();

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            controllerTypeOffset = nif->getInt();

        if (nifVer >= 0x14010003) // 20.1.0.3
            variable1 =  nif->getSkyrimString(nifVer, strings);
        else if (nifVer == 0x0a01006a) // 10.1.0.106
            variable1 = nif->getString();

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            variable1Offset = nif->getInt();

        if (nifVer >= 0x14010003) // 20.1.0.3
            variable2 =  nif->getSkyrimString(nifVer, strings);
        else if (nifVer == 0x0a01006a) // 10.1.0.106
            variable2 = nif->getString();

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            variable2Offset = nif->getInt();
    }

    void post(NIFFile *nif, unsigned int nifVer)
    {
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            controller.post(nif);

        if (nifVer >= 0x0a01006a) // from 10.1.0.106
        {
            interpolator.post(nif);
            controller2.post(nif);
        }

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            stringPalette.post(nif);
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
        name = nif->getSkyrimString(nifVer, Record::strings);

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
            controlledBlocks[i].read(nif, nifVer, Record::strings);
    }

    void post(NIFFile *nif)
    {
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            textKeys.post(nif);

        for (unsigned int i = 0; i < controlledBlocks.size(); ++i)
            controlledBlocks[i].post(nif, nifVer);
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

            if (nifVer >= 0x14020007 && !Record::strings->empty()) // from 20.2.0.7 (Skyrim)
            {
                unsigned int index = nif->getUInt();
                if (index == -1)
                    targetName = "";
                else
                    targetName = (*Record::strings)[index]; // FIXME: validate index size
            }
            else
                targetName = nif->getString(); // FIXME just a guess

            if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
                stringPalette.read(nif);

            if (nifVer >= 0x14020007 && userVer >= 11 && (userVer2 >= 24 && userVer2 <= 28))
                nif->getInt(); // FIXME BSAnimNotesPtr

            if (nifVer >= 0x14020007 && userVer2 > 28)
                nif->getUShort(); // Unknown Short 1
        }
    }

    void post(NIFFile *nif)
    {
        NiSequence::post(nif);

        if (nifVer >= 0x0a01006a) // from 10.1.0.106
        {
            textKeys2.post(nif);
            manager.post(nif);
            if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
                stringPalette.post(nif);
        }
    }
};

} // Namespace
#endif
