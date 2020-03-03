/*
  Copyright (C) 2018-2020 cc9cii

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
#ifndef NIBTOGRE_NIMESHLOADER_H
#define NIBTOGRE_NIMESHLOADER_H

#include <map>
#include <cstdint>
//#include <string>

#include <OgreSingleton.h>
#include <OgreResource.h>
#include <OgreSharedPtr.h>

namespace Ogre
{
    class Mesh;
}

namespace NiBtOgre
{
    class NiModel;

    class NiMeshLoader : public Ogre::ManualResourceLoader
    {
        enum ModelBuildType
        {
            MBT_Object,
            MBT_Skinned,
            MBT_Morphed
        };

        struct ModelBuildInfo
        {
            ModelBuildType type;
            const NiModel *model;
            Ogre::String skeleton;
            Ogre::ResourcePtr modelPtr; // only for morphed mesh
            std::int32_t ninode;  // NiNodeRef
        };

        static std::map<Ogre::Resource*, ModelBuildInfo> sModelBuildInfoMap;

        Ogre::MeshPtr createManual(const Ogre::String& name, const Ogre::String& group);

        void loadManualMesh(Ogre::Mesh* pMesh, const ModelBuildInfo& params);
        void loadManualSkinnedMesh(Ogre::Mesh* pMesh, const ModelBuildInfo& params);
        void loadManualMorphedMesh(Ogre::Mesh* pMesh, const ModelBuildInfo& params);

    public:

        NiMeshLoader();
        ~NiMeshLoader();

        // creates normal (static?) meshes as well as skinned meshes
        Ogre::MeshPtr createMesh(const Ogre::String& name, const Ogre::String& group,
                NiModel *model, std::int32_t ninode, const Ogre::String skeleton = "");

        Ogre::MeshPtr createMorphedMesh(const Ogre::String& name, const Ogre::String& group,
                const Ogre::String& morphedTexture, Ogre::ResourcePtr model, std::int32_t ninode);

        // reimplement Ogre::ManualResourceLoader
        virtual void loadResource(Ogre::Resource *resource);
    };
}

#endif // NIBTOGRE_NIMESHLOADER_H
