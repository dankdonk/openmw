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

  Some of the Ogre code in this file is based on v0.36 of OpenMW.
*/
#include "meshloader.hpp"

#include <OgreMesh.h>
#include <OgreSubMesh.h>

#include "nigeometry.hpp"
#include "nidata.hpp"
#include "nimodel.hpp"
#include "boundsfinder.hpp"

NiBtOgre::MeshLoader::MeshLoader(const NiModel& model) : mModel(model)
{
}

/*std::uint32_t*/void NiBtOgre::MeshLoader::registerSubMeshGeometry(NiTriBasedGeom* geometry)
{
    mSubMeshGeometry.push_back(geometry);

    //return (std::uint32_t)mSubMeshGeometry.size() - 1;
}

// This method is not called until the associated Ogre::Entity is created in
// BtOgreInst::buildMeshAndEntity()
void NiBtOgre::MeshLoader::loadResource(Ogre::Resource *resource)
{
    Ogre::Mesh *mesh = static_cast<Ogre::Mesh*>(resource);

    BoundsFinder bounds;
    bool needTangents = false;

    // create and update (i.e. apply materials, properties and controllers)
    // an Ogre::SubMesh for each in mSubMeshGeometry
    for (size_t i = 0; i < mSubMeshGeometry.size(); ++i)
    {
        needTangents |= mSubMeshGeometry.at(i)->createSubMesh(mesh, bounds);
    }

    // build tangents if at least one of the sub-mesh's material needs them
    // TODO: is it possible to use the ones in the NIF files?
    if (needTangents)
    {
        unsigned short src,dest;
        if (!mesh->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src, dest))
            mesh->buildTangentVectors(Ogre::VES_TANGENT, src, dest);
    }

    mesh->_setBounds(Ogre::AxisAlignedBox(bounds.minX()-0.5f, bounds.minY()-0.5f, bounds.minZ()-0.5f,
                                          bounds.maxX()+0.5f, bounds.maxY()+0.5f, bounds.maxZ()+0.5f));
    mesh->_setBoundingSphereRadius(bounds.getRadius());

    // FIXME: exception
    // mMaterialName = "meshes\\oblivion\\gate\\oblivionarchgate01.nif@Meteor01:0"
    // cow "daperyiterealm" 16 15
    // FIXME: how to tell which Mesh needs the skeleton?
    //if (!mesh->hasSkeleton() && mModel.hasSkeleton())
        //mesh->setSkeletonName(mModel.getModelName());
}
