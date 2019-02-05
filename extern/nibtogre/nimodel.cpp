/*
  Copyright (C) 2015-2019 cc9cii

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
#include "nimodel.hpp"

#include <memory>
#include <stdexcept>
#include <iostream> // FIXME: debugging only

#include <OgreMeshManager.h>
#include <OgreMesh.h>
#include <OgreEntity.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreSkeletonManager.h>
#include <OgreSkeleton.h>
#include <OgreSkeletonInstance.h>
#include <OgreAnimationState.h>
#include <OgreBone.h>

#include "niobject.hpp"
#include "bhkrefobject.hpp" // bhkConstraint
#include "btogreinst.hpp"
#include "meshloader.hpp"
#include "skeletonloader.hpp"
#include "ninode.hpp"
#include "nisequence.hpp"

// "name" is the full path to the mesh from the resource directory/BSA added to Ogre::ResourceGroupManager.
// This name is required later for Ogre resource managers such as MeshManager.
// The file is opened by mNiStream::mStream.
//
// FIXME: there could be duplicates b/w TES3 and TES4/5
NiBtOgre::NiModel::NiModel(Ogre::ResourceManager *creator, const Ogre::String& name, Ogre::ResourceHandle handle,
                           const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
                           bool showEditorMarkers)
    : Resource(creator, name, handle, group, isManual, loader)
    , mNiStream(name), mHeader(mNiStream), mGroup(group), mModelName(name), mModelData(*this)
    , mShowEditorMarkers(showEditorMarkers)
{
    mModelData.mIsSkeleton = false; // FIXME: hack, does not belong here
}

NiBtOgre::NiModel::~NiModel()
{
    //unload(); // FIXME: caused exceptions in the destructor of AnimSource
}

// only called if this resource is not being loaded from a ManualResourceLoader
void NiBtOgre::NiModel::loadImpl()
{
    mObjects.resize(mHeader.numBlocks());
    if (mNiStream.nifVer() >= 0x0a000100) // from 10.0.1.0
    {
        for (std::uint32_t i = 0; i < mHeader.numBlocks(); ++i)
        {
            //std::cout << "Block " << mHeader.blockType(i) << std::endl; // FIXME: for testing only
            mCurrIndex = i; // FIXME: debugging only

            // From ver 10.0.1.0 (i.e. TES4) we already know the object types from the header.
            mObjects[i] = NiObject::create(mHeader.blockType(i), i, mNiStream, *this, mModelData);
        }
    }
    else
    {
        for (std::uint32_t i = 0; i < mHeader.numBlocks(); ++i)
        {
            mCurrIndex = i; // FIXME: debugging only
//#if 0
            std::string blockName = mNiStream.readString();
            //if (blockName == "RootCollisionNode")
                //std::cout << name << " : " << "RootCollisionNode" << std::endl;
            //if (blockName == "AvoidNode")
                //std::cout << name << " : " << "AvoidNode" << std::endl;
            //if (blockName == "NiStringExtraData")
                //std::cout << name << " : " << "NiStringExtraData" << std::endl;
            //std::cout << name << " : " << "BoundingBox" << std::endl;
            mObjects[i] = NiObject::create(blockName, i, mNiStream, *this, mModelData);
            //std::cout << "Block " << blockName << std::endl; // FIXME: for testing only
//#endif
            // For TES3, the object type string is read first to determine the type.
            //mObjects[i] = NiObject::create(mNiStream.readString(), mNiStream, *this);
        }
    }

    // TODO: should assert that the first object, i.e. mObjects[0], is either a NiNode (TES3/TES4)
    //       or BSFadeNode (TES5)

    // read the footer to check for root nodes
    std::uint32_t numRoots = 0;
    mNiStream.read(numRoots);

    mRoots.resize(numRoots);
    for (std::uint32_t i = 0; i < numRoots; ++i)
        mNiStream.read(mRoots.at(i));

    if (numRoots == 0)
        throw std::runtime_error(mModelName + " has no roots");
    else if (numRoots > 1) // FIXME: debugging only, to find out which NIF has multiple roots
        throw std::runtime_error(mModelName + " has too many roots");
        //std::cout << name << " has numRoots: " << numRoots << std::endl;

    // FIXME: testing only
    //if (mNiStream.nifVer() >= 0x0a000100)
        //std::cout << "roots " << mHeader.blockType(mRoots[0]) << std::endl;
    if (mModelData.mSkelLeafIndicies.size() > 1)
    {
        // find the bones, if any (i.e. prepare for the skeleton loader)
        // FIXME: mRoots[0] is just an assumption
        int32_t index;
        NiNode *node;
        for (unsigned int i = 0; i < mModelData.mSkelLeafIndicies.size(); ++i)
        {
            index = mModelData.mSkelLeafIndicies[i];
            node = getRef<NiNode>(index);
            node->findBones(mRoots[0]);

            mObjectPalette[node->getNiNodeName()] = index;
        }
    }
}

void NiBtOgre::NiModel::build(BtOgreInst *inst, Ogre::SkeletonPtr skeleton)
{
    // FIXME: model name may clash with TES3 model names? e.g. characters/_male/skeleton.nif
    // (maybe not if the full path is used?)
    if (!skeleton && mModelData.mSkelLeafIndicies.size() > 1)
    {
        mModelData.mSkeleton = Ogre::SkeletonManager::getSingleton().getByName(getModelName(), mGroup);
        if (!mModelData.mSkeleton)
        {
            SkeletonLoader loader(*this);
            mModelData.mSkeleton
                = Ogre::static_pointer_cast<Ogre::Skeleton>(Ogre::SkeletonManager::getSingleton().load(
                    getModelName(), // use the NIF name, since there is one skeleton per NIF
                    mGroup,         // Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
                    true,           // isManual
                    &loader));
        }



        // doesn't seem to change the vertical position
        //mModelData.mSkeleton->setBindingPose(); // FIXME: experimental
    }

    // for building body parts
    if (skeleton)
        mModelData.mSkeleton = skeleton;

    // build the first root
    mObjects[mRoots[0]]->build(inst, &mModelData); // FIXME: what to do with other roots?

    //if (mModelData.mBtShapeLoaders.size() > 1)
        //std::cout << "more than 1 rigid body " << getModelName() << std::endl;

    //buildMeshAndEntity(inst);
    //inst->instantiate();  // FIXME: probably doesn't belong here

    // FIXME: testing
//  std::map<std::int32_t, std::pair<std::string, std::unique_ptr<BtShapeLoader> > >::iterator iter;
//  for (iter = mModelData.mBtShapeLoaders.begin(); iter != mModelData.mBtShapeLoaders.end(); ++iter)
//  {
//      std::cout << iter->second.first << std::endl;
//  }

    // build any constraints that were deferred while building the rigid bodies
//  for (size_t i = 0; i < inst->mbhkConstraints.size(); ++i)
//  {
//      //inst->mbhkConstraints[i].first->linkBodies(inst, inst->mbhkConstraints[i].second);
//  }
}

// FIXME: maybe pass a parameter here indicating static mesh? (create a "static" group?)
// Or group should come from the classes, e.g. static, misc, furniture, etc
void NiBtOgre::NiModel::buildMeshAndEntity(BtOgreInst* inst)
{
    Ogre::MeshManager& meshManager = Ogre::MeshManager::getSingleton();

    // There are several types of NIFs that have more than one Mesh/Entity:
    //
    // 1. Skinned with a Skeleton
    //    e.g. Clutter\UpperClass\UpperClassDisplayCasePurple01.NIF
    //
    //    (NOTE: some skinned NIFs do not have more than one mesh/entity
    //     e.g. clothes\robemage\m\robemage_m.gnd)
    //
    // 2. Ragdoll
    //    These require an additional Ogre::SceneNode per Entity.
    //
    //    e.g. Architecture\Cathedral\Crypt\CathedralCryptLight02.NIF
    //    e.g. Clutter\FightersGuild\TargetHeavy01.NIF
    //
    // 3. More than one mesh/entity for no particular reason
    //    Must rememer to move all the related entities for any havok objects.
    //
    //    e.g. Weapons\Steel\Longsword.NIF
    //

    // iterate through the mesh build map
    //
    // NOTE: If the model/object is static, we only need one child scenenode from the
    // basenode.  Else we need one for each NiNode that has a mesh (and collision shape?).
    // FIXME: how to do this?
    std::map<NiNodeRef, NiNode*>::iterator iter;
    for (iter = mModelData.mMeshBuildList.begin(); iter != mModelData.mMeshBuildList.end(); ++iter)
    {
        MeshLoader loader = MeshLoader(iter->second);
        // The model name and parent node name are concatenated for use with Ogre::MeshManager
        // without triggering exeptions due to duplicates.
        // e.g. meshes\\architecture\\imperialcity\\icwalltower01.nif@ICWallTower01
        //
        // FIXME: probably should normalise the names to lowercase
        // FIXME: failsafe - check if mParent->getNodeName() returns blank, in which case use block number?
        // FIXME: consider the use of a hash (possibly the same as BSA) + block number for performance
        std::string meshName = getModelName() + "@" + iter->second->getNiNodeName();

        // TODO: probably room for optimising the use of the "group" parameter
        Ogre::MeshPtr mesh = meshManager.getByName(meshName, mGroup);
        if (!mesh)
            mesh = meshManager.createManual(meshName, mGroup, &loader);



        if (!mModelData.mSkeleton.isNull() && mesh->hasSkeleton() && mesh->getSkeleton() != mModelData.mSkeleton)
        {
            // need to redo bone asignments
            //mesh = meshManager.createManual(meshName, mGroup, &loader);
        }




        mesh->setAutoBuildEdgeLists(false);

        // we can use either the mesh's name or shared pointer
        Ogre::Entity *entity = inst->mBaseSceneNode->getCreator()->createEntity(/*iter->second.first*/mesh);

        // associate controllers to sub entities
        //
        // WARNING: Assumed that the sub entity order is the same as the order in which the
        //          sub-meshes are created in NiMeshLoader::loadResource

        entity->setVisible(true /*!(flags&Nif::NiNode::Flag_Hidden)*/); // FIXME not for newer NIF






        // FIXME: experimental
        // set mSkelBase for Animation::isSkinned()
        if (mesh->hasSkeleton() && (blockType(iter->second->index()) == "NiNode"))
        {
            std::string nodeName = iter->second->getNiNodeName();
            if (nodeName == "Scene Root")
            {
                inst->mObjectScene->mSkelBase = entity;
            }
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

        // update ObjectScene with all vertex animated entities
        if (entity->hasVertexAnimation())
            inst->mObjectScene->mVertexAnimEntities.push_back(entity);

        // update ObjectScene with the name of the skeletal animations and the associated entities
        if (entity->hasSkeleton() && entity->getSkeleton()->getNumAnimations() > 0)
        {
            // iterate through all the skeletal animations in the NIF
            std::map<std::string, std::vector<std::string> >::const_iterator it
                = mModelData.mMovingBoneNameMap.begin();
            for (; it != mModelData.mMovingBoneNameMap.end(); ++it)
            {
                // find all the bones for the animation (the same NiNode name is used as the Bone name)
                //
                // entity->getName() returns a unique, autogenerated name...
                // instead get the encoded NiNode name from the unique Mesh name
                //size_t pos = iter->second.first.find_last_of('@');
                //std::string meshName = iter->second.first.substr(pos+1);
                std::string meshName = entity->getMesh()->getName();
                size_t pos = meshName.find_last_of('@');
                meshName = meshName.substr(pos+1); // discard the NIF file name to get NiNode name
                for (unsigned int i = 0; i < it->second.size(); ++i)
                {
                    if (meshName == it->second[i]) // matches the Bone name, update ObjectScene
                    {
                        std::map<std::string, std::vector<Ogre::Entity*> >& skelMap
                            = inst->mObjectScene->mSkeletonAnimEntities;

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

        inst->mObjectScene->mEntities.push_back(entity);
#if 0
        const std::vector<NiTriBasedGeom*>& subMeshGeometry = iter->second.second->getSubMeshGeometry();
        for (unsigned int j = 0; j < subMeshGeometry.size(); ++j)
        {
            // FIXME: testing only
            //std::cout << "nameIndex " << subMeshGeometry[j]->getNameIndex() << std::endl;
            //std::cout << entity->getSubEntity(j)->getSubMesh()->getMaterialName() << std::endl;
        }
#endif
        // FIXME: do we need a map of entities for deleting later?
        //        for now get ObjectScene to do that
    }

    // FIXME: experimental
    // skeleton.nif doesn't have any NiTriBasedGeom so no entities would have been created
    if (mModelData.mIsSkeleton && (blockType(mRoots[0]) == "NiNode"))
    {
        NiNode *rootNode = getRef<NiNode>(mRoots[0]);
        std::string rootName = rootNode->getNiNodeName();
        if (rootName == "Scene Root")
        {
            std::string meshName = getModelName() + "@" + rootName;
            MeshLoader loader = MeshLoader(rootNode);

            Ogre::MeshPtr mesh = meshManager.getByName(meshName);
            if (!mesh)
                mesh = meshManager.createManual(meshName, mGroup, &loader);

            mesh->setSkeletonName(getModelName());

            Ogre::Entity *entity = inst->mBaseSceneNode->getCreator()->createEntity(mesh);
            inst->mObjectScene->mSkelBase = entity;
            inst->mObjectScene->mEntities.push_back(entity); // experimental for position

            // FIXME: experimental
            Ogre::SkeletonInstance *skelinst = inst->mObjectScene->mSkelBase->getSkeleton();
            Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
            while(boneiter.hasMoreElements())
                boneiter.getNext()->setManuallyControlled(true);
        }
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

// NOTE: 'model' should be updated each time a weapon (e.g. bow) is equipped or unequipped
//       since that will affect the object palette.  But the object palette in its current form
//       returns object index based on a text string name.  The indicies from another model
//       (e.g.  bow) won't be useful.  i.e. we need a different solution.
//       TODO: check if the object names can clash
void NiBtOgre::NiModel::buildAnimation(Ogre::Entity *skelBase, NiModelPtr anim,
        std::multimap<float, std::string>& textKeys,
        std::vector<Ogre::Controller<Ogre::Real> >& controllers,
        NiModel *skeleton, NiModel *bow)
{
    getRef<NiControllerSequence>(mRoots[0])->build(skelBase, anim, textKeys, controllers, *skeleton, skeleton->getObjectPalette());
}

void NiBtOgre::NiModel::buildSkeleton()
{
    if (mModelData.mSkelLeafIndicies.size() > 1)
    {
        mModelData.mSkeleton = Ogre::SkeletonManager::getSingleton().getByName(getModelName(), mGroup);
        if (!mModelData.mSkeleton)
        {
            SkeletonLoader loader(*this);
            mModelData.mSkeleton
                = Ogre::static_pointer_cast<Ogre::Skeleton>(Ogre::SkeletonManager::getSingleton().load(
                    getModelName(), // use the NIF name, since there is one skeleton per NIF
                    mGroup,         // Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
                    true,           // isManual
                    &loader));
        }
    }
}

void NiBtOgre::ModelData::setNiNodeParent(NiAVObjectRef child, NiNode *parent)
{
    //if (child == -1) // already checked in NiNode before calling this method
        //return;

    std::map<NiAVObjectRef, NiNode*>::iterator lb = mNiNodeMap.lower_bound(child);

    if (lb != mNiNodeMap.end() && !(mNiNodeMap.key_comp()(child, lb->first)))
    {
        if (lb->second != parent)
            throw std::logic_error("NiNode parent map: multiple parents");
        // else the same entry already there for some reason, ignore for now
    }
    else
        mNiNodeMap.insert(lb, std::make_pair(child, parent)); // None found, create one
}

/*const*/ NiBtOgre::NiNode& NiBtOgre::ModelData::getNiNodeParent(NiAVObjectRef child) const
{
    std::map<NiAVObjectRef, NiNode*>::const_iterator it = mNiNodeMap.find(child);
    if (it != mNiNodeMap.cend())
        return *it->second;
    else
    {
        //throw std::logic_error("NiNode parent map: parent not found");
        std::cerr << mModel.getModelName() << " : NiNode parent not found - " << child << std::endl;
        // FIXME: it turns out that some parents have higher index than children (i.e. occurs
        // later in the file) - to fix this properly quite a bit a change will be required
        // - although it might be possible to post process before mParent and transforms are
        // required
        // architecture\arena\arenaspectatorm01.nif : NiNode parent not found - 128
        // architecture\arena\arenaspectatorm01.nif : NiNode parent not found - 130
        // architecture\arena\arenaspectatorm01.nif : NiNode parent not found - 202
        // architecture\arena\arenaspectatorm01.nif : NiNode parent not found - 215
        // architecture\arena\arenaspectatorm01.nif : NiNode parent not found - 217
        // architecture\arena\arenaspectatorf01.nif : NiNode parent not found - 132
        // architecture\arena\arenaspectatorf01.nif : NiNode parent not found - 134
        // architecture\arena\arenaspectatorf01.nif : NiNode parent not found - 208
        // architecture\arena\arenaspectatorf01.nif : NiNode parent not found - 218
        // architecture\arena\arenaspectatorf01.nif : NiNode parent not found - 220
        return *mModel.getRef<NiNode>(0);
    }
}

void NiBtOgre::ModelData::addNewSkelLeafIndex(NiNodeRef leaf)
{
    if (std::find(mSkelLeafIndicies.begin(), mSkelLeafIndicies.end(), leaf) == mSkelLeafIndicies.end())
        mSkelLeafIndicies.push_back(leaf);
}

bool NiBtOgre::ModelData::hasBoneLeaf(NiNodeRef leaf) const
{
     return std::find(mSkelLeafIndicies.begin(), mSkelLeafIndicies.end(), leaf) != mSkelLeafIndicies.end();
}
