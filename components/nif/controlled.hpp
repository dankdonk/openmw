/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (controlled.h) is part of the OpenMW package.

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

#ifndef OPENMW_COMPONENTS_NIF_CONTROLLED_HPP
#define OPENMW_COMPONENTS_NIF_CONTROLLED_HPP

#include "base.hpp"

namespace Nif
{

class NiSourceTexture : public Named
{
public:
    // Is this an external (references a separate texture file) or
    // internal (data is inside the nif itself) texture?
    bool external;

    std::string filename; // In case of external textures
    NiPixelDataPtr data;  // In case of internal textures
    std::string originalFile;

    /* Pixel layout
        0 - Palettised
        1 - High color 16
        2 - True color 32
        3 - Compressed
        4 - Bumpmap
        5 - Default */
    int pixel;

    /* Mipmap format
        0 - no
        1 - yes
        2 - default */
    int mipmap;

    /* Alpha
        0 - none
        1 - binary
        2 - smooth
        3 - default (use material alpha, or multiply material with texture if present)
    */
    int alpha;
    int directRenderer;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiParticleGrowFade : public NiParticleModifier
{
public:
    float growTime;
    float fadeTime;

    void read(NIFStream *nif);
};

class NiParticleColorModifier : public NiParticleModifier
{
public:
    NiColorDataPtr data;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiGravity : public NiParticleModifier
{
public:
    float mForce;
    /* 0 - Wind (fixed direction)
     * 1 - Point (fixed origin)
     */
    int mType;
    Ogre::Vector3 mPosition;
    Ogre::Vector3 mDirection;

    void read(NIFStream *nif);
};

// NiPinaColada
class NiPlanarCollider : public NiParticleModifier
{
public:
    void read(NIFStream *nif);
};

class NiParticleRotation : public NiParticleModifier
{
public:
    void read(NIFStream *nif);
};

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

} // Namespace
#endif
