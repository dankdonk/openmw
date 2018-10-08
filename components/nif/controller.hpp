/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (controller.h) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_CONTROLLER_HPP
#define OPENMW_COMPONENTS_NIF_CONTROLLER_HPP

#include "base.hpp"
#include "node.hpp" // NodeGroup

namespace Nif
{

class NiParticleSystemController : public Controller
{
public:
    struct Particle {
        Ogre::Vector3 velocity;
        float lifetime;
        float lifespan;
        float timestamp;
        int vertex;
    };

    float velocity;
    float velocityRandom;

    float verticalDir; // 0=up, pi/2=horizontal, pi=down
    float verticalAngle;
    float horizontalDir;
    float horizontalAngle;

    float size;
    float startTime;
    float stopTime;

    float emitRate;
    float lifetime;
    float lifetimeRandom;

    enum EmitFlags
    {
        NoAutoAdjust = 0x1 // If this flag is set, we use the emitRate value. Otherwise,
                           // we calculate an emit rate so that the maximum number of particles
                           // in the system (numParticles) is never exceeded.
    };
    int emitFlags;

    Ogre::Vector3 offsetRandom;

    NodePtr emitter;

    int numParticles;
    int activeCount;
    std::vector<Particle> particles;

    NiParticleModifierPtr extra;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        velocity = nif->getFloat();
        velocityRandom = nif->getFloat();
        verticalDir = nif->getFloat();
        verticalAngle = nif->getFloat();
        horizontalDir = nif->getFloat();
        horizontalAngle = nif->getFloat();
        /*normal?*/ nif->getVector3();
        /*color?*/ nif->getVector4();
        size = nif->getFloat();
        startTime = nif->getFloat();
        stopTime = nif->getFloat();
        nif->getChar();
        emitRate = nif->getFloat();
        lifetime = nif->getFloat();
        lifetimeRandom = nif->getFloat();

        emitFlags = nif->getUShort();
        offsetRandom = nif->getVector3();

        emitter.read(nif);

        /* Unknown Short, 0?
         * Unknown Float, 1.0?
         * Unknown Int, 1?
         * Unknown Int, 0?
         * Unknown Short, 0?
         */
        nif->skip(16);

        numParticles = nif->getUShort();
        activeCount = nif->getUShort();

        particles.resize(numParticles);
        for(size_t i = 0;i < particles.size();i++)
        {
            particles[i].velocity = nif->getVector3();
            nif->getVector3(); /* unknown */
            particles[i].lifetime = nif->getFloat();
            particles[i].lifespan = nif->getFloat();
            particles[i].timestamp = nif->getFloat();
            nif->getUShort(); /* unknown */
            particles[i].vertex = nif->getUShort();
        }

        nif->getUInt(); /* -1? */
        extra.read(nif); // Ref<NiParticleModifier>
        nif->getUInt(); /* -1? */
        nif->getChar();
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        emitter.post(nif);
        extra.post(nif);
    }
};
typedef NiParticleSystemController NiBSPArrayController;

class NiMaterialColorController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    unsigned short targetColor;
    NiPosDataPtr data;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);
        if (nifVer >= 0x0a010000) // from 10.1.0.0
            targetColor = nif->getUShort();
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            data.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            data.post(nif);
    }
};

class NiPathController : public Controller
{
public:
    NiPosDataPtr posData;
    NiFloatDataPtr floatData;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        if (nifVer >= 0x0a010000) // from 10.1.0.0
            nif->getUShort();

        /*
           int = 1
           2xfloat
           short = 0 or 1
        */
        nif->skip(14);
        posData.read(nif);
        floatData.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);

        posData.post(nif);
        floatData.post(nif);
    }
};

class NiUVController : public Controller
{
public:
    NiUVDataPtr data;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        nif->getUShort(); // always 0
        data.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);
    }
};

class NiKeyframeController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    NiKeyframeDataPtr data;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            data.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            data.post(nif);
    }
};

class NiAlphaController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    NiFloatDataPtr data;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            data.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            data.post(nif);
    }
};

struct MorphWeight
{
    NiInterpolatorPtr interpolator;
    float weight;
};

class NiGeomMorpherController : public Controller
{
public:
    unsigned short extraFlags;
    NiMorphDataPtr data;
    std::vector<NiInterpolatorPtr> interpolators;
    std::vector<MorphWeight> interpolatorWeights;
    std::vector<unsigned int> unknownInts;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a000102) // from 10.0.1.2
            extraFlags = nif->getUShort();
        if (nifVer == 0x0a01006a) // 10.1.0.106
            nif->getChar();
        data.read(nif);
        nif->getChar(); // always 0
        if (nifVer >= 0x0a01006a) // from 10.1.0.106
        {
            unsigned int numInterpolators = nif->getUInt();
            if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            {
                interpolators.resize(numInterpolators);
                for (unsigned int i = 0; i < numInterpolators; ++i)
                    interpolators[i].read(nif);
            }
            if (nifVer >= 0x14010003) // from 20.1.0.3
            {
                interpolatorWeights.resize(numInterpolators);
                for (unsigned int i = 0; i < numInterpolators; ++i)
                {
                    interpolatorWeights[i].interpolator.read(nif);
                    interpolatorWeights[i].weight = nif->getFloat();
                }
            }
            if (nifVer >= 0x14000004 && nifVer <= 0x14000005 && userVer >=10)
            {
                unsigned int count = nif->getUInt();
                unknownInts.resize(count);
                for (unsigned int i = 0; i < count; ++i)
                    unknownInts[i] = nif->getUInt();
            }
        }
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        data.post(nif);

        if (nifVer >= 0x0a01006a) // from 10.1.0.106
        {
            if (nifVer >= 0x0a020000 && nifVer <= 0x14000005)
            {
                for (unsigned int i = 0; i < interpolators.size(); ++i)
                    interpolators[i].post(nif);
            }
        }
    }
};

class NiVisController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    NiVisDataPtr data;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            data.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            data.post(nif);
    }
};

class NiFlipController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    int mTexSlot; // NiTexturingProperty::TextureType
    float mDelta; // Time between two flips. delta = (start_time - stop_time) / num_sources
    NiSourceTextureList mSources;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);
        mTexSlot = nif->getUInt();
        if (nifVer >= 0x04000000  && nifVer <= 0x0a010000)
            /*unknown=*/nif->getUInt();/*0?*/
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            mDelta = nif->getFloat();
        mSources.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
        mSources.post(nif);
    }
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

} // Namespace
#endif
