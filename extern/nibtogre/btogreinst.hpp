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
#ifndef NIBTOGRE_BTOGREINST_H
#define NIBTOGRE_BTOGREINST_H

#include <vector>
#include <map>
#include <cstdint>
#include <memory>

#include <btBulletDynamicsCommon.h>

#include <OgreController.h>

//#include <components/nifogre/ogrenifloader.hpp> // ObjectScenePtr

#include "nimodel.hpp"
#include "nidata.hpp"

namespace Ogre
{
    class SceneNode;
}

namespace OEngine
{
    namespace Physic
    {
        class RigidBody;
    }
}

namespace NiBtOgre
{
    struct bhkEntity;
    struct bhkConstraint;
    struct NiGeometry;

    struct BtOgreInst
    {
        // A NiNode with NiGeometry children may be an Ogre::Entity.
#if 0
        struct EntityConstructionInfo
        {
            // A unique name for the mesh.  Uniqueness is needed by the Ogre 1.x MeshManager,
            // possibly Ogre 2.x as well. Concatenation of model name, "@", parent NiNode name.
            // (e.g. meshes\\architecture\\imperialcity\\icwalltower01.nif:ICWallTower01)
            std::string                 mMeshAndNodeName;

            std::auto_ptr<NiMeshLoader> mMeshLoader; // FIXME: was unique_ptr but can't remember why

            // Each sub-entity may have associated controllers.
            //
            // The key to the map is the index of the sub-mesh that has NiProperty with NiTimeControllers
            // (note that there may be multiple controllers, hence a vector).
            std::map<std::uint32_t, std::vector<Ogre::Controller<float> > > mSubEntityControllers;
        };
#endif
        // Keep the model around
        NiModelPtr mModel;

        Ogre::SceneNode *mBaseSceneNode;
        int mFlags; // some global properties

        std::string mTargetBone;
        bool mIsSkinned;

        inline bool havokEnabled() const { return mModel->buildData().havokEnabled(); }

        //std::vector<std::pair<bhkConstraint*, bhkEntity*> > mbhkConstraints;

        // The key to the map is the block index of the parent NiNode; each child may add a mesh loader.
        // The first of the construction info is 'name' parameter in registerNiTriBasedGeom (see below).
        //std::map<std::uint32_t, EntityConstructionInfo> mEntityCIMap;

        std::vector<AnimTrackInterpolator<float>*> mInterpolators; // prevent memory leak

        // key is the block index of the target object (i.e. typically NiNode)
        std::map<std::uint32_t, std::shared_ptr<OEngine::Physic::RigidBody> > mRigidBodies;

        // btCollisionShapes
        // btConstraints

        Ogre::Entity *mSkeletonRoot; // assume only one
        std::map<NiNodeRef, Ogre::Entity*> mEntities;
        std::vector<Ogre::Entity*> mVertexAnimEntities;
        std::map<std::string, std::vector<Ogre::Entity*> > mSkeletonAnimEntities;

        //std::multimap<float, std::string> mTextKeys;
        const std::multimap<float, std::string>& modelTextKeys() const { return mModel->textKeys(); }
        //std::vector<Ogre::Controller<Ogre::Real> > mControllers;
        const std::vector<Ogre::Controller<Ogre::Real> >& modelControllers() const { return mModel->controllers(); }

        std::vector<std::string> mFlameNodes;
        std::vector<std::string> mAttachLights;

        BtOgreInst(Ogre::SceneNode *baseNode, const std::string& name, const std::string& group);
        BtOgreInst(NiModelPtr model, Ogre::SceneNode *baseNode);
        ~BtOgreInst();

        // register with bullet dynamics, make entities visible, etc
        void instantiate();

        // for building body parts using the supplied skeleton
        void instantiateBodyPart(Ogre::SceneNode* baseNode, Ogre::Entity* skelBase);

        bool hasAnimation(const std::string& animName) const;

    private:
        void buildEntities();
    };
}

#endif // NIBTOGRE_BTOGREINST_H
