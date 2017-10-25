/*
  Copyright (C) 2015-2017 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

*/
#include "nidata.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

#include "nistream.hpp"
#include "niavobject.hpp" // static_cast NiNode
#include "nimodel.hpp"

NiBtOgre::ATextureRenderData::ATextureRenderData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    if (stream.nifVer() > 0x14000004)
        throw std::runtime_error("NiBtOgre::ATextureRenderData::unsupported NIF file version");

    stream.read(mPixelFormat);
    stream.read(mRedMask);
    stream.read(mGreenMask);
    stream.read(mBlueMask);
    stream.read(mAlphaMask);
    stream.read(mBitsPerPixel);

    mUnknown3Bytes.resize(3);
    for (unsigned int i = 0; i < 3; ++i)
        stream.read(mUnknown3Bytes.at(i));

    mUnknown8Bytes.resize(8);
    for (unsigned int i = 0; i < 8; ++i)
        stream.read(mUnknown8Bytes.at(i));

    if (stream.nifVer() >= 0x0a010000 && stream.nifVer() <= 0x0a020000)
        stream.skip(sizeof(std::uint32_t));

    stream.read(mPaletteIndex);
    stream.read(mNumMipmaps);
    stream.read(mBytesPerPixel);

    mMipmaps.resize(mNumMipmaps);
    for (unsigned int i = 0; i < mNumMipmaps; ++i)
    {
        stream.read(mMipmaps[i].width);
        stream.read(mMipmaps[i].height);
        stream.read(mMipmaps[i].offset);
    }
}

NiBtOgre::NiPixelData::NiPixelData(NiStream& stream, const NiModel& model)
    : ATextureRenderData(stream, model)
{
    if (stream.nifVer() > 0x14000004)
        throw std::runtime_error("NiBtOgre::NiPixelData::unsupported NIF file version");

    stream.read(mNumPixels);
    mPixelData.resize(mNumPixels);
    for (unsigned int i = 0; i < mNumPixels; ++i)
        stream.read(mPixelData.at(i));
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSMultiBound::BSMultiBound(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    stream.read(mDataIndex);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSMultiBoundOBB::BSMultiBoundOBB(NiStream& stream, const NiModel& model)
    : BSMultiBoundData(stream, model)
{
    stream.read(mCenter);
    stream.read(mSize);
    stream.read(mRotation);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiDefaultAVObjectPalette::NiDefaultAVObjectPalette(NiStream& stream, const NiModel& model)
    : NiAVObjectPalette(stream, model)
{
    stream.skip(sizeof(std::uint32_t));

    std::uint32_t numObjs = 0;
    stream.read(numObjs);

    mObjs.resize(numObjs);
    for (unsigned int i = 0; i < numObjs; i++)
    {
        stream.readSizedString(mObjs.at(i).name);
        stream.read(mObjs.at(i).avObjectIndex);
    }
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSShaderTextureSet::BSShaderTextureSet(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    std::uint32_t numTextures = 0;
    stream.read(numTextures);

    mTextures.resize(numTextures);
    for (unsigned int i = 0; i < numTextures; ++i)
        stream.readSizedString(mTextures.at(i));
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBSplineBasisData::NiBSplineBasisData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    stream.read(mNumControlPoints);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBSplineData::NiBSplineData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    stream.readVector<float>(mFloatControlPoints);
    stream.readVector<std::int16_t>(mShortControlPoints);
}

#if 0
template<>
NiBtOgre::KeyGroup<char>::KeyGroup(NiStream& stream)
{
    // FIXME
}

template<>
NiBtOgre::KeyGroup<Ogre::Vector4>::KeyGroup(NiStream& stream)
{
    // FIXME
}

template<>
NiBtOgre::KeyGroup<float>::KeyGroup(NiStream& stream)
{
    // FIXME
}
#endif

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBoolData::NiBoolData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    mData.read(stream);
}

NiBtOgre::NiColorData::NiColorData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    mData.read(stream);
}

NiBtOgre::NiExtraData::NiExtraData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    //if (stream.nifVer() >= 0x0a000100) // from 10.0.1.0
        stream.readLongString(mName);

    if (stream.nifVer() <= 0x04020200) // up to 4.2.2.0
        stream.read(mNextIndex);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSBehaviorGraphExtraData::BSBehaviorGraphExtraData(NiStream& stream, const NiModel& model)
    : NiExtraData(stream, model)
{
    stream.readLongString(mBehaviourGraphFile);
    stream.read(mControlBaseSkeleton);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::BSBound::BSBound(NiStream& stream, const NiModel& model)
    : NiExtraData(stream, model)
{
    stream.read(mCenter);
    stream.read(mDimensions);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSDecalPlacementVectorExtraData::BSDecalPlacementVectorExtraData(NiStream& stream, const NiModel& model)
    : NiExtraData(stream, model)
{
    stream.read(mUnknown1);

    std::uint16_t numVectorBlocks;
    stream.read(numVectorBlocks);

    mVectorBlocks.resize(numVectorBlocks);
    for (unsigned int i = 0; i < numVectorBlocks; ++i)
    {
        std::uint16_t numVectors;
        stream.read(numVectors);

        mVectorBlocks[i].points.resize(numVectors);
        for (unsigned int j = 0; j < numVectors; ++j)
            stream.read(mVectorBlocks[i].points.at(j));

        mVectorBlocks[i].normals.resize(numVectors);
        for (unsigned int j = 0; j < numVectors; ++j)
            stream.read(mVectorBlocks[i].normals.at(j));
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::BSFurnitureMarker::BSFurnitureMarker(NiStream& stream, const NiModel& model)
    : NiExtraData(stream, model)
{
    std::uint32_t numPos;
    stream.read(numPos);

    mPositions.resize(numPos);
    for (unsigned int i = 0; i < numPos; ++i)
    {
        stream.read(mPositions[i].offset);
        if (stream.userVer() <= 11)
        {
            stream.read(mPositions[i].orientation);
            stream.read(mPositions[i].posRef1);
            stream.read(mPositions[i].posRef2);
        }
        if (stream.nifVer() >= 0x14020007 && stream.userVer() >= 12) // from 20.2.0.7
        {
            stream.read(mPositions[i].heading);
            stream.read(mPositions[i].animationType);
            stream.read(mPositions[i].entryProperties);
        }
    }
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSInvMarker::BSInvMarker(NiStream& stream, const NiModel& model)
    : NiExtraData(stream, model)
{
    stream.read(mRotationX);
    stream.read(mRotationY);
    stream.read(mRotationZ);
    stream.read(mZoom);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBinaryExtraData::NiBinaryExtraData(NiStream& stream, const NiModel& model)
    : NiExtraData(stream, model)
{
    stream.readVector<char>(mData);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBooleanExtraData::NiBooleanExtraData(NiStream& stream, const NiModel& model)
    : NiExtraData(stream, model)
{
    stream.read(mBooleanData);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiFloatExtraData::NiFloatExtraData(NiStream& stream, const NiModel& model)
    : NiExtraData(stream, model)
{
    stream.read(mFloatData);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiIntegerExtraData::NiIntegerExtraData(NiStream& stream, const NiModel& model)
    : NiExtraData(stream, model)
{
    stream.read(mIntegerData);
}

NiBtOgre::NiStringExtraData::NiStringExtraData(NiStream& stream, const NiModel& model)
    : NiExtraData(stream, model)
{
    if (stream.nifVer() <= 0x04020200) // up to 4.2.2.0
        stream.skip(sizeof(std::uint32_t));

    stream.readLongString(mStringData);
}

NiBtOgre::NiTextKeyExtraData::NiTextKeyExtraData(NiStream& stream, const NiModel& model)
    : NiExtraData(stream, model)
{
    if (stream.nifVer() <= 0x04020200) // up to 4.2.2.0
        stream.skip(sizeof(std::uint32_t)); // always 0?

    std::uint32_t numTextKeys;
    stream.read(numTextKeys);
    mTextKeys.resize(numTextKeys);
    for (unsigned int i = 0; i < numTextKeys; ++i)
    {
        stream.read(mTextKeys.at(i).time);
        stream.readLongString(mTextKeys.at(i).text);
    }
}

NiBtOgre::NiVertWeightsExtraData::NiVertWeightsExtraData(NiStream& stream, const NiModel& model)
    : NiExtraData(stream, model)
{
    std::uint32_t numBytes;
    stream.read(numBytes);

    std::uint16_t numVertices;
    stream.read(numVertices);

    stream.skip(numVertices * sizeof(float));
}

NiBtOgre::NiFloatData::NiFloatData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    mData.read(stream);
}

NiBtOgre::NiGeometryData::NiGeometryData(NiStream& stream, const NiModel& model, bool isNiPSysData)
    : NiObject(stream, model), mNumVertices(0), mNumUVSets(0), mBSNumUVSets(0) , mIsNiPSysData(isNiPSysData)
{
    if (stream.nifVer() >= 0x0a020000) // from 10.2.0.0
    {
        std::int32_t unknownInt;
        stream.read(unknownInt); // always 0
    }

    if (!mIsNiPSysData || (mIsNiPSysData && (stream.nifVer() < 0x14020007 || stream.userVer() < 11)))
        stream.read(mNumVertices);

    if (mIsNiPSysData && stream.nifVer() >= 0x14020007 && stream.userVer() >= 11)
        stream.read(mBSMaxVertices);

    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
    {
        stream.read(mKeepFlags);
        stream.read(mCompressFlags);
    }

    if (stream.getBool()) // has vertices
    {
        stream.readVector<Ogre::Vector3>(mVertices, mNumVertices);
    }

    if (stream.nifVer() >= 0x0a000100) // from 10.0.1.0
    {
        if (stream.nifVer() < 0x14020007 || stream.userVer() < 11)
            stream.read(mNumUVSets);

        if (stream.nifVer() >= 0x14020007 && stream.userVer() >= 11)
            stream.read(mBSNumUVSets);
    }

    if (!mIsNiPSysData && stream.nifVer() >= 0x14020007 && stream.userVer() == 12)
        stream.skip(sizeof(std::uint32_t)); // Unknown Int 2

    bool hasNormals = stream.getBool();
    if (hasNormals)
        stream.readVector<Ogre::Vector3>(mNormals, mNumVertices);

    if (hasNormals && ((mNumUVSets & 0xf000) || (mBSNumUVSets & 0xf000)))
    {
        stream.readVector<Ogre::Vector3>(mTangents, mNumVertices);
        stream.readVector<Ogre::Vector3>(mBitangents, mNumVertices);
    }

    stream.read(mCenter);
    stream.read(mRadius);

    if (stream.getBool()) // has vertex colors
        stream.readVector<Ogre::Vector4>(mVertexColors, mNumVertices);

    std::uint16_t numUVSets = 0;
    if (stream.nifVer() <= 0x04020200) // up to 4.2.2.0
    {
        stream.read(numUVSets);
        numUVSets &= 0x3f;
    }
    if (stream.nifVer() >= 0x0a000100) // from 10.0.1.0
        numUVSets = mNumUVSets & 0x3f;
    if (stream.nifVer() >= 0x14020007 && stream.userVer() >= 11)
        numUVSets |= mBSNumUVSets & 1;

    if (stream.nifVer() <= 0x04000002) // up to 4.0.0.2
        bool hasUV = stream.getBool(); // is this used anywhere?

    mUVSets.resize(numUVSets);
    for (unsigned int i = 0; i < numUVSets; ++i)
    {
        mUVSets.at(i).resize(mNumVertices);
        for (unsigned int j = 0; j < mNumVertices; ++j)
        {
            stream.read(mUVSets.at(i).at(j).u);
            stream.read(mUVSets.at(i).at(j).v);
        }
    }

    if (stream.nifVer() >= 0x0a000100) // from 10.0.1.0
        if (stream.userVer() < 12 || (stream.userVer() >= 12 && !mIsNiPSysData))
            stream.read(mConsistencyFlags);

    if (stream.nifVer() >= 0x14000004) // from 20.0.0.4
        if (stream.userVer() < 12 || (stream.userVer() >= 12 && !mIsNiPSysData))
            stream.read(mAdditionalDataIndex);
}

NiBtOgre::NiParticlesData::NiParticlesData(NiStream& stream, const NiModel& model, bool isNiPSysData)
    : NiGeometryData(stream, model, isNiPSysData)
{
    if (stream.nifVer() <= 0x04020200) // up to 4.2.2.0
        stream.read(mNumParticles);

    if (stream.nifVer() <= 0x0a000100) // up to 10.0.1.0
        stream.read(mParticleRadius);

    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
    {
        if (stream.getBool() && !(stream.nifVer() >= 0x14020007 && stream.userVer() >= 11))
            stream.readVector<float>(mRadii, mNumVertices);
    }

    stream.read(mNumActive);

    if (stream.getBool() && !(stream.nifVer() >= 0x14020007 && stream.userVer() >= 11))
        stream.readVector<float>(mSizes, mNumVertices);

    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
    {
        if (stream.getBool() && !(stream.nifVer() >= 0x14020007 && stream.userVer() >= 11))
            stream.readVector<Ogre::Quaternion>(mRotations, mNumVertices);
    }

    if (stream.nifVer() >= 0x14020007 && stream.userVer() >= 12)
    {
        stream.skip(sizeof(char)); // unknown byte 1
        NiObjectRef unknownLink;
        stream.read(unknownLink);
    }

    if (stream.nifVer() >= 0x14000004) // from 20.0.0.4
    {
        if (stream.getBool() && !(stream.nifVer() >= 0x14020007 && stream.userVer() >= 11))
            stream.readVector<float>(mRotationAngles, mNumVertices);
    }

    if (stream.nifVer() >= 0x14000004) // from 20.0.0.4
    {
        if (stream.getBool() && !(stream.nifVer() >= 0x14020007 && stream.userVer() >= 11))
            stream.readVector<Ogre::Vector3>(mRotationAxes, mNumVertices);
    }

    if (stream.nifVer() >= 0x14020007 && stream.userVer() == 11)
    {
        bool hasUVQuads = stream.getBool();
        unsigned char numUVQuadrants = 0;
        stream.read(numUVQuadrants);
        if (hasUVQuads)
            stream.readVector<Ogre::Vector4>(mUVQuadrants, numUVQuadrants);
    }

    if (stream.nifVer() == 0x14020007 && stream.userVer() >= 11)
        stream.skip(sizeof(char)); // unknown byte 2
}

NiBtOgre::NiRotatingParticlesData::NiRotatingParticlesData(NiStream& stream, const NiModel& model,
        bool isNiPSysData)
    : NiParticlesData(stream, model, isNiPSysData)
{
    if (stream.nifVer() <= 0x04020200) // up to 4.2.2.0
        if (stream.getBool())
            stream.readVector<Ogre::Quaternion>(mRotations2, mNumVertices);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPSysData::NiPSysData(NiStream& stream, const NiModel& model)
    : NiRotatingParticlesData(stream, model, true)
{
    if (!(stream.nifVer() >= 0x14020007 && stream.userVer() >= 11))
    {
        mParticleDescriptions.resize(mNumVertices);
        for (unsigned int i = 0; i < mNumVertices; ++i)
        {
            // ParticleDesc
            stream.read(mParticleDescriptions.at(i).translation);
            if (stream.nifVer() <= 0x0a040001) // up to 10.4.0.1
                stream.read(mParticleDescriptions.at(i).unknownFloats);
            stream.read(mParticleDescriptions.at(i).unknown1);
            stream.read(mParticleDescriptions.at(i).unknown2);
            stream.read(mParticleDescriptions.at(i).unknown3);
            stream.read(mParticleDescriptions.at(i).unknown);
        }
    }

    if (stream.nifVer() >= 0x14000004 && !(stream.nifVer() >= 0x14020007 && stream.userVer() >= 11))
    {
        if (stream.getBool())
            stream.readVector<float>(mUnknownFloats3, mNumVertices);
    }

    if (!(stream.nifVer() >= 0x14020007 && stream.userVer() == 11))
    {
        stream.skip(sizeof(std::uint16_t));
        stream.skip(sizeof(std::uint16_t));
    }

    if (stream.nifVer() >= 0x14020007 && stream.userVer() >= 12)
    {
        bool hasSubTexOffsetUVs = stream.getBool();
        std::uint32_t numSubTexOffsetUVs;
        stream.read(numSubTexOffsetUVs);
        stream.read(mAspectRatio);
        if (hasSubTexOffsetUVs)
            stream.readVector<Ogre::Vector4>(mSubTextureOffsetUVs, numSubTexOffsetUVs);
        stream.skip(sizeof(std::uint32_t)); // unknown Int 4
        stream.skip(sizeof(std::uint32_t)); // unknown Int 5
        stream.skip(sizeof(std::uint32_t)); // unknown Int 6
        stream.skip(sizeof(std::uint16_t)); // unknown short 3
        stream.skip(sizeof(char)); // unknown byte 4
    }
}

// Seen in NIF version 20.2.0.7
NiBtOgre::BSStripPSysData::BSStripPSysData(NiStream& stream, const NiModel& model)
    : NiPSysData(stream, model)
{
    stream.read(mUnknown5);
    stream.read(mUnknown6);
    stream.read(mUnknown7);
    stream.read(mUnknown8);
}

NiBtOgre::NiTriBasedGeomData::NiTriBasedGeomData(NiStream& stream, const NiModel& model)
    : NiGeometryData(stream, model)
{
    stream.read(mNumTriangles);
}

NiBtOgre::NiTriShapeData::NiTriShapeData(NiStream& stream, const NiModel& model)
    : NiTriBasedGeomData(stream, model)
{
    std::uint32_t numTrianglePoints; // mNumTriangles times 3
    stream.read(numTrianglePoints);

    bool hasTriangles = false;
    if (stream.nifVer() >= 0x0a000100) // from 10.0.1.0
        hasTriangles = stream.getBool();
    else
        hasTriangles = true;

    if (hasTriangles)
        stream.readVector<std::uint16_t>(mTriangles, numTrianglePoints);

    std::uint16_t numMatchGroups;
    stream.read(numMatchGroups);

    std::uint16_t numVertices;
    for (unsigned int i = 0; i < numMatchGroups; ++i)
    {
        stream.read(numVertices);
        stream.skip(numVertices * sizeof(std::uint16_t));
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiTriStripsData::NiTriStripsData(NiStream& stream, const NiModel& model)
    : NiTriBasedGeomData(stream, model)
{
    std::uint16_t numStrips;
    stream.read(numStrips);

    mStripLengths.resize(numStrips);
    for (unsigned int i = 0; i < numStrips; ++i)
        stream.read(mStripLengths.at(i));

    bool hasPoints = false;
    if (stream.nifVer() <= 0x0a000102) // up to 10.0.1.2
        hasPoints = true;
    else
        hasPoints = stream.getBool();

    if (hasPoints)
    {
        mPoints.resize(numStrips);
        for (unsigned int i = 0; i < numStrips; ++i)
        {
            mPoints[i].resize(mStripLengths[i]);
            for (unsigned int j = 0; j < mStripLengths[i]; ++j)
                stream.read(mPoints.at(i).at(j));
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
        base = static_cast<unsigned int>(mTriangles.size());
        mTriangles.resize(base + (mStripLengths[i]-2)*3);
        for (unsigned int j = 0; j < (unsigned int)(mStripLengths[i]-2); ++j)
        {
            if (j & 1)
            {
                mTriangles[base+j*3]   = mPoints[i][j];
                mTriangles[base+j*3+1] = mPoints[i][j+2];
                mTriangles[base+j*3+2] = mPoints[i][j+1];
            }
            else
            {
                mTriangles[base+j*3]   = mPoints[i][j];
                mTriangles[base+j*3+1] = mPoints[i][j+1];
                mTriangles[base+j*3+2] = mPoints[i][j+2];
            }
        }
    }
}

NiBtOgre::NiKeyframeData::NiKeyframeData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model), mRotationType(1/*LINEAR_KEY*/)
{
    std::uint32_t numRotationKeys = 0;
    stream.read(numRotationKeys);

    if (numRotationKeys != 0)
        stream.read(mRotationType);

    if (mRotationType != 4) // not XYZ_ROTATION_KEY
    {
        mQuaternionKeys.resize(numRotationKeys);
        for (unsigned int i = 0; i < numRotationKeys; ++i)
        {
            stream.read(mQuaternionKeys.at(i).time);
            stream.read(mQuaternionKeys.at(i).value);
            if (mRotationType == 3/* TBC_KEY */)
            {
                stream.read(mQuaternionKeys.at(i).tension);
                stream.read(mQuaternionKeys.at(i).bias);
                stream.read(mQuaternionKeys.at(i).continuity);
            }
        }
    }
    else                    // XYZ_ROTATION_KEY
    {
        if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
            stream.skip(sizeof(float)); // unknown float
        mXRotations.read(stream);
        mYRotations.read(stream);
        mZRotations.read(stream);
    }
    mTranslations.read(stream);
    mScales.read(stream);
}

NiBtOgre::NiMorphData::NiMorphData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    std::uint32_t numMorphs;
    stream.read(numMorphs);

    std::uint32_t numVertices;
    stream.read(numVertices);

    stream.skip(sizeof(char)); // relative targets

    mMorphs.resize(numMorphs);
    for (unsigned int i = 0; i < numMorphs; ++i)
    {
        if (stream.nifVer() >= 0x0a01006a) // from 10.1.0.106
            stream.readLongString(mMorphs.at(i).mFrameName);

        if (stream.nifVer() <= 0x0a010000) // up to 10.1.0.0
        {
            std::uint32_t numKeys;
            stream.read(numKeys);
            std::uint32_t interpolation;
            stream.read(interpolation);

            mMorphs.at(i).mKeys.resize(numKeys);
            for (unsigned int j = 0; j < numMorphs; ++j)
            {
                Key<float> key;
                key.read(stream, interpolation);
            }
        }

        if (stream.nifVer() >= 0x0a01006a && stream.nifVer() <= 0x0a020000)
            stream.skip(sizeof(std::uint32_t));

        if (stream.nifVer() >= 0x14000004 && stream.nifVer() <= 0x14000005 && stream.userVer() == 0)
            stream.skip(sizeof(std::uint32_t));

        stream.readVector<Ogre::Vector3>(mMorphs.at(i).mVectors, numVertices);
    }
}

NiBtOgre::NiPosData::NiPosData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    mData.read(stream);
}

NiBtOgre::NiSkinData::NiSkinData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model), mSkinPartitionIndex(-1)
{
    stream.read(mSkinTransform.rotation);
    stream.read(mSkinTransform.translation);
    stream.read(mSkinTransform.scale);

    std::uint32_t numBones;
    stream.read(numBones);
    if (/*stream.nifVer() >= 0x04000002 && */stream.nifVer() <= 0x0a010000) // we don't support < 4.0.0.2
        stream.read(mSkinPartitionIndex);

    unsigned char hasVertexWeights = 0;
    if (stream.nifVer() >= 0x04020100) // from 4.2.1.0 (should this be 4.2.2.0?)
        stream.read(hasVertexWeights);

    mBoneList.resize(numBones);
    for (unsigned int i = 0; i < numBones; ++i)
    {
        SkinData &skin = mBoneList.at(i);

        stream.read(skin.skinTransform.rotation);
        stream.read(skin.skinTransform.translation);
        stream.read(skin.skinTransform.scale);
        stream.read(skin.boundingSphereOffset);
        stream.read(skin.boundingSphereRadius);

        std::uint16_t numVertices;
        stream.read(numVertices);
        if (stream.nifVer() <= 0x04020100 || (stream.nifVer() >= 0x04020200 && hasVertexWeights))
        {
            skin.vertexWeights.resize(numVertices);
            for (unsigned int j = 0; j < numVertices; ++j)
            {
                stream.read(skin.vertexWeights[j].vertex);
                stream.read(skin.vertexWeights[j].weight);
            }
        }
    }
}

NiBtOgre::NiSkinInstance::NiSkinInstance(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    stream.read(mDataIndex);
    if (stream.nifVer() >= 0x0a020000) // from 10.2.0.0
        stream.read(mSkinPartitionIndex);
    //stream.getPtr<NiNode>(mSkeletonRoot, model.objects());
    std::int32_t index = -1;
    stream.read(index);
    mSkeletonRoot = model.getRef<NiNode>(index);

    std::uint32_t numBones;
    stream.read(numBones);
    mBones.resize(numBones);
    for (unsigned int i = 0; i < numBones; ++i)
    {
        stream.read(mBones.at(i));
    }
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
void NiBtOgre::NiSkinPartition::SkinPartition::read(NiStream& stream)
{
    stream.read(numVertices);
    stream.read(numTriangles);
    stream.read(numBones);
    stream.read(numStrips);
    stream.read(numWeightsPerVertex);

    bones.resize(numBones);
    for (unsigned int i = 0; i < numBones; ++i)
        stream.read(bones.at(i));

    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        hasVertexMap = stream.getBool();
    if (stream.nifVer() <= 0x0a000102 || (stream.nifVer() >= 0x0a010000 && hasVertexMap))
    {
        vertexMap.resize(numVertices);
        for (unsigned int i = 0; i < numVertices; ++i)
            stream.read(vertexMap.at(i));
    }

    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        hasVertexWeights = stream.getBool();
    if (stream.nifVer() <= 0x0a000102 || (stream.nifVer() >= 0x0a010000 && hasVertexWeights))
    {
        vertexWeights.resize(numVertices);
        for (unsigned int i = 0; i < numVertices; ++i)
        {
            vertexWeights[i].resize(numWeightsPerVertex);
            for (unsigned int j = 0; j < numWeightsPerVertex; ++j)
            {
                stream.read(vertexWeights.at(i).at(j));
            }
        }
    }

    stripLengths.resize(numStrips);
    for (unsigned int i = 0; i < numStrips; ++i)
        stream.read(stripLengths.at(i));

    if (stream.nifVer() >= 0x0a010000) // from 10.1.0.0
        hasFaces = stream.getBool();

    if (hasFaces && numStrips != 0)
    {
        if (stream.nifVer() <= 0x0a000102 || (stream.nifVer() >= 0x0a010000 && hasFaces))
        {
            strips.resize(numStrips);
            for (unsigned int i = 0; i < numStrips; ++i)
            {
                strips[i].resize(stripLengths[i]);
                for (unsigned int j = 0; j < stripLengths[i]; ++j)
                {
                    stream.read(strips.at(i).at(j));
                }
            }
        }
    }
    else // numStrips == 0
    {
        if (stream.nifVer() <= 0x0a000102 || (stream.nifVer() >= 0x0a010000 && hasFaces))
        {
            triangles.resize(numTriangles);
            for (unsigned int i = 0; i < numTriangles; ++i)
            {
                stream.read(triangles.at(i).v1);
                stream.read(triangles.at(i).v2);
                stream.read(triangles.at(i).v3);
            }
        }
    }

    hasBoneIndices = stream.getBool();
    if (hasBoneIndices)
    {
        boneIndices.resize(numVertices);
        for (unsigned int i = 0; i < numVertices; ++i)
        {
            boneIndices[i].resize(numWeightsPerVertex);
            for (unsigned int j = 0; j < numWeightsPerVertex; ++j)
            {
                stream.read(boneIndices.at(i).at(j));
            }
        }
    }

    if (stream.userVer() >= 12)
        stream.skip(sizeof(std::uint16_t));

    // FIXME: more unknowns here for version 10.2.0.0
}

NiBtOgre::NiSkinPartition::NiSkinPartition(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    stream.read(mNumSkinPartitionBlocks);
    mSkinPartitionBlocks.resize(mNumSkinPartitionBlocks);
    for (unsigned int i = 0; i < mNumSkinPartitionBlocks; ++i)
        mSkinPartitionBlocks[i].read(stream);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiStringPalette::NiStringPalette(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    stream.readSizedString(mPalette);
    stream.read(mLength); // TODO: validate against mPalette.size()
}

NiBtOgre::NiUVData::NiUVData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    mUVGroups0.read(stream);
    mUVGroups1.read(stream);
    mUVGroups2.read(stream);
    mUVGroups3.read(stream);
}

NiBtOgre::NiVisData::NiVisData(NiStream& stream, const NiModel& model)
    : NiObject(stream, model)
{
    std::uint32_t numKeys;
    stream.read(numKeys);

    mKeys.resize(numKeys);
    for (unsigned int i = 0; i < numKeys; ++i)
        mKeys[i].read(stream, 1); // LINEAR_KEY
}
