/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (data.h) is part of the OpenMW package.

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

#ifndef OPENMW_COMPONENTS_NIF_DATA_HPP
#define OPENMW_COMPONENTS_NIF_DATA_HPP

#include <sstream>
#include <iostream>
#include <string>

#include "base.hpp"

namespace Nif
{

// Common ancestor for several data classes
class ShapeData : public Record // NiGeometryData
{
protected:
    bool mIsNiPSysData;
public:
    char keepFlags;
    char compressFlags;
    unsigned short bsMaxVerts;
    unsigned short numUVSets;
    unsigned short bsNumUVSets;
    std::vector<Ogre::Vector3> vertices, normals;
    std::vector<Ogre::Vector3> tangents, bitangents;
    std::vector<Ogre::Vector4> colors;
    std::vector< std::vector<Ogre::Vector2> > uvlist;
    Ogre::Vector3 center;
    float radius;
    unsigned short consistencyFlags;
    //refAbstractAdditionalGeometryDataPtr additionalData; // FIXME

    ShapeData();

    void read(NIFStream *nif);
};

class NiTriShapeData : public ShapeData
{
public:
    // Triangles, three vertex indices per triangle
    std::vector<short> triangles;

    void read(NIFStream *nif);
};

// FIXME: below looks wrong
#if 0
class NiAutoNormalParticlesData : public ShapeData
{
public:
    int numParticles;

    float particleRadius;

    int activeCount;

    std::vector<float> sizes;

    void read(NIFStream *nif)
    {
        ShapeData::read(nif);

        // Should always match the number of vertices
        numParticles = nif->getUShort();

        particleRadius = nif->getFloat();
        activeCount = nif->getUShort();

        if(nif->getInt())
        {
            // Particle sizes
            nif->getFloats(sizes, vertices.size());
        }
    }
};
#endif

class NiParticlesData : public ShapeData
{
public:
    int numParticles;

    float particleRadius;

    bool hasRadii;
    std::vector<float> radii;
    bool hasRotations;
    std::vector<Ogre::Quaternion> rotations;
    bool hasRotationAngles;
    std::vector<float> rotationAngles;
    bool hasRotationAxes;
    std::vector<Ogre::Vector3> rotationAxes;
    bool hasUVQuads;
    std::vector<Ogre::Vector4> uvQuads;

    int activeCount;

    bool hasSizes;
    std::vector<float> sizes;

    void read(NIFStream *nif);
};

class NiAutoNormalParticlesData : public NiParticlesData {};

class NiRotatingParticlesData : public NiParticlesData
{
public:
    std::vector<Ogre::Quaternion> rotations;

    void read(NIFStream *nif);
};

struct Particle
{
    Ogre::Vector3 translation;
    std::vector<float> unknownFloats;
    float unknown1;
    float unknown2;
    float unknown3;
    int unknown;

    void read(NIFStream *nif, unsigned int nifVer);
};

class NiPSysData : public NiRotatingParticlesData
{
public:
    std::vector<Particle> particleDesc;
    std::vector<float> unknownFloats3;
    bool hasSubTextureUVs;
    std::vector<Ogre::Vector4> subTextureUVs;
    float aspectRatio;

    void read(NIFStream *nif);
};

class BSStripPSysData : public NiPSysData
{
public:
    short unknown5;
    char unknown6;
    int unknown7;
    float unknown8;

    void read(NIFStream *nif);
};

class NiPosData : public Record
{
public:
    Vector3KeyMap mKeyList;

    void read(NIFStream *nif);
};

class NiUVData : public Record
{
public:
    FloatKeyMap mKeyList[4];

    void read(NIFStream *nif);
};

class NiFloatData : public Record
{
public:
    FloatKeyMap mKeyList;

    void read(NIFStream *nif);
};

class NiBoolData : public Record
{
public:
    BoolKeyMap mKeyList;

    void read(NIFStream *nif);
};

class NiPixelData : public Record
{
public:
    unsigned int rmask, gmask, bmask, amask;
    int bpp, mips;

    void read(NIFStream *nif);
};

class NiColorData : public Record
{
public:
    Vector4KeyMap mKeyMap;

    void read(NIFStream *nif);
};

class NiVisData : public Record
{
public:
    struct VisData {
        float time;
        bool isSet;
    };
    std::vector<VisData> mVis;

    void read(NIFStream *nif);
};

class NiSkinPartition : public Record
{
public:
    struct Triangle
    {
        unsigned short v1;
        unsigned short v2;
        unsigned short v3;
    };

    struct SkinPartitionBlock
    {
        unsigned short numVerts;
        unsigned short numTriangles;
        unsigned short numBones;
        unsigned short numStrips;
        unsigned short numWeightsPerVert;
        std::vector<unsigned short> bones;
        bool hasVertMap;
        std::vector<unsigned short> vertMap;
        bool hasVertWeights;
        std::vector<std::vector<float> > vertWeights;
        std::vector<unsigned short> stripLengths;
        bool hasFaces;
        std::vector<std::vector<float> > strips;
        std::vector<Triangle> triangles;
        bool hasBoneIndicies;
        std::vector<std::vector<unsigned char> > boneIndicies;

        void read(NIFStream *nif, unsigned int nifVer, unsigned int userVer);
    };

    unsigned int numSkinPartitionBlocks;
    std::vector<SkinPartitionBlock> skinPartitionBlocks;

    void read(NIFStream *nif);
};

class NiSkinInstance : public Record
{
public:
    NiSkinDataPtr data;
    NiSkinPartitionPtr skinPartition;
    NodePtr root;
    NodeList bones;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class NiSkinData : public Record
{
public:
    struct BoneTrafo
    {
        Ogre::Matrix3 rotation; // Rotation offset from bone?
        Ogre::Vector3 trans;    // Translation
        float scale;            // Probably scale (always 1)
    };

    struct VertWeight
    {
        short vertex;
        float weight;
    };

    struct BoneInfo
    {
        BoneTrafo trafo;
        Ogre::Vector4 unknown;
        std::vector<VertWeight> weights;
    };

    BoneTrafo trafo;
    std::vector<BoneInfo> bones;
    //NiSkinPartitionPtr skinPartition;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

struct NiMorphData : public Record
{
    struct MorphData {
        std::string mFrameName;
        FloatKeyMap mData;
        std::vector<Ogre::Vector3> mVertices;
    };
    std::vector<MorphData> mMorphs;

    void read(NIFStream *nif);
};

struct NiKeyframeData : public Record
{
    QuaternionKeyMap mRotations;

    FloatKeyMap mXRotations;
    FloatKeyMap mYRotations;
    FloatKeyMap mZRotations;

    Vector3KeyMap mTranslations;
    FloatKeyMap mScales;

    void read(NIFStream *nif);
};

struct NiTransformData : public NiKeyframeData {};

// Ogre doesn't seem to allow meshes to be created using triangle strips
// (unless using ManualObject)
class NiTriStripsData : public ShapeData
{
public:
    std::vector<unsigned short> stripLengths;
    std::vector<std::vector<unsigned short> > points;

    // Triangles, three vertex indices per triangle
    std::vector<short> triangles;

    void read(NIFStream *nif);
};

struct NiBSplineData : public Record
{
    std::vector<float> floatControlPoints;
    std::vector<short> shortControlPoints;

    void read(NIFStream *nif);
};

struct NiBSplineBasisData : public Record
{
    unsigned int numControlPoints;

    void read(NIFStream *nif);
};

class NiStringPalette : public Record
{
public:
    std::vector<char> buffer;
    //std::vector<std::string> palette;

    void read(NIFStream *nif);
};

struct AVObject
{
    std::string name;
    NodePtr avObject;

    void post(NIFFile *nif);
};

struct NiDefaultAVObjectPalette : public Record
{
    std::vector<AVObject> objs;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

class BSShaderTextureSet : public Record
{
public:
    std::vector<std::string> textures;

    void read(NIFStream *nif);
};

} // Namespace
#endif
