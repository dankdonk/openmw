#include "base.hpp"

#include "extra.hpp"

void Nif::Extra::read(NIFStream *nif)
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
        if (extras.length() > 0)
            hasExtras = true;
    }
}

void Nif::Extra::post(NIFFile *nif)
{
    if (nifVer <= 0x04020200) // up to 4.2.2.0
        extra.post(nif);

    if (nifVer >= 0x0a000100) // from 10.0.1.0
        extras.post(nif);
}

void Nif::Controller::read(NIFStream *nif)
{
    next.read(nif);

    flags = nif->getUShort();

    frequency = nif->getFloat();
    phase = nif->getFloat();
    timeStart = nif->getFloat();
    timeStop = nif->getFloat();

    target.read(nif);
}

void Nif::Controller::post(NIFFile *nif)
{
    Record::post(nif);
    next.post(nif);
    target.post(nif);
}

void Nif::Controlled::read(NIFStream *nif)
{
    Extra::read(nif);

    controller.read(nif);
}

void Nif::Controlled::post(NIFFile *nif)
{
    Extra::post(nif);

    controller.post(nif);
}

void Nif::NiParticleModifier::read(NIFStream *nif)
{
    extra.read(nif);
    controller.read(nif);
}

void Nif::NiParticleModifier::post(NIFFile *nif)
{
    extra.post(nif);
    controller.post(nif);
}

void Nif::Named::read(NIFStream *nif)
{
    name = nif->getSkyrimString(nifVer, Record::strings);

    Controlled::read(nif);   // read NiObjectNET
}
