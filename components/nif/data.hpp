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

    ShapeData() : mIsNiPSysData(false), numUVSets(0), bsNumUVSets(0) { }


    void read(NIFStream *nif)
    {
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            int unknownInt = nif->getInt(); // always 0

        unsigned int verts = 0;
        if (!mIsNiPSysData || (mIsNiPSysData && (nifVer < 0x14020007 || userVer < 11)))
            verts = nif->getUShort();

        if (mIsNiPSysData && nifVer >= 0x14020007 && userVer >= 11)
            bsMaxVerts = nif->getUShort();

        if (nifVer >= 0x0a010000) // from 10.1.0.0
        {
            keepFlags = nif->getChar();
            compressFlags = nif->getChar();
        }

        if(nif->getBool(nifVer)) // has vertices
            nif->getVector3s(vertices, verts);

        if (nifVer >= 0x0a000100) // from 10.0.1.0
        {
            if (nifVer < 0x14020007 || userVer < 11)
                numUVSets = nif->getUShort();

            if (nifVer >= 0x14020007 && userVer >= 11)
                bsNumUVSets = nif->getUShort();
        }

        if (!mIsNiPSysData && nifVer >= 0x14020007 && userVer == 12)
            nif->getUInt(); // Unknown Int 2

        bool hasNormals = nif->getBool(nifVer);
        if(hasNormals)
            nif->getVector3s(normals, verts);

        if (hasNormals && ((numUVSets & 0xf000) || (bsNumUVSets & 0xf000)))
        {
            nif->getVector3s(tangents, verts);
            nif->getVector3s(bitangents, verts);
        }

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
        if (nifVer >= 0x14020007 && userVer >= 11)
            uvs |= bsNumUVSets & 1;

        if (nifVer <= 0x04000002) // up to 4.0.0.2
            bool hasUV = nif->getBool(nifVer);

        if(uvs)
        {
            uvlist.resize(uvs);
            for(int i = 0;i < uvs;i++)
                nif->getVector2s(uvlist[i], verts);
        }

        consistencyFlags = 0;
        if (nifVer >= 0x0a000100) // from 10.0.1.0
            if (userVer < 12 || (userVer >= 12 && !mIsNiPSysData))
                consistencyFlags = nif->getUShort();

        if (nifVer >= 0x14000004) // from 20.0.0.4
            if (userVer < 12 || (userVer >= 12 && !mIsNiPSysData))
                /*additionalData.read(nif);*/ nif->getInt(); // FIXME
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
            if (hasRadii && !(nifVer >= 0x14020007 && userVer >= 11))
                nif->getFloats(radii, vertices.size());
        }

        activeCount = nif->getUShort();

        hasSizes = nif->getBool(nifVer);
        // Particle sizes
        if (hasSizes && !(nifVer >= 0x14020007 && userVer >= 11))
            nif->getFloats(sizes, vertices.size());

        if (nifVer >= 0x0a010000) // from 10.1.0.0
        {
            hasRotations = nif->getBool(nifVer);
            if (hasRotations && !(nifVer >= 0x14020007 && userVer >= 11))
                nif->getQuaternions(rotations, vertices.size());
        }

        if (nifVer >= 0x14020007 && userVer >= 12)
        {
            nif->getChar(); // Unknown byte 1
            nif->getInt(); // NiNodePtr
        }

        if (nifVer >= 0x14000004) // from 20.0.0.4
        {
            hasRotationAngles = nif->getBool(nifVer);
            if (hasRotationAngles && !(nifVer >= 0x14020007 && userVer >= 11))
                nif->getFloats(rotationAngles, vertices.size());
        }

        if (nifVer >= 0x14000004) // from 20.0.0.4
        {
            hasRotationAxes = nif->getBool(nifVer);
            if (hasRotationAxes && !(nifVer >= 0x14020007 && userVer >= 11))
                nif->getVector3s(rotationAxes, vertices.size());
        }

        if (nifVer >= 0x14020007 && userVer == 11)
        {
            hasUVQuads = nif->getBool(nifVer);
            unsigned char numUVQuads = nif->getChar();
            if (hasUVQuads)
                nif->getVector4s(uvQuads, numUVQuads);
        }

        if (nifVer == 0x14020007 && userVer >= 11)
            nif->getChar(); // Unknown byte 2
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

class NiSkinInstance : public Record
{
public:
    NiSkinDataPtr data;
    NiSkinPartitionPtr skinPartition;
    NodePtr root;
    NodeList bones;

    void read(NIFStream *nif)
    {
        data.read(nif);
        if (nifVer >= 0x0a020000) // from 10.2.0.0
            skinPartition.read(nif);
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

    void post(NIFFile *nif)
    {
        //if (nifVer >= 0x04000002 && nifVer <= 0x0a010000)
            //skinPartition.post(nif);
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
                mMorphs[i].mFrameName = nif->getSkyrimString(nifVer, Record::strings);

            if (nifVer <= 0x0a010000) // up to 10.1.0.0
            {
                mMorphs[i].mData.read(nif, true);
                if (nifVer >= 0x0a01006a && nifVer <= 0x0a020000)
                    nif->getUInt();
                if (nifVer >= 0x14000004 && nifVer <= 0x14000005 && userVer == 0)
                    nif->getUInt();
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

} // Namespace
#endif
