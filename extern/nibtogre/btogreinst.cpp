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
#include "btogreinst.hpp"

#include <iostream> // FIXME: for debugging only

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreMesh.h>
#include <OgreSkeleton.h>
#include <OgreSkeletonInstance.h>
#include <OgreBone.h>

#include "nimodelmanager.hpp"
#include "nimodel.hpp"
#include "ninode.hpp"

namespace Ogre
{
    class SceneNode;
}

NiBtOgre::BtOgreInst::BtOgreInst(Ogre::SceneNode *baseNode, const std::string& name, const std::string& group)
    : mBaseSceneNode(baseNode), mFlags(0), mSkeletonRoot(nullptr)
{
    mModel = NiBtOgre::NiModelManager::getSingleton().getOrLoadByName(name, group);
}

// for manually created NiModel (e.g. morphed, skinned)
NiBtOgre::BtOgreInst::BtOgreInst(NiModelPtr model, Ogre::SceneNode *baseNode)
    : mBaseSceneNode(baseNode), mFlags(0), mSkeletonRoot(nullptr)
{
    mModel = model;
}

//NiBtOgre::BtOgreInst::BtOgreInst(Ogre::SceneNode *baseNode, const std::string& name, const std::string& group,
//        NiModelPtr skeleton)
//    : mBaseSceneNode(baseNode), mFlags(0), mSkeletonRoot(nullptr)
//{
//    mModel = NiBtOgre::NiModelManager::getSingleton().getOrLoadByName(name, group);
//}

NiBtOgre::BtOgreInst::~BtOgreInst()
{
    // FIXME: destroy others !!!

    for (unsigned int i = 0; i < mInterpolators.size(); ++i)
        delete mInterpolators[i];

    std::map<NiNodeRef, Ogre::Entity*>::iterator iter(mEntities.begin());
    for (; iter != mEntities.end(); ++iter)
    {
        if (iter->second)
        {
            //it->second->stopSharingSkeletonInstance(); // FIXME: is this needed, too?
            iter->second->detachFromParent();
            mBaseSceneNode->getCreator()->destroyEntity(iter->second);
            iter->second = 0;
        }
        mEntities.erase(iter);
    }

    mModel.reset();
}

// for building body part models using the supplied creature/character skeleton for skinning.
// FIXME: skinned objects don't attach to a target bone?
void NiBtOgre::BtOgreInst::instantiateBodyPart(Ogre::SceneNode *baseNode, Ogre::Entity *skelBase)
{
    // make a convenience copy
    mIsSkinned = mModel->buildData().mIsSkinned;

    buildEntities();

    std::map<int32_t, Ogre::Entity*>::const_iterator iter(mEntities.begin());
    for (; iter != mEntities.end(); ++iter)
    {
        if (mIsSkinned)
        {
            // FIXME: why do we need to check this?
            if (skelBase->getMesh()->getSkeleton() == iter->second->getMesh()->getSkeleton())
                iter->second->shareSkeletonInstanceWith(skelBase);
            else
                std::cout << "no anim " << skelBase->getMesh()->getName() << " different skeleton to "
                          << iter->second->getMesh()->getName() << std::endl;

            baseNode->attachObject(iter->second);
        }
        else
        {
            mTargetBone = mModel->targetBone(); // see if there is a target bone
            if (mTargetBone == "")
                mTargetBone = "Bip01 Head"; // FIXME: just a guess, likely wrong

            // TODO: do we need to from foreignnpcanimation detach later like below?
            //
            // std::map<int32_t, Ogre::Entity*>::const_iterator it
            //     = mObjectParts[type]->mForeignObj->mEntities.begin();
            // for (; it != mObjectParts[type]->mForeignObj->mEntities.end(); ++it)
            //     mSkelBase->detachObjectFromBone(it->second);
            //
            // mObjectParts[type]->mForeignObj.reset();
            // mObjectParts[type].reset();
            skelBase->attachObjectToBone(mTargetBone, iter->second);
        }
    }
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

    // make a convenience copy
    mIsSkinned = mModel->buildData().mIsSkinned;

    buildEntities();
    //copyControllers();

    // build any constraints that were deferred while building the rigid bodies
    //for (size_t i = 0; i < inst->mbhkConstraints.size(); ++i)
    {
        //inst->mbhkConstraints[i].first->linkBodies(inst->mRigidBodies, inst->mbhkConstraints[i].second);
    }
}

void NiBtOgre::BtOgreInst::buildEntities()
{
    //if (mModel.getModelName().find("marker") != std::string::npos)
        //return; // FIXME: testing oil puddle
    //if (mModel.getModelName().find("vgeardoor01") != std::string::npos)
        //std::cout << "door" << std::endl;

    const BuildData& buildData = mModel->buildData();

    const NiNode* skeletonRoot = mModel->skeletonRoot();
    const std::vector<std::pair<Ogre::MeshPtr, NiNode*> >& meshes = mModel->meshes();
    for (std::size_t i = 0; i < meshes.size(); ++i)
    {
        Ogre::Entity *entity = mBaseSceneNode->getCreator()->createEntity(meshes[i].first);

        // FIXME: don't want skeleton.nif to be visible?
        entity->setVisible(true /*!(flags&Nif::NiNode::Flag_Hidden)*/); // FIXME not for newer NIF


        // TODO: associate controllers to sub entities
        //
        // WARNING: Assumed that the sub entity order is the same as the order in which the
        //          sub-meshes are created in NiMeshLoader::loadResource


        // set mSkelBase for Animation::isSkinned()
        if (meshes[i].first->hasSkeleton() && (mModel->blockType(meshes[i].second->selfRef()) == "NiNode")) // FIXME
        {
            if (meshes[i].second == skeletonRoot)
                mSkeletonRoot = entity;
        }
        else if (meshes[i].first->hasSkeleton() && (mModel->blockType(meshes[i].second->selfRef()) == "BSFadeNode"))
        {
            if (meshes[i].second == skeletonRoot)
                mSkeletonRoot = entity;
        }



        // FIXME: move this block to animation?
        Ogre::AnimationStateSet *anims = entity->getAllAnimationStates();
        if (anims)
        {
            Ogre::AnimationStateIterator i = anims->getAnimationStateIterator();
            while (i.hasMoreElements())
            {
                Ogre::AnimationState *state = i.getNext();
                if (state->getAnimationName() == "Idle" || state->getAnimationName() == "SpecialIdle")
                    state->setEnabled(true);
                else
                    state->setEnabled(false);
                state->setLoop(true); // FIXME: enable looping for all while testing only
            }
        }

        // update all vertex animated entities
        if (entity->hasVertexAnimation())
            mVertexAnimEntities.push_back(entity);

        // update the name of the skeletal animations and the associated entities
        if (1)//entity->hasSkeleton() && entity->getSkeleton()->getNumAnimations() > 0)
        {
            // iterate through all the skeletal animations in the NIF
            std::map<std::string, std::vector<std::string> >::const_iterator it
                = buildData.mMovingBoneNameMap.begin();
            for (; it != buildData.mMovingBoneNameMap.end(); ++it)
            {
                // find all the bones for the animation (the same NiNode name is used as the Bone name)
                //
                // entity->getName() returns a unique, autogenerated name...
                // instead get the encoded NiNode name from the unique Mesh name
                //size_t pos = iter->second.first.find_last_of('@');
                //std::string meshName = iter->second.first.substr(pos+1);
                std::string meshName = entity->getMesh()->getName();
                //size_t pos = meshName.find_first_of('#');
                size_t pos = meshName.find_last_of('@');
                std::string boneName = meshName.substr(pos+1); // discard the NIF file name to get NiNode name
                for (unsigned int i = 0; i < it->second.size(); ++i)
                {
                    if (boneName == it->second[i]) // matches the Bone name, update BtOgreInst
                    {
                        std::map<std::string, std::vector<Ogre::Entity*> >& skelMap
                            = mSkeletonAnimEntities;

                        // does the anim entry exist in the map?
                        std::map<std::string, std::vector<Ogre::Entity*> >::iterator lb
                            = skelMap.lower_bound(it->first);

                        if (lb != skelMap.end() && !(skelMap.key_comp()(it->first, lb->first)))
                        {
                            lb->second.push_back(entity); // found anim, add entity to the list
                        }
                        else // none found, create an entry in the map
                        {
                            skelMap.insert(lb, std::make_pair(it->first, std::vector<Ogre::Entity*> { entity }));
                        }
                    }
                }
            }
        }

        mEntities[meshes[i].second->selfRef()] = entity;
    }

    // FIXME: experimental
    // skeleton.nif doesn't have any NiTriBasedGeom so no entities would have been created
    if (mSkeletonRoot/*buildData.isSkeletonTES4()*/
            &&
            (
             mModel->blockType(mModel->rootIndex()) == "NiNode"
             ||
             mModel->blockType(mModel->rootIndex()) == "BSFadeNode"
            )
       )//&& mModelName != "meshes\\morroblivion\\creatures\\wildlife\\kagouti\\skeleton.nif") // FIXME
    {
        // FIXME: experimental
        Ogre::SkeletonInstance *skelinst = mSkeletonRoot->getSkeleton();

        //if (!skelinst)
            //return; // FIXME: morroblivion\creatures\wildlife\kagouti\skeleton.nif

        Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
        while(boneiter.hasMoreElements())
            boneiter.getNext()->setManuallyControlled(true);

//#if 0
        // FIXME: just some testing
        if (buildData.isSkeletonTES4())
        {
            const std::map<std::string, NiAVObjectRef>& objPalette = mModel->getObjectPalette();
            boneiter = skelinst->getBoneIterator();
            while(boneiter.hasMoreElements())
            {
                std::string name = boneiter.getNext()->getName();
                if (objPalette.find(name) == objPalette.end())
                    std::cout << "missing bone name " << name << " in ObjectPalette" << std::endl;
            }
        }
//#endif
    }

    // FIXME: maybe just have pointers to the vectors rather than copying all the content?
    for (size_t i = 0; i < buildData.mFlameNodes.size(); ++i)
    {
        mFlameNodes.push_back(buildData.mFlameNodes[i]->getNiNodeName());
        //std::cout << "flame " << getModelName() << " " << inst->mFlameNodes.back() << std::endl;
    }

    for (size_t i = 0; i < buildData.mAttachLights.size(); ++i)
    {
        mAttachLights.push_back(buildData.mAttachLights[i]->getNiNodeName());
        //std::cout << "light " << getModelName() << " " << inst->mAttachLights.back() << std::endl;
    }

#if 0
    std::map<std::uint32_t, EntityConstructionInfo>::iterator iter = mEntityCIMap.begin();

    // one Ogre::Mesh for each NiNode whose children NiGeometry are sub meshes
    for (; iter != mEntityCIMap.end(); ++iter)
    {
        // iter-second.first = model name + "@" + parent NiNode block name
        // iter->second.second = unique_ptr to NiMeshLoader
        //
        // FIXME: work around duplicates for now, should solve it by caching NiModel instead
        //        MeshManager needs a unique string, so probably need to supply the full
        //        name starting from "\\mesh"
        // FIXME: probably room for optimising the use of the "group" parameter
        Ogre::MeshPtr mesh = meshManager.getByName(iter->second.mMeshAndNodeName, "General");
        if (!mesh)
        {
            mesh = meshManager.createManual(iter->second.mMeshAndNodeName, "General",
                                            iter->second.mMeshLoader.get());
        }
        mesh->setAutoBuildEdgeLists(false);

        // either use the mesh's name or shared pointer
        Ogre::Entity *entity = mBaseSceneNode->getCreator()->createEntity(/*iter->second.first*/mesh);





        // associate controllers to sub entities
        //
        // WARNING: Assumed that the sub entity order is the same as the order in which the
        //          sub-meshes are created in NiMeshLoader::loadResource
        unsigned int numSubEntities = entity->getNumSubEntities();

        for (unsigned int j = 0; j < numSubEntities; ++j)
        {
            // FIXME: testing only
            std::cout << entity->getSubEntity(j)->getSubMesh()->getMaterialName() << std::endl;
        }

        // FIXME: do we need a map of entities for deleting later?
    }
#endif
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
