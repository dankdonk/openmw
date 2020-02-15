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
                           const Ogre::NameValuePairList* createParams/*bool showEditorMarkers*/)
    : Resource(creator, name, handle, group, isManual, loader)
    , mNiStream(name), mHeader(mNiStream), mGroup(group), mModelName(name), mBuildData(*this)
    , mShowEditorMarkers(false/*showEditorMarkers*/)
{
    //mBuildData.mIsSkeleton = false; // FIXME: hack, does not belong here
    mBuildData.mSkeleton.setNull();
}

NiBtOgre::NiModel::~NiModel()
{
    //unload(); // FIXME: caused exceptions in the destructor of AnimSource
}

// only called if this resource is not being loaded from a ManualResourceLoader
void NiBtOgre::NiModel::loadImpl()
{
    //if (mModelName.find("smeltermarker") != std::string::npos)/*fxambwatersalmon02b*/
        //std::cout << mModelName << std::endl;
    //if (getModelName().find("vgeardoor01") != std::string::npos)
        //std::cout << "door" << std::endl;

    mObjects.resize(mHeader.numBlocks());
    if (mNiStream.nifVer() >= 0x0a000100) // from 10.0.1.0
    {
        for (std::uint32_t i = 0; i < mHeader.numBlocks(); ++i)
        {
            //if (blockType(0) == "NiTriShape")
                //return; // FIXME: morroblivion\environment\bittercoast\bcscum03.nif

            //std::cout << "Block " << mHeader.blockType(i) << std::endl; // FIXME: for testing only
            mCurrIndex = i; // FIXME: debugging only

            // From ver 10.0.1.0 (i.e. TES4) we already know the object types from the header.
            mObjects[i] = NiObject::create(mHeader.blockType(i), i, mNiStream, *this, mBuildData);
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
            mObjects[i] = NiObject::create(blockName, i, mNiStream, *this, mBuildData);
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
        std::cout << mModelName << " has numRoots: " << numRoots << std::endl;
        // Creatures\Bear\forward.kf had 3 roots
        //throw std::runtime_error(mModelName + " has too many roots");

    // find the bones, if any (i.e. prepare for the skeleton loader)
    // NOTE: ideally we can defer this step of finding the potential bones (potential because
    //       we may decide not to create a skeleton) but we need to populate mObjectPalette for
    //       buildAnimation()
    if (mBuildData.mSkelLeafIndicies.size() > 1)
    {
        int32_t index;
        NiNode *node;
        for (unsigned int i = 0; i < mBuildData.mSkelLeafIndicies.size(); ++i)
        {
            index = mBuildData.mSkelLeafIndicies[i];
            //if (blockType(index) != "NiNode")
                //continue; // FIXME: morroblivion\flora\bushes\corkbulb01anim.nif, index 0x20

            node = getRef<NiNode>(index);
            node->findBones(mRoots[0]); // FIXME: assumed only one root and skeleton root is the same

            mObjectPalette[node->getNiNodeName()] = index;
        }
    }
}

// for building body parts
void NiBtOgre::NiModel::buildBodyPart(BtOgreInst *inst, Ogre::SkeletonPtr skeleton)
{
    // for building body parts use the supplied skeleton
    mBuildData.mSkeleton = skeleton;
    // ultimately calls NiNode::build (always?)
    mObjects[mRoots[0]]->build(inst, &mBuildData); // FIXME: what to do with other roots?

    if (blockType(mObjects[1]->selfRef()) == "NiStringExtraData")
        inst->mTargetBone = indexToString(static_cast<NiStringExtraData*>(mObjects[1].get())->mStringData);
    else
        inst->mTargetBone = "";

    // make a convenience copy
    inst->mIsSkinned = mBuildData.mIsSkinned;
}

// build the skeleton and node controllers
void NiBtOgre::NiModel::build(BtOgreInst *inst)
{
    if (blockType(0) == "NiTriShape")
        return; // FIXME: morroblivion\environment\bittercoast\bcscum03.nif

    // build skeleton
    if (mBuildData.mSkeleton.isNull() && mBuildData.mSkelLeafIndicies.size() > 1)
    {
        mBuildData.mSkeleton = Ogre::SkeletonManager::getSingleton().getByName(getModelName(), mGroup);
        if (!mBuildData.mSkeleton)
        {
            SkeletonLoader loader(*this);
            mBuildData.mSkeleton
                = Ogre::static_pointer_cast<Ogre::Skeleton>(Ogre::SkeletonManager::getSingleton().load(
                    getModelName(), // use the NIF name, since there is one skeleton per NIF
                    mGroup,         // Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
                    true,           // isManual
                    &loader));
        }
    }

    // build the first root
    mObjects[mRoots[0]]->build(inst, &mBuildData); // FIXME: what to do with other roots?

    //if (mBuildData.mBtShapeLoaders.size() > 1)
        //std::cout << "more than 1 rigid body " << getModelName() << std::endl;

    //buildMeshAndEntity(inst);
    //inst->instantiate();  // FIXME: probably doesn't belong here

    // FIXME: testing
//  std::map<std::int32_t, std::pair<std::string, std::unique_ptr<BtShapeLoader> > >::iterator iter;
//  for (iter = mBuildData.mBtShapeLoaders.begin(); iter != mBuildData.mBtShapeLoaders.end(); ++iter)
//  {
//      std::cout << iter->second.first << std::endl;
//  }

    // build any constraints that were deferred while building the rigid bodies
//  for (size_t i = 0; i < inst->mbhkConstraints.size(); ++i)
//  {
//      //inst->mbhkConstraints[i].first->linkBodies(inst, inst->mbhkConstraints[i].second);
//  }
}

void NiBtOgre::NiModel::buildMeshAndEntity(BtOgreInst* inst, const std::string& npcName, std::vector<Ogre::Vector3>& vertices)
{
    // if a pre-morphed vertices are supplied there should be just one in mMeshBuildList (prob. "Scene Root")
    // e.g. headhuman.nif
    if (mBuildData.mMeshBuildList.size() > 1)
        std::cout << "unexpected mesh " << getModelName() << std::endl; // FIXME: should throw

    mBuildData.mMeshBuildList.begin()->second->setVertices(vertices);
    buildMeshAndEntity(inst, npcName);
}

// FIXME: maybe pass a parameter here indicating static mesh? (create a "static" group?)
// Or group should come from the classes, e.g. static, misc, furniture, etc
void NiBtOgre::NiModel::buildMeshAndEntity(BtOgreInst* inst, const std::string& meshExt)
{
    //if (getModelName().find("marker") != std::string::npos)
        //return; // FIXME: testing oil puddle
    //if (getModelName().find("vgeardoor01") != std::string::npos)
        //std::cout << "door" << std::endl;


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
    for (iter = mBuildData.mMeshBuildList.begin(); iter != mBuildData.mMeshBuildList.end(); ++iter)
    {
        // FIXME: need to make this persist somehow
        std::unique_ptr<MeshLoader> loader(new MeshLoader(iter->second));
        mMeshLoaders.push_back(std::move(loader));

        // The model name and parent node name are concatenated for use with Ogre::MeshManager
        // without triggering exeptions due to duplicates.
        // e.g. meshes\\architecture\\imperialcity\\icwalltower01.nif@ICWallTower01
        //
        // FIXME: TES5 Architecture\Solitude\SolitudeBase.nif has several NiNodes with the same
        // Name.
        //
        // FIXME: probably should normalise the names to lowercase
        // FIXME: failsafe - check if mParent->getNodeName() returns blank, in which case use block number?
        // FIXME: consider the use of a hash (possibly the same as BSA) + block number for performance
        std::string meshName = getModelName() + meshExt +
                               "#" + std::to_string(iter->second->selfRef()) + // node index
                               "@" + iter->second->getNiNodeName();            // node name

        // TODO: probably room for optimising the use of the "group" parameter
        Ogre::MeshPtr mesh = meshManager.getByName(meshName, mGroup);
        if (!mesh)
            mesh = meshManager.createManual(meshName, mGroup, mMeshLoaders.back().get());



        // FIXME: testing VGearDoor01.NIF
        if (0)//!mBuildData.mSkeleton.isNull() && !mesh->hasSkeleton() && getModelName().find("geardoor") != std::string::npos)
        {
            mesh->setSkeletonName(getModelName());
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
        if (mesh->hasSkeleton() && (blockType(iter->second->selfRef()) == "NiNode")) // FIXME
        {
            std::string nodeName = iter->second->getNiNodeName();
            if (iter->second->selfRef() == 0)
            //if (nodeName == "Scene Root")
            {
                inst->mSkeletonRoot = entity;
            }
        }
        else if (mesh->hasSkeleton() && (blockType(iter->second->selfRef()) == "BSFadeNode"))
        {
            inst->mSkeletonRoot = entity;
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

        // update BtOgreInst with all vertex animated entities
        if (entity->hasVertexAnimation())
            inst->mVertexAnimEntities.push_back(entity);

        // update BtOgreInst with the name of the skeletal animations and the associated entities
        if (1)//entity->hasSkeleton() && entity->getSkeleton()->getNumAnimations() > 0)
        {
            // iterate through all the skeletal animations in the NIF
            std::map<std::string, std::vector<std::string> >::const_iterator it
                = mBuildData.mMovingBoneNameMap.begin();
            for (; it != mBuildData.mMovingBoneNameMap.end(); ++it)
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
                            = inst->mSkeletonAnimEntities;

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

        inst->mEntities[iter->first] = entity;
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
        //        for now get BtOgreInst to do that
    } // create mesh


    // should have set mSkelBase by now - else there mustn't have been an entity at the root node
    // FIXME: for ghost skeleton.nif where there is no entity at rootnode
    if (!inst->mSkeletonRoot &&
        inst->mEntities.size() != 0 &&
        (inst->mEntities.begin()->second->getMesh()->hasSkeleton()


        || getModelName().find("geardoor") != std::string::npos) // FIXME: hack testing

       )

    {
        NiNode *rootNode = getRef<NiNode>(mRoots[0]);
        std::string rootName = rootNode->getNiNodeName();
        if (rootName == "Scene Root" || blockType(0) == "BSFadeNode") // FIXME: HACK not always "Scene Root"
        {
            std::string meshName = getModelName() + "@" + rootName;
            MeshLoader loader = MeshLoader(rootNode);

            Ogre::MeshPtr mesh = meshManager.getByName(meshName);
            if (!mesh)
                mesh = meshManager.createManual(meshName, mGroup, &loader);

            mesh->setSkeletonName(getModelName());

            Ogre::Entity *entity = inst->mBaseSceneNode->getCreator()->createEntity(mesh);
            inst->mSkeletonRoot = entity;
            inst->mEntities[0] = entity; // experimental for position
#if 0
            // FIXME: experimental
            Ogre::SkeletonInstance *skelinst = inst->mObjectScene->mSkelBase->getSkeleton();
            Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
            while(boneiter.hasMoreElements())
                boneiter.getNext()->setManuallyControlled(true);
#endif
        }
    }




    // FIXME: experimental
    // skeleton.nif doesn't have any NiTriBasedGeom so no entities would have been created
    if (!inst->mSkeletonRoot && mBuildData.isSkeleton() && (blockType(mRoots[0]) == "NiNode" || blockType(mRoots[0]) == "BSFadeNode"))
        //&& mModelName != "meshes\\morroblivion\\creatures\\wildlife\\kagouti\\skeleton.nif") // FIXME
    {
        NiNode *rootNode = getRef<NiNode>(mRoots[0]);
        std::string rootName = rootNode->getNiNodeName();
        if (rootName == "Scene Root" || rootName == "skeleton.nif")
        {
            std::string meshName = getModelName() + "@" + rootName;
            MeshLoader loader = MeshLoader(rootNode);
            //std::cout << meshName << std::endl;

            Ogre::MeshPtr mesh = meshManager.getByName(meshName);
            if (!mesh)
                mesh = meshManager.createManual(meshName, mGroup, &loader);

            //if (!mesh)
                //return; // FIXME: morroblivion\creatures\wildlife\kagouti\skeleton.nif

            mesh->setSkeletonName(getModelName());

            Ogre::Entity *entity = inst->mBaseSceneNode->getCreator()->createEntity(mesh);
            inst->mSkeletonRoot = entity;
            inst->mEntities[0] = entity; // experimental for position

            // FIXME: experimental
            Ogre::SkeletonInstance *skelinst = inst->mSkeletonRoot->getSkeleton();

            //if (!skelinst)
                //return; // FIXME: morroblivion\creatures\wildlife\kagouti\skeleton.nif

            Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
            while(boneiter.hasMoreElements())
                boneiter.getNext()->setManuallyControlled(true);
        }
    }

    // FIXME: maybe just have pointers to the vectors rather than copying all the content?
    for (size_t i = 0; i < mBuildData.mFlameNodes.size(); ++i)
    {
        inst->mFlameNodes.push_back(mBuildData.mFlameNodes[i]->getNiNodeName());
        //std::cout << "flame " << getModelName() << " " << inst->mFlameNodes.back() << std::endl;
    }

    for (size_t i = 0; i < mBuildData.mAttachLights.size(); ++i)
    {
        inst->mAttachLights.push_back(mBuildData.mAttachLights[i]->getNiNodeName());
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
    if (mBuildData.mSkelLeafIndicies.size() > 1)
    {
        mBuildData.mSkeleton = Ogre::SkeletonManager::getSingleton().getByName(getModelName(), mGroup);
        if (!mBuildData.mSkeleton)
        {
            SkeletonLoader loader(*this);
            mBuildData.mSkeleton
                = Ogre::static_pointer_cast<Ogre::Skeleton>(Ogre::SkeletonManager::getSingleton().load(
                    getModelName(), // use the NIF name, since there is one skeleton per NIF
                    mGroup,         // Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
                    true,           // isManual
                    &loader));
        }
    }
}

void NiBtOgre::BuildData::setNiNodeParent(NiAVObjectRef child, NiNode *parent)
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

/*const*/ NiBtOgre::NiNode& NiBtOgre::BuildData::getNiNodeParent(NiAVObjectRef child) const
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

void NiBtOgre::BuildData::addNewSkelLeafIndex(NiNodeRef leaf)
{
    if (std::find(mSkelLeafIndicies.begin(), mSkelLeafIndicies.end(), leaf) == mSkelLeafIndicies.end())
        mSkelLeafIndicies.push_back(leaf);
}

bool NiBtOgre::BuildData::hasBoneLeaf(NiNodeRef leaf) const
{
     return std::find(mSkelLeafIndicies.begin(), mSkelLeafIndicies.end(), leaf) != mSkelLeafIndicies.end();
}
