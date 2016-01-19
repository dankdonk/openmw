/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (extra.h) is part of the OpenMW package.

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

#ifndef OPENMW_COMPONENTS_NIF_EXTRA_HPP
#define OPENMW_COMPONENTS_NIF_EXTRA_HPP

#include "base.hpp"

namespace Nif
{

class NiExtraData : public Record
{
public:
    std::string name;
    NiExtraDataPtr next;

    void read(NIFStream *nif)
    {
        if (nifVer >= 0x0a000100) // from 10.0.1.0
            name = nif->getSkyrimString(nifVer, Record::strings);

        if (nifVer <= 0x04020200) // up to 4.2.2.0
            next.read(nif);
    }
};

class BSBehaviorGraphExtraData : public NiExtraData
{
public:
    std::string behaviourGraphFile;
    unsigned char controlBaseSkeleton;

    void read(NIFStream *nif)
    {
        NiExtraData::read(nif);

        behaviourGraphFile = nif->getSkyrimString(nifVer, Record::strings);

        controlBaseSkeleton = nif->getChar();
    }
};

struct DecalVectorArray
{
    std::vector<Ogre::Vector3> points;
    std::vector<Ogre::Vector3> normals;

    void read(NIFStream *nif)
    {
        short numVectors = nif->getShort();
        nif->getVector3s(points, numVectors);
        nif->getVector3s(normals, numVectors);
    }
};

class BSDecalPlacementVectorExtraData : public NiExtraData
{
public:
    float unknown1;
    std::vector<DecalVectorArray> vectorBlocks;

    void read(NIFStream *nif)
    {
        NiExtraData::read(nif);

        unknown1 = nif->getFloat();

        short numVectorBlocks = nif->getShort();
        vectorBlocks.resize(numVectorBlocks);
        for (int i = 0; i < numVectorBlocks; ++i)
            vectorBlocks[i].read(nif);
    }
};

class BSBound : public NiExtraData
{
public:
    Ogre::Vector3 center;
    Ogre::Vector3 dimensions;

    void read(NIFStream *nif)
    {
        NiExtraData::read(nif);

        center = nif->getVector3();
        dimensions = nif->getVector3();
    }
};

struct FurniturePosition
{
    Ogre::Vector3 offset;
    // Oblivion
    unsigned short orientation;
    unsigned char posRef1;
    unsigned char posRef2;
    // Skyrim
    float heading;
    unsigned short animType;
    unsigned short entryProperties;
};

class BSFurnitureMarker : public NiExtraData
{
public:
    std::string name;
    std::vector<FurniturePosition> positions;

    void read(NIFStream *nif)
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
};

class BSFurnitureMarkerNode : public BSFurnitureMarker {};

class BSInvMarker : public NiExtraData
{
public:
    unsigned short rotationX;
    unsigned short rotationY;
    unsigned short rotationZ;
    float zoom;

    void read(NIFStream *nif)
    {
        NiExtraData::read(nif);

        rotationX = nif->getUShort();
        rotationY = nif->getUShort();
        rotationZ = nif->getUShort();
        zoom = nif->getFloat();
    }
};

class NiBinaryExtraData : public NiExtraData
{
public:
    std::string string;
    std::vector<char> data;

    void read(NIFStream *nif)
    {
        NiExtraData::read(nif);

        unsigned int size = nif->getUInt();
        data.resize(size);
        for(unsigned int i = 0; i< size; i++)
        {
            data[i] = nif->getChar();
        }
    }
};

class NiBooleanExtraData : public NiExtraData
{
public:
    unsigned char booleanData;

    void read(NIFStream *nif)
    {
        NiExtraData::read(nif);

        booleanData = nif->getChar();
    }
};

class NiFloatExtraData : public NiExtraData
{
public:
    float floatData;

    void read(NIFStream *nif)
    {
        NiExtraData::read(nif);

        floatData = nif->getFloat();
    }
};

class BSXFlags : public NiExtraData
{
public:
    unsigned int integerData;

    void read(NIFStream *nif)
    {
        NiExtraData::read(nif);

        integerData = nif->getUInt(); // http://niftools.sourceforge.net/doc/nif/BSXFlags.html
    }
};

class NiStringExtraData : public NiExtraData
{
public:
    /* Two known meanings:
       "MRK" - marker, only visible in the editor, not rendered in-game
       "NCO" - no collision
    */
    std::string stringData;

    void read(NIFStream *nif)
    {
        NiExtraData::read(nif);

        if (nifVer <= 0x04020200) // up to 4.2.2.0
            nif->getInt(); // size of string + 4. Really useful...

        stringData = nif->getSkyrimString(nifVer, Record::strings);
    }
};

class NiTextKeyExtraData : public NiExtraData
{
public:
    struct TextKey
    {
        float time;
        std::string text;
    };
    std::vector<TextKey> list;

    void read(NIFStream *nif)
    {
        NiExtraData::read(nif);

        if (nifVer <= 0x04020200) // up to 4.2.2.0
            nif->getInt(); // 0

        int keynum = nif->getInt();
        list.resize(keynum);
        for(int i=0; i<keynum; i++)
        {
            list[i].time = nif->getFloat();
            list[i].text = nif->getSkyrimString(nifVer, Record::strings);
        }
    }
};

class NiVertWeightsExtraData : public NiExtraData
{
public:
    void read(NIFStream *nif)
    {
        NiExtraData::read(nif);

        // We should have s*4+2 == i, for some reason. Might simply be the
        // size of the rest of the record, unhelpful as that may be.
        /*int i =*/ nif->getInt();
        int s = nif->getUShort();

        nif->skip(s * sizeof(float)); // vertex weights I guess
    }
};

} // Namespace
#endif
