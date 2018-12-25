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

} // Namespace
#endif
