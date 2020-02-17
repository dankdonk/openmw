/*
  Copyright (C) 2019, 2020 cc9cii

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

  Based on the cryptic notes on https://facegen.com/dl/sdk/doc/manual/indepth.html
  and https://facegen.com/dl/sdk/doc/manual/fileformats.html.

*/
#include "fgegm.hpp"

//#include <memory>
#include <stdexcept>
#include <iostream> // FIXME: for debugging only

#include "fgstream.hpp"
#include "fgtri.hpp"

// "name" is the full path to the mesh from the resource directory/BSA added to Ogre::ResourceGroupManager.
// This name is required later for Ogre resource managers such as MeshManager.
// The file is opened by mFgStream::mStream.
//NiBtOgre::FgEgm::FgEgm(Ogre::ResourceManager *creator, const Ogre::String& name, Ogre::ResourceHandle handle,
//                           const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
//                           const Ogre::NameValuePairList* createParams/*bool showEditorMarkers*/)
//    : Resource(creator, name, handle, group, isManual, loader)
//    , mFgStream(name), mGroup(group), mName(name)
//{
//}

//NiBtOgre::FgEgm::FgEgm(const Ogre::String& name, const Ogre::String& group)
//    : mFgStream(name), mGroup(group), mName(name)
//{
//}

namespace NiBtOgre
{
#if 0
// only called if this resource is not being loaded from a ManualResourceLoader
void NiBtOgre::FgEgm::loadImpl()
{
    mFgStream.read(mFileType); // FIXME: assert that it is "FREGM002"
    mFgStream.read(mNumVertices);
    mFgStream.read(mNumSymMorphModes);  // FIXME: assert that it is 50
    mFgStream.read(mNumAsymMorphModes); // FIXME: assert that it is 30
    mFgStream.read(mGeometryBasisVersion);

    mFgStream.skip(40); // Reserved

    boost::scoped_array<float> mSymMorphModeScales(new float[50/*mNumSymMorphModes*/]);
    boost::scoped_array<float> mAsymMorphModeScales(new float[30/*mNumAsymMorphModes*/]);

    //    0  1      49 50 51      99 100 101    149 150 151   199 200   <- array index
    //
    //   V0 V0 .... V0 V0 V0 .... V0  V0 V0 .... V0  V1 V1 ... V1 V1    <- vertices
    //   \___________/ \___________/  \___________/  \__________/
    //       xMorph        yMorph         zMorph         xMorph
    //
    //    ^             ^              ^              ^            ^
    //    |             |              |              |            |
    //
    //    0             1              2              3            4    <- read sequence
    //
    std::size_t size = 3 * mNumVertices * 50/*mNumSymMorphModes*/;
    boost::scoped_array<std::int16_t> mSymMorphModes(new std::int16_t[size]);

    for (std::size_t i = 0; i < 50/*mNumSymMorphModes*/; ++i)
    {
        mFgStream.read(mSymMorphModeScales[i]);
        for (std::size_t j = 0; j < mNumVertices; ++j)
        {
            // i = 0, j = 0, morph = x: 0
            // i = 0, j = 0, morph = y: 50
            // i = 0, j = 0, morph = z: 100
            // i = 0, j = 1, morph = x: 150
            // i = 0, j = 1, morph = y: 200
            // i = 0, j = 1, morph = z: 250
            // i = 0, j = 2, morph = x: 300
            // i = 0, j = 2, morph = y: 350
            // i = 0, j = 2, morph = z: 400
            mFgStream.read(mSymMorphModes[i + (3*j+0) * 50/*mNumSymMorphModes*/]);
            mFgStream.read(mSymMorphModes[i + (3*j+1) * 50/*mNumSymMorphModes*/]);
            mFgStream.read(mSymMorphModes[i + (3*j+2) * 50/*mNumSymMorphModes*/]);
        }
    }

    size = 3 * mNumVertices * 30/*mNumAsymMorphModes*/;
    boost::scoped_array<std::int16_t> mAsymMorphModes(new std::int16_t[size]);

    for (std::size_t i = 0; i < 30/*mNumAsymMorphModes*/; ++i)
    {
        mFgStream.read(mAsymMorphModeScales[i]);
        for (std::size_t j = 0; j < mNumVertices; ++j)
        {
            mFgStream.read(mAsymMorphModes[i + (3*j+0) * 30/*mNumAsymMorphModes*/]);
            mFgStream.read(mAsymMorphModes[i + (3*j+1) * 30/*mNumAsymMorphModes*/]);
            mFgStream.read(mAsymMorphModes[i + (3*j+2) * 30/*mNumAsymMorphModes*/]);
        }
    }
}

// co-eff.at(i) / 1000 * scale.at(i) * xMorph.at(j) = x.at(i)
// co-eff.at(i) / 1000 * scale.at(i) * yMorph.at(j) = y.at(i)
// co-eff.at(i) / 1000 * scale.at(i) * zMorph.at(j) = z.at(i)
//     plus
// co-eff.at(i+1) / 1000 * scale.at(i+1) * xMorph.at(j) = x.at(i+1)
// co-eff.at(i+1) / 1000 * scale.at(i+1) * yMorph.at(j) = y.at(i+1)
// co-eff.at(i+1) / 1000 * scale.at(i+1) * zMorph.at(j) = z.at(i+1)
//     equals
// x.at(i) + x.at(i+1)
// y.at(i) + y.at(i+1)
// z.at(i) + z.at(i+1)
const std::int16_t *NiBtOgre::FgEgm::getSymMorph(std::size_t vertIndex) const
{
    return &mSymMorphModes[vertIndex * 3 * 50/*mNumSymMorphModes*/];
}

const std::int16_t *NiBtOgre::FgEgm::getAsymMorph(std::size_t vertIndex) const
{
    return &mAsymMorphModes[vertIndex * 3 * 30/*mNumAsymMorphModes*/];
}
#else
#if 0
// only called if this resource is not being loaded from a ManualResourceLoader
void NiBtOgre::FgEgm::loadImpl()
{
    mFgStream.read(mFileType); // FIXME: assert that it is "FREGM002"
    mFgStream.read(mNumVertices);
    mFgStream.read(mNumSymMorphModes);
    if (mNumSymMorphModes != 50)
        throw std::runtime_error("Number of Symmetric Morph Modes is not 50");
    mFgStream.read(mNumAsymMorphModes);
    if (mNumAsymMorphModes != 30)
        throw std::runtime_error("Number of Asymmetric Morph Modes is not 30");
    mFgStream.read(mGeometryBasisVersion);

    mFgStream.skip(40); // Reserved

    mSymMorphModeScales.resize(50/*mNumSymMorphModes*/);
    mAsymMorphModeScales.resize(30/*mNumAsymMorphModes*/);

    std::int16_t xMorph, yMorph, zMorph;
    size_t index;

    //    0  1      49 50 51      99 100 101    149 150 151   199 200   <- vector index
    //
    //   V0 V0 .... V0 V1 V1 .... V1  V2 V2 .... V2  V3 V3 ... V3 V4    <- vertices
    //   \___________/ \___________/  \___________/  \__________/
    //
    std::size_t size = mNumVertices * 50/*mNumSymMorphModes*/;
    mSymMorphModes.resize(size);

    for (std::size_t j = 0; j < 50/*mNumSymMorphModes*/; ++j)
    {
        mFgStream.read(mSymMorphModeScales.at(j));
        for (std::size_t i = 0; i < mNumVertices; ++i)
        {
            mFgStream.read(xMorph);
            mFgStream.read(yMorph);
            mFgStream.read(zMorph);

            index = j + i * 50/*mNumSymMorphModes*/;
            mSymMorphModes[index]
                = Ogre::Vector3(float(xMorph), float(yMorph), float(zMorph)) * mSymMorphModeScales.at(j);
        }
    }

    size = mNumVertices * 30/*mNumAsymMorphModes*/;  // WARN: size reused
    mAsymMorphModes.resize(size);

    for (std::size_t k = 0; k < 30/*mNumAsymMorphModes*/; ++k)
    {
        mFgStream.read(mAsymMorphModeScales.at(k));
        for (std::size_t j = 0; j < mNumVertices; ++j)
        {
            mFgStream.read(xMorph);
            mFgStream.read(yMorph);
            mFgStream.read(zMorph);

            index = k + j * 30/*mNumSymMorphModes*/;
            mAsymMorphModes[index]
                = Ogre::Vector3(float(xMorph), float(yMorph), float(zMorph)) * mAsymMorphModeScales.at(k);
        }
    }
}
#endif
#endif
    FgEgm::FgEgm(const std::string& name, const FgTri& tri)
    {
        FgStream egm(name);

        egm.read(mFileType); // FIXME: assert that it is "FREGM002"
        egm.read(mNumVertices);
        if (mNumVertices != (tri.numVertices() + tri.numMorphVertices()))
            throw std::runtime_error("EGM: Number of vertices does not match that of TRI");
        egm.read(mNumSymMorphModes);
        if (mNumSymMorphModes != 50)
            throw std::runtime_error("EGM: Number of Symmetric Morph Modes is not 50");
        egm.read(mNumAsymMorphModes);
        if (mNumAsymMorphModes != 30)
            throw std::runtime_error("EGM: Number of Asymmetric Morph Modes is not 30");
        egm.read(mGeometryBasisVersion);

        egm.skip(40); // Reserved

        std::size_t index;
        boost::scoped_array<float> symMorphModeScales(new float[50]);
        boost::scoped_array<std::int16_t> symMorphModes((new std::int16_t[3 * mNumVertices * 50]));
        for (std::size_t j = 0; j < 50; ++j)
        {
            egm.read(symMorphModeScales[j]);
            index = 3 * mNumVertices * j;
            for (std::size_t v = 0; v < 3 * mNumVertices; ++v)
                egm.read(symMorphModes[index + v]);
        }
        mSymMorphModeScales.swap(symMorphModeScales);
        mSymMorphModes.swap(symMorphModes);

        boost::scoped_array<float> asymMorphModeScales(new float[30]);
        boost::scoped_array<std::int16_t> asymMorphModes((new std::int16_t[3 * mNumVertices * 30]));
        for (std::size_t k = 0; k < 30; ++k)
        {
            egm.read(asymMorphModeScales[k]);
            index = 3 * mNumVertices * k; // WARN: index reused
            for (std::size_t v = 0; v < 3 * mNumVertices; ++v)
                egm.read(asymMorphModes[index + v]);
        }
        mAsymMorphModeScales.swap(asymMorphModeScales);
        mAsymMorphModes.swap(asymMorphModes);
    }

    FgEgm::~FgEgm()
    {
    }
}
