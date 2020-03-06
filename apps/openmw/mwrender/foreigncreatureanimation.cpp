#include "foreigncreatureanimation.hpp"

#include <iostream> // FIXME

#include <OgreEntity.h>
#include <OgreSkeletonInstance.h>
#include <OgreBone.h>
#include <OgreSceneNode.h>
#include <OgreMesh.h>

#include <extern/esm4/crea.hpp>
#include <extern/nibtogre/btogreinst.hpp>

#include <components/esm/loadcrea.hpp>

#include "../mwbase/world.hpp"
//#include "../mwbase/environment.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "renderconst.hpp"

namespace MWRender
{

void ForeignCreatureAnimation::play(const std::string &groupname, int priority, int groups, bool autodisable,
              float speedmult, const std::string &start, const std::string &stop,
              float startpoint, size_t loops, bool loopfallback)
{
    if (mPtr.getClass().getCreatureStats(mPtr).isDead())
        std::cout << "dead" << std::endl;

    //MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();
    //std::cout << ref->mBase->mEditorId << " play " << groupname << std::endl;

    Animation::play(groupname, priority, groups, autodisable, speedmult, start, stop, startpoint, loops, loopfallback);
}

ForeignCreatureAnimation::ForeignCreatureAnimation(const MWWorld::Ptr &ptr, const std::string& skeletonModel)
  : Animation(ptr, ptr.getRefData().getBaseNode())
{
    MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();

    //std::cout << ref->mBase->mEditorId << " " << ref->mBase->mFullName << std::endl;

    if(skeletonModel.empty())
        return;

    std::string skeletonName = skeletonModel;
    Misc::StringUtils::lowerCaseInPlace(skeletonName);

    setForeignObjectRootBase(skeletonName);
    //setRenderProperties(mObjectRoot, RV_Actors, RQG_Main, RQG_Alpha);

    if (mObjectRoot->mForeignObj)
    {
        mObjectRoot->mSkelBase = mObjectRoot->mForeignObj->mSkeletonRoot;
        mSkelBase = mObjectRoot->mForeignObj->mSkeletonRoot;
    }

    if (mObjectRoot->mSkelBase == nullptr) // FIXME: FO3
        return;

    if (mObjectRoot->mSkelBase->getSkeleton()->hasBone("Bip01"))
    {
        Ogre::Bone* b = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip01");
        Ogre::Bone* bna = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip01 NonAccum");
        if (b && bna)
        {
            //std::cout << ref->mBase->mEditorId << " Bip01 " << ref->mBase->mFullName << std::endl;
            // FIXME: needs a good cleanup
            if (1)//ref->mBase->mFullName == "Storm Atronach")
            {
                std::map<int32_t, Ogre::Entity*>::const_iterator it(mObjectRoot->mForeignObj->mEntities.begin());
                for (; it != mObjectRoot->mForeignObj->mEntities.end(); ++it)
                {
                    if (!it->second->isAttached())
                    {
                        it->second->shareSkeletonInstanceWith(mSkelBase);
                        mInsert->attachObject(it->second);
                    }
                }
            }
    // "Creatures\\StormAtronach\\Skeleton.nif"

            Ogre::Quaternion qb = b->getOrientation(); // skeleton.nif has -90 roll
            Ogre::Quaternion qbna = bna->getOrientation(); // has 0 roll
            //b->setOrientation(qbna);
            //bna->setOrientation(qb);
            //b->setOrientation(qb * Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y));

            // FIXME: all the skinned meshes seems to be offset by this, probably something to do
            // with the binding position
            Ogre::Vector3 vb = b->getPosition();
            //Ogre::Vector3 ins = mInsert->getPosition();

            //bna->setPosition(vb);

            //Ogre::Bone* rootBone = mObjectRoot->mSkelBase->getSkeleton()->getBone("Scene Root");
            //Ogre::Vector3 vRoot = rootBone->convertLocalToWorldPosition(rootBone->_getDerivedPosition());
            //Ogre::Vector3 vBase = mSkelBase->getParentSceneNode()->_getDerivedPosition();
            //std::cout << vRoot.x << " " << vBase.x << std::endl;
            //std::cout << vRoot.y << " " << vBase.y << std::endl;
            //std::cout << vRoot.z << " " << vBase.z << std::endl;
        }
    }
    else if (mObjectRoot->mSkelBase->getSkeleton()->hasBone("Bip02"))
    {
    std::cout << ref->mBase->mEditorId << " Bip02 " << ref->mBase->mFullName << std::endl;
        Ogre::Bone* b2 = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip02");
        Ogre::Bone* bna2 = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip02 NonAccum");
        if (b2 && bna2)
        {
            Ogre::Quaternion qb = b2->getOrientation(); // skeleton.nif has -90 roll
            Ogre::Quaternion qbna = bna2->getOrientation(); // has 0 roll
            //b2->setOrientation(qbna);
            //bna2->setOrientation(qb);
            //Ogre::Vector3 vb = b2->getPosition();
            //Ogre::Vector3 ins = mInsert->getPosition();
        }
    }

    //if((ref->mBase->mFlags&ESM::Creature::Bipedal))
        //addAnimSource("meshes\\xbase_anim.nif");
    addAnimSource(skeletonName);

    size_t pos = skeletonName.find_last_of('\\');
    std::string path = skeletonName.substr(0, pos+1); // +1 for '\\'
    std::string group("General");

    NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();
    for (std::size_t i = 0; i < ref->mBase->mNif.size(); ++i)
    {
        //std::cout << ref->mBase->mNif[i] << std::endl;
        std::string meshName = path+ref->mBase->mNif[i];

        // initially assume a skinned model
        NiModelPtr object = modelManager.getByName(skeletonName + "_" + meshName, group);
        // if not found just create a non-skinned model to check
        if (!object)
        {
            object = modelManager.getOrLoadByName(meshName, group);
            if (object->buildData().mIsSkinned)
            {
                // was skinned after all
                object.reset();
                object = modelManager.createSkinnedModel(meshName, group, mObjectRoot->mForeignObj->mModel.get());
            }
        }

        // create an instance of the model
        NifOgre::ObjectScenePtr scene
            = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));

        scene->mForeignObj = std::make_unique<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(object,
                             mInsert->createChildSceneNode()));
#if 1
        scene->mForeignObj->instantiate(mInsert, mSkelBase);
#else
        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            if (scene->mForeignObj->mIsSkinned)
            {
                it->second->shareSkeletonInstanceWith(mSkelBase);
                mInsert->attachObject(it->second);
            }
            else
                mSkelBase->attachObjectToBone(scene->mForeignObj->mTargetBone, it->second);
        }
#endif
        mObjectParts.push_back(scene);
    }

    // the rotation is caused by the animations and can't be fixed here
#if 0
    MWBase::World *world = MWBase::Environment::get().getWorld();
    world->rotateObject(mPtr, 0.f, 0.f, 90.f, true);
    // FIXME: 90 deg issue - does not work
    mInsert->rotate(//Ogre::Quaternion(mHeadYaw, Ogre::Vector3::UNIT_Z)
               //* Ogre::Quaternion(mHeadPitch, Ogre::Vector3::UNIT_X)
               Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Z)
                 ,Ogre::Node::TS_WORLD);
#endif

    mSkelBase->getSkeleton()->reset(true);
}

void ForeignCreatureAnimation::addAnimSource(const std::string &skeletonName)
{
    size_t pos = skeletonName.find_last_of('\\');
    std::string path = skeletonName.substr(0, pos+1); // +1 for '\\'

    MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();

    std::string animName;
    addForeignAnimSource(skeletonName, path + "castself.kf");
    addForeignAnimSource(skeletonName, path + "backward.kf");
    addForeignAnimSource(skeletonName, path + "idle.kf");
    addForeignAnimSource(skeletonName, path + "forward.kf");
    //addForeignAnimSource(skeletonName, path + "fastforward.kf");
    for (unsigned int i = 0; i < ref->mBase->mKf.size(); ++i)
    {
        //std::cout << ref->mBase->mKf[i] << std::endl;
        animName = path + ref->mBase->mKf[i];
        addForeignAnimSource(skeletonName, animName);
    }
}

void ForeignCreatureAnimation::addForeignAnimSource(const std::string& model, const std::string &animName)
{
    // Check whether the kf file exists
    if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(animName))
        return;
    std::string group("General"); // FIXME
#if 0
    NiModelPtr creatureModel = NiBtOgre::NiModelManager::getSingleton().getOrLoadByName(model, group);
    //creatureModel->buildSkeleton(); // FIXME: hack
#else

    NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();
    NiModelPtr skeleton = modelManager.getByName(model, "General");
    if (!skeleton)
        skeleton = modelManager.createSkeletonModel(model, "General");

#endif
    assert(!skeleton.isNull() && "skeleton.nif should have been built already");
    NiModelPtr anim = NiBtOgre::NiModelManager::getSingleton().getOrLoadByName(animName, group);

    // Animation::AnimSource : public Ogre::AnimationAlloc
    //   (has a) std::multimap<float, std::string> mTextKeys
    //   (also has a vector of 4 Ogre real controllers)  TODO: check if 4 is enough
    Ogre::SharedPtr<AnimSource> animSource(OGRE_NEW AnimSource);
    std::vector<Ogre::Controller<Ogre::Real> > controllers;
    anim->buildAnimation(mSkelBase, anim, animSource->mTextKeys, controllers, /*mObjectRoot->skeleton.get()*/skeleton.get());

    if (animSource->mTextKeys.empty() || controllers.empty())
        return;

    mAnimSources.push_back(animSource);

    std::vector<Ogre::Controller<Ogre::Real> > *grpctrls = animSource->mControllers;
    for (size_t i = 0; i < controllers.size(); i++)
    {
        NifOgre::NodeTargetValue<Ogre::Real> *dstval;
        dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(controllers[i].getDestination().get());

        size_t grp = detectAnimGroup(dstval->getNode());

        if (!mAccumRoot && grp == 0)
        {
            mNonAccumRoot = dstval->getNode();
            mAccumRoot = mNonAccumRoot->getParent();
            if (!mAccumRoot)
            {
                std::cerr << "Non-Accum root for " << mPtr.getCellRef().getRefId() << " is skeleton root??" << std::endl;
                mNonAccumRoot = NULL;
            }
        }

        if (grp == 0 && (dstval->getNode()->getName() == "Bip01 NonAccum"))
        {
            mNonAccumRoot = dstval->getNode();
            mAccumRoot = mNonAccumRoot->getParent(); // should be "Bip01"
            if (!mAccumRoot)
            {
                std::cerr << "Non-Accum root for " << mPtr.getCellRef().getRefId() << " is skeleton root??" << std::endl;
                mNonAccumRoot = NULL;
            }
        }

        controllers[i].setSource(mAnimationTimePtr[grp]);
        grpctrls[grp].push_back(controllers[i]);
    }

    // FIXME: debugging
    NifOgre::NodeTargetValue<Ogre::Real> *dstval;
    dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(controllers[0].getDestination().get());
    Ogre::Node *node = dstval->getNode();
    while (node && node->getName() != "Bip01 NonAccum")// && (node->getName() != "Bip02 NonAccum")))
    {
        node = node->getParent();
    }

    if (node && node->getName() == "Bip01 NonAccum")// || (node->getName() == "Bip02 NonAccum")))
    {
    //MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();
    //std::cout << ref->mBase->mEditorId << " root " << ref->mBase->mFullName << std::endl;
        mNonAccumRoot = node;
        mAccumRoot = mNonAccumRoot->getParent();
    }
    else if (0)//node && ((node->getName() == "Bip01") || (node->getName() == "Bip02")))
    {
    //MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();
    //std::cout << ref->mBase->mEditorId << " no root " << ref->mBase->mFullName << std::endl;
        mAccumRoot = node;
    }
    // else throw?
    else if (0)
    {
    //MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();
    //std::cout << ref->mBase->mEditorId << " bone root " << ref->mBase->mFullName << std::endl;
        if (mObjectRoot->mSkelBase->getSkeleton()->hasBone("Bip01"))
        {
            mAccumRoot = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip01");
            mNonAccumRoot = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip01 NonAccum");
        }
        else if (mObjectRoot->mSkelBase->getSkeleton()->hasBone("Bip02"))
        {
            mAccumRoot = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip02");
            mNonAccumRoot = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip02 NonAccum");
        }
    }

    //if ((mNonAccumRoot->getName() != "Bip01 NonAccum" || mAccumRoot->getName() != "Bip01")
        //|| (mNonAccumRoot->getName() != "Bip02 NonAccum" || mAccumRoot->getName() != "Bip02"))
        //std::cout << mAccumRoot->getName() << std::endl;
    // end debugging

    for (unsigned int i = 0; i < mObjectRoot->mControllers.size(); ++i)
    {
        if (!mObjectRoot->mControllers[i].getSource())
            mObjectRoot->mControllers[i].setSource(mAnimationTimePtr[0]);
    }
}

Ogre::Vector3 ForeignCreatureAnimation::runAnimation(float timepassed)
{
    Ogre::Vector3 ret = Animation::runAnimation(timepassed);

    //if (mSkelBase)
        //pitchSkeleton(mPtr.getRefData().getPosition().rot[0], mSkelBase->getSkeleton());

    if (0)//mSkelBase)
    {
        Ogre::SkeletonInstance *baseinst = mSkelBase->getSkeleton();
        Ogre::Node* node = baseinst->getBone("Bip01");
        if (node)
            node->rotate(Ogre::Quaternion(Ogre::Degree(0), Ogre::Vector3::UNIT_Z)
                       * Ogre::Quaternion(Ogre::Degree(0), Ogre::Vector3::UNIT_X)
                       * Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y)
                         ,Ogre::Node::TS_WORLD);
    }

    for(size_t i = 0; i < mObjectParts.size(); ++i)
    {
        std::vector<Ogre::Controller<Ogre::Real> >::iterator ctrl(mObjectParts[i]->mControllers.begin());
        for(;ctrl != mObjectParts[i]->mControllers.end();++ctrl)
            ctrl->update();

        if (mObjectParts[i]->mForeignObj->mEntities.size() == 0 ||
                !mObjectParts[i]->mForeignObj->mEntities.begin()->second->hasSkeleton()) // FIXME: maybe cache the result of isSkinned()?
            continue;

        if (0)//mSkelBase)
            updateSkeletonInstance(mSkelBase->getSkeleton(), mObjectParts[i]->mSkelBase->getSkeleton());

        mObjectParts[i]->mForeignObj->mSkeletonRoot->getAllAnimationStates()->_notifyDirty();
    }

    return ret;
}

ForeignCreatureWeaponAnimation::ForeignCreatureWeaponAnimation(const MWWorld::Ptr &ptr, const std::string& model)
    : Animation(ptr, ptr.getRefData().getBaseNode())
    , mShowWeapons(false)
    , mShowCarriedLeft(false)
{
    MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();

    if(!model.empty())
    {
        setObjectRoot(model, false);
        setRenderProperties(mObjectRoot, RV_Actors, RQG_Main, RQG_Alpha);

        if((ref->mBase->mFlags&ESM::Creature::Bipedal))
            addAnimSource("meshes\\xbase_anim.nif");
        addAnimSource(model);

        mPtr.getClass().getInventoryStore(mPtr).setListener(this, mPtr);

        updateParts();
    }

    mWeaponAnimationTime = Ogre::SharedPtr<WeaponAnimationTime>(new WeaponAnimationTime(this));
}

void ForeignCreatureWeaponAnimation::showWeapons(bool showWeapon)
{
    if (showWeapon != mShowWeapons)
    {
        mShowWeapons = showWeapon;
        updateParts();
    }
}

void ForeignCreatureWeaponAnimation::showCarriedLeft(bool show)
{
    if (show != mShowCarriedLeft)
    {
        mShowCarriedLeft = show;
        updateParts();
    }
}

void ForeignCreatureWeaponAnimation::updateParts()
{
    mWeapon.reset();
    mShield.reset();

    if (mShowWeapons)
        updatePart(mWeapon, MWWorld::InventoryStore::Slot_CarriedRight);
    if (mShowCarriedLeft)
        updatePart(mShield, MWWorld::InventoryStore::Slot_CarriedLeft);
}

void ForeignCreatureWeaponAnimation::updatePart(NifOgre::ObjectScenePtr& scene, int slot)
{
    if (!mSkelBase)
        return;

    MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ContainerStoreIterator it = inv.getSlot(slot);

    if (it == inv.end())
    {
        scene.reset();
        return;
    }
    MWWorld::Ptr item = *it;

    std::string bonename;
    if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
        bonename = "Weapon Bone";
    else
        bonename = "Shield Bone";

    scene = NifOgre::Loader::createObjects(mSkelBase, bonename, bonename, mInsert, item.getClass().getModel(item));
    Ogre::Vector3 glowColor = getEnchantmentColor(item);

    setRenderProperties(scene, RV_Actors, RQG_Main, RQG_Alpha, 0,
                        !item.getClass().getEnchantment(item).empty(), &glowColor);

    // Crossbows start out with a bolt attached
    if (slot == MWWorld::InventoryStore::Slot_CarriedRight &&
            item.getTypeName() == typeid(ESM4::Weapon).name() /*&&
            item.get<ESM4::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow*/) // FIXME: temp comment out to compile
    {
        MWWorld::ContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
        if (ammo != inv.end() /*&& ammo->get<ESM4::Weapon>()->mBase->mData.mType == ESM::Weapon::Bolt*/) // FIXME: temp comment out
            attachArrow();
        else
            mAmmunition.reset();
    }
    else
        mAmmunition.reset();

    if(scene->mSkelBase)
    {
        Ogre::SkeletonInstance *skel = scene->mSkelBase->getSkeleton();
// FIXME: what does this block do?
#if 0
        if(scene->mSkelBase->isParentTagPoint())
        {
            Ogre::Node *root = scene->mSkelBase->getParentNode();
            if(skel->hasBone("BoneOffset"))
            {
                Ogre::Bone *offset = skel->getBone("BoneOffset");

                root->translate(offset->getPosition());

                // It appears that the BoneOffset rotation is completely bogus, at least for light models.
                //root->rotate(offset->getOrientation());
                root->pitch(Ogre::Degree(-90.0f));

                root->scale(offset->getScale());
                root->setInitialState();
            }
        }
#endif
        updateSkeletonInstance(mSkelBase->getSkeleton(), skel);
    }

    std::vector<Ogre::Controller<Ogre::Real> >::iterator ctrl(scene->mControllers.begin());
    for(;ctrl != scene->mControllers.end();++ctrl)
    {
        if(!ctrl->getSource())
        {
            if (slot == MWWorld::InventoryStore::Slot_CarriedRight)
                ctrl->setSource(mWeaponAnimationTime);
            else
                ctrl->setSource(Ogre::SharedPtr<NullAnimationTime>(new NullAnimationTime()));
        }
    }
}

void ForeignCreatureWeaponAnimation::configureAddedObject(NifOgre::ObjectScenePtr object, MWWorld::Ptr ptr, int slot)
{
    Ogre::Vector3 glowColor = getEnchantmentColor(ptr);

    setRenderProperties(object, RV_Actors, RQG_Main, RQG_Alpha, 0,
                        !ptr.getClass().getEnchantment(ptr).empty(), &glowColor);
}

void ForeignCreatureWeaponAnimation::attachArrow()
{
    WeaponAnimation::attachArrow(mPtr);
}

void ForeignCreatureWeaponAnimation::releaseArrow()
{
    WeaponAnimation::releaseArrow(mPtr);
}

Ogre::Vector3 ForeignCreatureWeaponAnimation::runAnimation(float duration)
{
    Ogre::Vector3 ret = Animation::runAnimation(duration);

    if (mSkelBase)
        pitchSkeleton(mPtr.getRefData().getPosition().rot[0], mSkelBase->getSkeleton());

    if (mWeapon)
    {
        for (unsigned int i=0; i<mWeapon->mControllers.size(); ++i)
            mWeapon->mControllers[i].update();
    }
    if (mShield)
    {
        for (unsigned int i=0; i<mShield->mControllers.size(); ++i)
            mShield->mControllers[i].update();
    }

    return ret;
}

}
