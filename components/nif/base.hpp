///This file holds the main classes of NIF Records used by everything else.
#ifndef OPENMW_COMPONENTS_NIF_BASE_HPP
#define OPENMW_COMPONENTS_NIF_BASE_HPP

#include "record.hpp"
#include "niffile.hpp"
#include "recordptr.hpp"
#include "nifstream.hpp"
#include "nifkey.hpp"

namespace Nif
{
/** A record that can have extra data. The extra data objects
    themselves descend from the Extra class, and all the extra data
    connected to an object form a linked list
*/
class Extra : public Record
{
public:
    NiExtraDataPtr extra;
    NiExtraDataList extras;
    bool hasExtras; // FIXME: how to make this part of extras rather than keep separate members?

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};
#if 0
// same as Controller
class NiTimeController : public Record
{
public:
    NiTimeControllerPtr next;
    unsigned short flags;
    float frequency, phase;
    float timeStart, timeStop;
    NiObjectNETPtr target;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

// same as Named
class NiObjectNET : public Record
{
public:
    std::string name;
    NiExtraDataPtr extra;
    NiExtraDataList extras;
    bool hasExtras; // FIXME
    NiTimeControllerPtr controller;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

// same as Node
class NiAVObject : public NiObjectNET
{
public:
    unsigned short flags;
    Transformation transfrom;
    Ogre::Vector3 velocity; // Unused? Might be a run-time game state
    NiPropertyList props;

    bool hasBounds = false; // NOTE: this needs to be set to false for NiTriStrips (see ManualBulletShapeLoader)
    Ogre::Vector3 translation;
    Ogre::Matrix3 rotation;
    Ogre::Vector3 raidus;   // per direction

    NiCollisionObjectPtr collision;

    void read(NIFStream *nif);
    void post(NIFFile *nif);

    NiNode *parent; // FIXME: move this to NiNode?

    const NiSkinData::BoneTrafo *boneTrafo;
    const NiSkinData::BoneInfo *boneInfo;
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
#endif
class Controller : public Record
{
public:
    ControllerPtr next;
    int flags;
    float frequency, phase;
    float timeStart, timeStop;
    ControlledPtr target;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

/// Anything that has a controller
class Controlled : public Extra // FIXME: should be changed from "is an Extra" to "has an Extra"
{
public:
    ControllerPtr controller;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiParticleModifier : public Record
{
public:
    NiParticleModifierPtr extra;
    ControllerPtr controller;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

/// Has name, extra-data and controller
class Named : public Controlled
{
public:
    std::string name;

    void read(NIFStream *nif);
};
typedef Named NiSequenceStreamHelper;

} // Namespace
#endif
