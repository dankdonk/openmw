/*
  Copyright (C) 2019 cc9cii

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
#ifndef NIBTOGRE_FGTRI_H
#define NIBTOGRE_FGTRI_H

#include <string>
#include <cstdint>
//#include <memory>

#include <boost/scoped_array.hpp>

#include <OgreResource.h>
#include <OgreVector3.h>

#include "fgstream.hpp"

namespace Ogre
{
    class ResourceManager;
    class ManualResourceLoader;
}

namespace NiBtOgre
{
    class FgTri //: public Ogre::Resource
    {
        FgStream mFgStream;

        const std::string mGroup;  // Ogre group

        std::string mName;

        std::uint64_t mFileType;
        std::uint32_t mNumVertices;
        std::uint32_t mNumTriangles;
        std::uint32_t mNumQuads;
        std::uint32_t mNumLabelledVertices;
        std::uint32_t mNumLabelledSurfacePoints;
        std::uint32_t mNumTextureCoordinates;
        std::uint32_t mExtensionInfo;
        std::uint32_t mNumLabelledDiffMorphs;
        std::uint32_t mNumLabelledStatMorphs;
        std::uint32_t mNumTotalStatMorphVertices;

        //boost::scoped_array<float> mVertices;
        std::vector<Ogre::Vector3> mVertices;
        boost::scoped_array<std::int32_t> mTriangleIndicies;
        boost::scoped_array<std::int32_t> mQuadIndicies;

        // default, copy and assignment not allowed
        FgTri();
        FgTri(const FgTri& other);
        FgTri& operator=(const FgTri& other);

    public:
    //protected:
        void loadImpl();
        void unloadImpl() {}

    public:
        // The parameter 'name' refers to those in the Ogre::ResourceGroupManager
        // e.g. full path from the directory/BSA added in Bsa::registerResources(), etc
        //
        // NOTE: the constructor may throw
//      FgTri(Ogre::ResourceManager *creator, const Ogre::String& name, Ogre::ResourceHandle handle,
//              const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
//              const Ogre::NameValuePairList* createParams=nullptr/*bool showEditorMarkers=false*/);
        FgTri(const Ogre::String& name, const Ogre::String& group); // FIXME
        ~FgTri();

        const std::string& getOgreGroup() const { return mGroup; }
        const std::string& getName() const { return mName; }

        //const float *getVertices() const { return &mVertices[0]; }
        inline const std::uint32_t getNumVertices() const { return mNumVertices; }
        inline const std::vector<Ogre::Vector3>& getVertices() const { return mVertices; }
    };
}

#endif // NIBTOGRE_FGTRI_H
