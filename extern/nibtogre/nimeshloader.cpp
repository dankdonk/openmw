/*
  Copyright (C) 2018 cc9cii

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
#include "nimeshloader.hpp"

#include <OgreMesh.h>
#include <OgreSubMesh.h>

#include "nigeometry.hpp"

namespace NiBtOgre
{
    // This method is not called until the associated entity is created
    void NiMeshLoader::loadResource(Ogre::Resource *resource)
    {
        Ogre::Mesh *mesh = static_cast<Ogre::Mesh*>(resource);

        // create and update (i.e. apply materials, properties and controllers)
        // an Ogre::SubMesh for each in mMeshGeometry
        for (size_t i = 0; i < mMeshGeometry.size(); ++i)
        {
            mMeshGeometry[i]->createSubMesh(mInstance, mesh);
        }

        // update bounds including all the sub meshes
        //mesh->_setBounds()
        //mesh->_setBoundingSphereRadius)
    }
}
