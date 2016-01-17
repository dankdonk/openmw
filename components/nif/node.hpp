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

        if (nifVer < 0x14020007) // less than 20.2.0.7 (or user version <= 11)
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
    //BSMultiBoundDataPtr data;
    void read(NIFStream *nif)
    {
        nif->getInt(); // data.read(nif);
    }
};

class BSMultiBoundOBB : public Record
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
};

class BSMultiBoundNode : public NiNode
{
public:

    void read(NIFStream *nif)
    {
        NiNode::read(nif);

        nif->getInt(); // BSMultiBoundPtr
        if (nifVer >= 0x14020007) // from 20.2.0.7
            nif->getUInt();
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
};

struct TexCoord
{
    float u;
    float v;
};

class BSLightingShaderProperty : public Named
{
public:
    unsigned int skyrimShaderType;
    unsigned int shaderFlags1;
    unsigned int shaderFlags2;
    TexCoord uvOffset;
    TexCoord uvScale;
    //BSShaderTextureSetPtr textureSet;
    Ogre::Vector3 emissiveColor;
    float emissiveMultiple;
    unsigned int textureClampMode;
    float alpha;
    float unknown2;
    float glossiness;
    Ogre::Vector3 specularColor;
    float specularStrength;
    float lightingEffect1;
    float lightingEffect2;

    float envMapScale;
    Ogre::Vector3 skinTintColor;
    Ogre::Vector3 hairTintColor;
    float maxPasses;
    float scale;
    float parallaxInnerLayerThickness;
    float parallaxRefractionScale;
    TexCoord parallaxInnerLayerTextureScale;
    float parallaxEnvmapStrength;
    Ogre::Vector4 sparkleParm;
    float eyeCubemapScale;
    Ogre::Vector3 leftEyeReflectionCenter;
    Ogre::Vector3 rightEyeReflectionCenter;

    void read(NIFStream *nif)
    {
        if (userVer >= 12)
            skyrimShaderType = nif->getUInt();
        else
            skyrimShaderType = 0;

        Named::read(nif);

        if (userVer == 12)
        {
            shaderFlags1 = nif->getUInt();
            shaderFlags2 = nif->getUInt();
        }
        uvOffset.u = nif->getFloat();
        uvOffset.v = nif->getFloat();
        uvScale.u = nif->getFloat();
        uvScale.v = nif->getFloat();
        nif->getInt();//textureSet.read(nif);
        emissiveColor = nif->getVector3();
        emissiveMultiple = nif->getFloat();
        textureClampMode = nif->getUInt();
        alpha = nif->getFloat();
        unknown2 = nif->getFloat();
        glossiness = nif->getFloat();
        specularColor = nif->getVector3();
        specularStrength = nif->getFloat();
        lightingEffect1 = nif->getFloat();
        lightingEffect2 = nif->getFloat();

        if (skyrimShaderType == 1)
            envMapScale = nif->getFloat();
        else if (skyrimShaderType == 5)
            skinTintColor = nif->getVector3();
        else if (skyrimShaderType == 6)
            hairTintColor = nif->getVector3();
        else if (skyrimShaderType == 7)
        {
            maxPasses = nif->getFloat();
            scale = nif->getFloat();
        }
        else if (skyrimShaderType == 11)
        {
            parallaxInnerLayerThickness = nif->getFloat();
            parallaxRefractionScale = nif->getFloat();
            parallaxInnerLayerTextureScale.u = nif->getFloat();
            parallaxInnerLayerTextureScale.v = nif->getFloat();
            parallaxEnvmapStrength = nif->getFloat();
        }
        else if (skyrimShaderType == 14)
            sparkleParm = nif->getVector4();
        else if (skyrimShaderType == 16)
        {
            eyeCubemapScale = nif->getFloat();
            leftEyeReflectionCenter = nif->getVector3();
            rightEyeReflectionCenter = nif->getVector3();
        }
    }
};

class BSEffectShaderProperty : public Named
{
public:
    unsigned int shaderFlags1;
    unsigned int shaderFlags2;
    TexCoord uvOffset;
    TexCoord uvScale;
    std::string sourceTexture;
    unsigned int textureClampMode;
    float falloffStartAngle;
    float falloffStopAngle;
    float falloffStartOpacity;
    float falloffStopOpacity;
    Ogre::Vector4 emissiveColor;
    float emissiveMultiple;
    float softFalloffDepth;
    std::string greyscaleTexture;

    void read(NIFStream *nif)
    {
        Named::read(nif);

        shaderFlags1 = nif->getUInt();
        shaderFlags2 = nif->getUInt();
        uvOffset.u = nif->getFloat();
        uvOffset.v = nif->getFloat();
        uvScale.u = nif->getFloat();
        uvScale.v = nif->getFloat();
        unsigned int size = nif->getUInt();
        sourceTexture = nif->getString(size);
        textureClampMode = nif->getUInt();
        falloffStartAngle = nif->getFloat();
        falloffStopAngle = nif->getFloat();
        falloffStartOpacity = nif->getFloat();
        falloffStopOpacity = nif->getFloat();
        emissiveColor = nif->getVector4();
        emissiveMultiple = nif->getFloat();
        softFalloffDepth = nif->getFloat();
        size = nif->getUInt();
        greyscaleTexture = nif->getString(size);
    }
};

class BSWaterShaderProperty : public Named
{
public:
    unsigned int shaderFlags1;
    unsigned int shaderFlags2;
    TexCoord uvOffset;
    TexCoord uvScale;
    unsigned char waterShaderFlags;
    unsigned char waterDirection;
    unsigned short unknownS3;

    void read(NIFStream *nif)
    {
        Named::read(nif);

        shaderFlags1 = nif->getUInt();
        shaderFlags2 = nif->getUInt();
        uvOffset.u = nif->getFloat();
        uvOffset.v = nif->getFloat();
        uvScale.u = nif->getFloat();
        uvScale.v = nif->getFloat();
        waterShaderFlags = nif->getChar();
        waterDirection = nif->getChar();
        unknownS3 = nif->getUShort();
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
        //materialSkyrim = nif->getUInt(); // not sure if this is version dependent
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
    unsigned int material;
    //unsigned int materialSkyrim;
    float radius;

    void read(NIFStream *nif)
    {
        material = nif->getUInt();
        //materialSkyrim = nif->getUInt(); // not sure if this is version dependent
        radius = nif->getFloat();
    }
};

struct SphereBV
{
    Ogre::Vector3 center;
    float radius;
};

class bhkMultiSphereShape : public bhkShape
{
public:
    unsigned int material;
    //unsigned int materialSkyrim;
    float radius;
    float unknown1;
    float unknown2;
    std::vector<SphereBV> spheres;

    void read(NIFStream *nif)
    {
        material = nif->getUInt();
        //materialSkyrim = nif->getUInt(); // not sure if this is version dependent
        radius = nif->getFloat();
        unknown1 = nif->getFloat();
        unknown2 = nif->getFloat();
        unsigned int numSpheres = nif->getUInt();
        spheres.resize(numSpheres);
        for (unsigned int i = 0; i < numSpheres; ++i)
        {
            spheres[i].center = nif->getVector3();
            spheres[i].radius = nif->getFloat();
        }
    }
};

class bhkTransformShape : public bhkShape
{
public:
    bhkShapePtr shape;
    unsigned int material;
    //unsigned int materialSkyrim;
    Ogre::Vector4 transform;

    void read(NIFStream *nif)
    {
        shape.read(nif);
        material = nif->getUInt();
        //materialSkyrim = nif->getUInt();  // not sure if this is version dependent
        nif->getFloat(); // unknown
        for (int i = 0; i < 8; ++i)
            nif->getChar(); // unknown
        for (int i = 0; i < 16; ++i)
            nif->getFloat(); // FIXME: construct transform
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
    std::vector<unsigned char> unknown;
    float unknownF1;
    Ogre::Vector3 origin;
    float scale;
    std::vector<unsigned char> moppData;

    void read(NIFStream *nif)
    {
        shape.read(nif);
        material = nif->getUInt(); //if userVer >= 12, Skyrim material

        unknown.resize(8);
        for (int i = 0; i < 8; ++i)
            unknown[i] = nif->getChar();

        unknownF1 = nif->getFloat();

        unsigned int dataSize = nif->getUInt();
        origin = nif->getVector3();
        scale = nif->getFloat();

        moppData.resize(dataSize);
        for (unsigned int i = 0; i < dataSize; ++i)
            moppData[i] = nif->getChar();

        if (nifVer >= 0x14020005 && userVer >= 12) // from 20.2.0.7
            nif->getChar(); // Unknown Byte1
    }

    void post(NIFFile *nif)
    {
        // FIXME
    }
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

    void read(NIFStream *nif)
    {
        target.read(nif);
        materialSkyrim = nif->getUInt();
        nif->getFloat();
        unknown.resize(4);
        for (int i = 0; i < 4; ++i)
            unknown[i] = nif->getChar();
        nif->getVector4();
        radius = nif->getFloat();
        scale = nif->getFloat();
        nif->getFloat();
        nif->getFloat();
        nif->getFloat();
        data.read(nif);
    }

    void post(NIFFile *nif)
    {
        // FIXME
    }
};

class bhkConstraint : public Record
{
public:
    std::vector<bhkEntityPtr> entities;
    unsigned int priority;

    void read(NIFStream *nif)
    {
        unsigned int numEntities = nif->getUInt();
        entities.resize(numEntities);
        for (unsigned int i = 0; i < numEntities; ++i)
        {
            entities[i].read(nif);
        }
        priority = nif->getUInt();
    }
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

    void read(NIFStream *nif, unsigned int nifVer)
    {
        if (nifVer <= 0x14000005)
        {
            pivotA = nif->getVector4();
            planeA = nif->getVector4();
            twistA = nif->getVector4();
            planeB = nif->getVector4();
            pivotB = nif->getVector4();
            twistB = nif->getVector4();
        }

        if (nifVer >= 0x14020007)
        {
            twistA = nif->getVector4();
            planeA = nif->getVector4();
            motorA = nif->getVector4();
            pivotA = nif->getVector4();
            twistB = nif->getVector4();
            planeB = nif->getVector4();
            motorB = nif->getVector4();
            pivotB = nif->getVector4();
        }

        coneMaxAngle = nif->getFloat();
        planeMinAngle = nif->getFloat();
        planeMaxAngle = nif->getFloat();
        twistMinAngle = nif->getFloat();
        twistMaxAngle = nif->getFloat();
        maxFriction = nif->getFloat();

        if (nifVer >= 0x14020007)
        {
            bool enableMotor = nif->getBool(nifVer);
            if (enableMotor)
            {
                nif->getFloat(); // unknown float 1
                nif->getFloat(); // unknown float 2
                nif->getFloat(); // unknown float 3
                nif->getFloat(); // unknown float 4
                nif->getFloat(); // unknown float 5
                nif->getFloat(); // unknown float 6
                nif->getChar();  // unknown byte 1
            }
        }
    }
};

class bhkRagdollConstraint : public bhkConstraint
{
public:
    RagdollDescriptor ragdoll;

    void read(NIFStream *nif)
    {
        bhkConstraint::read(nif);
        ragdoll.read(nif, nifVer);
    }
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

    void read(NIFStream *nif, unsigned int nifVer)
    {
        if (nifVer <= 0x14000005)
        {
            pivotA = nif->getVector4();
            axleA = nif->getVector4();
            perp2AxleA1 = nif->getVector4();
            perp2AxleA2 = nif->getVector4();
            pivotB = nif->getVector4();
            axleB = nif->getVector4();
            perp2AxleB2 = nif->getVector4();
        }

        if (nifVer >= 0x14020007)
        {
            axleA = nif->getVector4();
            perp2AxleA1 = nif->getVector4();
            perp2AxleA2 = nif->getVector4();
            pivotA = nif->getVector4();
            axleB = nif->getVector4();
            perp2AxleB1 = nif->getVector4();
            perp2AxleB2 = nif->getVector4();
            pivotB = nif->getVector4();
        }

        minAngle = nif->getFloat();
        maxAngle = nif->getFloat();
        maxFriction = nif->getFloat();

        if (nifVer >= 0x14020007)
        {
            enableMotor = nif->getBool(nifVer);
            if (enableMotor)
            {
                nif->getFloat(); // unknown float 1
                nif->getFloat(); // unknown float 2
                nif->getFloat(); // unknown float 3
                nif->getFloat(); // unknown float 4
                nif->getFloat(); // unknown float 5
                nif->getFloat(); // unknown float 6
                nif->getChar();  // unknown byte 1
            }
        }
    }
};

class bhkLimitedHingeConstraint : public bhkConstraint
{
public:
    LimitedHingeDescriptor limitedHinge;

    void read(NIFStream *nif)
    {
        bhkConstraint::read(nif);
        limitedHinge.read(nif, nifVer);
    }
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

    void read(NIFStream *nif)
    {
        bhkConstraint::read(nif);

        if (nifVer <= 0x14000005)
        {
            pivotA = nif->getVector4();
            rotationMatrixA.resize(4);
            for (int i = 0; i < 4; ++i)
                rotationMatrixA[i] = nif->getVector4();
            pivotB = nif->getVector4();
            slidingB = nif->getVector4();
            planeB = nif->getVector4();
        }

        if (nifVer >= 0x14020007)
        {
            slidingA = nif->getVector4();
            rotationA = nif->getVector4();
            planeA = nif->getVector4();
            pivotA = nif->getVector4();
            slidingB = nif->getVector4();
            rotationB = nif->getVector4();
            planeB = nif->getVector4();
            pivotB = nif->getVector4();
        }

        minDistance = nif->getFloat();
        maxDistance = nif->getFloat();
        friction = nif->getFloat();

        if (nifVer >= 0x14020007)
            nif->getChar();
    }
};

class bhkStiffSpringConstraint : public bhkConstraint
{
public:
    Ogre::Vector4 pivotA;
    Ogre::Vector4 pivotB;
    float length;

    void read(NIFStream *nif)
    {
        bhkConstraint::read(nif);

        pivotA = nif->getVector4();
        pivotB = nif->getVector4();
        length = nif->getFloat();
    }
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

    void read(NIFStream *nif)
    {
        bhkConstraint::read(nif);

        if (userVer <= 11)
        {
            for (int i = 0; i < 41; ++i)
                nif->getInt();
            nif->getShort();
        }

        if (userVer == 12)
        {
            unknownI1 = nif->getUInt();

            unsigned int numEnt2 = nif->getUInt();
            entities2.resize(numEnt2);
            for (unsigned int i = 0; i < numEnt2; ++i)
                entities2[i].read(nif);

            priority2 = nif->getUInt();

            unknownI2 = nif->getUInt();
            position = nif->getVector3();
            rotation = nif->getVector3();
            unknownI3 = nif->getUInt();
            threshold = nif->getFloat();
            if (unknownI1 >= 1)
                unknownF1 = nif->getFloat();
            nif->getChar();
        }
    }
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

    void read(NIFStream *nif, unsigned int nifVer)
    {
        if (nifVer <= 0x14000005)
        {
            pivotA = nif->getVector4();
            perp2AxleA1 = nif->getVector4();
            perp2AxleA2 = nif->getVector4();
            pivotB = nif->getVector4();
            axleB = nif->getVector4();
        }

        if (nifVer >= 0x14020007)
        {
            axleA = nif->getVector4();
            perp2AxleA1 = nif->getVector4();
            perp2AxleA2 = nif->getVector4();
            pivotA = nif->getVector4();
            axleB = nif->getVector4();
            perp2AxleB1 = nif->getVector4();
            perp2AxleB2 = nif->getVector4();
            pivotB = nif->getVector4();
        }
    }
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

    void read(NIFStream *nif)
    {
        bhkConstraint::read(nif);

        type = nif->getUInt();
        unknown2 = nif->getUInt();
        link1.read(nif);
        link2.read(nif);
        unknown3 = nif->getUInt();

        if (type == 1)
            hinge.read(nif, nifVer);
        else if (type == 2)
            limitedHinge.read(nif, nifVer);
        else if (type == 7)
            ragdoll.read(nif, nifVer);

        if (nifVer <= 0x14000005) // up to 20.0.0.5
            tau = nif->getFloat();
        damping = nif->getFloat();
    }
};

class bhkEntity : public Record
{
public:

    bhkShapePtr shape;
    unsigned char layer; // http://niftools.sourceforge.net/doc/nif/OblivionLayer.html
    unsigned char collisionFilter;
    unsigned short unknownShort;

    void read(NIFStream *nif)
    {
        shape.read(nif);
        layer = nif->getChar();
        collisionFilter = nif->getChar();
        unknownShort = nif->getUShort();
    }
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

    void read(NIFStream *nif)
    {
        bhkEntity::read(nif);

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
        if (userVer >= 12)
        {
            gravityFactor1 = nif->getFloat();
            gravityFactor2 = nif->getFloat();
        }
        friction = nif->getFloat();
        if (userVer >= 12)
            rollingFrictionMultiplier = nif->getFloat();
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
        if (userVer >= 12)
            nif->getInt();
        unsigned int numConst = nif->getUInt();
        constraints.resize(numConst);
        for(size_t i = 0; i < numConst; i++)
            constraints[i].read(nif);
        if (userVer <= 11)
            unknownInt9 = nif->getInt();
        if (userVer >= 12)
            unknownS9 = nif->getUShort();
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

class bhkBlendCollisionObject : public bhkCollisionObject
{
public:
    float unknown1;
    float unknown2;

    void read(NIFStream *nif)
    {
        bhkCollisionObject::read(nif);
        unknown1 = nif->getFloat();
        unknown2 = nif->getFloat();
    }
};

struct bhkSimpleShapePhantom: public Record
{
    bhkShapePtr shape;
    unsigned char oblivionLayer;
    unsigned char colFilter;

    void read(NIFStream *nif)
    {
        shape.read(nif);
        oblivionLayer = nif->getChar();
        colFilter = nif->getChar();

        nif->getUShort();
        for (unsigned int i = 0; i < 23; ++i) // 7 + 3*5 +1
            nif->getFloat();
    }
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

        if (nifVer >= 0x1401003) // 20.1.0.3
            getSkyrimString(nodeName, nif, nifVer, strings);
        else if (nifVer == 0x0a01006a) // 10.1.0.106
            nodeName = nif->getString();

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            nodeNameOffset = nif->getInt();

        if (nifVer >= 0x1401003) // 20.1.0.3
            getSkyrimString(propertyType, nif, nifVer, strings);
        else if (nifVer == 0x0a01006a) // 10.1.0.106
            propertyType = nif->getString();

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            propertyTypeOffset = nif->getInt();

        if (nifVer >= 0x1401003) // 20.1.0.3
            getSkyrimString(controllerType, nif, nifVer, strings);
        else if (nifVer == 0x0a01006a) // 10.1.0.106
            controllerType = nif->getString();

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            controllerTypeOffset = nif->getInt();

        if (nifVer >= 0x1401003) // 20.1.0.3
            getSkyrimString(variable1, nif, nifVer, strings);
        else if (nifVer == 0x0a01006a) // 10.1.0.106
            variable1 = nif->getString();

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            variable1Offset = nif->getInt();

        if (nifVer >= 0x1401003) // 20.1.0.3
            getSkyrimString(variable2, nif, nifVer, strings);
        else if (nifVer == 0x0a01006a) // 10.1.0.106
            variable2 = nif->getString();

        if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            variable2Offset = nif->getInt();
    }

private:
    void getSkyrimString(std::string& str, NIFStream *nif, unsigned int nifVer, std::vector<std::string> *strings)
    {
        if (nifVer >= 0x14020007 && !strings->empty()) // from 20.2.0.7 (Skyrim)
        {
            unsigned int index = nif->getUInt();
            if (index == -1)
                str = "";
            else
                str = (*strings)[index]; // FIXME: validate index size
        }
        else
            str = nif->getString(); // FIXME just a guess
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
    }
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

    void read(NIFStream *nif, unsigned int nifVer)
    {
        translation = nif->getVector4();
        materialIndex = nif->getUInt();
        unknown1 = nif->getUShort();
        transformIndex = nif->getUShort();
        unsigned int numVert = nif->getUInt();
        vertices.resize(numVert);
        for (unsigned int i = 0; i < numVert; ++i)
            vertices[i] = nif->getUShort();
        unsigned int numIndicies = nif->getUInt();
        indicies.resize(numIndicies);
        for (unsigned int i = 0; i < numIndicies; ++i)
            indicies[i] = nif->getUShort();
        unsigned int numStrips = nif->getUInt();
        strips.resize(numStrips);
        for (unsigned int i = 0; i < numStrips; ++i)
            strips[i] = nif->getUShort();
        unsigned int numIndicies2 = nif->getUInt();
        indicies2.resize(numIndicies2);
        for (unsigned int i = 0; i < numIndicies2; ++i)
            indicies2[i] = nif->getUShort();
    }
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

    void read(NIFStream *nif)
    {
        bitsPerIndex = nif->getUInt();
        bitsPerWIndex = nif->getUInt();
        maskWIndex = nif->getUInt();
        maskIndex = nif->getUInt();
        error = nif->getFloat();
        boundsMin = nif->getVector4();
        boundsMax = nif->getVector4();
        nif->getChar();
        nif->getInt();
        nif->getInt();
        nif->getInt();
        nif->getChar();
        unsigned int numMat = nif->getUInt();
        chunkMaterials.resize(numMat);
        for (unsigned int i = 0; i < numMat; ++i)
        {
            chunkMaterials[i].skyrimMaterial = nif->getUInt();
            chunkMaterials[i].unknown = nif->getUInt();
        }
        nif->getInt();
        unsigned int numTrans = nif->getUInt();
        chunkTransforms.resize(numTrans);
        for (unsigned int i = 0; i < numTrans; ++i)
        {
            chunkTransforms[i].translation = nif->getVector4();
            chunkTransforms[i].rotation = nif->getQuaternion();
        }
        unsigned int numBigVerts = nif->getUInt();
        bigVerts.resize(numBigVerts);
        for (unsigned int i = 0; i < numBigVerts; ++i)
            bigVerts[i] = nif->getVector4();
        unsigned int numBigTris = nif->getUInt();
        bigTris.resize(numBigTris);
        for (unsigned int i = 0; i < numBigTris; ++i)
        {
            bigTris[i].triangle1 = nif->getUShort();
            bigTris[i].triangle2 = nif->getUShort();
            bigTris[i].triangle3 = nif->getUShort();
            bigTris[i].unknown1 = nif->getUInt();
            bigTris[i].unknown2 = nif->getUShort();
        }
        unsigned int numChunks = nif->getUInt();
        chunks.resize(numChunks);
        for (unsigned int i = 0; i < numChunks; ++i)
            chunks[i].read(nif, nifVer);
        nif->getInt();
    }
};

} // Namespace
#endif
