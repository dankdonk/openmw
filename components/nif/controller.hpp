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

    void read(NIFStream *nif);
    void post(NIFFile *nif);
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

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiPathController : public Controller
{
public:
    NiPosDataPtr posData;
    NiFloatDataPtr floatData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiUVController : public Controller
{
public:
    NiUVDataPtr data;

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

class NiKeyframeController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    NiKeyframeDataPtr data;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiAlphaController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    NiFloatDataPtr data;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
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

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiVisController : public Controller
{
public:
    NiVisDataPtr data;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiFlipController : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    int mTexSlot; // NiTexturingProperty::TextureType
    float mDelta; // Time between two flips. delta = (start_time - stop_time) / num_sources
    NiSourceTextureList mSources;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiPSysEmitterCtlr : public Controller
{
public:
    NiInterpolatorPtr interpolator;
    std::string modifierName;
    NiInterpolatorPtr visibilityInterpolator;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
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
    NiTransformDataPtr transform;

    void read(NIFStream *nif);
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

class NiMultiTargetTransformController : public Controller
{
public:
    unsigned short numExtraTargets;
    std::vector<NamedPtr> extraTargets;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};
} // Namespace
#endif
