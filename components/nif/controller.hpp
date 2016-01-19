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

class NiInterpolator : public Record
{
    void read(NIFStream *nif) {}; // FIXME: for debugging (printing recName)
};

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

class BSLightingShaderPropertyColorController : public Controller
{
public:
    unsigned int targetVariable;
    NiInterpolatorPtr interpolator;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);

        targetVariable = nif->getUInt();
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
    }
};

// FIXME looks identical to BSLightingShaderPropertyColorController
class BSLightingShaderPropertyFloatController : public Controller
{
public:
    unsigned int targetVariable;
    NiInterpolatorPtr interpolator;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);

        targetVariable = nif->getUInt();
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
    }
};

// FIXME looks identical to BSLightingShaderPropertyColorController
class BSEffectShaderPropertyColorController : public Controller
{
public:
    unsigned int targetVariable;
    NiInterpolatorPtr interpolator;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);

        targetVariable = nif->getUInt();
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
    }
};

// FIXME looks identical to BSLightingShaderPropertyColorController
class BSEffectShaderPropertyFloatController : public Controller
{
public:
    unsigned int targetVariable;
    NiInterpolatorPtr interpolator;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);

        targetVariable = nif->getUInt();
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
    }
};

class BSLagBoneController : public Controller
{
public:
    float linearVelocity;
    float linearRotation;
    float maxDistance;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        linearVelocity = nif->getFloat();
        linearRotation = nif->getFloat();
        maxDistance = nif->getFloat();
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
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
            unsigned int count = nif->getUInt();
            unknownInts.resize(count);
            for (unsigned int i = 0; i < count; ++i)
                unknownInts[i] = nif->getUInt();
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
    NiVisDataPtr data;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        data.read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
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

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);
        modifierName = nif->getSkyrimString(nifVer, Record::strings);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            nif->getInt(); // NiVisDataPtr
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
        //if (nifVer <= 0x0a010000) // up to 10.1.0.0
            // NiVisDataPtr
    }
};

class NiPSysEmitterInitialRadiusCtlr : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string modifierName;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);
        modifierName = nif->getSkyrimString(nifVer, Record::strings);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            nif->getFloat(); // NiFloatDataPtr
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
        //if (nifVer <= 0x0a010000) // up to 10.1.0.0
            // NiFloatDataPtr
    }
};

class NiPSysEmitterLifeSpanCtlr : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string modifierName;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);
        modifierName = nif->getSkyrimString(nifVer, Record::strings);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            nif->getFloat(); // NiFloatDataPtr
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
        //if (nifVer <= 0x0a010000) // up to 10.1.0.0
            // NiFloatDataPtr
    }
};

class NiPSysEmitterSpeedCtlr : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string modifierName;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);
        modifierName = nif->getSkyrimString(nifVer, Record::strings);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            nif->getFloat(); // NiFloatDataPtr
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
        //if (nifVer <= 0x0a010000) // up to 10.1.0.0
            // NiFloatDataPtr
    }
};

class NiTextureTransformController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    unsigned int textureSlot;
    unsigned int operation;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);
        nif->getChar();
        textureSlot = nif->getUInt();
        operation = nif->getUInt();
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            nif->getFloat(); // NiFloatDataPtr
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
        //if (nifVer <= 0x0a010000) // up to 10.1.0.0
            // NiFloatDataPtr
    }
};

class NiPSysGravityStrengthCtlr : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string modifierName;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.read(nif);
        modifierName = nif->getSkyrimString(nifVer, Record::strings);
        if (nifVer <= 0x0a010000) // up to 10.1.0.0
            nif->getFloat(); // NiFloatDataPtr
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
        //if (nifVer <= 0x0a010000) // up to 10.1.0.0
            // NiFloatDataPtr
    }
};

struct NodeGroup
{
    std::vector<NodePtr> nodes;

    void read(NIFStream *nif, unsigned int nifVer)
    {
        unsigned int numNodes = nif->getUInt();
        nodes.resize(numNodes);
        for (unsigned int i = 0; i < numNodes; ++i)
            nodes[i].read(nif);
    }

    void post(NIFFile *nif)
    {
        for (unsigned int i = 0; i < nodes.size(); ++i)
            nodes[i].post(nif);
    }
};

struct SkinShape
{
    NiGeometryPtr shape;
    NiSkinInstancePtr skin;

    void read(NIFStream *nif, unsigned int nifVer)
    {
        shape.read(nif);
        skin.read(nif);
    }

    void post(NIFFile *nif)
    {
        shape.post(nif);
        skin.post(nif);
    }
};

struct SkinShapeGroup
{
    std::vector<SkinShape> linkPairs;

    void read(NIFStream *nif, unsigned int nifVer)
    {
        unsigned int numLinkPairs = nif->getUInt();
        linkPairs.resize(numLinkPairs);
        for (unsigned int i = 0; i < numLinkPairs; ++i)
            linkPairs[i].read(nif, nifVer);
    }

    void post(NIFFile *nif)
    {
        for (unsigned int i = 0; i < linkPairs.size(); ++i)
            linkPairs[i].post(nif);
    }
};

class NiBSBoneLODController : public Controller
{
public:
    std::vector<NodeGroup> nodeGroups;
    std::vector<SkinShapeGroup> shapeGroups;
    std::vector<NiGeometryPtr> shapeGroups2;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        nif->getUInt();
        unsigned int numNodeGrp = nif->getUInt();
        unsigned int numNodeGrp2 = nif->getUInt();

        nodeGroups.resize(numNodeGrp);
        for (unsigned int i = 0; i < numNodeGrp; ++i)
            nodeGroups[i].read(nif, nifVer);

#if 0 // seems to be user version controlled
        if (nifVer >= 0x04020200) // from 4.2.2.0
        {
            unsigned int numShapeGrp = nif->getUInt();
            shapeGroups.resize(numShapeGrp);
            for (unsigned int i = 0; i < numShapeGrp; ++i)
                shapeGroups[i].read(nif, nifVer);

            unsigned int numShapeGrp2 = nif->getUInt();
            shapeGroups2.resize(numShapeGrp2);
            for (unsigned int i = 0; i < numShapeGrp2; ++i)
                shapeGroups2[i].read(nif);
        }
#endif
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);

        // FIXME: the rest
    }
};

class bhkBlendController : public Controller
{
public:
    unsigned int unknown;

    void read(NIFStream *nif)
    {
        Controller::read(nif);
        unknown = nif->getUInt();
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);
    }
};

class NiFloatExtraDataController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string controllerData;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
        {
            interpolator.read(nif);
            controllerData = nif->getSkyrimString(nifVer, Record::strings);
        }

        if (nifVer <= 0x0a010000) // up to 10.1.0.0
        {
            unsigned char numExtra = nif->getChar();
            for (unsigned int i = 0; i < 7; ++i)
                nif->getChar();
            for (unsigned int i = 0; i < numExtra; ++i)
                nif->getChar();
        }
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);

        if (nifVer >= 0x0a020000) // from 10.2.0.0
            interpolator.post(nif);
    }
};

class NiBoolInterpolator : public NiInterpolator
{
public:
    bool value;
    NiBoolDataPtr boolData;

    void read(NIFStream *nif)
    {
        value = nif->getBool(nifVer);
        boolData.read(nif);
    }

    void post(NIFFile *nif)
    {
        boolData.post(nif);
    }
};

class NiBoolTimelineInterpolator : public NiBoolInterpolator {};

class NiBlendBoolInterpolator : public NiInterpolator
{
public:
    unsigned char value;

    void read(NIFStream *nif)
    {
        nif->getUShort();
        nif->getUInt();

        value = nif->getChar();
    }
};

class NiFloatInterpolator : public NiInterpolator
{
public:
    float value;
    NiFloatDataPtr floatData;

    void read(NIFStream *nif)
    {
        value = nif->getFloat();
        floatData.read(nif);
    }

    void post(NIFFile *nif)
    {
        floatData.post(nif);
    }
};

class NiBlendFloatInterpolator : public NiInterpolator
{
public:
    float value;

    void read(NIFStream *nif)
    {
        nif->getUShort();
        nif->getUInt();

        value = nif->getFloat();
    }
};

class NiBlendTransformInterpolator : public NiInterpolator
{
public:

    void read(NIFStream *nif)
    {
        nif->getUShort();
        nif->getUInt();
    }
};

class NiPathInterpolator : public NiInterpolator
{
public:
    NiPosDataPtr posData;
    NiFloatDataPtr floatData;

    void read(NIFStream *nif)
    {
        nif->getUShort();
        nif->getUInt();
        nif->getFloat();
        nif->getFloat();
        nif->getUShort();

        posData.read(nif);
        floatData.read(nif);
    }

    void post(NIFFile *nif)
    {
        posData.post(nif);
        floatData.post(nif);
    }
};

class NiPoint3Interpolator : public NiInterpolator
{
public:
    Ogre::Vector3 value;
    NiPosDataPtr posData;

    void read(NIFStream *nif)
    {
        value = nif->getVector3();

        posData.read(nif);
    }

    void post(NIFFile *nif)
    {
        posData.post(nif);
    }
};

// should inherit from NiBlendInterpolator
class NiBlendPoint3Interpolator : public NiInterpolator
{
public:
    Ogre::Vector3 value;

    void read(NIFStream *nif)
    {
        nif->getUShort();
        nif->getInt();

        value = nif->getVector3();
    }
};

class NiTransformInterpolator : public NiInterpolator
{
public:
    Ogre::Vector3 translation;
    Ogre::Quaternion rotation;
    float scale;
    NiTransformDataPtr transform;

    void read(NIFStream *nif)
    {
        translation = nif->getVector3();
        rotation = nif->getQuaternion();
        scale = nif->getFloat();

        transform.read(nif);
    }
};

// replaces NiKeyframeController
class NiTransformController : public Controller
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

class NiMultiTargetTransformController : public Controller
{
public:
    unsigned short numExtraTargets;
    std::vector<NamedPtr> extraTargets;

    void read(NIFStream *nif)
    {
        Controller::read(nif);

        numExtraTargets = nif->getUShort();
        extraTargets.resize(numExtraTargets);
        for (unsigned int i = 0; i < numExtraTargets; ++i)
            extraTargets[i].read(nif);
    }

    void post(NIFFile *nif)
    {
        Controller::post(nif);

        for (unsigned int i = 0; i < extraTargets.size(); ++i)
            extraTargets[i].post(nif);
    }
};

class NiPSysModifier : public Record
{
public:
    std::string name;
    unsigned int order;
    NiParticleSystemPtr target;
    bool active;

    void read(NIFStream *nif)
    {
        name = nif->getSkyrimString(nifVer, Record::strings);
        order = nif->getUInt();
        target.read(nif);
        active = nif->getBool(nifVer);
    }

    void post(NIFFile *nif)
    {
        target.post(nif);
    }

};

class BSWindModifier : public NiPSysModifier
{
public:
    float strength;

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        strength = nif->getFloat();
    }
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

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        startFrame = nif->getUInt();
        startFrameFudge = nif->getFloat();
        endFrame = nif->getFloat();
        loopStartFrame = nif->getFloat();
        loopStartFrameFudge = nif->getFloat();
        frameCount = nif->getFloat();
        frameCountFudge = nif->getFloat();
    }
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

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        bombObject.read(nif);
        bombAxis = nif->getVector3();
        decay = nif->getFloat();
        deltaV = nif->getFloat();
        decayType = nif->getUInt();
        symmetryType = nif->getUInt();
    }
};

class BSPSysInheritVelocityModifier : public NiPSysModifier
{
public:
    unsigned int unknownI1;
    float unknownF1;
    float unknownF2;
    float unknownF3;

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        unknownI1 = nif->getUInt();
        unknownF1 = nif->getFloat();
        unknownF2 = nif->getFloat();
        unknownF3 = nif->getFloat();
    }
};

class BSPSysLODModifier : public NiPSysModifier
{
public:
    float unknown1;
    float unknown2;
    float unknown3;
    float unknown4;

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        unknown1 = nif->getFloat();
        unknown2 = nif->getFloat();
        unknown3 = nif->getFloat();
        unknown4 = nif->getFloat();
    }
};

class BSPSysScaleModifier : public NiPSysModifier
{
public:
    std::vector<float> floats;

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        unsigned int numFloats = nif->getUInt();
        nif->getFloats(floats, numFloats);
    }
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

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        fadeInPercent = nif->getFloat();
        fadeOutPercent = nif->getFloat();
        color1EndPerCent = nif->getFloat();
        color1StartPerCent = nif->getFloat();
        color2EndPerCent = nif->getFloat();
        color2StartPerCent = nif->getFloat();

        nif->getVector4s(colors, 3);
    }
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

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        speed = nif->getFloat();
        speedVar = nif->getFloat();
        declination = nif->getFloat();
        declinationVar = nif->getFloat();
        planarAngle = nif->getFloat();
        planarAngleVar = nif->getFloat();
        initialColor = nif->getVector4();
        initialRadius = nif->getFloat();
        radiusVar = nif->getFloat();
        lifeSpan = nif->getFloat();
        lifeSpanVar = nif->getFloat();
    }
};

class NiPSysBoxEmitter : public NiPSysEmitter
{
public:
    NodePtr emitteObj;
    float width;
    float height;
    float depth;

    void read(NIFStream *nif)
    {
        NiPSysEmitter::read(nif);

        emitteObj.read(nif);
        width = nif->getFloat();
        height = nif->getFloat();
        depth = nif->getFloat();
    }
};

class NiPSysCylinderEmitter : public NiPSysEmitter
{
public:
    NodePtr emitteObj;
    float radius;
    float height;

    void read(NIFStream *nif)
    {
        NiPSysEmitter::read(nif);

        emitteObj.read(nif);
        radius = nif->getFloat();
        height = nif->getFloat();
    }
};

class NiPSysMeshEmitter : public NiPSysEmitter
{
public:
    std::vector<NiGeometryPtr> emitterMeshes; // NiTriBasedGeom
    unsigned int velocityType;
    unsigned int emissionType;
    Ogre::Vector3 emissionAxis;

    void read(NIFStream *nif)
    {
        NiPSysEmitter::read(nif);

        unsigned int numMeshes = nif->getUInt();
        emitterMeshes.resize(numMeshes);
        for (unsigned int i = 0; i < numMeshes; ++i)
            emitterMeshes[i].read(nif);

        velocityType = nif->getUInt();
        emissionType = nif->getUInt();
        emissionAxis = nif->getVector3();
    }

    void post(NIFFile *nif)
    {
        for (unsigned int i = 0; i < emitterMeshes.size(); ++i)
            emitterMeshes[i].post(nif);
    }
};

class NiPSysSphereEmitter : public NiPSysEmitter
{
public:
    NodePtr emitteObj;
    float radius;

    void read(NIFStream *nif)
    {
        NiPSysEmitter::read(nif);

        emitteObj.read(nif);
        radius = nif->getFloat();
    }
};

class NiPSysAgeDeathModifier : public NiPSysModifier
{
public:
    bool spawnOnDeath;
    NiPSysSpawnModifierPtr spawnModifier;

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        spawnOnDeath = nif->getBool(nifVer);
        spawnModifier.read(nif);
    }

    void post(NIFFile *nif)
    {
        spawnModifier.post(nif);
    }
};

class NiPSysDragModifier : public NiPSysModifier
{
public:
    NodePtr parent;
    Ogre::Vector3 dragAxis;
    float percent;
    float range;
    float rangeFalloff;

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        parent.read(nif);
        dragAxis = nif->getVector3();
        percent = nif->getFloat();
        range = nif->getFloat();
        rangeFalloff = nif->getFloat();
    }
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

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        numSpawnGen = nif->getUShort();
        percentSpawn = nif->getFloat();
        minSpawn = nif->getUShort();
        maxSpawn = nif->getUShort();
        spawnSpeedChaos = nif->getFloat();
        spawnDirChaos = nif->getFloat();
        lifeSpan = nif->getFloat();
        lifeSpanVar = nif->getFloat();
    }
};

class NiPSysGrowFadeModifier : public NiPSysModifier
{
public:
    float growTime;
    unsigned short growGen;
    float fadeTime;
    unsigned short fadeGen;
    float baseScale;

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        growTime = nif->getFloat();
        growGen = nif->getUShort();
        fadeTime = nif->getFloat();
        fadeGen = nif->getUShort();
        if (nifVer >= 0x14020007) // from 20.2.0.7
            baseScale = nif->getFloat();
    }
};

class NiPSysColorModifier : public NiPSysModifier
{
public:
    NiColorDataPtr colorData;

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        colorData.read(nif);
    }

    void post(NIFFile *nif)
    {
        colorData.post(nif);
    }
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

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        gravityObj.read(nif);
        gravityAxis = nif->getVector3();
        decay = nif->getFloat();
        strength = nif->getFloat();
        forceType = nif->getUInt();
        turbulence = nif->getFloat();
        turbulenceScale = nif->getFloat();
        if (nifVer >= 0x14020007) // from 20.2.0.7
            nif->getChar();
    }
};

class NiPSysPositionModifier : public NiPSysModifier {};

class NiPSysBoundUpdateModifier : public NiPSysModifier
{
public:

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        nif->getUShort();
    }
};

class NiPSysRotationModifier : public NiPSysModifier
{
public:
    float initialRotSpeed;
    float initialRotSpeedVar;
    float initialRotAngle;
    float initialRotAngleVar;
    bool randomRotSpeedSign;

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        initialRotSpeed = nif->getFloat();

        if (nifVer >= 0x14000004) // from 20.0.0.4
        {
            initialRotSpeedVar = nif->getFloat();
            initialRotAngle = nif->getFloat();
            initialRotAngleVar = nif->getFloat();
            randomRotSpeedSign = nif->getBool(nifVer);
        }
        nif->getBool(nifVer);
        nif->getVector3();
    }
};

class BSParentVelocityModifier : public NiPSysModifier
{
public:
    float damping;

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

        damping = nif->getFloat();
    }
};

class NiPSysColliderManager : public NiPSysModifier
{
public:
    NiPSysColliderPtr collider;

    void read(NIFStream *nif)
    {
        NiPSysModifier::read(nif);

       collider.read(nif);
    }

    void post(NIFFile *nif)
    {
        collider.post(nif);
    }
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

    void read(NIFStream *nif)
    {
        bounce = nif->getFloat();
        spawnOnCollide = nif->getBool(nifVer);
        dieOnCollide = nif->getBool(nifVer);
        nif->getInt(); // spawnModifier.read(nif);
        parent.read(nif);
        nextCollider.read(nif);
        colliderObj.read(nif);
    }

    void post(NIFFile *nif)
    {
        //spawnModifier.post(nif);
    }
};

class NiPSysPlanarCollider : public NiPSysCollider
{
public:
    float width;
    float height;
    Ogre::Vector3 xAxis;
    Ogre::Vector3 yAxis;

    void read(NIFStream *nif)
    {
        NiPSysCollider::read(nif);

        width = nif->getFloat();
        height = nif->getFloat();
        xAxis = nif->getVector3();
        yAxis = nif->getVector3();
    }
};

class NiPSysSphericalCollider : public NiPSysCollider
{
public:
    float radius;

    void read(NIFStream *nif)
    {
        NiPSysCollider::read(nif);

        radius = nif->getFloat();
    }
};

} // Namespace
#endif
