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
#include "btogreinst.hpp"

//#include <OgreMeshManager.h>
#//include <OgreMesh.h>
//#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#//include <OgreEntity.h>
//#include <OgreSubEntity.h> // FIXME: testing only
//#include <OgreSubMesh.h> // FIXME: testing only

#include "nigeometry.hpp"

NiBtOgre::BtOgreInst::BtOgreInst(Ogre::SceneNode *baseNode, NifOgre::ObjectScenePtr scene)
    : mBaseSceneNode(baseNode), mObjectScene(scene), mFlags(0)//, mSkeletonBuilt(false)
{
};


void NiBtOgre::BtOgreInst::instantiate()
{
    // FIXME: howto transform nodes to root transform after the meshes have been built?
    // e.g. NiNode CathedralCryptChain11 (18) has 1 mesh CathedralCryptChain11:36 (24)
    //      but it is a child to NiNode CathedralCryptChain (14) which as its own
    //      transform.
    //
    // Is it possible to leave that for Ogre::SceneNode to take care of?
    // i.e. for each NiNode with a mesh create a child scenenode

    mModel->buildMeshAndEntity(this);
}
