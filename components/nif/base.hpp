///This file holds the main classes of NIF Records used by everything else.
#ifndef OPENMW_COMPONENTS_NIF_BASE_HPP
#define OPENMW_COMPONENTS_NIF_BASE_HPP

#include <iostream> // FIXME

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
    ExtraPtr extra; // FIXME: how to make this part of extras rather than keep separate members?
    ExtraList extras;
    bool hasExtras;

    void read(NIFStream *nif)
    {
        // FIXME: this is a linked list
        if (nifVer <= 0x04020200) // up to 4.2.2.0
        {
            extra.read(nif);
            hasExtras = false;
        }

        // FIXME: this is a vector
        if (nifVer >= 0x0a000100) // from 10.0.1.0
        {
            extras.read(nif);
            hasExtras = true;
        }
    }
    void post(NIFFile *nif)
    {
        if (nifVer <= 0x04020200) // up to 4.2.2.0
            extra.post(nif);

        if (nifVer >= 0x0a000100) // from 10.0.1.0
            extras.post(nif);
    }
};

// NiTimeController
class Controller : public Record
{
public:
    ControllerPtr next;
    int flags;
    float frequency, phase;
    float timeStart, timeStop;
    ControlledPtr target;

    void read(NIFStream *nif)
    {
        next.read(nif);

        flags = nif->getUShort();

        frequency = nif->getFloat();
        phase = nif->getFloat();
        timeStart = nif->getFloat();
        timeStop = nif->getFloat();

        target.read(nif);
    }

    void post(NIFFile *nif)
    {
        Record::post(nif);
        next.post(nif);
        target.post(nif);
    }
};

/// Anything that has a controller
class Controlled : public Extra // FIXME: should be changed from "is an Extra" to "has an Extra"
{
    // NiObjectNET (part)
public:
    ControllerPtr controller;

    void read(NIFStream *nif)
    {
        Extra::read(nif);

        controller.read(nif);
    }

    void post(NIFFile *nif)
    {
        Extra::post(nif);

        controller.post(nif);
    }
};

class NiParticleModifier : public Record
{
public:
    NiParticleModifierPtr extra;
    ControllerPtr controller;

    void read(NIFStream *nif)
    {
        extra.read(nif);
        controller.read(nif);
    }

    void post(NIFFile *nif)
    {
        extra.post(nif);
        controller.post(nif);
    }
};

/// Has name, extra-data and controller
class Named : public Controlled
{
    // NiAVObject
public:
    std::string name;

    void read(NIFStream *nif)
    {
        name = nif->getString(); // according to niftools docs this string is part of NiObjectNET
        Controlled::read(nif);   // read NiObjectNET
    }
};
typedef Named NiSequenceStreamHelper;

} // Namespace
#endif
