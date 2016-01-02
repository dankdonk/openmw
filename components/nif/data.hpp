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
class ShapeData : public Record
{
public:
    char keepFlags;
    char compressFlags;
    unsigned short numUVSets;
    std::vector<Ogre::Vector3> vertices, normals;
    std::vector<Ogre::Vector4> colors;
    std::vector< std::vector<Ogre::Vector2> > uvlist;
    Ogre::Vector3 center;
    float radius;
    unsigned short consistencyFlags;
    int refAbstractAdditionalGeometryData; // FIXME

    void read(NIFStream *nif)
    {
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            int unknownInt = nif->getInt(); // always 0

        int verts = nif->getUShort();

        if (nifVer >= 0x0a020000) // from 10.2.0.0
        {
            keepFlags = nif->getChar();
            compressFlags = nif->getChar();
        }

        if(nif->getBool(nifVer)) // has vertices
            nif->getVector3s(vertices, verts);

        if (nifVer >= 0x0a000100) // from 10.0.1.0
            numUVSets = nif->getUShort();

        if(nif->getBool(nifVer)) // has normals
            nif->getVector3s(normals, verts);

        center = nif->getVector3();
        radius = nif->getFloat();

        if(nif->getBool(nifVer)) // has vertex colors
            nif->getVector4s(colors, verts);

        int uvs = 0;
        // Only the first 6 bits are used as a count. I think the rest are
        // flags of some sort.
        if (nifVer <= 0x04020200) // up to 4.2.2.0
            uvs = nif->getUShort() & 0x3f;
        if (nifVer >= 0x0a000100) // from 10.0.1.0
            uvs = numUVSets & 0x3f;

        if (nifVer <= 0x04000002) // up to 4.0.0.2
            bool hasUV = nif->getBool(nifVer);

        if(uvs)
        {
            uvlist.resize(uvs);
            for(int i = 0;i < uvs;i++)
                nif->getVector2s(uvlist[i], verts);
        }

        if (nifVer >= 0x0a000100) // from 10.0.1.0
            consistencyFlags = nif->getUShort();

        if (nifVer >= 0x14000004) // from 20.0.0.4
            refAbstractAdditionalGeometryData = nif->getInt(); // FIXME
    }
};

class NiTriShapeData : public ShapeData
{
public:
    // Triangles, three vertex indices per triangle
    std::vector<short> triangles;

    void read(NIFStream *nif)
    {
        ShapeData::read(nif); // NiGeometryData

        unsigned short tris = nif->getUShort();

        // We have three times as many vertices as triangles, so this
        // is always equal to tris*3.
        unsigned int cnt = nif->getUInt();

        bool hasTriangles = false;
        if (nifVer <= 0x0a000100) // up to 10.0.1.0
            hasTriangles = true;
        else
            hasTriangles = nif->getBool(nifVer);

        if (hasTriangles)
            nif->getShorts(triangles, cnt);

        // Read the match list, which lists the vertices that are equal to
        // vertices. We don't actually need this for anything, so
        // just skip it.
        unsigned short verts = nif->getUShort();
        for(int i=0;i < verts;i++)
        {
            // Number of vertices matching vertex 'i'
            int num = nif->getUShort();
            nif->skip(num * sizeof(short));
        }
    }
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
    //bool hasUVQuads;
    //std::vector<Ogre::Vector4> uvQuads;

    int activeCount;

    bool hasSizes;
    std::vector<float> sizes;

    void read(NIFStream *nif)
    {
        ShapeData::read(nif);

        // Should always match the number of vertices
        if (nifVer <= 0x04020200) // up to 4.2.2.0
            numParticles = nif->getUShort();

        if (nifVer <= 0x0a000100) // up to 10.0.1.0
            particleRadius = nif->getFloat();

        if (nifVer >= 0x0a010000) // from 10.1.0.0
        {
            hasRadii = nif->getBool(nifVer);
            if (hasRadii)
                nif->getFloats(radii, vertices.size());
        }

        activeCount = nif->getUShort();

        hasSizes = nif->getBool(nifVer);
        // Particle sizes
        if (hasSizes)
            nif->getFloats(sizes, vertices.size());

        if (nifVer >= 0x0a010000) // from 10.1.0.0
        {
            hasRotations = nif->getBool(nifVer);
            if (hasRotations)
                nif->getQuaternions(rotations, vertices.size());
        }

        if (nifVer >= 0x14000004) // from 20.0.0.4
        {
            hasRotationAngles = nif->getBool(nifVer);
            if (hasRotationAngles)
                nif->getFloats(rotationAngles, vertices.size());

            hasRotationAxes = nif->getBool(nifVer);
            if (hasRotationAxes)
                nif->getVector3s(rotationAxes, vertices.size());
        }

        // FIXME: these require User Version to detect properly
        //hasUVQuads = nif->getBool(nifVer);
        //unsigned char numUVQuads = nif->getChar();
        //if (hasUVQuads)
            //nif->getVector4s(uvQuads, numUVQuads);
        //nif->getChar(); // unknown
    }
};

class NiAutoNormalParticlesData : public NiParticlesData {};

class NiRotatingParticlesData : public NiParticlesData
{
public:
    std::vector<Ogre::Quaternion> rotations;

    void read(NIFStream *nif)
    {
        NiParticlesData::read(nif);

        if (nifVer <= 0x04020200 && nif->getBool(nifVer)) // up to 4.2.2.0
        {
            // Rotation quaternions.
            nif->getQuaternions(rotations, vertices.size());
        }
    }
};

struct Particle
{
    Ogre::Vector3 translation;
    std::vector<float> unknownFloats;
    float unknown1;
    float unknown2;
    float unknown3;
    int unknown;

    void read(NIFStream *nif, unsigned int nifVer)
    {
        translation = nif->getVector3();
        if (nifVer <= 0x0a040001) // up to 10.4.0.1
            nif->getFloats(unknownFloats, 3);
        unknown1 = nif->getFloat();
        unknown2 = nif->getFloat();
        unknown3 = nif->getFloat();
        unknown = nif->getInt();
    }
};

class NiPSysData : public NiRotatingParticlesData
{
public:
    std::vector<Particle> particleDesc;
    std::vector<float> unknownFloats3;
    bool hasSubTextureUVs;
    std::vector<Ogre::Vector4> subTextureUVs;
    float aspectRatio;

    void read(NIFStream *nif)
    {
        NiRotatingParticlesData::read(nif);

        particleDesc.resize(vertices.size());
        for (unsigned int i = 0; i < vertices.size(); ++i)
            particleDesc[i].read(nif, nifVer);

        if (nifVer >= 0x14000004) // from 20.0.0.4
        {
            if (nif->getBool(nifVer))
                nif->getFloats(unknownFloats3, 3);
        }
        nif->getUShort();
        nif->getUShort();
        // FIXME: these require User Version to detect properly
        //hasSubTextureUVs = nif->getBool(nifVer);
        //unsigned int numUVs;
        //aspectRatio = nif->getFloat();
        //nif->getVector4s(subTextureUVs, numUVs);

        //nif->getUInt();
        //nif->getUInt();
        //nif->getUInt();
        //nif->getUShort();
        //nif->getChar();
    }
};

class NiPosData : public Record
{
public:
    Vector3KeyMap mKeyList;

    void read(NIFStream *nif)
    {
        mKeyList.read(nif);
    }
};

class NiUVData : public Record
{
public:
    FloatKeyMap mKeyList[4];

    void read(NIFStream *nif)
    {
        for(int i = 0;i < 4;i++)
            mKeyList[i].read(nif);
    }
};

class NiFloatData : public Record
{
public:
    FloatKeyMap mKeyList;

    void read(NIFStream *nif)
    {
        mKeyList.read(nif);
    }
};

class NiBoolData : public Record
{
public:
    BoolKeyMap mKeyList;

    void read(NIFStream *nif)
    {
        mKeyList.read(nif);
    }
};

class NiPixelData : public Record
{
public:
    unsigned int rmask, gmask, bmask, amask;
    int bpp, mips;

    void read(NIFStream *nif)
    {
        nif->getInt(); // always 0 or 1

        rmask = nif->getInt(); // usually 0xff
        gmask = nif->getInt(); // usually 0xff00
        bmask = nif->getInt(); // usually 0xff0000
        amask = nif->getInt(); // usually 0xff000000 or zero

        bpp = nif->getInt();

        // Unknown
        nif->skip(12);

        mips = nif->getInt();

        // Bytes per pixel, should be bpp * 8
        /*int bytes =*/ nif->getInt();

        for(int i=0; i<mips; i++)
        {
            // Image size and offset in the following data field
            /*int x =*/ nif->getInt();
            /*int y =*/ nif->getInt();
            /*int offset =*/ nif->getInt();
        }

        // Skip the data
        unsigned int dataSize = nif->getInt();
        nif->skip(dataSize);
    }
};

class NiColorData : public Record
{
public:
    Vector4KeyMap mKeyMap;

    void read(NIFStream *nif)
    {
        mKeyMap.read(nif);
    }
};

class NiVisData : public Record
{
public:
    struct VisData {
        float time;
        bool isSet;
    };
    std::vector<VisData> mVis;

    void read(NIFStream *nif)
    {
        int count = nif->getInt();
        mVis.resize(count);
        for(size_t i = 0;i < mVis.size();i++)
        {
            mVis[i].time = nif->getFloat();
            mVis[i].isSet = nif->getChar() != 0;
        }
    }
};

// FIXME: incomplete
class NiSkinPartition : public Record
{
public:
    unsigned short num;

    void read(NIFStream *nif)
    {
    }
};

class NiSkinInstance : public Record
{
public:
    NiSkinDataPtr data;
    //NiSkinPartitionPtr skinPartition;
    NodePtr root;
    NodeList bones;

    void read(NIFStream *nif)
    {
        data.read(nif);
        //if (nifVer >= 0x04020000) // from 4.2.0.0
            //skinPartition.read(nif);
        root.read(nif);
        bones.read(nif);
    }

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

    void read(NIFStream *nif)
    {
        trafo.rotation = nif->getMatrix3();
        trafo.trans = nif->getVector3();
        trafo.scale = nif->getFloat();
        unsigned char hasVertexWeights;

        int boneNum = nif->getInt();
        if (nifVer >= 0x04000002 && nifVer <= 0x0a010000)
            nif->getInt(); // NiSkinPartitionPtr.read(nif);
        if (nifVer >= 0x04020100) // from 4.2.1.0
            hasVertexWeights = nif->getChar();

        bones.resize(boneNum);
        for(int i=0;i<boneNum;i++)
        {
            BoneInfo &bi = bones[i];

            bi.trafo.rotation = nif->getMatrix3();
            bi.trafo.trans = nif->getVector3();
            bi.trafo.scale = nif->getFloat();
            bi.unknown = nif->getVector4();

            // Number of vertex weights
            bi.weights.resize(nif->getUShort());
            for(size_t j = 0;j < bi.weights.size();j++)
            {
                bi.weights[j].vertex = nif->getUShort();
                bi.weights[j].weight = nif->getFloat();
            }
        }
    }
};

struct NiMorphData : public Record
{
    struct MorphData {
        std::string mFrameName;
        FloatKeyMap mData;
        std::vector<Ogre::Vector3> mVertices;
    };
    std::vector<MorphData> mMorphs;

    void read(NIFStream *nif)
    {
        int morphCount = nif->getInt();
        int vertCount  = nif->getInt();
        /*relative targets?*/nif->getChar();

        mMorphs.resize(morphCount);
        for(int i = 0;i < morphCount;i++)
        {
            if (nifVer >= 0x0a01006a) // from 10.1.0.106
                mMorphs[i].mFrameName = nif->getString();

            if (nifVer <= 0x0a010000) // up to 10.1.0.0
            {
                mMorphs[i].mData.read(nif, true);
                if (nifVer >= 0x0a01006a && nifVer <= 0x0a020000)
                    nif->getUInt();
                // FIXME: need to check UserVersion == 0
                //if (nifVer >= 0x14000004 && nifVer <= 0x14000005)
                    //nif->getUInt();
            }
            nif->getVector3s(mMorphs[i].mVertices, vertCount);
        }
    }
};

struct NiKeyframeData : public Record
{
    QuaternionKeyMap mRotations;

    FloatKeyMap mXRotations;
    FloatKeyMap mYRotations;
    FloatKeyMap mZRotations;

    Vector3KeyMap mTranslations;
    FloatKeyMap mScales;

    void read(NIFStream *nif)
    {
        mRotations.read(nif);
        if(mRotations.mInterpolationType == mRotations.sXYZInterpolation)
        {
            if (nifVer <= 0x0a010000) // up to 10.1.0.0
                nif->getFloat(); //Chomp unused float
            mXRotations.read(nif, true);
            mYRotations.read(nif, true);
            mZRotations.read(nif, true);
        }
        mTranslations.read(nif);
        mScales.read(nif);
    }
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

    void read(NIFStream *nif)
    {
        ShapeData::read(nif);            // NiGeometryData

        /*int tris =*/ nif->getUShort(); // NiTriBasedGeomData

        unsigned short numStrips = nif->getUShort();
        stripLengths.resize(numStrips);
        for (unsigned int i = 0; i < numStrips; ++i)
        {
            stripLengths[i] = nif->getUShort();
        }

        bool hasPoints = false;
        if (nifVer <= 0x0a000102) // up to 10.0.1.2
            hasPoints = true;
        else
            hasPoints = nif->getBool(nifVer);

        if (hasPoints)
        {
            points.resize(numStrips);
            for (unsigned int i = 0; i < numStrips; ++i)
            {
                points[i].resize(stripLengths[i]);
                for (unsigned int j = 0; j < stripLengths[i]; ++j)
                    points[i][j] = nif->getUShort();
            }
        }

        // there are (N-2)*3 vertex indicies for triangles
        // where N = stripLengths[stripIndex]
        //
        // e.g. strip length = 150
        //      (150-2)*3 = 148*3 = 444
        unsigned int base = 0;
        for (unsigned int i = 0; i < numStrips; ++i)
        {
            base = triangles.size();
            triangles.resize(base + (stripLengths[i]-2)*3);
            for (unsigned int j = 0; j < (unsigned int)(stripLengths[i]-2); ++j)
            {
                if (j & 1)
                {
                    triangles[base+j*3]   = points[i][j];
                    triangles[base+j*3+1] = points[i][j+2];
                    triangles[base+j*3+2] = points[i][j+1];
                }
                else
                {
                    triangles[base+j*3]   = points[i][j];
                    triangles[base+j*3+1] = points[i][j+1];
                    triangles[base+j*3+2] = points[i][j+2];
                }
            }
        }
    }
};

struct hkTriangle
{
    std::vector<short> triangle;
    unsigned short weldingInfo;
    Ogre::Vector3 normal; // up to 20.0.0.5 only
};

struct hkPackedNiTriStripsData : public Record
{
    std::vector<hkTriangle> triangles;
    std::vector<Ogre::Vector3> vertices;

    void read(NIFStream *nif)
    {
        int tris = nif->getUInt();
        triangles.resize(tris);
        for(int i = 0; i < tris; i++)
        {
            triangles[i].triangle.resize(3);
            triangles[i].triangle[0] = nif->getShort();
            triangles[i].triangle[1] = nif->getShort();
            triangles[i].triangle[2] = nif->getShort();
            triangles[i].weldingInfo = nif->getUShort();
            triangles[i].normal = nif->getVector3();
        }

        int verts = nif->getUInt();
        vertices.resize(verts);
        for(int i = 0; i < verts; i++)
            vertices[i] = nif->getVector3();
    }
};

class NiStringPalette : public Record
{
    std::vector<std::string> palette;

    void read(NIFStream *nif)
    {
        unsigned int lth = nif->getUInt();

        std::istringstream buf(nif->getString(lth).c_str());
        std::string s;
        while (std::getline(buf, s, '\0'))
            palette.push_back(s);

        unsigned int check = nif->getUInt();
    }
};

struct AVObject
{
    std::string name;
    NodePtr avObject;
};

struct NiDefaultAVObjectPalette : public Record
{
    std::vector<AVObject> objs;

    void read(NIFStream *nif)
    {
        nif->getUInt();
        unsigned int numObjs = nif->getUInt();
        objs.resize(numObjs);
        for(unsigned int i = 0; i < numObjs; i++)
        {
            objs[i].name = nif->getString();
            objs[i].avObject.read(nif);
        }
    }
};

} // Namespace
#endif
