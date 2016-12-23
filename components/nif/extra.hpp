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

#include "record.hpp"
#include "recordptr.hpp"

namespace Nif
{

class NiExtraData : public Record
{
public:
    std::string name;
    NiExtraDataPtr next;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSBehaviorGraphExtraData : public NiExtraData
{
public:
    std::string behaviourGraphFile;
    unsigned char controlBaseSkeleton;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

struct DecalVectorArray
{
    std::vector<Ogre::Vector3> points;
    std::vector<Ogre::Vector3> normals;

    void read(NIFStream *nif);
};

class BSDecalPlacementVectorExtraData : public NiExtraData
{
public:
    float unknown1;
    std::vector<DecalVectorArray> vectorBlocks;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSBound : public NiExtraData
{
public:
    Ogre::Vector3 center;
    Ogre::Vector3 dimensions;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
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

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSFurnitureMarkerNode : public BSFurnitureMarker {};

class BSInvMarker : public NiExtraData
{
public:
    unsigned short rotationX;
    unsigned short rotationY;
    unsigned short rotationZ;
    float zoom;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiBinaryExtraData : public NiExtraData
{
public:
    std::string string;
    std::vector<char> data;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiBooleanExtraData : public NiExtraData
{
public:
    unsigned char booleanData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiIntegerExtraData : public NiExtraData
{
public:
    unsigned int integerData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiFloatExtraData : public NiExtraData
{
public:
    float floatData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSXFlags : public NiExtraData // FIXME: should inherit from NiIntegerData
{
public:
    unsigned int integerData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiStringExtraData : public NiExtraData
{
public:
    /* Two known meanings:
       "MRK" - marker, only visible in the editor, not rendered in-game
       "NCO" - no collision
    */
    std::string stringData;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
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

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiVertWeightsExtraData : public NiExtraData
{
public:
    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

} // Namespace
#endif
