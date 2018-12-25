#ifndef OPENMW_COMPONENTS_NIF_FOREIGN_HPP
#define OPENMW_COMPONENTS_NIF_FOREIGN_HPP

#include "record.hpp"
#include "recordptr.hpp"

#include "controller.hpp"
#include "controlled.hpp"

namespace Nif
{
;
/* --------------------------------------------------------- */


class NiPSysModifier : public Record
{
public:
    std::string name;
    unsigned int order;
    NiParticleSystemPtr target;
    bool active;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSWindModifier : public NiPSysModifier
{
public:
    float strength;

    void read(NIFStream *nif);
};

class BSPSysSubTexModifier : public NiPSysModifier
{
public:
    unsigned int startFrame;
    float startFrameFudge;
    float endFrame;
    float loopStartFrame;
    float loopStartFrameFudge;
    float frameCount;
    float frameCountFudge;

    void read(NIFStream *nif);
};

class NiPSysBombModifier : public NiPSysModifier
{
public:
    NiNodePtr bombObject;
    Ogre::Vector3 bombAxis;
    float decay;
    float deltaV;
    unsigned int decayType;
    unsigned int symmetryType;

    void read(NIFStream *nif);
};

class BSPSysInheritVelocityModifier : public NiPSysModifier
{
public:
    unsigned int unknownI1;
    float unknownF1;
    float unknownF2;
    float unknownF3;

    void read(NIFStream *nif);
};

class BSPSysLODModifier : public NiPSysModifier
{
public:
    float unknown1;
    float unknown2;
    float unknown3;
    float unknown4;

    void read(NIFStream *nif);
};

class BSPSysScaleModifier : public NiPSysModifier
{
public:
    std::vector<float> floats;

    void read(NIFStream *nif);
};

class BSPSysSimpleColorModifier : public NiPSysModifier
{
public:
    float fadeInPercent;
    float fadeOutPercent;
    float color1EndPerCent;
    float color1StartPerCent;
    float color2EndPerCent;
    float color2StartPerCent;
    std::vector<Ogre::Vector4> colors;

    void read(NIFStream *nif);
};

class BSPSysStripUpdateModifier : public NiPSysModifier
{
public:
    float updateDeltaTime;

    void read(NIFStream *nif);
};

class NiPSysEmitter : public NiPSysModifier
{
public:
    float speed;
    float speedVar;
    float declination;
    float declinationVar;
    float planarAngle;
    float planarAngleVar;
    Ogre::Vector4 initialColor;
    float initialRadius;
    float radiusVar;
    float lifeSpan;
    float lifeSpanVar;

    void read(NIFStream *nif);
};

class NiPSysBoxEmitter : public NiPSysEmitter
{
public:
    NodePtr emitteObj;
    float width;
    float height;
    float depth;

    void read(NIFStream *nif);
};

class NiPSysCylinderEmitter : public NiPSysEmitter
{
public:
    NodePtr emitteObj;
    float radius;
    float height;

    void read(NIFStream *nif);
};

class NiPSysMeshEmitter : public NiPSysEmitter
{
public:
    std::vector<NiGeometryPtr> emitterMeshes; // NiTriBasedGeom
    unsigned int velocityType;
    unsigned int emissionType;
    Ogre::Vector3 emissionAxis;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiPSysSphereEmitter : public NiPSysEmitter
{
public:
    NodePtr emitteObj;
    float radius;

    void read(NIFStream *nif);
};

class NiPSysAgeDeathModifier : public NiPSysModifier
{
public:
    bool spawnOnDeath;
    NiPSysSpawnModifierPtr spawnModifier;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiPSysDragModifier : public NiPSysModifier
{
public:
    NodePtr parent;
    Ogre::Vector3 dragAxis;
    float percent;
    float range;
    float rangeFalloff;

    void read(NIFStream *nif);
};

class NiPSysSpawnModifier : public NiPSysModifier
{
public:
    unsigned short numSpawnGen;
    float percentSpawn;
    unsigned short minSpawn;
    unsigned short maxSpawn;
    float spawnSpeedChaos;
    float spawnDirChaos;
    float lifeSpan;
    float lifeSpanVar;

    void read(NIFStream *nif);
};

class NiPSysGrowFadeModifier : public NiPSysModifier
{
public:
    float growTime;
    unsigned short growGen;
    float fadeTime;
    unsigned short fadeGen;
    float baseScale;

    void read(NIFStream *nif);
};

class NiPSysColorModifier : public NiPSysModifier
{
public:
    NiColorDataPtr colorData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiPSysGravityModifier : public NiPSysModifier
{
public:
    NodePtr gravityObj;
    Ogre::Vector3 gravityAxis;
    float decay;
    float strength;
    unsigned int forceType;
    float turbulence;
    float turbulenceScale;

    void read(NIFStream *nif);
};

class NiPSysPositionModifier : public NiPSysModifier {};

class NiPSysBoundUpdateModifier : public NiPSysModifier
{
public:

    void read(NIFStream *nif);
};

class NiPSysRotationModifier : public NiPSysModifier
{
public:
    float initialRotSpeed;
    float initialRotSpeedVar;
    float initialRotAngle;
    float initialRotAngleVar;
    bool randomRotSpeedSign;

    void read(NIFStream *nif);
};

class BSParentVelocityModifier : public NiPSysModifier
{
public:
    float damping;

    void read(NIFStream *nif);
};

class NiPSysColliderManager : public NiPSysModifier
{
public:
    NiPSysColliderPtr collider;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiPSysCollider : public Record
{
public:
    float bounce;
    bool spawnOnCollide;
    bool dieOnCollide;
    //NiPSysSpawnModifierPtr spawnModifier;
    NodePtr parent;
    NodePtr nextCollider;
    NodePtr colliderObj;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiPSysPlanarCollider : public NiPSysCollider
{
public:
    float width;
    float height;
    Ogre::Vector3 xAxis;
    Ogre::Vector3 yAxis;

    void read(NIFStream *nif);
};

class NiPSysSphericalCollider : public NiPSysCollider
{
public:
    float radius;

    void read(NIFStream *nif);
};


/* --------------------------------------------------------- */


class NiPSysEmitterCtlr : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string modifierName;
    NiInterpolatorPtr visibilityInterpolator;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);
        modifierName = nif->getSkyrimString(nifVer, Record::strings);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            nif->getInt(); // NiPSysEmtterCtlrDataPtr
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            visibilityInterpolator.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
        //if (nifVer <= 0x0a010000) // up to 10.1.0.0
            // NiPSysEmtterCtlrDataPtr
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            visibilityInterpolator.post(nif);
    }
};

class NiPSysUpdateCtlr : public Controller {};

class NiPSysModifierActiveCtlr : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string modifierName;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiPSysEmitterInitialRadiusCtlr : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string modifierName;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiPSysEmitterLifeSpanCtlr : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string modifierName;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiPSysEmitterSpeedCtlr : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string modifierName;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiTextureTransformController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    unsigned int textureSlot;
    unsigned int operation;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiPSysGravityStrengthCtlr : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string modifierName;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

struct SkinShape
{
    NiGeometryPtr shape;
    NiSkinInstancePtr skin;

    void read(NIFStream *nif, unsigned int nifVer);
    void post(NIFFile *nif);
};

struct SkinShapeGroup
{
    std::vector<SkinShape> linkPairs;

    void read(NIFStream *nif, unsigned int nifVer);
    void post(NIFFile *nif);
};

class NiBSBoneLODController : public Controller
{
public:
    std::vector<NodeGroup> nodeGroups;
    std::vector<SkinShapeGroup> shapeGroups;
    std::vector<NiGeometryPtr> shapeGroups2;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class bhkBlendController : public Controller
{
public:
    unsigned int unknown;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiFloatExtraDataController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string controllerData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiInterpolator : public Record
{
    void read(NIFStream *nif) {}; // FIXME: for debugging (printing recName)
};

class NiBoolInterpolator : public NiInterpolator
{
public:
    bool value;
    NiBoolDataPtr boolData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiBoolTimelineInterpolator : public NiBoolInterpolator {};

class NiBlendBoolInterpolator : public NiInterpolator
{
public:
    unsigned char value;

    void read(NIFStream *nif);
};

class NiFloatInterpolator : public NiInterpolator
{
public:
    float value;
    NiFloatDataPtr floatData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiBlendFloatInterpolator : public NiInterpolator
{
public:
    float value;

    void read(NIFStream *nif);
};

class NiBlendTransformInterpolator : public NiInterpolator
{
public:
    unsigned short unknown1;
    unsigned int unknown2;

    void read(NIFStream *nif);
};

class NiPathInterpolator : public NiInterpolator
{
public:
    NiPosDataPtr posData;
    NiFloatDataPtr floatData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiPoint3Interpolator : public NiInterpolator
{
public:
    Ogre::Vector3 value;
    NiPosDataPtr posData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

// should inherit from NiBlendInterpolator
class NiBlendPoint3Interpolator : public NiInterpolator
{
public:
    Ogre::Vector3 value;

    void read(NIFStream *nif);
};

class NiTransformInterpolator : public NiInterpolator
{
public:
    Ogre::Vector3 translation;
    Ogre::Quaternion rotation;
    float scale;
    NiTransformDataPtr transformData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiLookAtInterpolator : public NiInterpolator
{
public:
    unsigned short unknown;
    NiNodePtr lookAt;
    std::string target;
    Ogre::Vector3 translation;
    Ogre::Quaternion rotation;
    float scale;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiBSplineInterpolator : public NiInterpolator
{
public:
    float startTime;
    float stopTime;
    NiBSplineDataPtr splineData;
    NiBSplineBasisDataPtr basisData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiBSplineFloatInterpolator : public NiBSplineInterpolator {};

class NiBSplinePoint3Interpolator : public NiBSplineInterpolator
{
public:
    float unknown1;
    float unknown2;
    float unknown3;
    float unknown4;
    float unknown5;
    float unknown6;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiBSplineTransformInterpolator : public NiBSplineInterpolator
{
public:
    Ogre::Vector3 translation;
    Ogre::Quaternion rotation;
    float scale;
    unsigned int translationOffset;
    unsigned int rotationOffset;
    unsigned int scaleOffset;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiBSplineCompTransformInterpolator : public NiBSplineTransformInterpolator
{
public:
    float translationBias;
    float translationMultiplier;
    float rotationBias;
    float rotationMultiplier;
    float scaleBias;
    float scaleMultiplier;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiMultiTargetTransformController : public Controller
{
public:
    unsigned short numExtraTargets;
    std::vector<NamedPtr> extraTargets;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSFrustumFOVController : public Controller
{
public:
    NiFloatInterpolatorPtr interpolator;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiControllerManager : public Controller
{
public:
    bool cumulative;
    std::vector<NiControllerSequencePtr> controllerSequences;
    //NiDefaultAvObjectPalettePtr objectPalette;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
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

    void read(NIFStream *nif, unsigned int nifVer, std::vector<std::string> *strings);
    void post(NIFFile *nif, unsigned int nifVer);
};

class NiSequence : public Record
{
public:
    std::string name;
    std::string textKeysName;
    NiTextKeyExtraDataPtr textKeys;
    std::vector<ControllerLink> controlledBlocks;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiControllerSequence : public NiSequence
{
public:
    float weight;
    NiTextKeyExtraDataPtr textKeys;
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

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSLightingShaderPropertyColorController : public Controller
{
public:
    unsigned int targetVariable;
    NiInterpolatorPtr interpolator;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

// FIXME looks identical to BSLightingShaderPropertyColorController
class BSLightingShaderPropertyFloatController : public Controller
{
public:
    unsigned int targetVariable;
    NiInterpolatorPtr interpolator;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

// FIXME looks identical to BSLightingShaderPropertyColorController
class BSEffectShaderPropertyColorController : public Controller
{
public:
    unsigned int targetVariable;
    NiInterpolatorPtr interpolator;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

// FIXME looks identical to BSLightingShaderPropertyColorController
class BSEffectShaderPropertyFloatController : public Controller
{
public:
    unsigned int targetVariable;
    NiInterpolatorPtr interpolator;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSLagBoneController : public Controller
{
public:
    float linearVelocity;
    float linearRotation;
    float maxDistance;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

// replaces NiKeyframeController
class NiTransformController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    NiKeyframeDataPtr data;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

// construct NiAlphaController instead
//class BSNiAlphaPropertyTestRefController : public NiAlphaController {};

struct Particle
{
    Ogre::Vector3 translation;
    std::vector<float> unknownFloats;
    float unknown1;
    float unknown2;
    float unknown3;
    int unknown;

    void read(NIFStream *nif, unsigned int nifVer);
};

class NiPSysData : public NiRotatingParticlesData
{
public:
    std::vector<Particle> particleDesc;
    std::vector<float> unknownFloats3;
    bool hasSubTextureUVs;
    std::vector<Ogre::Vector4> subTextureUVs;
    float aspectRatio;

    void read(NIFStream *nif);
};

class BSStripPSysData : public NiPSysData
{
public:
    short unknown5;
    char unknown6;
    int unknown7;
    float unknown8;

    void read(NIFStream *nif);
};

class NiBoolData : public Record
{
public:
    BoolKeyMap mKeyList;

    void read(NIFStream *nif)
    {
        mKeyList.read(nif);
    }
};

class BSDismemberSkinInstance : public NiSkinInstance
{
public:
    // TODO

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiSkinPartition : public Record
{
public:
    struct Triangle
    {
        unsigned short v1;
        unsigned short v2;
        unsigned short v3;
    };

    struct SkinPartitionBlock
    {
        unsigned short numVerts;
        unsigned short numTriangles;
        unsigned short numBones;
        unsigned short numStrips;
        unsigned short numWeightsPerVert;
        std::vector<unsigned short> bones;
        bool hasVertMap;
        std::vector<unsigned short> vertMap;
        bool hasVertWeights;
        std::vector<std::vector<float> > vertWeights;
        std::vector<unsigned short> stripLengths;
        bool hasFaces;
        std::vector<std::vector<float> > strips;
        std::vector<Triangle> triangles;
        bool hasBoneIndicies;
        std::vector<std::vector<unsigned char> > boneIndicies;

        void read(NIFStream *nif, unsigned int nifVer, unsigned int userVer);
    };

    unsigned int numSkinPartitionBlocks;
    std::vector<SkinPartitionBlock> skinPartitionBlocks;

    void read(NIFStream *nif);
};

struct NiTransformData : public NiKeyframeData {};

// Ogre doesn't seem to allow meshes to be created using triangle strips
// (unless using ManualObject)
class NiTriStripsData : public ShapeData
{
public:
    std::vector<unsigned short> stripLengths;
    std::vector<std::vector<unsigned short> > points;

    // Triangles, three vertex indices per triangle
    std::vector<short> triangles;

    void read(NIFStream *nif);
};

struct NiBSplineData : public Record
{
    std::vector<float> floatControlPoints;
    std::vector<short> shortControlPoints;

    void read(NIFStream *nif);
};

struct NiBSplineBasisData : public Record
{
    unsigned int numControlPoints;

    void read(NIFStream *nif);
};

class NiStringPalette : public Record
{
public:
    std::vector<char> buffer;
    //std::vector<std::string> palette;

    void read(NIFStream *nif);
};

struct AVObject
{
    std::string name;
    NodePtr avObject;

    void post(NIFFile *nif);
};

struct NiDefaultAVObjectPalette : public Record
{
    std::vector<AVObject> objs;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSShaderTextureSet : public Record
{
public:
    std::vector<std::string> textures;

    void read(NIFStream *nif);
};
}
#endif
