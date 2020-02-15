/*
  Copyright (C) 2018, 2019 cc9cii

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

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

#include "nimodelmanager.hpp"
#include "nimodel.hpp"

namespace Ogre
{
    class SceneNode;
}

NiBtOgre::BtOgreInst::BtOgreInst(Ogre::SceneNode *baseNode, const std::string& name, const std::string& group)
    : mBaseSceneNode(baseNode), mFlags(0), mSkeletonRoot(nullptr)
{
    mModel = NiBtOgre::NiModelManager::getSingleton().getOrLoadByName(name, group);
};

// for building body part models using the supplied creature/character skeleton for skinning.
void NiBtOgre::BtOgreInst::instantiate(Ogre::SkeletonPtr skeleton, const std::string& meshExt)
{
    mModel->buildBodyPart(this, skeleton);
    mModel->buildMeshAndEntity(this, meshExt);
}

// for building fg morphed mesh
void NiBtOgre::BtOgreInst::instantiate(Ogre::SkeletonPtr skeleton, const std::string& npcName, std::vector<Ogre::Vector3>& vertices)
{
    mModel->buildBodyPart(this, skeleton);
    mModel->buildMeshAndEntity(this, npcName, vertices);
}

void NiBtOgre::BtOgreInst::instantiate()
{
    // FIXME: howto transform nodes to root transform after the meshes have been built?
    // e.g. NiNode CathedralCryptChain11 (18) has 1 mesh CathedralCryptChain11:36 (24)
    //      but it is a child to NiNode CathedralCryptChain (14) which has its own
    //      transform.
    //
    // Is it possible to leave that for Ogre::SceneNode to take care of?
    // i.e. for each NiNode with a mesh create a child scenenode

    mModel->build(this);
    mModel->buildMeshAndEntity(this);
}

bool NiBtOgre::BtOgreInst::hasAnimation(const std::string& animName) const
{
    if (mSkeletonAnimEntities.size() > 0) // "fake skin" node animation
    {
        return mSkeletonAnimEntities.find(animName) != mSkeletonAnimEntities.end();
    }
    else                                  // controller based node animation
    {
        return false;
    }
}
