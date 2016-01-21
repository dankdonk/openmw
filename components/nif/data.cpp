#include "data.hpp"

#include "node.hpp"

Nif::ShapeData::ShapeData() : mIsNiPSysData(false), numUVSets(0), bsNumUVSets(0)
{
}

void Nif::ShapeData::read(NIFStream *nif)
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

void Nif::NiTriShapeData::read(NIFStream *nif)
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

void Nif::NiParticlesData::read(NIFStream *nif)
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

void Nif::NiRotatingParticlesData::read(NIFStream *nif)
{
    NiParticlesData::read(nif);

    if (nifVer <= 0x04020200 && nif->getBool(nifVer)) // up to 4.2.2.0
    {
        // Rotation quaternions.
        nif->getQuaternions(rotations, vertices.size());
    }
}

void Nif::Particle::read(NIFStream *nif, unsigned int nifVer)
    {
        translation = nif->getVector3();
        if (nifVer <= 0x0a040001) // up to 10.4.0.1
            nif->getFloats(unknownFloats, 3);
        unknown1 = nif->getFloat();
        unknown2 = nif->getFloat();
        unknown3 = nif->getFloat();
        unknown = nif->getInt();
    }

void Nif::NiPSysData::read(NIFStream *nif)
{
    mIsNiPSysData = true;

    NiRotatingParticlesData::read(nif);

    if (!(nifVer >= 0x14020007 && userVer >= 11))
    {
        particleDesc.resize(vertices.size());
        for (unsigned int i = 0; i < vertices.size(); ++i)
            particleDesc[i].read(nif, nifVer);
    }

    if (nifVer >= 0x14000004 && !(nifVer >= 0x14020007 && userVer >= 11))
    {
        if (nif->getBool(nifVer))
            nif->getFloats(unknownFloats3, vertices.size());
    }

    if (!(nifVer >= 0x14020007 && userVer == 11))
    {
        nif->getUShort(); // Unknown short 1
        nif->getUShort(); // Unknown short 2
    }

    if (nifVer >= 0x14020007 && userVer >= 12)
    {
        std::vector<Ogre::Vector4> subTexOffsetUVs;

        bool hasSubTexOffsetUVs = nif->getBool(nifVer);
        unsigned int numSubTexOffsetUVs = nif->getUInt();
        float aspectRatiro = nif->getFloat();
        if (hasSubTexOffsetUVs)
            nif->getVector4s(subTexOffsetUVs, numSubTexOffsetUVs);
        nif->getUInt(); // Unknown Int 4
        nif->getUInt(); // Unknown Int 5
        nif->getUInt(); // Unknown Int 6
        nif->getUShort(); // Unknown short 3
        nif->getChar(); // Unknown byte 4
    }
}

void Nif::BSStripPSysData::read(NIFStream *nif)
{
    NiPSysData::read(nif);

    unknown5 = nif->getShort();
    unknown6 = nif->getChar();
    unknown7 = nif->getInt();
    unknown8 = nif->getFloat();
}

void Nif::NiPosData::read(NIFStream *nif)
{
    mKeyList.read(nif);
}

void Nif::NiUVData::read(NIFStream *nif)
{
    for(int i = 0;i < 4;i++)
        mKeyList[i].read(nif);
}

void Nif::NiFloatData::read(NIFStream *nif)
{
    mKeyList.read(nif);
}

void Nif::NiBoolData::read(NIFStream *nif)
{
    mKeyList.read(nif);
}

void Nif::NiPixelData::read(NIFStream *nif)
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

void Nif::NiColorData::read(NIFStream *nif)
{
    mKeyMap.read(nif);
}

void Nif::NiVisData::read(NIFStream *nif)
{
    int count = nif->getInt();
    mVis.resize(count);
    for(size_t i = 0;i < mVis.size();i++)
    {
        mVis[i].time = nif->getFloat();
        mVis[i].isSet = nif->getChar() != 0;
    }
}

void Nif::NiSkinPartition::SkinPartitionBlock::read(NIFStream *nif,
        unsigned int nifVer, unsigned int userVer)
{
    numVerts = nif->getUShort();
    numTriangles = nif->getUShort();
    numBones = nif->getUShort();
    numStrips = nif->getUShort();
    numWeightsPerVert = nif->getUShort();

    bones.resize(numBones);
    for (unsigned int i = 0; i < numBones; ++i)
        bones[i] = nif->getUShort();

    hasVertMap = nif->getBool(nifVer);
    if (hasVertMap)
    {
        vertMap.resize(numVerts);
        for (unsigned int i = 0; i < numVerts; ++i)
        {
            vertMap[i] = nif->getUShort();
        }
    }

    hasVertWeights = nif->getBool(nifVer);
    if (hasVertWeights)
    {
        vertWeights.resize(numVerts);
        for (unsigned int i = 0; i < numVerts; ++i)
        {
            vertWeights[i].resize(numWeightsPerVert);
            for (unsigned int j = 0; j < numWeightsPerVert; ++j)
            {
                vertWeights[i][j] = nif->getFloat();
            }
        }
    }

    stripLengths.resize(numStrips);
    for (unsigned int i = 0; i < numStrips; ++i)
        stripLengths[i] = nif->getUShort();

    hasFaces = nif->getBool(nifVer);
    if (hasFaces && numStrips != 0)
    {
        strips.resize(numStrips);
        for (unsigned int i = 0; i < numStrips; ++i)
        {
            strips[i].resize(stripLengths[i]);
            for (unsigned int j = 0; j < stripLengths[i]; ++j)
            {
                strips[i][j] = nif->getUShort();
            }
        }
    }
    else if (hasFaces && numStrips == 0)
    {
        triangles.resize(numTriangles);
        for (unsigned int i = 0; i < numTriangles; ++i)
        {
            triangles[i].v1 = nif->getUShort();
            triangles[i].v2 = nif->getUShort();
            triangles[i].v3 = nif->getUShort();
        }
    }

    hasBoneIndicies = nif->getBool(nifVer);
    if (hasBoneIndicies)
    {
        boneIndicies.resize(numVerts);
        for (unsigned int i = 0; i < numVerts; ++i)
        {
            boneIndicies[i].resize(numWeightsPerVert);
            for (unsigned int j = 0; j < numWeightsPerVert; ++j)
            {
                boneIndicies[i][j] = nif->getChar();
            }
        }
    }

    if (userVer >= 12)
        nif->getUShort();
}

void Nif::NiSkinPartition::read(NIFStream *nif)
{
    numSkinPartitionBlocks = nif->getUInt();
    skinPartitionBlocks.resize(numSkinPartitionBlocks);
    for (unsigned int i = 0; i < numSkinPartitionBlocks; ++i)
        skinPartitionBlocks[i].read(nif, nifVer, userVer);
}

void Nif::NiSkinInstance::read(NIFStream *nif)
{
    data.read(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        skinPartition.read(nif);
    root.read(nif);
    bones.read(nif);
}

void Nif::NiSkinInstance::post(NIFFile *nif)
{
    data.post(nif);
    if (nifVer >= 0x0a020000) // from 10.2.0.0
        skinPartition.post(nif);
    root.post(nif);
    bones.post(nif);

    if(data.empty() || root.empty())
        nif->fail("NiSkinInstance missing root or data");

    size_t bnum = bones.length();
    if(bnum != data->bones.size())
        nif->fail("Mismatch in NiSkinData bone count");

    root->makeRootBone(&data->trafo);

    for(unsigned int i = 0; i < bnum; i++)
    {
        if(bones[i].empty())
            nif->fail("Oops: Missing bone! Don't know how to handle this.");
        bones[i]->makeBone(i, data->bones[i]);
    }
}

void Nif::NiSkinData::read(NIFStream *nif)
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

void Nif::NiSkinData::post(NIFFile *nif)
{
    //if (nifVer >= 0x04000002 && nifVer <= 0x0a010000)
        //skinPartition.post(nif);
}

void Nif::NiMorphData::read(NIFStream *nif)
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

void Nif::NiKeyframeData::read(NIFStream *nif)
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

void Nif::NiTriStripsData::read(NIFStream *nif)
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
        base = static_cast<unsigned int>(triangles.size());
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

void Nif::NiStringPalette::read(NIFStream *nif)
{
    unsigned int lth = nif->getUInt();

    std::istringstream buf(nif->getString(lth).c_str());
    std::string s;
    while (std::getline(buf, s, '\0'))
        palette.push_back(s);

    unsigned int check = nif->getUInt();
}

void Nif::NiDefaultAVObjectPalette::read(NIFStream *nif)
{
    nif->getUInt();
    unsigned int numObjs = nif->getUInt();
    objs.resize(numObjs);
    for(unsigned int i = 0; i < numObjs; i++)
    {
        objs[i].name = nif->getString(); // TODO: sized string?
        objs[i].avObject.read(nif);
    }
}

void Nif::AVObject::post(NIFFile *nif)
{
    avObject.post(nif);
}

void Nif::NiDefaultAVObjectPalette::post(NIFFile *nif)
{
    for (unsigned int i = 0; i < objs.size(); ++i)
        objs[i].post(nif);
}

void Nif::BSShaderTextureSet::read(NIFStream *nif)
{
    unsigned int numTextures = nif->getUInt();
    textures.resize(numTextures);
    for (unsigned int i = 0; i < numTextures; ++i)
    {
        unsigned int size = nif->getUInt();
        textures[i] = nif->getString(size);
    }
}
