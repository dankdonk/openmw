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
#ifndef NIBTOGRE_BTOGREINST_H
#define NIBTOGRE_BTOGREINST_H

#include <vector>
#include <map>
#include <cstdint>
#include <memory>

#include <btBulletDynamicsCommon.h>

#include "nimodel.hpp"
#include "nimeshloader.hpp"

namespace Ogre
{
    class SceneNode;
}

namespace NiBtOgre
{
    struct bhkEntity;
    struct bhkConstraint;

    struct BtOgreInst
    {
        // keep it around in case Ogre wants to load the resource (i.e. Mesh) again
        std::auto_ptr<NiBtOgre::NiModel> mModel;

        int mFlags; // some global properties
        Ogre::SceneNode *mBaseNode;
        std::vector<std::pair<bhkConstraint*, bhkEntity*> > mbhkConstraints;

        // key is the block index of the parent NiNode
        std::map<std::uint32_t, std::pair<std::string, std::unique_ptr<NiMeshLoader> > > mMeshes;

        // parent NiNode's block index and name are supplied
        void addMeshGeometry(std::uint32_t index, const std::string& name, NiGeometry* geometry);

        void buildMeshAndEntity();

        // key is the block index of the target object (i.e. typically NiNode)
        std::map<std::uint32_t, std::unique_ptr<btRigidBody> > mRigidBodies;

        // btCollisionShapes
        // btConstraints

        BtOgreInst(Ogre::SceneNode *baseNode);
    };
}

#endif // NIBTOGRE_BTOGREINST_H
