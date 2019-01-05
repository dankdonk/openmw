#include "foreigncreatureanimation.hpp"

#include <OgreEntity.h>
#include <OgreSkeletonInstance.h>
#include <OgreBone.h>

#include <extern/esm4/crea.hpp>

#include <components/esm/loadcrea.hpp>

#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"

#include "renderconst.hpp"

namespace MWRender
{


ForeignCreatureAnimation::ForeignCreatureAnimation(const MWWorld::Ptr &ptr, const std::string& model)
  : Animation(ptr, ptr.getRefData().getBaseNode())
{
    MWWorld::LiveCellRef<ESM4::Creature> *ref = mPtr.get<ESM4::Creature>();

    if(!model.empty())
    {
        setObjectRoot(model, false);
        setRenderProperties(mObjectRoot, RV_Actors, RQG_Main, RQG_Alpha);

        if((ref->mBase->mFlags&ESM::Creature::Bipedal))
            addAnimSource("meshes\\xbase_anim.nif");
        addAnimSource(model);
    }
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
