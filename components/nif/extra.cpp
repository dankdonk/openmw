#include "extra.hpp"

void Nif::NiExtraData::read(NIFStream *nif)
{
    if (nifVer >= 0x0a000100) // from 10.0.1.0
        name = nif->getSkyrimString(nifVer, Record::strings);

    if (nifVer <= 0x04020200) // up to 4.2.2.0
        next.read(nif);
}

void Nif::NiExtraData::post(NIFFile *nif)
{
    if (nifVer <= 0x04020200) // up to 4.2.2.0
        next.post(nif);
}

void Nif::BSBehaviorGraphExtraData::read(NIFStream *nif)
{
    NiExtraData::read(nif);

    behaviourGraphFile = nif->getSkyrimString(nifVer, Record::strings);

    controlBaseSkeleton = nif->getChar();
}

void Nif::BSBehaviorGraphExtraData::post(NIFFile *nif)
{
    NiExtraData::post(nif);
}

void Nif::DecalVectorArray::read(NIFStream *nif)
{
    short numVectors = nif->getShort();
    nif->getVector3s(points, numVectors);
    nif->getVector3s(normals, numVectors);
}

void Nif::BSDecalPlacementVectorExtraData::read(NIFStream *nif)
{
    NiExtraData::read(nif);

    unknown1 = nif->getFloat();

    short numVectorBlocks = nif->getShort();
    vectorBlocks.resize(numVectorBlocks);
    for (int i = 0; i < numVectorBlocks; ++i)
        vectorBlocks[i].read(nif);
}

void Nif::BSDecalPlacementVectorExtraData::post(NIFFile *nif)
{
    NiExtraData::post(nif);
}

void Nif::BSBound::read(NIFStream *nif)
{
    NiExtraData::read(nif);

    center = nif->getVector3();
    dimensions = nif->getVector3();
}

void Nif::BSBound::post(NIFFile *nif)
{
    NiExtraData::post(nif);
}

void Nif::BSFurnitureMarker::read(NIFStream *nif)
{
    NiExtraData::read(nif);

    unsigned int numPos = nif->getUInt();
    positions.resize(numPos);
    for (unsigned int i = 0; i < numPos; ++i)
    {
        positions[i].offset = nif->getVector3();
        if (userVer <= 11)
        {
            positions[i].orientation = nif->getUShort();
            positions[i].posRef1 = nif->getChar();
            positions[i].posRef2 = nif->getChar();
        }
        if (nifVer >= 0x14020007 && userVer >= 12) // from 20.2.0.7
        {
            positions[i].heading = nif->getFloat();
            positions[i].animType = nif->getUShort();
            positions[i].entryProperties = nif->getUShort();
        }
    }
}

void Nif::BSFurnitureMarker::post(NIFFile *nif)
{
    NiExtraData::post(nif);
}

void Nif::BSInvMarker::read(NIFStream *nif)
{
    NiExtraData::read(nif);

    rotationX = nif->getUShort();
    rotationY = nif->getUShort();
    rotationZ = nif->getUShort();
    zoom = nif->getFloat();
}

void Nif::BSInvMarker::post(NIFFile *nif)
{
    NiExtraData::post(nif);
}

void Nif::NiBinaryExtraData::read(NIFStream *nif)
{
    NiExtraData::read(nif);

    unsigned int size = nif->getUInt();
    data.resize(size);
    for(unsigned int i = 0; i< size; i++)
    {
        data[i] = nif->getChar();
    }
}

void Nif::NiBinaryExtraData::post(NIFFile *nif)
{
    NiExtraData::post(nif);
}

void Nif::NiBooleanExtraData::read(NIFStream *nif)
{
    NiExtraData::read(nif);

    booleanData = nif->getChar();
}

void Nif::NiBooleanExtraData::post(NIFFile *nif)
{
    NiExtraData::post(nif);
}

void Nif::NiIntegerExtraData::read(NIFStream *nif)
{
    NiExtraData::read(nif);

    integerData = nif->getUInt();
}

void Nif::NiIntegerExtraData::post(NIFFile *nif)
{
    NiExtraData::post(nif);
}

void Nif::NiFloatExtraData::read(NIFStream *nif)
{
    NiExtraData::read(nif);

    floatData = nif->getFloat();
}

void Nif::NiFloatExtraData::post(NIFFile *nif)
{
    NiExtraData::post(nif);
}

void Nif::BSXFlags::read(NIFStream *nif)
{
    NiExtraData::read(nif);

    integerData = nif->getUInt(); // http://niftools.sourceforge.net/doc/nif/BSXFlags.html
}

void Nif::BSXFlags::post(NIFFile *nif)
{
    NiExtraData::post(nif);
}
