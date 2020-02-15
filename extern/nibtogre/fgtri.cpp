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
#include "fgtri.hpp"

#include <memory>
#include <stdexcept>
#include <iostream> // FIXME: debugging only


// "name" is the full path to the mesh from the resource directory/BSA added to Ogre::ResourceGroupManager.
// This name is required later for Ogre resource managers such as MeshManager.
// The file is opened by mFgStream::mStream.
//NiBtOgre::FgTri::FgTri(Ogre::ResourceManager *creator, const Ogre::String& name, Ogre::ResourceHandle handle,
//                           const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
//                           const Ogre::NameValuePairList* createParams/*bool showEditorMarkers*/)
//    : Resource(creator, name, handle, group, isManual, loader)
//    , mFgStream(name), mGroup(group), mName(name)
//{
//}

NiBtOgre::FgTri::FgTri(const Ogre::String& name, const Ogre::String& group)
    : mFgStream(name), mGroup(group), mName(name)
{
}

NiBtOgre::FgTri::~FgTri()
{
}

// only called if this resource is not being loaded from a ManualResourceLoader
void NiBtOgre::FgTri::loadImpl()
{
    mFgStream.read(mFileType); // FIXME: assert that it is "FRTRI003"
    mFgStream.read(mNumVertices);
    mFgStream.read(mNumTriangles);
    mFgStream.read(mNumQuads);
    mFgStream.read(mNumLabelledVertices);
    mFgStream.read(mNumLabelledSurfacePoints);
    mFgStream.read(mNumTextureCoordinates);
    mFgStream.read(mExtensionInfo);
    mFgStream.read(mNumLabelledDiffMorphs);
    mFgStream.read(mNumLabelledStatMorphs);
    mFgStream.read(mNumTotalStatMorphVertices);

    mFgStream.skip(16); // Reserved

    float x, y, z;
#if 0
    boost::scoped_array<float> mVertices(new float[3 * (mNumVertices + mNumTotalStatMorphVertices)]);
    for (std::size_t i = 0; i < (mNumVertices + mNumTotalStatMorphVertices); ++i)
    {
        mFgStream.read(mVertices[i+0]);
        mFgStream.read(mVertices[i+1]);
        mFgStream.read(mVertices[i+2]);
    }
#else
    mVertices.resize(mNumVertices + mNumTotalStatMorphVertices);
    for (std::size_t i = 0; i < (mNumVertices + mNumTotalStatMorphVertices); ++i)
    {
        mFgStream.read(x);
        mFgStream.read(y);
        mFgStream.read(z);
        mVertices[i] = Ogre::Vector3(x, y, z);
    }
#endif

    boost::scoped_array<std::int32_t> mTriangleIndicies(new std::int32_t[3 * mNumTriangles]);
    for (std::size_t i = 0; i < mNumTriangles; ++i)
    {
        mFgStream.read(mTriangleIndicies[i+0]);
        mFgStream.read(mTriangleIndicies[i+1]);
        mFgStream.read(mTriangleIndicies[i+2]);
    }

    boost::scoped_array<std::int32_t> mQuadIndicies(new std::int32_t[3 * mNumQuads]);
    for (std::size_t i = 0; i < mNumQuads; ++i)
    {
        mFgStream.read(mQuadIndicies[i+0]);
        mFgStream.read(mQuadIndicies[i+1]);
        mFgStream.read(mQuadIndicies[i+2]);
    }

    // ignore the rest of the file for now
}
