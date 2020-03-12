/*
  Copyright (C) 2015-2020 cc9cii

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

#include <boost/algorithm/string.hpp>

#include <OgreMeshManager.h>
#include <OgreMesh.h>
#include <OgreSkeletonManager.h>
#include <OgreSkeleton.h>

#include "niobject.hpp"
#include "bhkrefobject.hpp" // bhkConstraint
#include "btogreinst.hpp"
#include "ninode.hpp"
#include "nisequence.hpp"
#include "nigeometry.hpp" // for getVertices()

// "nif" is the full path to the mesh from the resource directory/BSA added to Ogre::ResourceGroupManager.
// This name is required later for Ogre resource managers such as MeshManager.
// The file is opened by mNiStream::mStream.
//
// FIXME: there could be duplicates b/w TES3 and TES4/5
NiBtOgre::NiModel::NiModel(Ogre::ResourceManager *creator, const Ogre::String& name, Ogre::ResourceHandle handle,
                           const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
                           const Ogre::String& nif/*, bool showEditorMarkers*/)
    : Resource(creator, name, handle, group, isManual, loader)
    , mGroup(group)
    , mCurrIndex(-1) // WARN: for debugging only, may be removed in future
    , mModelName(name)
    , mNif(nif)
    , mRootNode(nullptr)
    , mBuildData(*this)
    , mShowEditorMarkers(false/*showEditorMarkers*/)
{
    mObjects.clear();
    mSkeleton.reset();
}

NiBtOgre::NiModel::~NiModel()
{
    //unload(); // FIXME: causes exceptions trying to notify the creator (NiModelManager)?
}

void NiBtOgre::NiModel::prepareImpl()
{
    mNiStream = std::make_unique<NiBtOgre::NiStream>(mNif); // WARN: may throw
    mHeader = std::make_unique<NiBtOgre::NiHeader>(mNiStream.get());
}

void NiBtOgre::NiModel::unprepareImpl()
{
    mHeader.reset();
    mNiStream.reset();
}

void NiBtOgre::NiModel::unloadImpl()
{
    for (std::size_t i = 0; i < mMeshes.size(); ++i)
        mMeshes[i].first.reset();

    mSkeleton.reset();
}

void NiBtOgre::NiModel::preLoadImpl() // for manual loading
{
    prepareImpl();
}

void NiBtOgre::NiModel::preUnloadImpl() // for manual loading
{
    unprepareImpl();
}

// only called if this resource is not being loaded from a ManualResourceLoader
void NiBtOgre::NiModel::loadImpl()
{
    createNiObjects();
    findBoneNodes(true); // not really needed, but this allows NiNode::addBones() to search for names
    // objects being built may require the skeleton to exist, so load it now
    buildSkeleton(true/*load*/);
    //if (blockType(0) == "NiTriShape")
        //return; // FIXME: morroblivion\environment\bittercoast\bcscum03.nif
    createMesh();
    buildModel();
}

void NiBtOgre::NiModel::createNiObjects()
{
    if (!mObjects.empty())
        return; // don't create the objects again

    //if (mModelName.find("smeltermarker") != std::string::npos)/*fxambwatersalmon02b*/
        //std::cout << mModelName << std::endl;
    //if (getModelName().find("vgeardoor01") != std::string::npos)
        //std::cout << "door" << std::endl;

    mObjects.resize(mHeader->numBlocks());
    if (mNiStream->nifVer() >= 0x0a000100) // from 10.0.1.0
    {
        for (std::uint32_t i = 0; i < mHeader->numBlocks(); ++i)
        {
            mCurrIndex = i; // FIXME: debugging only
#if 0
            if (blockType(0) == "NiTriShape")
                return; // FIXME: morroblivion\environment\bittercoast\bcscum03.nif
#endif
            // From ver 10.0.1.0 (i.e. TES4) we already know the object types from the header.
            mObjects[i] = NiObject::create(mHeader->blockType(i), i, mNiStream.get(), *this, mBuildData);
        }
    }
    else
    {
        for (std::uint32_t i = 0; i < mHeader->numBlocks(); ++i)
        {
            mCurrIndex = i; // FIXME: debugging only
#if 0
            std::string blockName = mNiStream->readString();
            if (blockName == "RootCollisionNode")
                std::cout << name << " : " << "RootCollisionNode" << std::endl;
            if (blockName == "AvoidNode")
                std::cout << name << " : " << "AvoidNode" << std::endl;
            //std::cout << name << " : " << "BoundingBox" << std::endl;
            mObjects[i] = NiObject::create(blockName, i, mNiStream.get(), *this, mBuildData);
#else
            // For TES3, the object type string is read first to determine the type.
            mObjects[i] = NiObject::create(mNiStream->readString(), i, mNiStream.get(), *this, mBuildData);
#endif
        }
    }

    // TODO: should assert that the first object, i.e. mObjects[0], is either a NiNode (TES3/TES4)
    //       or BSFadeNode (TES5)

    // read the footer to check for root nodes
    std::uint32_t numRoots = 0;
    mNiStream->read(numRoots);

    mRoots.resize(numRoots);
    for (std::uint32_t i = 0; i < numRoots; ++i)
        mNiStream->read(mRoots.at(i));

    if (numRoots == 0)
        throw std::runtime_error(mModelName + " has no roots");
    else if (numRoots > 1) // FIXME: debugging only, to find out which NIF has multiple roots
        //throw std::runtime_error(mModelName + " has too many roots");
        // Creatures\Bear\forward.kf had 3 roots
        std::cout << "NOTE: " << mModelName << " has " << numRoots << " numRoots." << std::endl;

    if (numRoots = 1)
        mRootNode = getRef<NiNode>(mRoots[0]);
}

// find the bones, if any (i.e. prepare for the skeleton loader)
void NiBtOgre::NiModel::findBoneNodes(bool buildObjectPalette, size_t rootIndex)
{
    if (mBuildData.mSkelLeafIndicies.size() > 1) // TODO: should we allow skeleton with a single bone?
    {
        int32_t index;
        NiNode *node;
        for (std::size_t i = 0; i < mBuildData.mSkelLeafIndicies.size(); ++i)
        {
            index = mBuildData.mSkelLeafIndicies[i];
            // dungeons\ayleidruins\exterior\arwellgrate01.nif has a NiBillboardNode target
            if (blockType(index) != "NiNode" && blockType(index) != "NiBillboardNode")
                continue; // FIXME: morroblivion\flora\bushes\corkbulb01anim.nif, index 0x20

            node = getRef<NiNode>(index);
            NiNodeRef boneRoot = node->findBones(mRoots[rootIndex]);
            if (boneRoot != -1)
                mBoneRootNode = getRef<NiNode>(boneRoot); // just overwrite the previous result

            if (buildObjectPalette)
                mObjectPalette[node->getNiNodeName()] = index;
        }
    }
}

void NiBtOgre::NiModel::setSkinTexture(const std::string& texture)
{
    // see if we built the meshes already
    if (!mMeshes.empty())
        throw std::logic_error("NiModel: too late to set the texture to build the mesh");

    // NOTE: the texture may be used to replace the exposed skin
    // validate that there is only one sub-mesh for the texture
    //if (mBuildData.mMeshBuildList.size() != 1)
        //throw std::logic_error("NiModel: tooo many NiTriBasedGeom to set the texture");

    // do it for all meshes just in case - unfortunately can't predict which ones will need it
    std::map<NiNodeRef, NiNode*>::iterator iter;
    for (iter = mBuildData.mMeshBuildList.begin(); iter != mBuildData.mMeshBuildList.end(); ++iter)
        iter->second->setSkinTexture(texture);
}

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
//    Must remember to move all the related entities for any havok objects.
//
//    e.g. Weapons\Steel\Longsword.NIF
//
// FIXME: maybe pass a parameter here indicating static mesh? (create a "static" group?)
// Or group should come from the classes, e.g. static, misc, furniture, etc
void NiBtOgre::NiModel::createMesh(bool isMorphed, Ogre::SkeletonPtr skeleton)
{
    Ogre::MeshManager& meshManager = Ogre::MeshManager::getSingleton();
    NiMeshLoader& meshLoader = NiModelManager::getSingleton().meshLoader();

    // make the mesh names unique
    //
    // * the same NIF can be used with different externally supplied skeletons
    // * morphed vertices and textures of the same NIF can be used for many NPCs
    std::string skelName = "";
    if (skeleton)
        skelName = boost::to_lower_copy(skeleton->getName());

    std::string modelName;
    if (isMorphed || !skeleton)
        modelName = getModelName(); // WARN: morphed must have Npc::mEditorId + "_" in getModelName()
    else
        modelName = skelName + "_" + getModelName(); // getModelName() should return NIF name

    // iterate through the mesh build map
    //
    // NOTE: If the model/object is static, we only need one child scenenode from the
    // basenode.  Else we need one for each NiNode that has a mesh (and collision shape?).
    // FIXME: how to do this?
    std::map<NiNodeRef, NiNode*>::iterator iter;
    for (iter = mBuildData.mMeshBuildList.begin(); iter != mBuildData.mMeshBuildList.end(); ++iter)
    {
        // skeleton name: only used for skinned meshes (same mesh used for different skeletons)
        //   e.g. Armor\Thief\M\Boots.NIF, "COW "Tamriel" 5 11" (TES4)
        //   ShadySam (Breton, skeleton.nif) & DeadArgonianAgent (Argonian, SkeletonBeast.NIF)
        //
        // The model name and parent node name are concatenated for use with Ogre::MeshManager
        // without triggering exeptions due to duplicates.
        // e.g. meshes\\architecture\\imperialcity\\icwalltower01.nif@ICWallTower01
        //
        // NiNode index: some NIF files have the NiNode name for different nodes in the same NIF
        //   e.g. Architecture\Solitude\SolitudeBase.nif (TES5)
        //
        // NiNode name: technically not required but useful for debugging
        //
        // FIXME: failsafe - check if mParent->getNodeName() returns blank, in which case use block number?
        // FIXME: consider the use of a hash (possibly the same as BSA) + block number for performance
        std::string meshName = modelName +
                               "#" + std::to_string(iter->second->selfRef()) + // node index
                               "@" + iter->second->getNiNodeName();            // node name

        Ogre::MeshPtr mesh = meshManager.getByName(/*boost::algorithm::to_lower_copy(meshName)*/meshName, mGroup);
        if (!mesh)
            mesh = meshLoader.createMesh(meshName, mGroup, this, iter->second->selfRef(), skelName);

        if (skeleton)
            mesh->setSkeletonName(skelName);
        else if (mBuildData.isSkeletonTES4() && mSkeleton) // FIXME: FO3, etc, but not yet tested
            mesh->setSkeletonName(mSkeleton->getName()); // Storm Atronach skeleton.nif has its own mesh
#if 0
        // FIXME: testing VGearDoor01.NIF
        if (mSkeleton && !mesh->hasSkeleton() && getModelName().find("geardoor") != std::string::npos)
        {
            mesh->setSkeletonName(getModelName());
        }
#endif
        mesh->setAutoBuildEdgeLists(false);

        mMeshes.push_back(std::make_pair(mesh, iter->second));
    }

    // Create a dummy mesh if the root node does not have a mesh (and hence an Entity) to support
    // the current implementation of MWRender::Animation, since it requires an entity at the root
    // of the skeleton model. (see mObjectRoot->mSkelBase i.e. NifOgre::ObjectScene::mSkelBase)
    //
    // Most Skeleton.nif doesn't have any NiTriBasedGeom, and even Storm Atronach's skeleton
    // doesn't have one at the root node.
    //
    // e.g. Characters\_Male\Skeleton.nif (no mesh)
    // e.g. Creatures\StormAtronach\Skeleton.nif (also has normal mesh)
    //
    if (mBuildData.isSkeletonTES4()) // FIXME: may also be required for others, but not yet tested
    {
        // FIXME: despite the name, maybe this should be "Scene Root" or "BSFadeNode" instead?
        // mModelName != "meshes\\morroblivion\\creatures\\wildlife\\kagouti\\skeleton.nif") // FIXME
#if 0
        NiNode * rootNode = rootNode(); // WARN: maybe nullptr if there are multiple roots!
#else
        NiNode * rootNode = skeletonRoot();
#endif
        NiNodeRef rootIndex = rootNode->selfRef();

        // only if it has not been built already
        if (mBuildData.mMeshBuildList.find(rootIndex) == mBuildData.mMeshBuildList.end())
        {
            std::string meshName = getModelName() + "#0@" + rootNode->getNiNodeName();
            Ogre::MeshPtr mesh = meshManager.getByName(meshName);
            if (!mesh)
                mesh = meshLoader.createMesh(meshName, mGroup, this, rootIndex);

            if (skeleton)
                mesh->setSkeletonName(skelName);
            else if (mSkeleton)
                mesh->setSkeletonName(mSkeleton->getName());

            mMeshes.push_back(std::make_pair(mesh, rootNode));
        }
    }
}

// for building body parts
void NiBtOgre::NiModel::buildSkinnedModel(Ogre::SkeletonPtr skeleton)
{
    if (mSkeleton && skeleton)
        throw std::logic_error("NiModel: overwriting its own skeleton");

    // for building body parts use the supplied skeleton
    // but don't overwrite an existing one (for Storm Atronach)
    if (!mSkeleton)
        mSkeleton = skeleton;

    mObjects[rootIndex()]->build(&mBuildData); // FIXME: what to do with other roots?
}

// build the skeleton and node controllers
void NiBtOgre::NiModel::buildModel()
{
    mObjects[rootIndex()]->build(&mBuildData); // FIXME: what to do with other roots?
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
    getRef<NiControllerSequence>(rootIndex())->build(skelBase, anim, textKeys, controllers, *skeleton, skeleton->getObjectPalette());
}
#if 0
void NiBtOgre::NiModel::createCollisionshapes()
{
    const std::map<std::int32_t, /*std::pair<std::string,*/ int32_t/*>*/ >& rigidBodies = nimodel->getBhkRigidBodyMap();
    std::map<std::int32_t, /*std::pair<std::string, */int32_t/*>*/ >::const_iterator iter(rigidBodies.begin());
    for (; iter != rigidBodies.end(); ++iter)
    {
        //if (iter->second/*.second*/ == -1)
            //continue;  // e.g. fire/firetorchlargesmoke.nif@DamageSphere
        // FIXME: check for phantom

        std::int32_t bhkRef = iter->second/*.second*/;
        bhkSerializable *bhk = nimodel->getRef<bhkSerializable>(bhkRef);
        std::int32_t targetRef = iter->first;
        NiAVObject *target = nimodel->getRef<NiAVObject>(targetRef);

        mTargetNames[targetRef] = nimodel->indexToString(target->getNameIndex());

        // expectation is that each target has only one bhkRigidBody
        if (mBtCollisionShapeMap.find(targetRef) != mBtCollisionShapeMap.end())
            throw std::logic_error("target name collision "+nimodel->indexToString(targetRef));


        // get the bullet shape with the target as a parameter
        // TODO: cloning pre-pade shape (e.g. bhkRigidBody via unique_ptr) may be faster?
        mBtCollisionShapeMap[targetRef] = std::make_pair(target->getWorldTransform(), bhk->getShape(*target));
    }
}
#endif
void NiBtOgre::NiModel::buildSkeleton(bool load)
{
    if (mSkeleton)
        return; // we may already have an externally supplied skeleton

    if (mBuildData.mSkelLeafIndicies.size() > 1)
    {
        mSkeleton = Ogre::SkeletonManager::getSingleton().getByName(getModelName(), mGroup);
        if (!mSkeleton)
        {
            NiSkeletonLoader& skeletonLoader = NiModelManager::getSingleton().skeletonLoader();
            mSkeleton = skeletonLoader.createSkeleton(getModelName(), mGroup, this);
        }

        if (load)
            mSkeleton->load();
    }
}

// for non-skinned parts
std::string NiBtOgre::NiModel::targetBone() const
{
    NiBtOgre::NiNode *rootNode = getRef<NiBtOgre::NiNode>(rootIndex());
    return rootNode->getStringExtraData("Prn");
}

void NiBtOgre::NiModel::useFgMorphVertices()
{
    fgGeometry()->mUseMorphed = true;
}

const std::vector<Ogre::Vector3>& NiBtOgre::NiModel::fgVertices() const
{
    return fgGeometry()->getVertices(false/*morphed*/);
}

std::vector<Ogre::Vector3>& NiBtOgre::NiModel::fgMorphVertices()
{
    return fgGeometry()->mMorphVertices;
}

// WARN: returns the vertices from the first NiTriBasedGeom child of the root NiNode
//       (this can be used to get around the lack of TRI files for certain NIF models)
NiBtOgre::NiTriBasedGeom *NiBtOgre::NiModel::fgGeometry() const
{
    if (mObjects.empty())
        throw std::logic_error("NiModel attempting to retrieve an object that is not yet built.");

    NiNode *ninode = getRef<NiNode>(rootIndex());

    return ninode->getUniqueSubMeshChild();
}

NiBtOgre::NiNode *NiBtOgre::NiModel::skeletonRoot()
{
    if (!mBoneRootNode)
    {
        // FIXME: not even sure why we're building skeletons for skinned models
        // anyway these don't have "UPB"
        if (mBuildData.mIsSkinned)
        {
            mBoneRootNode = getRef<NiNode>(mBuildData.mSkeletonRoot);

            return mBoneRootNode;
        }

        // a bit dumb, won't remember previous unsuccessful search
        for (std::size_t i = 0; i < mRoots.size(); ++i)
        {
            findBoneNodes(false, mRoots[i]);
            if (mBoneRootNode)
                break; // more stupidity, won't search if there are any other bone roots
        }
    }

    return mBoneRootNode;
}

const Ogre::Quaternion NiBtOgre::NiModel::getBaseRotation() const
{
    return rootNode()->getLocalRotation();
}

std::uint32_t NiBtOgre::NiModel::rootIndex() const
{
    if (numRootNodes() > 1)
        throw std::logic_error("NiNode parent map: multiple parents");

    return mRoots[0];
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
    {
        return *it->second;
    }
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
