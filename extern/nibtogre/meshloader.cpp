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
#include "meshloader.hpp"

#include <OgreMesh.h>

#include "ninode.hpp"

std::map<std::string, NiBtOgre::MeshLoader> NiBtOgre::MeshLoader::sLoaders;

NiBtOgre::MeshLoader::MeshLoader(NiNode* node) : mNiNode(node)
{
}

// this method is not called until the associated Ogre::Entity is created
void NiBtOgre::MeshLoader::loadResource(Ogre::Resource *resource)
{
    Ogre::Mesh *mesh = static_cast<Ogre::Mesh*>(resource);

    mNiNode->buildMesh(mesh);
}
