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

#include <stdexcept>
#include <iostream> // FIXME: for debugging only

#include "fgstream.hpp"

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

//NiBtOgre::FgTri::FgTri(const Ogre::String& name, const Ogre::String& group)
//    : mFgStream(name), mGroup(group), mName(name)
//{
//}

namespace NiBtOgre
{
    FgTri::FgTri(const std::string& name)
    {
        FgStream tri(name);

        tri.read(mFileType); // FIXME: assert that it is "FRTRI003"
        tri.read(mNumVertices);
        tri.read(mNumTriangles);
        tri.read(mNumQuads);
        tri.read(mNumLabelledVertices);
        tri.read(mNumLabelledSurfacePoints);
        tri.read(mNumTextureCoordinates);
        tri.read(mExtensionInfo);
        tri.read(mNumLabelledDiffMorphs);
        tri.read(mNumLabelledStatMorphs);
        tri.read(mNumTotalStatMorphVertices);

        tri.skip(16); // Reserved

        boost::scoped_array<float> vertices(new float[3 * (mNumVertices + mNumTotalStatMorphVertices)]);
        for (std::size_t i = 0; i < 3 * (mNumVertices + mNumTotalStatMorphVertices); ++i)
            tri.read(vertices[i]);

        mVertices.swap(vertices);

        boost::scoped_array<std::int32_t> triangleIndicies(new std::int32_t[3 * mNumTriangles]);
        for (std::size_t i = 0; i < 3 * mNumTriangles; ++i)
            tri.read(triangleIndicies[i]);

        mTriangleIndicies.swap(triangleIndicies);

        boost::scoped_array<std::int32_t> quadIndicies(new std::int32_t[3 * mNumQuads]);
        for (std::size_t i = 0; i < 3 * mNumQuads; ++i)
            tri.read(quadIndicies[i]);

        mQuadIndicies.swap(quadIndicies);

        // FIXME the rest of the file
    }

    FgTri::~FgTri()
    {
    }
}
