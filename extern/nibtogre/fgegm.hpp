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

*/
#ifndef NIBTOGRE_FGEGM_H
#define NIBTOGRE_FGEGM_H

#include <cstdint>
#include <string>

#include <boost/scoped_array.hpp>

//#include <OgreResource.h>
//#include <OgreDataStream.h>
//#include <OgreVector3.h>

//namespace Ogre
//{
//    class ResourceManager;
//    class ManualResourceLoader;
//}

namespace NiBtOgre
{
    class FgTri;

    class FgEgm// : public Ogre::Resource
    {
        //const std::string mGroup;  // Ogre group
        //const std::string mName;   // file name

        std::uint64_t mFileType;
        std::uint32_t mNumVertices;
        std::uint32_t mNumSymMorphModes;  // should be 50
        std::uint32_t mNumAsymMorphModes; // should be 30
        std::uint32_t mGeometryBasisVersion;

        //std::vector<float> mSymMorphModeScales;  // should be 50
        //std::vector<float> mAsymMorphModeScales; // should be 30
        //std::vector<Ogre::Vector3> mSymMorphModes;
        //std::vector<Ogre::Vector3> mAsymMorphModes;
        boost::scoped_array<float> mSymMorphModeScales;  // should be 50
        boost::scoped_array<float> mAsymMorphModeScales; // should be 30
        boost::scoped_array<std::int16_t> mSymMorphModes;  // 3 * mNumVertices * mNumSymMorphModes
        boost::scoped_array<std::int16_t> mAsymMorphModes; // 3 * mNumVertices * mNumAsymMorphModes

        // default, copy and assignment not allowed
        FgEgm();
        FgEgm(const FgEgm& other);
        FgEgm& operator=(const FgEgm& other);

    //protected:
        //void loadImpl();
        //void unloadImpl() {}

    public:
        // The parameter 'name' refers to those in the Ogre::ResourceGroupManager
        // e.g. full path from the directory/BSA added in Bsa::registerResources(), etc
        //
        // NOTE: the constructor may throw
//      FgEgm(Ogre::ResourceManager *creator, const Ogre::String& name, Ogre::ResourceHandle handle,
//              const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
//              const Ogre::NameValuePairList* createParams=nullptr);
        //FgEgm(const Ogre::String& name, const Ogre::String& group); // FIXME: for testing only
        ~FgEgm();

        FgEgm(const std::string& name, const FgTri& tri);

        //const std::string& getOgreGroup() const { return mGroup; }
        //const std::string& getName() const { return mName; }

        inline const std::uint32_t numVertices() const { return mNumVertices; }

        inline const boost::scoped_array<float>& symMorphModeScales() const { return mSymMorphModeScales; }
        inline const boost::scoped_array<float>& asymMorphModeScales() const { return mAsymMorphModeScales; }
        inline const boost::scoped_array<std::int16_t>& symMorphModes() const { return mSymMorphModes; }
        inline const boost::scoped_array<std::int16_t>& asymMorphModes() const { return mAsymMorphModes; }
        //inline const std::vector<Ogre::Vector3>& getSymMorph() const { return mSymMorphModes; }
        //inline const std::vector<Ogre::Vector3>& getAsymMorph() const { return mAsymMorphModes; }
    };
}

#endif // NIBTOGRE_FGEGM_H
