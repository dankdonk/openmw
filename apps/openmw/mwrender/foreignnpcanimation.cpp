#include "foreignnpcanimation.hpp"

#include <memory>
#include <iostream>
#include <iomanip> // for debugging only setprecision

#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreParticleSystem.h>
#include <OgreSubEntity.h>
#include <OgreSkeleton.h>
#include <OgreSkeletonInstance.h>
#include <OgreSceneNode.h>
#include <OgreBone.h>
#include <OgreMesh.h>
#include <OgreTechnique.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreTexturemanager.h>
#include <OgrePixelFormat.h>
#include <OgreCommon.h> // Ogre::Box

#include <extern/shiny/Main/Factory.hpp>

#include <extern/esm4/lvlc.hpp>
#include <extern/esm4/formid.hpp> // mainly for debugging
#include <extern/nibtogre/btogreinst.hpp>
#include <extern/nibtogre/nimodelmanager.hpp>
#include <extern/fglib/fgsam.hpp>
#include <extern/fglib/fgctl.hpp>
#include <extern/fglib/fgfile.hpp>
//#include <extern/fglib/fgegm.hpp>
//#include <extern/fglib/fgtri.hpp>
#include <extern/fglib/fgegt.hpp>

#include <components/misc/rng.hpp>
#include <components/misc/stringops.hpp>
#include <components/misc/resourcehelpers.hpp>

#include <components/nifogre/ogrenifloader.hpp> // ObjectScenePtr

#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "renderconst.hpp"
#include "camera.hpp"

namespace
{

std::string getVampireHead(const std::string& race, bool female)
{
    static std::map <std::pair<std::string,int>, const ESM::BodyPart* > sVampireMapping;

    std::pair<std::string, int> thisCombination = std::make_pair(race, int(female));

    if (sVampireMapping.find(thisCombination) == sVampireMapping.end())
    {
        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const MWWorld::Store<ESM::BodyPart> &partStore = store.get<ESM::BodyPart>();
        for(MWWorld::Store<ESM::BodyPart>::iterator it = partStore.begin(); it != partStore.end(); ++it)
        {
            const ESM::BodyPart& bodypart = *it;
            if (!bodypart.mData.mVampire)
                continue;
            if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
                continue;
            if (bodypart.mData.mPart != ESM::BodyPart::MP_Head)
                continue;
            if (female != (bodypart.mData.mFlags & ESM::BodyPart::BPF_Female))
                continue;
            if (!Misc::StringUtils::ciEqual(bodypart.mRace, race))
                continue;
            sVampireMapping[thisCombination] = &*it;
        }
    }

    if (sVampireMapping.find(thisCombination) == sVampireMapping.end())
        sVampireMapping[thisCombination] = NULL;

    const ESM::BodyPart* bodyPart = sVampireMapping[thisCombination];
    if (!bodyPart)
        return std::string();
    return "meshes\\" + bodyPart->mModel;
}

bool isSkinned (NifOgre::ObjectScenePtr scene)
{
    if (scene->mSkelBase == NULL)
        return false;
    std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
    for (; it != scene->mForeignObj->mEntities.end(); ++it)
    {
        Ogre::Entity *ent = it->second;
        if(/*scene->mSkelBase != ent &&*/ ent->hasSkeleton())// FIXME: not sure why commented out check is needed
        {
            return true;
        }
    }
    return false;
}

// for adding skinned body parts, we will need to create different models for each of the
// skeleton types; the model names have to be different for each because they are cached in
// Ogre::ResourceManager
std::string bodyPartNameExt(const std::string& skeletonModel)
{
    size_t pos = skeletonModel.find_last_of('\\');
    if (pos == std::string::npos)
        return ""; // FIXME: should throw here

    std::string lowerName = Misc::StringUtils::lowerCase(skeletonModel.substr(pos+1)); // +1 for '\\'
    if (lowerName == "skeleton.nif")
        return "_skeleton";
    else if (lowerName == "skeleton_female.nif") // TES5 only
        return "_skeleton_female";
    else if (lowerName == "skeletonbeast_female.nif") // TES5 only
        return "_skeletonbeat_female";
    else if (lowerName == "skeletonbeast.nif")
        return "_beast";
    else // skeletonsesheogorath
        return "_sesheogorath";
}

}


namespace MWRender
{
ForeignHeadAnimationTime::ForeignHeadAnimationTime(MWWorld::Ptr reference)
    : mReference(reference), mTalkStart(0), mTalkStop(0), mBlinkStart(0), mBlinkStop(0), mEnabled(true), mValue(0)
{
    resetBlinkTimer();
}

void ForeignHeadAnimationTime::setEnabled(bool enabled)
{
    mEnabled = enabled;
}

void ForeignHeadAnimationTime::resetBlinkTimer()
{
    mBlinkTimer = -(2.0f + Misc::Rng::rollDice(6));
}

void ForeignHeadAnimationTime::update(float dt)
{
    if (!mEnabled)
        return;

    if (MWBase::Environment::get().getSoundManager()->sayDone(mReference))
    {
        mBlinkTimer += dt;

        float duration = mBlinkStop - mBlinkStart;

        if (mBlinkTimer >= 0 && mBlinkTimer <= duration)
        {
            mValue = mBlinkStart + mBlinkTimer;
        }
        else
            mValue = mBlinkStop;

        if (mBlinkTimer > duration)
            resetBlinkTimer();
    }
    else
    {
        mValue = mTalkStart +
            (mTalkStop - mTalkStart) *
            std::min(1.f, MWBase::Environment::get().getSoundManager()->getSaySoundLoudness(mReference)*2); // Rescale a bit (most voices are not very loud)
    }
}

float ForeignHeadAnimationTime::getValue() const
{
    return mValue;
}

void ForeignHeadAnimationTime::setTalkStart(float value)
{
    mTalkStart = value;
}

void ForeignHeadAnimationTime::setTalkStop(float value)
{
    mTalkStop = value;
}

void ForeignHeadAnimationTime::setBlinkStart(float value)
{
    mBlinkStart = value;
}

void ForeignHeadAnimationTime::setBlinkStop(float value)
{
    mBlinkStop = value;
}

// FIXME: need differnt bone names and parts
// TES4 BMDT
// 0x00000001 = Head
// 0x00000002 = Hair
// 0x00000004 = Upper Body
// 0x00000008 = Lower Body
// 0x00000010 = Hand
// 0x00000020 = Foot
// 0x00000040 = Right Ring
// 0x00000080 = Left Ring
// 0x00000100 = Amulet
// 0x00000200 = Weapon
// 0x00000400 = Back Weapon
// 0x00000800 = Side Weapon
// 0x00001000 = Quiver
// 0x00002000 = Shield
// 0x00004000 = Torch
// 0x00008000 = Tail
// -- General flags (part of BMDT)
// 0x00010000 = Hide Rings
// 0x00020000 = Hide Amulets
// 0x00400000 = Non-Playable (opposite to checkbox in the CS)
// 0x00800000 = Heavy armor
// 0xCD000000 = Unknown default value
//
// FO3 BMDT
// 0x00000001 = Head
// 0x00000002 = Hair
// 0x00000004 = Upper Body
// 0x00000008 = Left Hand
// 0x00000010 = Right Hand
// 0x00000020 = Weapon
// 0x00000040 = PipBoy
// 0x00000080 = Backpack
// 0x00000100 = Necklace
// 0x00000200 = Headband
// 0x00000400 = Hat
// 0x00000800 = Eye Glasses
// 0x00001000 = Nose Ring
// 0x00002000 = Earrings
// 0x00004000 = Mask
// 0x00008000 = Choker
// 0x00010000 = Mouth Object
// 0x00020000 = Body AddOn 1
// 0x00040000 = Body AddOn 2
// 0x00080000 = Body AddOn 3
// -- General flags are additional 8 bits
// 0x01       = ??
// 0x02       = ??
// 0x04       = ??
// 0x08       = ??
// 0x10       = ??
// 0x20       = Power Armor
// 0x40       = Non-Playable
// 0x80       = Heavy
//
// TES5 BODT
// 0x00000001 = Head
// 0x00000002 = Hair
// 0x00000004 = Body
// 0x00000008 = Hands
// 0x00000010 = Forearms
// 0x00000020 = Amulet
// 0x00000040 = Ring
// 0x00000080 = Feet
// 0x00000100 = Calves
// 0x00000200 = Shield
// 0x00000400 = Tail
// 0x00000800 = Long Hair
// 0x00001000 = Circlet
// 0x00002000 = Ears
// 0x00004000 = Body AddOn 3
// 0x00008000 = Body AddOn 4
// 0x00010000 = Body AddOn 5
// 0x00020000 = Body AddOn 6
// 0x00040000 = Body AddOn 7
// 0x00080000 = Body AddOn 8
// 0x00100000 = Decapitate Head
// 0x00200000 = Decapitate
// 0x00400000 = Body AddOn 9
// 0x00800000 = Body AddOn 10
// 0x01000000 = Body AddOn 11
// 0x02000000 = Body AddOn 12
// 0x04000000 = Body AddOn 13
// 0x08000000 = Body AddOn 14
// 0x10000000 = Body AddOn 15
// 0x20000000 = Body AddOn 16
// 0x40000000 = Body AddOn 17
// 0x80000000 = FX01
// -- General flags are separate
static ForeignNpcAnimation::PartBoneMap createPartListMap()
{
    ForeignNpcAnimation::PartBoneMap result;
    result.insert(std::make_pair(ESM::PRT_Head, "Head"));
    result.insert(std::make_pair(ESM::PRT_Hair, "Head")); // note it uses "Head" as attach bone, but "Hair" as filter
    result.insert(std::make_pair(ESM::PRT_Neck, "Neck"));
    result.insert(std::make_pair(ESM::PRT_Cuirass, "Chest"));
    result.insert(std::make_pair(ESM::PRT_Groin, "Groin"));
    result.insert(std::make_pair(ESM::PRT_Skirt, "Groin"));
    result.insert(std::make_pair(ESM::PRT_RHand, "Right Hand"));
    result.insert(std::make_pair(ESM::PRT_LHand, "Left Hand"));
    result.insert(std::make_pair(ESM::PRT_RWrist, "Right Wrist"));
    result.insert(std::make_pair(ESM::PRT_LWrist, "Left Wrist"));
    result.insert(std::make_pair(ESM::PRT_Shield, "Shield Bone"));
    result.insert(std::make_pair(ESM::PRT_RForearm, "Right Forearm"));
    result.insert(std::make_pair(ESM::PRT_LForearm, "Left Forearm"));
    result.insert(std::make_pair(ESM::PRT_RUpperarm, "Right Upper Arm"));
    result.insert(std::make_pair(ESM::PRT_LUpperarm, "Left Upper Arm"));
    result.insert(std::make_pair(ESM::PRT_RFoot, "Right Foot"));
    result.insert(std::make_pair(ESM::PRT_LFoot, "Left Foot"));
    result.insert(std::make_pair(ESM::PRT_RAnkle, "Right Ankle"));
    result.insert(std::make_pair(ESM::PRT_LAnkle, "Left Ankle"));
    result.insert(std::make_pair(ESM::PRT_RKnee, "Right Knee"));
    result.insert(std::make_pair(ESM::PRT_LKnee, "Left Knee"));
    result.insert(std::make_pair(ESM::PRT_RLeg, "Right Upper Leg"));
    result.insert(std::make_pair(ESM::PRT_LLeg, "Left Upper Leg"));
    result.insert(std::make_pair(ESM::PRT_RPauldron, "Right Clavicle")); // used for ear in TES4
    result.insert(std::make_pair(ESM::PRT_LPauldron, "Left Clavicle"));  // used for eye in TES4
    result.insert(std::make_pair(ESM::PRT_Weapon, "Weapon Bone"));
    result.insert(std::make_pair(ESM::PRT_Tail, "Tail"));                // used for tail in TES4
    return result;
}
const ForeignNpcAnimation::PartBoneMap ForeignNpcAnimation::sPartList = createPartListMap();

ForeignNpcAnimation::~ForeignNpcAnimation()
{
    if (!mListenerDisabled
            // No need to getInventoryStore() to reset, if none exists
            // This is to avoid triggering the listener via ensureCustomData()->autoEquip()->fireEquipmentChanged()
            // all from within this destructor. ouch!
           && mPtr.getRefData().getCustomData())
        mPtr.getClass().getInventoryStore(mPtr).setListener(NULL, mPtr);
}


ForeignNpcAnimation::ForeignNpcAnimation(const MWWorld::Ptr& ptr, Ogre::SceneNode* node, int visibilityFlags, bool disableListener, bool disableSounds, ViewMode viewMode)
  : Animation(ptr, node),
    mListenerDisabled(disableListener),
    mViewMode(viewMode),
    mShowWeapons(false),
    mShowCarriedLeft(true),
    mNpcType(Type_Normal),
    mVisibilityFlags(visibilityFlags),
    mFirstPersonOffset(0.f, 0.f, 0.f),
    mAlpha(1.f),
    mSoundsDisabled(disableSounds),
    mHeadYaw(0.f),
    mHeadPitch(0.f)
{
    mNpc = mPtr.get<ESM4::Npc>()->mBase;
    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    mRace = store.getForeign<ESM4::Race>().search(mNpc->mRace); // WARN: might be null

    mHeadAnimationTime = Ogre::SharedPtr<ForeignHeadAnimationTime>(new ForeignHeadAnimationTime(mPtr));
    mWeaponAnimationTime = Ogre::SharedPtr<WeaponAnimationTime>(new WeaponAnimationTime(this));

    // FIXME for foreign
    for(size_t i = 0;i < ESM::PRT_Count;i++)
    {
        mPartslots[i] = -1;  //each slot is empty
        mPartPriorities[i] = 0;
    }

    updateNpcBase();

    if (!disableListener)
        mPtr.getClass().getInventoryStore(mPtr).setListener(this, mPtr);
}

void ForeignNpcAnimation::setViewMode(ForeignNpcAnimation::ViewMode viewMode)
{
    assert(viewMode != VM_HeadOnly);
    if(mViewMode == viewMode)
        return;

    mViewMode = viewMode;
    rebuild();
}

void ForeignNpcAnimation::rebuild()
{
    updateNpcBase();

    MWBase::Environment::get().getMechanicsManager()->forceStateUpdate(mPtr);
}

// clearAnimSources() - FIXME may be need to cache them rather than clearing each time?
// get the npc model based on race, sex, etc
// setObjectRoot() - use the model to setup:
//    NifOgre::ObjectScenePtr mObjectRoot and Ogre::Entity *mSkelBase
// addAnimSource() - FIXME need to figure out how
// updateParts() - get the correct body parts based on inventory
//
// NOTE: maybe updateParts needs to come before updating the NPC models - this is due to the high
// cost of updating FaceGen models for head, hair, eyes, ears, etc.  Or at least consider if the
// relevant slot is already occupied by the equipped items.
void ForeignNpcAnimation::updateNpcBase()
{
    if (!mNpc || !mRace)
        return;

    clearAnimSources(); // clears *all* animations

    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();

    std::string skeletonModel;
    if (mNpc->mModel.empty() && mNpc->mBaseTemplate != 0) // TES5
    {
        uint32_t type = store.find(mNpc->mBaseTemplate);

        const ESM4::LeveledActor* lvlActor = store.getForeign<ESM4::LeveledActor>().search(mNpc->mBaseTemplate);
        if (type == MKTAG('_', 'N', 'P', 'C'))
        {
            if ((mNpc->mActorBaseConfig.flags & 0x1) != 0) // female
                skeletonModel = "meshes\\" + mRace->mModelFemale; // TODO: check if this can be empty
            else
                skeletonModel = "meshes\\" + mRace->mModelMale;
        }
        else
            return;
    }
    else if (!mNpc->mModel.empty()) // TES4
    {
        // Characters\_Male\skeleton.nif
        // Characters\_Male\skeletonbeast.nif
        // Characters\_Male\skeletonsesheogorath.nif
        skeletonModel = "meshes\\" + mNpc->mModel;
    }
    mBodyPartModelNameExt =  bodyPartNameExt(skeletonModel);


    if (mInsert->getScale().x != 1.0f) // WARN: assumed uniform scaling
        std::cout << "scale not 1.0 " << skeletonModel << std::endl;


    //skeletonModel = Misc::ResourceHelpers::correctActorModelPath(skeletonModel);
    setObjectRoot(skeletonModel, true);
    if (mObjectRoot->mForeignObj)
    {
        mObjectRoot->mSkelBase = mObjectRoot->mForeignObj->mSkeletonRoot;
        mSkelBase = mObjectRoot->mForeignObj->mSkeletonRoot;
    }
    if (mSkelBase == nullptr) // FIXME: FO3
        return;

    mSkelBase->getSkeleton()->reset(true); // seems to fix the twisted torso
    //mSkelBase->getMesh()->getSkeleton()->reset(true); // but this doesn't work

    //Ogre::Bone *fBone = mSkelBase->getSkeleton()->getBone("Bip01 L Forearm");
    //Ogre::Bone *tBone = mSkelBase->getSkeleton()->getBone("Bip01 L ForeTwist");
    //Ogre::Quaternion q = fBone->getOrientation();
    //tBone->setOrientation(q);//Ogre::Quaternion::IDENTITY);
    //tBone->setPosition(Ogre::Vector3(0.f,0.f, 0.f));
    //q.w = 0.f;
    //tBone->setOrientation(fBone->getOrientation());
    //fBone = mSkelBase->getSkeleton()->getBone("Bip01 R Forearm");
    //tBone = mSkelBase->getSkeleton()->getBone("Bip01 R ForeTwist");
    //tBone->setOrientation(fBone->getOrientation());
        //tBone->rotate(Ogre::Quaternion(Ogre::Degree(-45), Ogre::Vector3::UNIT_Z) ,Ogre::Node::TS_WORLD);


    // Animation at 90 deg issue:
    //
    // Apparently the TES4 engine does things a little differently to what the NIF fies suggest.  See:
    // https://forums.nexusmods.com/index.php?/topic/278149-animation-rotates-90-degrees-to-the-right/
    // http://wiki.tesnexus.com/index.php?title=How_to_fix_your_animation_FAQ
    // http://wiki.tesnexus.com/index.php/Avoiding_Blender_animation_pitfalls
    //
    // A quick workaround might be two swap the rotation values of "Bip01" and "Bip01 NonAccum".
    // Note that simple swapping causes walk forward animation to walk backwards.
    //
    // A better one might be not to apply rotations to "Bip01" (only apply to "Bip01 NonAccum")
    // but that doesn't seem to work?!
    Ogre::Bone* b = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip01");
    Ogre::Bone* bna = mObjectRoot->mSkelBase->getSkeleton()->getBone("Bip01 NonAccum");
    if (b && bna)
    {
        //Ogre::Quaternion qb = b->getOrientation(); // skeleton.nif has -90 roll
        //Ogre::Quaternion qbna = bna->getOrientation(); // has 0 roll
        //b->setOrientation(qbna);
        //bna->setOrientation(qb);






        // FIXME: all the skinned meshes seems to be offset by this, probably something to do
        // with the binding position
        Ogre::Vector3 vb = b->getPosition();
        Ogre::Vector3 vbna = bna->getPosition();
        Ogre::Vector3 ins = mInsert->getPosition();

        b->setPosition(vbna+ Ogre::Vector3(0,0,0.f)); // TEMP testing for FO3
//      bna->setPosition(vb);

        //Ogre::Bone* rootBone = mObjectRoot->mSkelBase->getSkeleton()->getBone("Scene Root");
        //Ogre::Vector3 vRoot = rootBone->convertLocalToWorldPosition(rootBone->_getDerivedPosition());
        //Ogre::Vector3 vBase = mSkelBase->getParentSceneNode()->_getDerivedPosition();
        //std::cout << vRoot.x << " " << vBase.x << std::endl;
        //std::cout << vRoot.y << " " << vBase.y << std::endl;
        //std::cout << vRoot.z << " " << vBase.z << std::endl;






    }

    if(mViewMode != VM_FirstPerson)
    {
        addAnimSource(skeletonModel);
#if 0
        if(!isWerewolf)
        {
            if(Misc::StringUtils::lowerCase(mNpc->mRace).find("argonian") != std::string::npos)
                addAnimSource("meshes\\xargonian_swimkna.nif");
            else if(!mNpc->isMale() && !isBeast)
                addAnimSource("meshes\\xbase_anim_female.nif");
            if(mNpc->mModel.length() > 0)
                addAnimSource("meshes\\x"+mNpc->mModel);
        }
#endif
    }
    else
    {
        bool isFemale = (mNpc->mBaseConfig.flags & 0x000001) != 0; // 0x1 means female
#if 0
        if(isWerewolf)
            addAnimSource(skeletonModel);
        else
        {
            /* A bit counter-intuitive, but unlike third-person anims, it seems
             * beast races get both base_anim.1st.nif and base_animkna.1st.nif.
             */
            addAnimSource("meshes\\xbase_anim.1st.nif");
            if(isBeast)
                addAnimSource("meshes\\xbase_animkna.1st.nif");
            if(isFemale && !isBeast)
                addAnimSource("meshes\\xbase_anim_female.1st.nif");
        }
#endif
    }

    //MWRender::Animation
    // Ogre::Entity    *mSkelBase
    // Ogre::SceneNode *mInsert
    // ObjectScenePtr   mObjectRoot
    // ?? reuse mInsert?
    std::string group("General");
    std::string modelName;

    // Slots for Bipid Object (from the Construction Set) - may occupy more than one slot (e.g. Robe)
    // See Armor::mArmorFlags and Clothing::mClothingFlags.  There are total 16 slots.
    //
    //     Head
    //     Hair
    //     UpperBody
    //     LowerBody
    //     Hand
    //     Foot
    //     RightRing
    //     LeftRing
    //     Amulet
    //     Weapon
    //     BackWeapon
    //     SideWeapon
    //     Quiver
    //     Shield
    //     Torch
    //     Tail
    //
    // Loop through to select the wearable items with the most value for a given slot.
    // Probably only Armor, Clothing and Weapon.
    //
    // FIXME: not sure how to compare items that have more than one slot?
    // One possibility might be to sort the inventory in value order and equip the items while
    // checking for slot clashes.
    //
    // This item equipping algorithm should be encapsulated as it is likely to have a large bearing
    // on the gameplay (and whether we emulate vanilla closely).
    //
    // TODO: NPC scripts may disallow some items to be equipped (TODO: confirm this)

    // FIXME: for testing only
    MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ContainerStoreIterator storeHelmet = inv.getSlot(MWWorld::InventoryStore::Slot_ForeignHair);
    std::uint16_t slot;
    for (size_t i = 0; i < mNpc->mInventory.size(); ++i)
    {
        // check flag for hide rings/amulet to skip
    }

    bool isTES4 = true;
    bool isFemale = (mNpc->mBaseConfig.flags & 0x1) != 0;
    std::string meshName;
    std::string textureName;

    FgLib::FgSam sam;

    const std::vector<float>& sRaceCoeff = mRace->mSymShapeModeCoefficients;
    const std::vector<float>& aRaceCoeff = mRace->mAsymShapeModeCoefficients;
    const std::vector<float>& sRaceTCoeff = mRace->mSymTextureModeCoefficients;
    const std::vector<float>& sCoeff = mNpc->mSymShapeModeCoefficients;
    const std::vector<float>& aCoeff = mNpc->mAsymShapeModeCoefficients;
    const std::vector<float>& sTCoeff = mNpc->mSymTextureModeCoefficients;

    // Hair is not considered as a "head part".
    // check NiObjectNET::mExtraDataIndexList (NiStringExtraData) for bone to attach
    if (storeHelmet == inv.end())
    {
        // FIXME: first check if the hair resource for this NPC has been created already
        // MeshManager and TextureManager or nimodelmanager?
        modelName = mNpc->mEditorId + "_Hair";
        //NiModelPtr hairModel = NiBtOgre::NiModelManager::getSingleton().getOrLoadByName(modelName, group);






        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const ESM4::Hair *hair = store.getForeign<ESM4::Hair>().search(mNpc->mHair);
        if (!hair)
        {
            // try to use the Race defaults
            hair = store.getForeign<ESM4::Hair>().search(mRace->mDefaultHair[isFemale ? 1 : 0]);
            if (!hair)
                throw std::runtime_error("Hair record not found.");
        }

        meshName = "meshes\\"+hair->mModel;
        textureName = "textures\\"+hair->mIcon;

        std::unique_ptr<std::vector<Ogre::Vector3> > fgVertices // NOTE: ownership passed to the model
            = std::make_unique<std::vector<Ogre::Vector3> >();

        sam.getMorphedVertices(fgVertices.get(), meshName, sRaceCoeff, aRaceCoeff, sCoeff, aCoeff);

        // FIXME
        //sam.getMorphedTexture(xxx, meshName, sRaceCoeff, aRaceCoeff, sCoeff, aCoeff);

        NifOgre::ObjectScenePtr sceneHair = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        sceneHair->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));

        sceneHair->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mNpc->mEditorId, std::move(fgVertices));

        Ogre::Bone *heBone = mSkelBase->getSkeleton()->getBone("Bip01 Head");
        Ogre::Quaternion heOrientation = heBone->getOrientation() *
                               Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y); // fix helmet issue
        //Ogre::Vector3 hePosition = Ogre::Vector3(/*up*/0.2f, /*forward*/1.7f, 0.f); // FIXME non-zero for FO3 to make hair fit better
        //Ogre::Vector3 hePosition = Ogre::Vector3(0.5f, -0.2f, 0.f);
        Ogre::Vector3 hePosition = Ogre::Vector3(0.f, -0.2f, 0.f);
        std::map<int32_t, Ogre::Entity*>::const_iterator it(sceneHair->mForeignObj->mEntities.begin());
        for (; it != sceneHair->mForeignObj->mEntities.end(); ++it)
        {



#if 1
            Ogre::MaterialPtr mat = sceneHair->mMaterialControllerMgr.getWritableMaterial(it->second);
            Ogre::Material::TechniqueIterator techIter = mat->getTechniqueIterator();
            while(techIter.hasMoreElements())
            {
                Ogre::Technique *tech = techIter.getNext();
                Ogre::Technique::PassIterator passes = tech->getPassIterator();
                while(passes.hasMoreElements())
                {
                    Ogre::Pass *pass = passes.getNext();
                    //Ogre::TextureUnitState *tex = pass->getTextureUnitState(0);
                    //tex->setColourOperation(Ogre::LBO_ALPHA_BLEND);
                    //tex->setColourOperation(Ogre::LBO_REPLACE);
                    //tex->setBlank(); // FIXME: testing
#if 1
                    Ogre::ColourValue ambient = pass->getAmbient();
                    ambient.r = (float)mNpc->mHairColour.red / 256.f;
                    ambient.g = (float)mNpc->mHairColour.green / 256.f;
                    ambient.b = (float)mNpc->mHairColour.blue / 256.f;
                    ambient.a = 1.f;
                    pass->setSceneBlending(Ogre::SBT_REPLACE);
                    pass->setAmbient(ambient);
                    pass->setVertexColourTracking(pass->getVertexColourTracking() &~Ogre::TVC_AMBIENT);
#endif
#if 0
                    Ogre::ColourValue diffuse = pass->getDiffuse();
                    diffuse.r = float((float)mNpc->mHairColour.red / 256);
                    diffuse.g = float((float)mNpc->mHairColour.green / 256);
                    diffuse.b = float((float)mNpc->mHairColour.blue / 256);
                    diffuse.a = 1.f;// (float)mNpc->mHairColour.custom / 256.f; //0.f;
                    pass->setSceneBlending(Ogre::SBT_REPLACE);
                    pass->setDiffuse(diffuse);
                    pass->setVertexColourTracking(pass->getVertexColourTracking() &~Ogre::TVC_DIFFUSE);
#endif
                }
            }
#endif




            mSkelBase->attachObjectToBone("Bip01 Head", it->second, heOrientation, hePosition);
        }
        mObjectParts[ESM::PRT_Hair] = sceneHair;
    }

    for (int index = ESM4::Race::Head; index < ESM4::Race::NumHeadParts; ++index)
    {
        // skip 2 if male, skip 1 if female (ears)
        if ((isFemale && index == ESM4::Race::EarMale) || (!isFemale && index == ESM4::Race::EarFemale))
            continue;

        // skip ears if wearing a helmet - check for the head slot
        if ((index == ESM4::Race::EarMale || index == ESM4::Race::EarFemale) && (storeHelmet != inv.end()))
            continue;

        // FIXME: skip mouth, teeth (upper/lower) and tongue for now
        if (index >= ESM4::Race::Mouth && index <= ESM4::Race::Tongue)
            continue;

        meshName = "meshes\\"+mRace->mHeadParts[index].mesh;

        // Get mesh and texture names from RACE except eye textures which are specified in
        // Npc::mEyes formid (NOTE: Oblivion.esm NPC_ records all have valid mEyes formid)
        if (index == ESM4::Race::EyeLeft || index == ESM4::Race::EyeRight)
        {
            const ESM4::Eyes* eyes = store.getForeign<ESM4::Eyes>().search(mNpc->mEyes);
            if (!eyes)
                continue; // FIXME: the character won't have any eyes, should throw?

            textureName = "textures\\"+eyes->mIcon;
        }
        else
            textureName = "textures\\"+mRace->mHeadParts[index].texture;

        // TODO: if the texture doesn't exist, then grab it from the mesh (but shouldn't happen, so
        // log an error before proceeding with the fallback)




        // FaceGen:
        // dependency: model and texture files found
        //
        // Find the corresponding EGM file in the same directory as the NIF file. If it doesn't
        // exist, log an error and abandon any morphs for this mesh.
        //
        // Find the corresponding TRI file in the same directory as the NIF file. In a few cases
        // they don't exist so construct a dummy one from the NIF file.
        //
        // Morph the vertices in the TRI file using the RACE and NPC_ morph coefficients and the EGM
        // file.
        //
        // Find the corresponding EGT file in same directory as the NIF file. If it doesn't
        // exist, log an error and abandon any morphs for this texture.
        //
        // FIXME: detail modulation, need to find the age from NPC_ symmetric morph coefficients.
        //
        // Morph the texture using the NPC_ morph coefficients, detail modulation and the EGT file.
        //
        // Find the detai map texture from "textures\\faces\\oblivion.esm\\" for the Npc::mFormId.
        //
        // Create the object using the morphed vertices, morphed texture and the detail map.
        //
        // FIXME: not sure what to do with si.ctl
        //
        // TODO: to save unnecessary searches for the resources, these info should be persisted

        // NOTE: morphed mesh and texture need to be treated as a "resource" and named with NPC's
        // editorId - the base headhuman.nif, for example, should not be modified

    }


    meshName = "meshes\\"+mRace->mHeadParts[0/*head*/].mesh;
    if (meshName.empty())
    {
        if (mRace->mEditorId == "Imperial" || mRace->mEditorId == "Nord"     ||
            mRace->mEditorId == "Breton"   || mRace->mEditorId == "Redguard" ||
            mRace->mEditorId == "HighElf"  || mRace->mEditorId == "DarkElf"  ||
            mRace->mEditorId == "WoodElf"  || mRace->mEditorId == "Dremora")
            meshName = "meshes\\Characters\\Imperial\\headhuman.nif";
        else if (mRace->mEditorId == "Argonian")
            meshName = "meshes\\Characters\\Argonian\\headargonian.nif";
        else if (mRace->mEditorId == "Orc")
            meshName = "meshes\\Characters\\Orc\\headorc.nif";
        else if (mRace->mEditorId == "Khajiit")
            meshName = "meshes\\Characters\\Khajiit\\headkhajiit.nif";
        else if (0) // FO3
        {
            isTES4 = false;
            isFemale = (mNpc->mActorBaseConfig.flags & 0x1) != 0;

            // FIXME: can be female, ghoul, child, old, etc
            if (mRace->mEditorId.find("Old") != std::string::npos)
            {
                if (isFemale)
                    meshName = "meshes\\Characters\\head\\headold.nif";
                else
                    meshName = "meshes\\Characters\\head\\headoldfemale.nif";
            }
            else if (mRace->mEditorId.find("Child") != std::string::npos)
            {
                if (isFemale)
                    meshName = "meshes\\Characters\\head\\headchildfemale.nif";
                else
                    meshName = "meshes\\Characters\\head\\headchild.nif";
            }
            else
            {
                if (isFemale)
                    meshName = "meshes\\Characters\\head\\headhumanfemale.nif";
                else
                    meshName = "meshes\\Characters\\head\\headhuman.nif";
            }
            // FO3 races
            // CaucasianOldAged
            // AfricanAmericanOldAged
            // AsianOldAged
            // HispanicOldAged
            // AfricanAmericanRaider
            // AsianRaider
            // HispanicRaider
            // CaucasianRaider
            // TestQACaucasian
            // HispanicOld
            // HispanicChild
            // CaucasianOld
            // CaucasianChild
            // AsianOld
            // AsianChild
            // AfricanAmericanOld
            // AfricanAmericanChild
            // AfricanAmerican
            // Ghoul
            // Asian
            // Hispanic
            // Caucasian
            //std::cout << "missing head " << mRace->mEditorId << std::endl;
            //else
                //return;
        }
    }

    size_t pos = meshName.find_last_of("nif");
    if (pos == std::string::npos)
        return ; // FIXME: should throw here

    // FIXME: use resource manager here to stop loading the same file multiple times
    //std::string path = Misc::StringUtils::lowerCase(meshName.substr(0, pos-2));
    //FgLib::FgCtl ctl("facegen\\si.ctl", "General");
    //ctl.loadImpl();
    //FgLib::FgSam sam;

    //const std::vector<float>& sRaceCoeff = mRace->mSymShapeModeCoefficients;
    //const std::vector<float>& aRaceCoeff = mRace->mAsymShapeModeCoefficients;
    //const std::vector<float>& sRaceTCoeff = mRace->mSymTextureModeCoefficients;
    //const std::vector<float>& sCoeff = mNpc->mSymShapeModeCoefficients;
    //const std::vector<float>& aCoeff = mNpc->mAsymShapeModeCoefficients;
    //const std::vector<float>& sTCoeff = mNpc->mSymTextureModeCoefficients;

    Ogre::Vector3 sym;
    Ogre::Vector3 asym;
    // NiModel is an Ogre::Resource
    //
    // FIXME:
    // - a resource may be unloaded, which means that it needs to be re-morphed when loaded again
    // - also, each NPC's head model may be unique, which means they need to be managed separately
    //   once morphed
    // - a solution might be to have a "MorphedModelManager" which will call FgSam as required
    //
    // head, ears, eyes left, eyes right, hair, teethupper, teeth lower, tongue, mouth
    // i.e. all the "head parts" and hair
    //
    // also, a few have egt but not egm (but these apply to all npcs in the same race?)
    //
#if 0
    // getOrLoadByName calls getResourceByName
    // mNpc is ESM4::Npc*
    NiModelPtr headModel = NiBtOgre::NiModelManager::getSingleton().getOrLoadByName(meshName, "General", mNpc);
    if (!headModel->hasMorphedVertices())
    {
        std::unique_ptr<std::vector<Ogre::Vector3> >
                fgVertices = std::make_unique<std::vector<Ogre::Vector3> >();
        sam.getMorphedVertices(fgVertices.get(), meshName, sRaceCoeff, aRaceCoeff, sCoeff, aCoeff);

        headModel->setVertices(std::move(fgVertices));
    }
    else
    {
        // have another BtOgreInst ctor to pass the NiModelPtr to remove the need to search for the model
        // WARN: assumed morphed texture is not possible without morphed vertices
    }

#endif



    std::unique_ptr<std::vector<Ogre::Vector3> > // NOTE: ownership passed to the head model
        fgVertices = std::make_unique<std::vector<Ogre::Vector3> >();

    sam.getMorphedVertices(fgVertices.get(), meshName, sRaceCoeff, aRaceCoeff, sCoeff, aCoeff);

    // FIXME: need to be able to check other than oblivion.esm
    std::string textureFile = "textures\\faces\\oblivion.esm\\"+ESM4::formIdToString(mNpc->mFormId)+"_0.dds";
    if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(textureFile))
        std::cout << mNpc->mEditorId << " does not have a facegen texture" << std::endl;
    else
        std::cout << mNpc->mEditorId << " detail " << ESM4::formIdToString(mNpc->mFormId) << std::endl;

    NifOgre::ObjectScenePtr scene = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
    scene->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), /*scene, */meshName, group));

    // IDEA: manage morphed vertices as a resource? Similar to having morphed textures managed by
    // Ogre::TextureManager.  That way if the same NPC is needed for another scene we don't need to
    // re-create the morph.
    //
    // Alternatively, since the NiModel for that NPC's head should be managed already, we don't need
    // to manage the vertices separately?

    // TODO: is it possible to get the texture name here or should it be hard coded?
    scene->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mNpc->mEditorId, std::move(fgVertices));


    // get the texture from mRace
    // FIXME: for now, get if from Ogre material

    std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
    for (; it != scene->mForeignObj->mEntities.end(); ++it)
    {
        Ogre::MaterialPtr mat = scene->mMaterialControllerMgr.getWritableMaterial(it->second);
        Ogre::Material::TechniqueIterator techIter = mat->getTechniqueIterator();
        while(techIter.hasMoreElements())
        {
            Ogre::Technique *tech = techIter.getNext();
            //tech->removeAllPasses();
            Ogre::Technique::PassIterator passes = tech->getPassIterator();
            while(passes.hasMoreElements())
            {
                Ogre::Pass *pass = passes.getNext();
                Ogre::TextureUnitState *tus = pass->getTextureUnitState(0);

                Ogre::PixelFormat pixelFormat = tus->_getTexturePtr()->getFormat();

                FgLib::FgFile<FgLib::FgEgt> egtFile;
                const FgLib::FgEgt *egt = egtFile.getOrLoadByName(meshName);

                // From: http://wiki.ogre3d.org/Creating+dynamic+textures
                // Create a target texture
                Ogre::TexturePtr texFg = Ogre::TextureManager::getSingleton().getByName(
                       "FaceGen"+mNpc->mEditorId, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                if (texFg.isNull())
                {
                    texFg = Ogre::TextureManager::getSingleton().createManual(
                        "FaceGen"+mNpc->mEditorId, // name
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        Ogre::TEX_TYPE_2D,  // type
                        egt->numRows(), egt->numColumns(), // width & height
                        0,                  // number of mipmaps; FIXME: should be 2? or 1?
                        Ogre::PF_BYTE_RGBA,
                        Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                            // textures updated very often (e.g. each frame)
                }

                Ogre::TexturePtr texFg2 = Ogre::TextureManager::getSingleton().getByName(
                       "FaceGen"+mNpc->mEditorId+"2", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                if (texFg2.isNull())
                {
                    texFg2 = Ogre::TextureManager::getSingleton().createManual(
                        "FaceGen"+mNpc->mEditorId+"2", // name
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                        Ogre::TEX_TYPE_2D,  // type
                        egt->numRows(), egt->numColumns(), // width & height
                        0,                  // number of mipmaps; FIXME: should be 2? or 1?
                        Ogre::PF_BYTE_RGBA,
                        Ogre::TU_DEFAULT);  // usage; should be TU_DYNAMIC_WRITE_ONLY_DISCARDABLE for
                                            // textures updated very often (e.g. each frame)
                }

                if (mNpc->mEditorId == "UrielSeptim")
                    textureFile = "textures\\characters\\imperial\\headhumanm60.dds"; // male
                else if (mNpc->mEditorId == "Rohssan")
                    textureFile = "textures\\characters\\imperial\\headhumanf60.dds"; // female
                else if (mNpc->mFormId == 0x0001C458) // BeggarICMarketSimplicia
                    textureFile = "textures\\characters\\imperial\\headhumanf60.dds"; // female

                Ogre::TexturePtr texDetail = Ogre::TextureManager::getSingleton().getByName(
                        textureFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                if (texDetail.isNull())
                {
                    // textureFile is the detailed texture in textures/faces/oblivion.esm/<formid>_.dds
                    texDetail = Ogre::TextureManager::getSingleton().create(
                        textureFile, // name
                        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME
                    );
                    texDetail->load();
                }

                // src: can be 128x128
                Ogre::HardwarePixelBufferSharedPtr pixelBufferSrc = tus->_getTexturePtr()->getBuffer();
                pixelBufferSrc->unlock(); // prepare for blit()

                // dest: usually 256x256 (egt.numRows()*egt.numColumns())
                Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texFg->getBuffer();
                pixelBuffer->unlock(); // prepare for blit()

                Ogre::HardwarePixelBufferSharedPtr pixelBuffer2 = texFg2->getBuffer();
                pixelBuffer2->unlock();

                // if source and destination dimensions don't match, scaling is done
                pixelBuffer->blit(pixelBufferSrc);

                // Lock the pixel buffer and get a pixel box
                pixelBufferSrc->lock(Ogre::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!
                pixelBuffer->lock(Ogre::HardwareBuffer::HBL_NORMAL); // for best performance use HBL_DISCARD!

                const Ogre::PixelBox& pixelBoxSrc = pixelBufferSrc->getCurrentLock();
                const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();

                uint8_t *pDest = static_cast<uint8_t*>(pixelBox.data);

                Ogre::HardwarePixelBufferSharedPtr pixelBufferDetail = texDetail->getBuffer();
                if (pixelBufferDetail.isNull())
                    std::cout << "detail texture null" << std::endl;

                pixelBufferDetail->unlock();
                pixelBuffer2->blit(pixelBufferDetail);

                pixelBuffer2->lock(Ogre::HardwareBuffer::HBL_NORMAL);
                pixelBufferDetail->lock(Ogre::HardwareBuffer::HBL_NORMAL);
                const Ogre::PixelBox& pixelBoxDetail = pixelBuffer2->getCurrentLock();
                uint8_t *pDetail = static_cast<uint8_t*>(pixelBoxDetail.data);
                if (!pDetail)
                    std::cout << "null detail" << std::endl;

                // update the pixels with SCM and detail texture
                // NOTE: mSymTextureModes is assumed to have the image pixels in row-major order
                for (size_t i = 0; i < egt->numRows()*egt->numColumns(); ++i) // height*width, should be 256*256
                {
                    // FIXME: for some reason adding the race coefficients makes it look worse
                    //        even though it is clear that for shapes they are needed
                    // sum all the symmetric texture modes for a given pixel i
                    sym = Ogre::Vector3::ZERO; // WARN: sym reused
                    for (size_t j = 0; j < 50/*mNumSymTextureModes*/; ++j)
                        sym += (sRaceTCoeff[j] + sTCoeff[j]) * egt->mSymTextureModes[50*i + j];

                    // Detail texture is applied after reconstruction of the colour map from the SCM.
                    // Using an average of the 3 colors makes the resulting texture less blotchy. Also see:
                    // "Each such factor is coded as a single unsigned byte in the range [0,255]..."
                    int t = *(pDetail+0) + *(pDetail+1) + *(pDetail+2);
                    float ft = t/192.f; // 64 * 3 = 192

                    int r = *(pDetail+0);
                    float fr = r/64.f;
                    fr = ft;
                    if (mNpc->mEditorId != "UrielSeptim" && mNpc->mEditorId != "Rohssan" &&
                        mNpc->mFormId != 0x0001C458)
                        fr = 1.f; // ignore age for now
                    r = std::min(int((*(pDest+0)+sym.x) * fr), 255);
                    if (mNpc->mEditorId != "UrielSeptim" && mNpc->mEditorId != "Rohssan" &&
                        mNpc->mFormId != 0x0001C458)
                        r = std::min(int(*(pDetail+0) * 2 *r /255.f), 255);

                    int g = *(pDetail+1);
                    float fg = g/64.f;
                    fg = ft;
                    if (mNpc->mEditorId != "UrielSeptim" && mNpc->mEditorId != "Rohssan" &&
                        mNpc->mFormId != 0x0001C458)
                        fg = 1.f;
                    g = std::min(int((*(pDest+1)+sym.y) * fg), 255);
                    if (mNpc->mEditorId != "UrielSeptim" && mNpc->mEditorId != "Rohssan" &&
                        mNpc->mFormId != 0x0001C458)
                        g = std::min(int(*(pDetail+1) * 2 * g /255.f), 255);

                    int b = *(pDetail+2);
                    float fb = b/64.f;
                    fb = ft;
                    if (mNpc->mEditorId != "UrielSeptim" && mNpc->mEditorId != "Rohssan" &&
                        mNpc->mFormId != 0x0001C458)
                        fb = 1.f;
                    b = std::min(int((*(pDest+2)+sym.z) * fb), 255);
                    if (mNpc->mEditorId != "UrielSeptim" && mNpc->mEditorId != "Rohssan" &&
                        mNpc->mFormId != 0x0001C458)
                        b = std::min(int(*(pDetail+2) * 2 * b /255.f), 255);

#if 0 // {{{
                    // Verify the byte order for PF_BYTE_RGBA using Ogre::PixelBox::getColourAt()
                    // NOTE: probably only works for this particular host (e.g. little endian)
                    if (mNpc->mEditorId == "UrielSeptim") // CoC "ImperialDungeon01"
                    {
                        int ir = (int)i / 256;
                        int ic = i % 256;

                        //      dest, sym 0 0
                        //      198.000 63.674 255.000
                        //      142.000 56.210 201.000
                        //      107.000 51.185 163.000
                        //      225.000
                        //      198 142 107 225.000
                        //      dest, sym 0 1
                        //      197.000 63.674 255.000
                        //      142.000 56.210 201.000
                        //      107.000 51.185 163.000
                        //      229.000
                        //      198 142 107 230.000
                        //      dest, sym 0 2
                        //      198.000 63.674 255.000
                        //      142.000 56.210 201.000
                        //      107.000 51.185 163.000
                        //      237.000
                        //      198 142 107 240.000
                        //      dest, sym 0 3
                        //      197.000 63.674 255.000
                        //      142.000 56.210 201.000
                        //      107.000 51.185 163.000
                        //      240.000
                        //      198 142 107 243.000
                        //      dest, sym 0 4
                        //      198.000 63.674 255.000
                        //      142.000 56.210 201.000
                        //      107.000 51.185 163.000
                        //      241.000
                        //      198 142 107 242.000
                        //      dest, sym 0 5
                        // R -> 197.000 63.674 255.000
                        // G -> 142.000 56.210 201.000
                        // B -> 107.000 51.185 163.000
                        // A -> 240.000
                        //      198 142 107 240.000
                        //       ^   ^   ^   ^
                        //       |   |   |   |
                        //       R   G   B   A
                        std::cout << std::fixed << std::setprecision(3)
                            << "dest, sym " << ir << " " << ic << " " << std::endl;
                        std::cout << float(*(pDest+0)) << " " << sym.x << " " << float(r) << std::endl;
                        std::cout << float(*(pDest+1)) << " " << sym.y << " " << float(g) << std::endl;
                        std::cout << float(*(pDest+2)) << " " << sym.z << " " << float(b) << std::endl;
                        std::cout << float(*(pDest+3)) << std::endl;
                        Ogre::ColourValue cv = pixelBox.getColourAt(ir, ic, 0); // [0.0..1.0]
                        cv *= 255;                                              // [0..255]
                        std::cout << std::dec <<
                            (int)cv.r << " " << int(cv.g) << " " << int(cv.b) << " " << cv.a << std::endl;
                    }

                    if (mNpc->mEditorId == "UrielSeptim") // press <F12> to get a screenshot
                    {
                        std::cout << std::fixed << std::setprecision(3) << "detail " << i << " | " <<
                            *(pDetail+0)/64.f << " " <<
                            *(pDetail+1)/64.f << " " <<
                            *(pDetail+2)/64.f << " " <<
                            *(pDetail+3)/1.f << std::endl;
                    }
#endif // }}}

                    *(pDest+0) = r;
                    *(pDest+1) = g;
                    *(pDest+2) = b;
                    pDest += 4;
                    pDetail += 4;
                }

                // Unlock the pixel buffers
                pixelBufferSrc->unlock();
                pixelBuffer->unlock();
                pixelBuffer2->unlock();

                pass->removeTextureUnitState(0);
                Ogre::TextureUnitState *newTUS = pass->createTextureUnitState("FaceGen" + mNpc->mEditorId);

            } // while pass
        } // while technique

        it->second->shareSkeletonInstanceWith(mSkelBase);
        mInsert->attachObject(it->second);
    }
    mObjectParts[ESM::PRT_Head] = scene;

#if 0
    if (mRace->mEditorId == "Imperial" || mRace->mEditorId == "Nord" ||
        mRace->mEditorId == "Breton"   || mRace->mEditorId == "Redguard" ||
        mRace->mEditorId == "HighElf"  || mRace->mEditorId == "DarkElf"  || mRace->mEditorId == "WoodElf")
    {
        meshName = "meshes\\Characters\\Imperial\\eyerighthuman.nif";
        NifOgre::ObjectScenePtr scene = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        scene->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        scene->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton());

        Ogre::Bone *heBone = mSkelBase->getSkeleton()->getBone("Bip01 Head");
        Ogre::Quaternion heOrientation = heBone->getOrientation() *
                               Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y); // fix helmet issue
        Ogre::Vector3 hePosition = Ogre::Vector3(0.4, -0.3, 0); // FIXME: make eyes fit better

        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            mSkelBase->attachObjectToBone("Bip01 Head", it->second, heOrientation, hePosition);
        }
        mObjectParts[ESM::PRT_RPauldron] = scene;

        meshName = "meshes\\Characters\\Imperial\\eyelefthuman.nif";
        NifOgre::ObjectScenePtr sceneL = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        sceneL->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        sceneL->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton());

        std::map<int32_t, Ogre::Entity*>::const_iterator itL(sceneL->mForeignObj->mEntities.begin());
        for (; itL != sceneL->mForeignObj->mEntities.end(); ++itL)
        {
            mSkelBase->attachObjectToBone("Bip01 Head", itL->second, heOrientation, hePosition);
        }
        mObjectParts[ESM::PRT_LPauldron] = sceneL;
    }
    else if (mRace->mEditorId == "Argonian")
    {
        meshName = "meshes\\Characters\\Argonian\\eyerightargonian.nif";
        NifOgre::ObjectScenePtr scene = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        scene->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        scene->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton());

        Ogre::Bone *heBone = mSkelBase->getSkeleton()->getBone("Bip01 Head");
        Ogre::Quaternion heOrientation = heBone->getOrientation() *
                               Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y); // fix helmet issue
        Ogre::Vector3 hePosition = Ogre::Vector3(0.4, -0.3, 0); // FIXME: make eyes fit better

        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            mSkelBase->attachObjectToBone("Bip01 Head", it->second, heOrientation, hePosition);
        }
        mObjectParts[ESM::PRT_RPauldron] = scene;

        meshName = "meshes\\Characters\\Argonian\\eyeleftargonian.nif";
        NifOgre::ObjectScenePtr sceneL = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        sceneL->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        sceneL->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton());

        std::map<int32_t, Ogre::Entity*>::const_iterator itL(sceneL->mForeignObj->mEntities.begin());
        for (; itL != sceneL->mForeignObj->mEntities.end(); ++itL)
        {
            mSkelBase->attachObjectToBone("Bip01 Head", itL->second, heOrientation, hePosition);
        }
        mObjectParts[ESM::PRT_LPauldron] = sceneL;

        meshName = "meshes\\Characters\\Argonian\\tail.nif";
        NifOgre::ObjectScenePtr sceneT = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        sceneT->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        sceneT->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton());

        std::map<int32_t, Ogre::Entity*>::const_iterator itT(sceneT->mForeignObj->mEntities.begin());
        for (; itT != sceneT->mForeignObj->mEntities.end(); ++itT)
        {
            // FIXME:
            if (mSkelBase->getMesh()->getSkeleton() == itT->second->getMesh()->getSkeleton())
                itT->second->shareSkeletonInstanceWith(mSkelBase);
            else
                std::cout << "no anim " << mSkelBase->getMesh()->getName()
                    << itT->second->getMesh()->getName() << std::endl;
            mInsert->attachObject(itT->second);
        }
        mObjectParts[ESM::PRT_Tail] = sceneT;
    }
    else if (mRace->mEditorId == "Orc")
    {
        meshName = "meshes\\Characters\\Orc\\earsorc.nif";
        NifOgre::ObjectScenePtr scene = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        scene->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        scene->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton());

        Ogre::Bone *heBone = mSkelBase->getSkeleton()->getBone("Bip01 Head");
        // no 90 deg issue with ears
        //Ogre::Quaternion heOrientation = heBone->getOrientation() *
                               //Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y); // fix helmet issue
        //Ogre::Vector3 hePosition = Ogre::Vector3(0.4, -0.3, 0); // FIXME: make eyes fit better

        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            mSkelBase->attachObjectToBone("Bip01 Head", it->second/*, heOrientation, hePosition*/);
        }
        mObjectParts[ESM::PRT_RPauldron] = scene; // FIXME
    }
    else if (mRace->mEditorId == "Khajiit")
    {
        meshName = "meshes\\Characters\\Khajiit\\earskhajiit.nif";
        NifOgre::ObjectScenePtr scene = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        scene->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        scene->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton());

        Ogre::Bone *heBone = mSkelBase->getSkeleton()->getBone("Bip01 Head");
        // no 90 deg issue with ears
        //Ogre::Quaternion heOrientation = heBone->getOrientation() *
                               //Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y); // fix helmet issue
        //Ogre::Vector3 hePosition = Ogre::Vector3(0.4, -0.3, 0); // FIXME: make eyes fit better

        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            mSkelBase->attachObjectToBone("Bip01 Head", it->second/*, heOrientation, hePosition*/);
        }
        mObjectParts[ESM::PRT_RPauldron] = scene; // FIXME

        meshName = "meshes\\Characters\\Khajiit\\khajiittail.nif";
        NifOgre::ObjectScenePtr sceneT = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        sceneT->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        sceneT->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mBodyPartModelNameExt);

        std::map<int32_t, Ogre::Entity*>::const_iterator itT(sceneT->mForeignObj->mEntities.begin());
        for (; itT != sceneT->mForeignObj->mEntities.end(); ++itT)
        {
            // FIXME:
            if (mSkelBase->getMesh()->getSkeleton() == itT->second->getMesh()->getSkeleton())
                itT->second->shareSkeletonInstanceWith(mSkelBase);
            else
                std::cout << "no anim " << mSkelBase->getMesh()->getName()
                    << itT->second->getMesh()->getName() << std::endl;
            mInsert->attachObject(itT->second);
        }
        mObjectParts[ESM::PRT_Tail] = sceneT;
    }
    else if (0)// FO3 FIXME: dremora can hit this block!
    {
        meshName = "meshes\\Characters\\head\\eyelefthuman.nif";
        NifOgre::ObjectScenePtr scene = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        scene->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        scene->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton());

        Ogre::Bone *heBone = mSkelBase->getSkeleton()->getBone("Bip01 Head");
        Ogre::Quaternion heOrientation = heBone->getOrientation() *
                               Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y); // fix helmet issue
        Ogre::Vector3 hePosition = Ogre::Vector3(0.4, -0.3, 0); // FIXME: make eyes fit better

        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            mSkelBase->attachObjectToBone("Bip01 Head", it->second);//, heOrientation, hePosition);
        }
        mObjectParts[ESM::PRT_RPauldron] = scene;

        meshName = "meshes\\Characters\\head\\eyerighthuman.nif";
        NifOgre::ObjectScenePtr sceneL = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        sceneL->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        sceneL->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton());

        std::map<int32_t, Ogre::Entity*>::const_iterator itL(sceneL->mForeignObj->mEntities.begin());
        for (; itL != sceneL->mForeignObj->mEntities.end(); ++itL)
        {
            mSkelBase->attachObjectToBone("Bip01 Head", itL->second);//, heOrientation, hePosition);
        }
        mObjectParts[ESM::PRT_LPauldron] = sceneL;




#if 0
        meshName = "meshes\\clutter\\food\\apple01.nif";
        NifOgre::ObjectScenePtr sceneA = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        std::auto_ptr<NiBtOgre::BtOgreInst> instA(new NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), sceneA, meshName, group));
        instA->instantiate(mSkelBase->getMesh()->getSkeleton());

        //Ogre::Bone *ABone = mSkelBase->getSkeleton()->getBone("Bip01 L Hand");
        mSkelBase->attachObjectToBone("Bip01 L ForeTwist", sceneA->mForeignObj->mEntities[0]);
        mObjectParts[ESM::PRT_RKnee] = sceneA;

        meshName = "meshes\\clutter\\food\\pear01.nif";
        NifOgre::ObjectScenePtr sceneP = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        std::auto_ptr<NiBtOgre::BtOgreInst> instP(new NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), sceneP, meshName, group));
        instP->instantiate(mSkelBase->getMesh()->getSkeleton());

        //Ogre::Bone *ABone = mSkelBase->getSkeleton()->getBone("Bip01 L Hand");
        mSkelBase->attachObjectToBone("Bip01 R ForeTwist", sceneP->mForeignObj->mEntities[0]);
        mObjectParts[ESM::PRT_LKnee] = sceneP;
#endif
    }

    // ------------------------------- EARS ---------------------------------------------------
    {
    //std::cout << mRace->mEditorId << std::endl;
    if (mRace->mEditorId == "Imperial" || mRace->mEditorId == "Nord" ||
        mRace->mEditorId == "Breton"   || mRace->mEditorId == "Redguard")
        meshName = Misc::StringUtils::lowerCase("meshes\\Characters\\Imperial\\earshuman.nif");
    else if (mRace->mEditorId == "HighElf")
        meshName = Misc::StringUtils::lowerCase("meshes\\Characters\\HighElf\\earshighelf.nif");
    else if (mRace->mEditorId == "DarkElf")
        meshName = Misc::StringUtils::lowerCase("meshes\\Characters\\DarkElf\\earsdarkelf.nif");
    else if (mRace->mEditorId == "WoodElf")
        meshName = Misc::StringUtils::lowerCase("meshes\\Characters\\WoodElf\\earswoodelf.nif");

        NifOgre::ObjectScenePtr sceneEar = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        sceneEar->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));

    size_t pos = meshName.find_last_of("nif");
    if (pos == std::string::npos)
        return ; // FIXME: should throw here
    std::string path = meshName.substr(0, pos-2);

    if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(path+"egm") ||
        !Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(path+"tri"))
    {
        sceneEar->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mNpc->mEditorId);
    }
    else
    {

    //NiBtOgre::FgEgm egm(path+"egm", "General");
    //egm.loadImpl();
    NiBtOgre::FgTri tri(path+"tri", "General");
    tri.loadImpl();
    NiBtOgre::FgEgt egt(path+"egt", "General");
    egt.loadImpl();
    const std::vector<Ogre::Vector3>& vertices = tri.getVertices(); // V + K

    const std::vector<float>& sCoeff = mNpc->mSymShapeModeCoefficients;
    const std::vector<float>& aCoeff = mNpc->mAsymShapeModeCoefficients;
    const std::vector<float>& sTCoeff = mNpc->mSymTextureModeCoefficients;

    Ogre::Vector3 sym;
    Ogre::Vector3 asym;
    std::vector<Ogre::Vector3> fgVertices;
    fgVertices.resize(tri.getNumVertices());
    for (size_t i = 0; i < fgVertices.size(); ++i)
    {
#if 0
        sym = Ogre::Vector3::ZERO;
        for (size_t j = 0; j < 50; ++j)
        {
            sym += sCoeff[j] * /*egm.mSymMorphModeScales[j] * */egm.mSymMorphModes[j + 50*i];
//          float factor = coeff[j] * egm.mSymMorphModeScales[j];
//          Ogre::Vector3 vert = egm.mSymMorphModes[j + 50 * i];
//          Ogre::Vector3 newVert = factor * vert;
//          sym += newVert;
        }

        asym = Ogre::Vector3::ZERO;
        for (size_t k = 0; k < 30; ++k)
        {
            asym += aCoeff[k] * /*egm.mAsymMorphModeScales[k] * */egm.mAsymMorphModes[k + 30*i];
        }
#endif
        fgVertices[i] = vertices[i] + sym + asym;
    }
        sam.getMorphedVertices(fgVertices, meshName, sRaceCoeff, aRaceCoeff, sCoeff, aCoeff);

        sceneEar->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mNpc->mEditorId, fgVertices);
    }

        Ogre::Bone *heBone = mSkelBase->getSkeleton()->getBone("Bip01 Head");
        Ogre::Quaternion heOrientation = heBone->getOrientation() *
                               Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y); // fix helmet issue
        //Ogre::Vector3 hePosition = Ogre::Vector3(/*up*/0.2f, /*forward*/1.7f, 0.f); // FIXME non-zero for FO3 to make hair fit better
        Ogre::Vector3 hePosition = Ogre::Vector3(0.f, 0.f, 0.f);
        std::map<int32_t, Ogre::Entity*>::const_iterator it(sceneEar->mForeignObj->mEntities.begin());
        for (; it != sceneEar->mForeignObj->mEntities.end(); ++it)
        {
            mSkelBase->attachObjectToBone("Bip01 Head", it->second, heOrientation, hePosition);
        }


        mObjectParts[ESM::PRT_Tail] = sceneEar;
    }















    const ESM4::Hair* hair = store.getForeign<ESM4::Hair>().search(mNpc->mHair);
    if (hair)
    {
        meshName = Misc::StringUtils::lowerCase("meshes\\"+hair->mModel);

        NifOgre::ObjectScenePtr sceneHair = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        sceneHair->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));



    size_t pos = meshName.find_last_of("nif");
    if (pos == std::string::npos)
        return ; // FIXME: should throw here

    std::string path = meshName.substr(0, pos-2);
    if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(path+"egm") ||
        !Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(path+"tri"))
    {
        sceneHair->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mNpc->mEditorId);
    }
    else
    {

    //NiBtOgre::FgEgm egm(path+"egm", "General");
    //egm.loadImpl();
    NiBtOgre::FgTri tri(path+"tri", "General");
    tri.loadImpl();
    const std::vector<Ogre::Vector3>& vertices = tri.vertices(); // V + K

    const std::vector<float>& sCoeff = mNpc->mSymShapeModeCoefficients;
    const std::vector<float>& aCoeff = mNpc->mAsymShapeModeCoefficients;

    Ogre::Vector3 sym;
    Ogre::Vector3 asym;
    std::vector<Ogre::Vector3> fgVertices;
    fgVertices.resize(tri.numVertices());
    for (size_t i = 0; i < fgVertices.size(); ++i)
    {
#if 0
        sym = Ogre::Vector3::ZERO;
        for (size_t j = 0; j < 50; ++j)
        {
            sym += sCoeff[j] * /*egm.mSymMorphModeScales[j] * */egm.mSymMorphModes[j + 50*i];
//          float factor = coeff[j] * egm.mSymMorphModeScales[j];
//          Ogre::Vector3 vert = egm.mSymMorphModes[j + 50 * i];
//          Ogre::Vector3 newVert = factor * vert;
//          sym += newVert;
        }

        asym = Ogre::Vector3::ZERO;
        for (size_t k = 0; k < 30; ++k)
        {
            asym += aCoeff[k] * /*egm.mAsymMorphModeScales[k] * */egm.mAsymMorphModes[k + 30*i];
        }
#endif
        fgVertices[i] = vertices[i] + sym + asym;
    }




        sceneHair->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mNpc->mEditorId, fgVertices);
    }

        Ogre::Bone *heBone = mSkelBase->getSkeleton()->getBone("Bip01 Head");
        Ogre::Quaternion heOrientation = heBone->getOrientation() *
                               Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y); // fix helmet issue
        //Ogre::Vector3 hePosition = Ogre::Vector3(/*up*/0.2f, /*forward*/1.7f, 0.f); // FIXME non-zero for FO3 to make hair fit better
        Ogre::Vector3 hePosition = Ogre::Vector3(0.5f, -0.2f, 0.f);
        std::map<int32_t, Ogre::Entity*>::const_iterator it(sceneHair->mForeignObj->mEntities.begin());
        for (; it != sceneHair->mForeignObj->mEntities.end(); ++it)
        {



#if 0
            //does not work
            it->second->getSubEntity(0)->getMaterial()->setDiffuse((float)mNpc->mHairColour.red / 256,
                (float)mNpc->mHairColour.green / 256,
                (float)mNpc->mHairColour.blue / 256,
                (float)mNpc->mHairColour.custom / 256);
#endif
#if 1
            Ogre::MaterialPtr mat = sceneHair->mMaterialControllerMgr.getWritableMaterial(it->second);
            Ogre::Material::TechniqueIterator techIter = mat->getTechniqueIterator();
            while(techIter.hasMoreElements())
            {
                Ogre::Technique *tech = techIter.getNext();
                Ogre::Technique::PassIterator passes = tech->getPassIterator();
                while(passes.hasMoreElements())
                {
                    Ogre::Pass *pass = passes.getNext();
                    //Ogre::TextureUnitState *tex = pass->getTextureUnitState(0);
                    //tex->setColourOperation(Ogre::LBO_ALPHA_BLEND);
                    //tex->setColourOperation(Ogre::LBO_REPLACE);
                    //tex->setBlank(); // FIXME: testing
#if 1
                    Ogre::ColourValue ambient = pass->getAmbient();
                    ambient.r = (float)mNpc->mHairColour.red / 256.f;
                    ambient.g = (float)mNpc->mHairColour.green / 256.f;
                    ambient.b = (float)mNpc->mHairColour.blue / 256.f;
                    ambient.a = 1.f;
                    pass->setSceneBlending(Ogre::SBT_REPLACE);
                    pass->setAmbient(ambient);
                    pass->setVertexColourTracking(pass->getVertexColourTracking() &~Ogre::TVC_AMBIENT);
#endif
#if 0
                    Ogre::ColourValue diffuse = pass->getDiffuse();
                    diffuse.r = float((float)mNpc->mHairColour.red / 256);
                    diffuse.g = float((float)mNpc->mHairColour.green / 256);
                    diffuse.b = float((float)mNpc->mHairColour.blue / 256);
                    diffuse.a = 1.f;// (float)mNpc->mHairColour.custom / 256.f; //0.f;
                    pass->setSceneBlending(Ogre::SBT_REPLACE);
                    pass->setDiffuse(diffuse);
                    pass->setVertexColourTracking(pass->getVertexColourTracking() &~Ogre::TVC_DIFFUSE);
#endif
                }
            }
#endif




            mSkelBase->attachObjectToBone("Bip01 Head", it->second, heOrientation, hePosition);
        }
        mObjectParts[ESM::PRT_Hair] = sceneHair;
    }

#endif
    if (isTES4)
    {
        {
        if (isFemale)
    meshName = "meshes\\characters\\_male\\femalehand.nif";
        else
    meshName = "meshes\\characters\\_male\\hand.nif";
        }
    }
    else
    {
    meshName = "meshes\\characters\\_male\\lefthand.nif";
    }
    NifOgre::ObjectScenePtr sceneRH = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
    sceneRH->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
    sceneRH->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mBodyPartModelNameExt);

    std::map<int32_t, Ogre::Entity*>::const_iterator itRH(sceneRH->mForeignObj->mEntities.begin());
    for (; itRH != sceneRH->mForeignObj->mEntities.end(); ++itRH)
    {
        // FIXME:
        if (mSkelBase->getMesh()->getSkeleton() == itRH->second->getMesh()->getSkeleton())
            itRH->second->shareSkeletonInstanceWith(mSkelBase);
        else
            std::cout << "no anim " << mSkelBase->getMesh()->getName()
                    << itRH->second->getMesh()->getName() << std::endl;
        mInsert->attachObject(itRH->second);
    }
    mObjectParts[ESM::PRT_RHand] = sceneRH;

    if (isTES4)
    {
        if (isFemale)
    meshName = "meshes\\characters\\_male\\femalefoot.nif";
        else
    meshName = "meshes\\characters\\_male\\foot.nif";
    }
    else
    meshName = "meshes\\characters\\_male\\righthand.nif";

    NifOgre::ObjectScenePtr sceneRF = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
    sceneRF->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
    sceneRF->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mBodyPartModelNameExt);

    std::map<int32_t, Ogre::Entity*>::const_iterator itRF(sceneRF->mForeignObj->mEntities.begin());
    for (; itRF != sceneRF->mForeignObj->mEntities.end(); ++itRF)
    {
        // FIXME:
        if (mSkelBase->getMesh()->getSkeleton() == itRF->second->getMesh()->getSkeleton())
            itRF->second->shareSkeletonInstanceWith(mSkelBase);
        else
            std::cout << "no anim " << mSkelBase->getMesh()->getName()
                    << itRF->second->getMesh()->getName() << std::endl;
        mInsert->attachObject(itRF->second);
    }
    mObjectParts[ESM::PRT_RFoot] = sceneRF;





#if 0
    NifOgre::ObjectScenePtr objectH
        = NifOgre::Loader::createObjects(mSkelBase,
                                         "Bip01 Head", // not used for skinned
                                         "",
                                         mInsert,
                                         "meshes\\characters\\imperial\\headhuman.nif");
    Ogre::SceneNode *nodeHand = mInsert->createChildSceneNode();
    NifOgre::ObjectScenePtr objectHand
        = NifOgre::Loader::createObjects(mSkelBase,
                                         "Bip01", // not used for skinned
                                         "",
                                         mInsert,
                                         "meshes\\characters\\_male\\hand.nif");
#endif

#if 0
    Ogre::Vector3 glowColor;
    setRenderProperties(scene,
                        (mViewMode == VM_FirstPerson) ? RV_FirstPerson : mVisibilityFlags,
                        RQG_Main, RQG_Alpha,
                        0,
                        false, /*enchantedGlow*/
                        &glowColor);
    setRenderProperties(objectHand,
                        (mViewMode == VM_FirstPerson) ? RV_FirstPerson : mVisibilityFlags,
                        RQG_Main, RQG_Alpha,
                        0,
                        false, /*enchantedGlow*/
                        &glowColor);
#endif

    //for(size_t i = 0;i < ESM::PRT_Count;i++)
        //removeIndividualPart((ESM::PartReferenceType)i);
    //updateParts();


    // check inventory
    for (unsigned int i = 0; i < mNpc->mInventory.size(); ++i)
    {
        switch (store.find(mNpc->mInventory[i].item))
        {
            case MKTAG('A','A','P','P'): /*std::cout << "Apparatus" << std::endl;*/ break;
            case MKTAG('O','A','R','M'):
            {
                const ESM4::Armor* armor
                    = store.getForeign<ESM4::Armor>().search(mNpc->mInventory[i].item);
                if (armor)
                {
//                  std::cout << "Inventory " << armor->mEditorId << std::endl;
//                  std::cout << "Inventory " << armor->mModel << std::endl;
    //addOrReplaceIndividualPart(ESM::PRT_RHand, -1, 1, "meshes\\characters\\_male\\hand.nif");
    // LegionBoots
    // Armor\Legion\M\Boots.NIF
    //if (armor->mEditorId == "LegionBoots")
    if ((armor->mArmorFlags & ESM4::Armor::TES4_Foot) != 0)
    {
        //addOrReplaceIndividualPart(ESM::PRT_RFoot, -1, 1, "meshes\\armor\\legion\\m\\boots.nif", false, &glowColor);



        if ((mNpc->mBaseConfig.flags & 0x01) != 0 && !armor->mModelFemale.empty()) // female
    meshName = "meshes\\"+armor->mModelFemale;
        else
    meshName = "meshes\\"+armor->mModelMale;
    //if (meshName == "meshes\\Armor\\Thief\\M\\Boots.NIF")
        //std::cout << "stop" << std::endl;
    NifOgre::ObjectScenePtr sceneB = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
    sceneB->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
    sceneB->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mBodyPartModelNameExt);

    std::map<int32_t, Ogre::Entity*>::const_iterator it(sceneB->mForeignObj->mEntities.begin());
    for (; it != sceneB->mForeignObj->mEntities.end(); ++it)
    {
        // FIXME:
        if (mSkelBase->getMesh()->getSkeleton() == it->second->getMesh()->getSkeleton())
            it->second->shareSkeletonInstanceWith(mSkelBase);
        else
            std::cout << "no anim " << mSkelBase->getMesh()->getName()
                    << it->second->getMesh()->getName() << std::endl;
        mInsert->attachObject(it->second);
    }
    mObjectParts[ESM::PRT_RFoot] = sceneB;



    }
    // LegionCuirass
    // Armor\Legion\M\Cuirass.NIF
    //if (armor->mEditorId == "LegionCuirass")
    else if (((armor->mArmorFlags & ESM4::Armor::TES4_UpperBody) != 0) ||
             ((armor->mArmorFlags & ESM4::Armor::FO3_UpperBody) != 0))
    {
        //addOrReplaceIndividualPart(ESM::PRT_Cuirass, -1, 1, "meshes\\armor\\legion\\m\\cuirass.nif", false, &glowColor);


        if (isFemale && !armor->mModelFemale.empty()) // female
    meshName = "meshes\\"+armor->mModelFemale;
        else
    meshName = "meshes\\"+armor->mModelMale;
    NifOgre::ObjectScenePtr sceneC = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
    sceneC->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
    sceneC->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mBodyPartModelNameExt);

    std::map<int32_t, Ogre::Entity*>::const_iterator it(sceneC->mForeignObj->mEntities.begin());
    for (; it != sceneC->mForeignObj->mEntities.end(); ++it)
    {
        // FIXME:
        if (mSkelBase->getMesh()->getSkeleton() == it->second->getMesh()->getSkeleton())
            it->second->shareSkeletonInstanceWith(mSkelBase);
        else
            std::cout << "no anim " << mSkelBase->getMesh()->getName()
                    << it->second->getMesh()->getName() << std::endl;
        mInsert->attachObject(it->second);
    }
            if (!mObjectParts[ESM::PRT_Cuirass].isNull())
                mObjectParts[ESM::PRT_Cuirass].reset();
    mObjectParts[ESM::PRT_Cuirass] = sceneC;


    }
    // LegionGauntlets
    // Armor\Legion\M\Gauntlets.NIF
    //if (armor->mEditorId == "LegionGauntlets")
    else if (((armor->mArmorFlags & ESM4::Armor::TES4_Hand) != 0) ||
             ((armor->mArmorFlags & ESM4::Armor::FO3_RightHand) != 0))
    {
        //addOrReplaceIndividualPart(ESM::PRT_RHand, -1, 1, "meshes\\armor\\legion\\m\\gauntlets.nif", false, &glowColor);
                // Not sure why detach is needed
        if (!mObjectParts[ESM::PRT_RHand].isNull() && mObjectParts[ESM::PRT_RHand]->mForeignObj)
        {
            std::map<int32_t, Ogre::Entity*>::const_iterator it
                = mObjectParts[ESM::PRT_RHand]->mForeignObj->mEntities.begin();
            for (; it != mObjectParts[ESM::PRT_RHand]->mForeignObj->mEntities.end(); ++it)
            {
                it->second->stopSharingSkeletonInstance();
                mInsert->detachObject(it->second);
            }
            mObjectParts[ESM::PRT_RHand]->mForeignObj.reset();
            mObjectParts[ESM::PRT_RHand].reset();
        }

        if (isFemale && !armor->mModelFemale.empty()) // female
    meshName = "meshes\\"+armor->mModelFemale;
        else
    meshName = "meshes\\"+armor->mModelMale;
    NifOgre::ObjectScenePtr sceneH = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
    sceneH->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
    sceneH->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mBodyPartModelNameExt);

    std::map<int32_t, Ogre::Entity*>::const_iterator it(sceneH->mForeignObj->mEntities.begin());
    for (; it != sceneH->mForeignObj->mEntities.end(); ++it)
    {
        // FIXME:
        if (mSkelBase->getMesh()->getSkeleton() == it->second->getMesh()->getSkeleton())
            it->second->shareSkeletonInstanceWith(mSkelBase);
        else
            std::cout << "no anim " << mSkelBase->getMesh()->getName()
                    << it->second->getMesh()->getName() << std::endl;
        mInsert->attachObject(it->second);
    }
    mObjectParts[ESM::PRT_RHand] = sceneH;

    }

    // LegionGreaves
    // Armor\Legion\M\Greaves.NIF
    //if (armor->mEditorId == "LegionGreaves")
    else if (((armor->mArmorFlags & ESM4::Armor::TES4_LowerBody) != 0) ||
             ((armor->mArmorFlags & ESM4::Armor::FO3_PipBoy) != 0))
    {
        //addOrReplaceIndividualPart(ESM::PRT_Groin, -1, 1, "meshes\\armor\\legion\\m\\greaves.nif", false, &glowColor);

        if (isFemale && !armor->mModelFemale.empty()) // female
    meshName = "meshes\\"+armor->mModelFemale;
        else
    meshName = "meshes\\"+armor->mModelMale;
    NifOgre::ObjectScenePtr sceneG = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
    sceneG->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
    sceneG->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mBodyPartModelNameExt);

    std::map<int32_t, Ogre::Entity*>::const_iterator it(sceneG->mForeignObj->mEntities.begin());
    for (; it != sceneG->mForeignObj->mEntities.end(); ++it)
    {
        // FIXME:
        if (mSkelBase->getMesh()->getSkeleton() == it->second->getMesh()->getSkeleton())
            it->second->shareSkeletonInstanceWith(mSkelBase);
        else
            std::cout << "no anim " << mSkelBase->getMesh()->getName()
                    << it->second->getMesh()->getName() << std::endl;
        mInsert->attachObject(it->second);
    }
    mObjectParts[ESM::PRT_Groin] = sceneG;


    }
    // LegionHelmet
    // Armor\LegionHorsebackGuard\Helmet.NIF

    //if (armor->mEditorId == "LegionHelmet")
    else if (((armor->mArmorFlags & ESM4::Armor::TES4_Hair) != 0) || // note helmets share hair slot
             ((armor->mArmorFlags & ESM4::Armor::FO3_Hair) != 0))
    {
        // Not sure why detach is needed
        if (!mObjectParts[ESM::PRT_Hair].isNull() && mObjectParts[ESM::PRT_Hair]->mForeignObj)
        {
            std::map<int32_t, Ogre::Entity*>::const_iterator it
                = mObjectParts[ESM::PRT_Hair]->mForeignObj->mEntities.begin();
            for (; it != mObjectParts[ESM::PRT_Hair]->mForeignObj->mEntities.end(); ++it)
            {
                mSkelBase->detachObjectFromBone(it->second);
            }
            mObjectParts[ESM::PRT_Hair]->mForeignObj.reset();
            mObjectParts[ESM::PRT_Hair].reset();
        }

        //addOrReplaceIndividualPart(ESM::PRT_Head, -1, 1, "meshes\\armor\\legionhorsebackguard\\helmet.nif", false, &glowColor);
        //mObjectParts[ESM::PRT_Head] = insertBoundedPart("meshes\\armor\\legionhorsebackguard\\helmet.nif", -1, "Bip01 Head", "", false, &glowColor);
        //Ogre::Bone* helmet = mSkelBase->getSkeleton()->createBone("Helmet"); // crashes!
        // assert(bone);
        //mSkelBase->getSkeleton()->getBone("Bip01 Head")->addChild(helmet);
//      NifOgre::ObjectScenePtr objectHelmet
//          = NifOgre::Loader::createObjects(mSkelBase,
//                                       "Bip01 Head",
//                                       "",
//                                       mInsert,
//                                       "meshes\\armor\\legionhorsebackguard\\helmet.nif");
//                                       //"meshes\\Characters\\Imperial\\headhuman.nif");
//      mObjectParts[ESM::PRT_Head] = objectHelmet; // FIXME

        if (isFemale && !armor->mModelFemale.empty()) // female
    meshName = "meshes\\"+armor->mModelFemale;
        else
    meshName = "meshes\\"+armor->mModelMale;
        //else
            //meshName = "meshes\\"+hair->mModel;

        NifOgre::ObjectScenePtr sceneHe = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        sceneHe->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        sceneHe->mForeignObj->instantiate();

        Ogre::Bone *heBone = mSkelBase->getSkeleton()->getBone("Bip01 Head");
        Ogre::Vector3 bodyPos = mSkelBase->getSkeleton()->getBone("Bip01")->getPosition();
        Ogre::Quaternion heOrientation = heBone->getOrientation() *
                               Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y); // fix helmet issue
        Ogre::Vector3 hePosition = Ogre::Vector3(0, 0, 0);//heBone->getPosition();
        std::map<int32_t, Ogre::Entity*>::const_iterator it(sceneHe->mForeignObj->mEntities.begin());
        for (; it != sceneHe->mForeignObj->mEntities.end(); ++it)
        {
            mSkelBase->attachObjectToBone("Bip01 Head", it->second, heOrientation, hePosition);
        }
        mObjectParts[ESM::PRT_Hair] = sceneHe;

    }
    else if ((armor->mArmorFlags & ESM4::Armor::TES4_Shield) != 0)
    {
        if (isFemale && !armor->mModelFemale.empty()) // female
    meshName = "meshes\\"+armor->mModelFemale;
        else
    meshName = "meshes\\"+armor->mModelMale;
    //else
        //meshName = "meshes\\"+hair->mModel;

    NifOgre::ObjectScenePtr sceneSh = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
    sceneSh->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
    sceneSh->mForeignObj->instantiate();

    std::map<int32_t, Ogre::Entity*>::const_iterator it(sceneSh->mForeignObj->mEntities.begin());
    for (; it != sceneSh->mForeignObj->mEntities.end(); ++it)
    {
        mSkelBase->attachObjectToBone("Bip01 L ForearmTwist", it->second);
    }
    mObjectParts[ESM::PRT_Shield] = sceneSh;
    }
    else
        std::cout << "unknown armor " << armor->mEditorId << " " << std::hex << armor->mArmorFlags << std::endl;


    // LegionShield
    // Armor\Legion\Shield.NIF
                }
                break;
            }
            case MKTAG('K','B','O','O'): /*std::cout << "Books" << std::endl;*/ break;
            case MKTAG('T','C','L','O'): /*std::cout << "Clothes" << std::endl; break;*/
            {
                const ESM4::Clothing* cloth
                    = store.getForeign<ESM4::Clothing>().search(mNpc->mInventory[i].item);
                if (cloth)
                {
                    //std::cout << "Inventory " << cloth->mEditorId << std::endl;
                    //std::cout << "Inventory " << cloth->mModel << std::endl;





                    std::string meshName;
        if (isFemale && !cloth->mModelFemale.empty()) // female
    meshName = Misc::StringUtils::lowerCase("meshes\\"+cloth->mModelFemale);
        else
    meshName = Misc::StringUtils::lowerCase("meshes\\"+cloth->mModelMale);

    size_t pos = meshName.find_last_of("nif");
    if (pos == std::string::npos)
        return ; // FIXME: should throw here
    std::string path = meshName.substr(0, pos-2);

        std::unique_ptr<std::vector<Ogre::Vector3> > fgVertices // NOTE: ownership passed to the model
            = std::make_unique<std::vector<Ogre::Vector3> >();

    if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(path+"egm"))
        sam.getMorphedVertices(fgVertices.get(), meshName, sRaceCoeff, aRaceCoeff, sCoeff, aCoeff);

        NifOgre::ObjectScenePtr scene = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        scene->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
    if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(path+"egm"))
        scene->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mBodyPartModelNameExt);
    else
        scene->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mBodyPartModelNameExt, std::move(fgVertices));

    MWWorld::ContainerStoreIterator storeChest = inv.getSlot(MWWorld::InventoryStore::Slot_ForeignUpperBody);
    MWWorld::ContainerStoreIterator storeLegs = inv.getSlot(MWWorld::InventoryStore::Slot_ForeignLowerBody);

    //std::pair<std::vector<int>, bool> slots = cloth->getEquipmentSlots

        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            if ((cloth->mClothingFlags & ESM4::Armor::TES4_RightRing) == 0 &&
                (cloth->mClothingFlags & ESM4::Armor::TES4_LeftRing) == 0)
            {
                // FIXME:
                if (mSkelBase->getMesh()->getSkeleton() == it->second->getMesh()->getSkeleton())
                    it->second->shareSkeletonInstanceWith(mSkelBase);
                else
                    std::cout << "no anim " << mSkelBase->getMesh()->getName()
                    << it->second->getMesh()->getName() << std::endl;

                if ((cloth->mClothingFlags & ESM4::Armor::TES4_UpperBody) != 0 &&
                    !mObjectParts[ESM::PRT_Cuirass].isNull())
                    continue;
                if ((cloth->mClothingFlags & ESM4::Armor::TES4_LowerBody) != 0 &&
                    !mObjectParts[ESM::PRT_Groin].isNull())
                    continue;
                mInsert->attachObject(it->second);
            }
            //else attach to bone?
        }
        if ((cloth->mClothingFlags & ESM4::Armor::TES4_UpperBody) != 0)
        {
            if (mObjectParts[ESM::PRT_Cuirass].isNull())
            {
            std::cout << cloth->mEditorId << " U " << std::hex << cloth->mClothingFlags << std::endl;
            mObjectParts[ESM::PRT_Cuirass] = scene;
            }
        }
        else if ((cloth->mClothingFlags & ESM4::Armor::TES4_LowerBody) != 0)
        {
            if (mObjectParts[ESM::PRT_Groin].isNull())
            {
            std::cout << cloth->mEditorId << " L " << std::hex << cloth->mClothingFlags << std::endl;
            mObjectParts[ESM::PRT_Groin] = scene;
            }
        }
        else if ((cloth->mClothingFlags & ESM4::Armor::TES4_Foot) != 0)
        {
            mObjectParts[ESM::PRT_RFoot] = scene;
        }






                }
                break;
            }
            case MKTAG('R','I','N','G'): /*std::cout << "Ingredients" << std::endl;*/ break;
            case MKTAG('C','M','I','S'): /*std::cout << "MiscItems" << std::endl;*/ break;
            case MKTAG('P','W','E','A'):
            {
                const ESM4::Weapon* weap
                    = store.getForeign<ESM4::Weapon>().search(mNpc->mInventory[i].item);
                if (weap)
                {
//                  std::cout << "Inventory " << weap->mEditorId << std::endl;
//                  std::cout << "Inventory " << weap->mModel << std::endl;
                }
                break;
            }
            case MKTAG('O','A','M','M'): /*std::cout << "Ammos" << std::endl;*/ break;
            case MKTAG('M','S','L','G'): /*std::cout << "SoulGems" << std::endl;*/ break;
            case MKTAG('M','K','E','Y'): /*std::cout << "Keys" << std::endl;*/ break;
            case MKTAG('H','A','L','C'): /*std::cout << "Potions" << std::endl;*/ break;
            case MKTAG('T','S','G','S'): /*std::cout << "SigilStones" << std::endl;*/ break;
            case MKTAG('E','N','O','T'): /*std::cout << "Notes" << std::endl;*/ break;
            case MKTAG('I','L','V','L'):
            {
                const ESM4::LeveledItem* lvli
                    = store.getForeign<ESM4::LeveledItem>().search(mNpc->mInventory[i].item);
                if (!lvli)
                {
//                  std::cout << "LvlItems not found" << std::endl;
                    break; // FIXME
                }

                for (size_t j = lvli->mLvlObject.size()-1; j < lvli->mLvlObject.size(); ++j)
                {
                    //std::cout << "LVLI " << lvli->mEditorId << " LVLO lev "
                        //<< lvli->mLvlObject[j].level << ", item " << std::hex << lvli->mLvlObject[j].item
                              //<< ", count " << lvli->mLvlObject[j].count << std::endl;
                    switch (store.find(lvli->mLvlObject[lvli->mLvlObject.size()-1].item)) // FIXME
                    {
                        case MKTAG('A','A','P','P'): /*std::cout << "lvl Apparatus" << std::endl;*/ break;
                        case MKTAG('I','L','V','L'):
                        {
                            //std::cout << "lvli again" << std::endl;
                            const ESM4::LeveledItem* lvli2
                                = store.getForeign<ESM4::LeveledItem>().search(lvli->mLvlObject[lvli->mLvlObject.size()-1].item);
                for (size_t j = lvli2->mLvlObject.size()-1; j < lvli2->mLvlObject.size(); ++j)
                {
                    if (store.find(lvli2->mLvlObject[lvli2->mLvlObject.size()-1].item) == MKTAG('O','A','R','M'))
                    {
                            const ESM4::Armor* armor
                                = store.getForeign<ESM4::Armor>().search(mNpc->mInventory[i].item);
                            if (armor)
                                std::cout << "found armor" << std::endl;
                    }
                }
                            break;
                        }
                        case MKTAG('O','A','R','M'):
                        {
                            //std::cout << "lvl Armors" << std::endl; break;
                            const ESM4::Armor* armor
                                = store.getForeign<ESM4::Armor>().search(mNpc->mInventory[i].item);
                            if (armor)
                            {


                    std::string meshName;
        if (isFemale && !armor->mModelFemale.empty()) // female
    meshName = "meshes\\"+armor->mModelFemale;
        else
    meshName = "meshes\\"+armor->mModelMale;

        NifOgre::ObjectScenePtr scene = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        scene->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        scene->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mBodyPartModelNameExt);

        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            if ((armor->mArmorFlags & ESM4::Armor::TES4_Hair) == 0) // note helmets share hair slot
            {
                // FIXME:
                if (mSkelBase->getMesh()->getSkeleton() == it->second->getMesh()->getSkeleton())
                    it->second->shareSkeletonInstanceWith(mSkelBase);
                else
                    std::cout << "no anim " << mSkelBase->getMesh()->getName()
                    << it->second->getMesh()->getName() << std::endl;
            }
            else
                mInsert->attachObject(it->second);
        }
        if ((armor->mArmorFlags & ESM4::Armor::TES4_UpperBody) != 0)
        {
            if (!mObjectParts[ESM::PRT_Cuirass].isNull())
                mObjectParts[ESM::PRT_Cuirass].reset();
            mObjectParts[ESM::PRT_Cuirass] = scene;
        }
        else if ((armor->mArmorFlags & ESM4::Armor::TES4_LowerBody) != 0)
        {
            mObjectParts[ESM::PRT_Groin] = scene;
        }
        else if ((armor->mArmorFlags & ESM4::Armor::TES4_Foot) != 0)
        {
            mObjectParts[ESM::PRT_RFoot].reset();
            mObjectParts[ESM::PRT_RFoot] = scene;
        }
                            }



















                        }
                        case MKTAG('K','B','O','O'): /*std::cout << "lvl Books" << std::endl;*/ break;
                        case MKTAG('T','C','L','O'):
                        {
                            const ESM4::Clothing* cloth
                                = store.getForeign<ESM4::Clothing>().search(lvli->mLvlObject[lvli->mLvlObject.size()-1].item);
                            if (cloth)
                            {
                                //std::cout << "LVLI " << lvli->mEditorId << " LVLO lev "
                                    //<< lvli->mLvlObject[j].level << std::endl;




//#if 0
                    std::string meshName;
        if (isFemale && !cloth->mModelFemale.empty()) // female
    meshName = "meshes\\"+cloth->mModelFemale;
        else
    meshName = "meshes\\"+cloth->mModelMale;

        NifOgre::ObjectScenePtr scene = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        scene->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        scene->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mBodyPartModelNameExt);

        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            if ((cloth->mClothingFlags & ESM4::Armor::TES4_RightRing) == 0 &&
                (cloth->mClothingFlags & ESM4::Armor::TES4_LeftRing) == 0)
            {
                // FIXME:
                if (mSkelBase->getMesh()->getSkeleton() == it->second->getMesh()->getSkeleton())
                    it->second->shareSkeletonInstanceWith(mSkelBase);
                else
                    std::cout << "no anim " << mSkelBase->getMesh()->getName() << std::endl;
                mInsert->attachObject(it->second);
            }
            //else attach to bone?
        }
        if ((cloth->mClothingFlags & ESM4::Armor::TES4_UpperBody) != 0)
        {
            std::cout << std::hex << cloth->mClothingFlags << std::endl;
            if (mObjectParts[ESM::PRT_Cuirass].isNull())
            mObjectParts[ESM::PRT_Cuirass] = scene;
        }
        else if ((cloth->mClothingFlags & ESM4::Armor::TES4_LowerBody) != 0)
        {
            if (mObjectParts[ESM::PRT_Groin].isNull())
            mObjectParts[ESM::PRT_Groin] = scene;
        }
        else if ((cloth->mClothingFlags & ESM4::Armor::TES4_Foot) != 0)
        {
            mObjectParts[ESM::PRT_RFoot].reset();
            mObjectParts[ESM::PRT_RFoot] = scene;
        }


//#endif



                            }
                            break;
                        }
                        case MKTAG('R','I','N','G'): /*std::cout << "lvl Ingredients" << std::endl;*/ break;
                        case MKTAG('C','M','I','S'):
                        {
                            const ESM4::MiscItem* misc
                                = store.getForeign<ESM4::MiscItem>().search(lvli->mLvlObject[lvli->mLvlObject.size()-1].item);
                            if (misc)
                            {
//                              std::cout << "LVLI " << lvli->mEditorId << " LVLO lev "
//                                  << lvli->mLvlObject[j].level << " "
//                                  << ESM4::formIdToString(lvli->mLvlObject[j].item) << std::endl;
                            }
                            break;
                        }
                        case MKTAG('P','W','E','A'): /*std::cout << "lvl Weapons" << std::endl;*/ break;
                        case MKTAG('O','A','M','M'): /*std::cout << "lvl Ammos" << std::endl;*/ break;
                        case MKTAG('M','S','L','G'): /*std::cout << "lvl SoulGems" << std::endl;*/ break;
                        case MKTAG('M','K','E','Y'): /*std::cout << "lvl Keys" << std::endl;*/ break;
                        case MKTAG('H','A','L','C'): /*std::cout << "lvl Potions" << std::endl;*/ break;
                        case MKTAG('T','S','G','S'): /*std::cout << "lvl SigilStones" << std::endl;*/ break;
                        case MKTAG('E','N','O','T'): /*std::cout << "lvl notes" << std::endl;*/ break;
                        default: break;
                    }
                }
                break;
            }
            default: /*std::cout << "unknown inventory" << std::endl;*/ break; // FIXME
        }
    }

    if (mObjectParts[ESM::PRT_Cuirass].isNull())
    {
        if (isFemale)
        meshName = "meshes\\characters\\_male\\femaleupperbody.nif";
        else
        meshName = "meshes\\characters\\_male\\upperbody.nif";
        NifOgre::ObjectScenePtr scene = NifOgre::ObjectScenePtr (new NifOgre::ObjectScene(mInsert->getCreator()));
        scene->mForeignObj = std::make_shared<NiBtOgre::BtOgreInst>(NiBtOgre::BtOgreInst(mInsert->createChildSceneNode(), meshName, group));
        scene->mForeignObj->instantiate(mSkelBase->getMesh()->getSkeleton(), mBodyPartModelNameExt);

        std::map<int32_t, Ogre::Entity*>::const_iterator it(scene->mForeignObj->mEntities.begin());
        for (; it != scene->mForeignObj->mEntities.end(); ++it)
        {
            // FIXME:
            if (mSkelBase->getMesh()->getSkeleton() == it->second->getMesh()->getSkeleton())
                it->second->shareSkeletonInstanceWith(mSkelBase);
            else
                std::cout << "no anim " << mSkelBase->getMesh()->getName() << std::endl;
            mInsert->attachObject(it->second);
        }
        mObjectParts[ESM::PRT_Cuirass] = scene;
    }


#if 0
    // ESM::PRT_Count is 27, see loadarmo.hpp
    // NifOgre::ObjectScenePtr mObjectParts[ESM::PRT_Count];
    mHeadModel = "meshes\\characters\\imperial\\headhuman.nif";
    mObjectParts[ESM::PRT_Head] = insertBoundedPart(mHeadModel, -1, "Bip01", "Head", false, 0);
    addOrReplaceIndividualPart(ESM::PRT_RHand, -1, 1, "meshes\\characters\\_male\\hand.nif");
#endif

    //if (mAccumRoot) mAccumRoot->setPosition(Ogre::Vector3());

    mWeaponAnimationTime->updateStartTime();
}

void ForeignNpcAnimation::addAnimSource(const std::string &model)
{
    OgreAssert(mInsert, "Object is missing a root!");
    if (!mSkelBase)
        return; // FIXME: should throw here (or assert)

    std::string lowerModel = model;
    Misc::StringUtils::lowerCaseInPlace(lowerModel);

    // First find the kf file.  For TES3 the kf file has the same name as the nif file.
    // For TES4, different animations (e.g. idle, block) have different kf files.
    size_t pos = lowerModel.find("skeleton.nif");
    if (pos == std::string::npos)
    {
        pos = lowerModel.find("skeletonbeast.nif");
        if (pos == std::string::npos)
            return; // FIXME: should throw here
        // TODO: skeletonsesheogorath
    }

    // FIXME: for testing just load idle
    std::string animName = model.substr(0, pos) + "handtohandattackleft_jab.kf";

    addForeignAnimSource(model, animName);
    //animName = model.substr(0, pos) + "handtohandattackright_hook.kf";
    //addForeignAnimSource(model, animName);
    //animName = model.substr(0, pos) + "blockidle.kf";
    //addForeignAnimSource(model, animName);
    //animName = model.substr(0, pos) + "sneakidle.kf";
    //addForeignAnimSource(model, animName);
    //animName = model.substr(0, pos) + "walkforward.kf";
    //addForeignAnimSource(model, animName);
    animName = model.substr(0, pos) + "twohandidle.kf";
    addForeignAnimSource(model, animName);
    animName = model.substr(0, pos) + "castself.kf";
    addForeignAnimSource(model, animName);
    //animName = model.substr(0, pos) + "swimfastforward.kf";
    //animName = model.substr(0, pos) + "walkfastforward.kf";
    //animName = model.substr(0, pos) + "swimbackward.kf";
    //animName = model.substr(0, pos) + "dodgeleft.kf";
    //animName = model.substr(0, pos) + "idleanims\\cheer01.kf";
    //animName = model.substr(0, pos) + "idleanims\\talk_armscrossed_motion.kf";
    //animName = model.substr(0, pos) + "castselfalt.kf";
    //animName = model.substr(0, pos) + "idleanims\\umpa_disco.kf";
    animName = model.substr(0, pos) + "walkforward.kf";
    animName = model.substr(0, pos) + "idle.kf";
    //
    //animName = model.substr(0, pos) + "idleanims\\talk_relaxed.kf";
    //animName = model.substr(0, pos) + "idleanims\\bdaycheera.kf";
    //animName = model.substr(0, pos) + "idleanims\\laugha.kf";
    //animName = model.substr(0, pos) + "locomotion\\male\\mtforward.kf";
    addForeignAnimSource(model, animName);
}

void ForeignNpcAnimation::addForeignAnimSource(const std::string& model, const std::string &animName)
{
    // Check whether the kf file exists
    if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(animName))
        return;

    std::string group("General"); // FIXME
    NiModelPtr npcModel = NiBtOgre::NiModelManager::getSingleton().getOrLoadByName(model, group);
    npcModel->buildSkeleton(); // FIXME: hack
    assert(!npcModel.isNull() && "skeleton.nif should have been built already");
    NiModelPtr anim = NiBtOgre::NiModelManager::getSingleton().getOrLoadByName(animName, group);

    // Animation::AnimSource : public Ogre::AnimationAlloc
    //   (has a) std::multimap<float, std::string> mTextKeys
    //   (also has a vector of 4 Ogre real controllers)  TODO: check if 4 is enough
    Ogre::SharedPtr<AnimSource> animSource(OGRE_NEW AnimSource);
    std::vector<Ogre::Controller<Ogre::Real> > controllers;
    anim->buildAnimation(mSkelBase, anim, animSource->mTextKeys, controllers, /*mObjectRoot->skeleton.get()*/npcModel.get()); // no bow

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
    while (node && node->getName() != "Bip01 NonAccum")
    {
        node = node->getParent();
    }
    if (node && node->getName() == "Bip01 NonAccum")
    {
        mNonAccumRoot = node;
        mAccumRoot = mNonAccumRoot->getParent();
    }
    // else throw?

    if (mNonAccumRoot->getName() != "Bip01 NonAccum" || mAccumRoot->getName() != "Bip01")
        std::cout << mAccumRoot->getName() << std::endl;
    // end debugging

    for (unsigned int i = 0; i < mObjectRoot->mControllers.size(); ++i)
    {
        if (!mObjectRoot->mControllers[i].getSource())
            mObjectRoot->mControllers[i].setSource(mAnimationTimePtr[0]);
    }
}

void ForeignNpcAnimation::updateParts()
{
    if (!mSkelBase)
        return;

    mAlpha = 1.f;
    const MWWorld::Class &cls = mPtr.getClass();

    NpcType curType = Type_Normal;
    //if (cls.getCreatureStats(mPtr).getMagicEffects().get(ESM::MagicEffect::Vampirism).getMagnitude() > 0)
        //curType = Type_Vampire;
    //if (cls.getNpcStats(mPtr).isWerewolf())
        //curType = Type_Werewolf;

    if (curType != mNpcType)
    {
        mNpcType = curType;
        rebuild();
        return;
    }

    // Equipment Slots
    //
    // Morrowind      Oblivion
    //
    // Robe           -- takes other slots
    // Weapon R       Weapon
    // Weapon L       Shield
    // Helmet         Helmet/Hood
    // Cuirass        Cuirass/Shirt        - upper body
    // Gauntlets      Gauntlets            - hand
    // Greaves        Greaves/Skirts/Pants - lower body
    // Boots/Shoes    Boots/Shoes          - feet
    // Shirt
    // Skirts/Pants
    // Pauldron R     -- doesn't exist
    // Pauldron L     -- doesn't exist
    // Ring           Ring
    // Ring           Ring
    // Amulet         Amulet


    static const struct {
        int mSlot;
        int mBasePriority;
    } slotlist[] = {
        // FIXME: Priority is based on the number of reserved slots. There should be a better way.
        { MWWorld::InventoryStore::Slot_Robe,         12 },
        { MWWorld::InventoryStore::Slot_Skirt,         3 },
        { MWWorld::InventoryStore::Slot_Helmet,        0 },
        { MWWorld::InventoryStore::Slot_Cuirass,       0 },
        { MWWorld::InventoryStore::Slot_Greaves,       0 },
        { MWWorld::InventoryStore::Slot_LeftPauldron,  0 },
        { MWWorld::InventoryStore::Slot_RightPauldron, 0 },
        { MWWorld::InventoryStore::Slot_Boots,         0 },
        { MWWorld::InventoryStore::Slot_LeftGauntlet,  0 },
        { MWWorld::InventoryStore::Slot_RightGauntlet, 0 },
        { MWWorld::InventoryStore::Slot_Shirt,         0 },
        { MWWorld::InventoryStore::Slot_Pants,         0 },
        { MWWorld::InventoryStore::Slot_CarriedLeft,   0 },
        { MWWorld::InventoryStore::Slot_CarriedRight,  0 },
        { MWWorld::InventoryStore::Slot_ForeignHead,      0 },
        { MWWorld::InventoryStore::Slot_ForeignHair,      0 },
        { MWWorld::InventoryStore::Slot_ForeignUpperBody, 0 },
        { MWWorld::InventoryStore::Slot_ForeignLowerBody, 0 },
        { MWWorld::InventoryStore::Slot_ForeignHand,      0 },
        { MWWorld::InventoryStore::Slot_ForeignFoot,      0 },
        { MWWorld::InventoryStore::Slot_ForeignRightRing, 0 },
        { MWWorld::InventoryStore::Slot_ForeignLeftRing,  0 },
        { MWWorld::InventoryStore::Slot_ForeignAmulet,    0 },
        { MWWorld::InventoryStore::Slot_ForeignWeapon,    0 },
        { MWWorld::InventoryStore::Slot_ForeignBackWeapon,0 },
        { MWWorld::InventoryStore::Slot_ForeignSideWeapon,0 },
        { MWWorld::InventoryStore::Slot_ForeignQuiver,    0 },
        { MWWorld::InventoryStore::Slot_ForeignShield,    0 },
        { MWWorld::InventoryStore::Slot_ForeignTorch,     0 },
        { MWWorld::InventoryStore::Slot_ForeignTail,      0 }
    };
    static const size_t slotlistsize = sizeof(slotlist)/sizeof(slotlist[0]);

    bool wasArrowAttached = (mAmmunition.get() != NULL);

    MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    for(size_t i = 0;i < slotlistsize && mViewMode != VM_HeadOnly;i++)
    {
        MWWorld::ContainerStoreIterator store = inv.getSlot(slotlist[i].mSlot);

        removePartGroup(slotlist[i].mSlot);

        if(store == inv.end())
            continue;

        if(slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Helmet)
            removeIndividualPart(ESM::PRT_Hair);

        int prio = 1;
        bool enchantedGlow = !store->getClass().getEnchantment(*store).empty();
        Ogre::Vector3 glowColor = getEnchantmentColor(*store);
        if(store->getTypeName() == typeid(ESM4::Clothing).name())
        {
            prio = ((slotlist[i].mBasePriority+1)<<1) + 0;
            const ESM4::Clothing *clothes = store->get<ESM4::Clothing>()->mBase;
            //addPartGroup(slotlist[i].mSlot, prio, clothes->mParts.mParts, enchantedGlow, &glowColor);
        }
        else if(store->getTypeName() == typeid(ESM4::Armor).name())
        {
            prio = ((slotlist[i].mBasePriority+1)<<1) + 1;
            const ESM4::Armor *armor = store->get<ESM4::Armor>()->mBase;
            //addPartGroup(slotlist[i].mSlot, prio, armor->mParts.mParts, enchantedGlow, &glowColor);
        }

        if(slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Robe)
        {
            ESM::PartReferenceType parts[] = {
                ESM::PRT_Groin, ESM::PRT_Skirt, ESM::PRT_RLeg, ESM::PRT_LLeg,
                ESM::PRT_RUpperarm, ESM::PRT_LUpperarm, ESM::PRT_RKnee, ESM::PRT_LKnee,
                ESM::PRT_RForearm, ESM::PRT_LForearm
            };
            size_t parts_size = sizeof(parts)/sizeof(parts[0]);
            for(size_t p = 0;p < parts_size;++p)
                reserveIndividualPart(parts[p], slotlist[i].mSlot, prio);
        }
        else if(slotlist[i].mSlot == MWWorld::InventoryStore::Slot_Skirt)
        {
            reserveIndividualPart(ESM::PRT_Groin, slotlist[i].mSlot, prio);
            reserveIndividualPart(ESM::PRT_RLeg, slotlist[i].mSlot, prio);
            reserveIndividualPart(ESM::PRT_LLeg, slotlist[i].mSlot, prio);
        }
    }

    if(mViewMode != VM_FirstPerson)
    {
        if(mPartPriorities[ESM::PRT_Head] < 1 && !mHeadModel.empty())
            addOrReplaceIndividualPart(ESM::PRT_Head, -1,1, mHeadModel);
        if(mPartPriorities[ESM::PRT_Hair] < 1 && mPartPriorities[ESM::PRT_Head] <= 1 && !mHairModel.empty())
            addOrReplaceIndividualPart(ESM::PRT_Hair, -1,1, mHairModel);
    }
    if(mViewMode == VM_HeadOnly)
        return;

    if(mPartPriorities[ESM::PRT_Shield] < 1)
    {
        MWWorld::ContainerStoreIterator store = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
        MWWorld::Ptr part;
        if(store != inv.end() && (part=*store).getTypeName() == typeid(ESM4::Light).name())
        {
            const ESM::Light *light = part.get<ESM::Light>()->mBase;
            addOrReplaceIndividualPart(ESM::PRT_Shield, MWWorld::InventoryStore::Slot_CarriedLeft,
                                       1, "meshes\\"+light->mModel);
            addExtraLight(mInsert->getCreator(), mObjectParts[ESM::PRT_Shield], light);
        }
    }

    showWeapons(mShowWeapons);
    showCarriedLeft(mShowCarriedLeft);

    // Remember body parts so we only have to search through the store once for each race/gender/viewmode combination
    static std::map< std::pair<std::string,int>,std::vector<const ESM::BodyPart*> > sRaceMapping;

    bool isWerewolf = (mNpcType == Type_Werewolf);
    int flags = (isWerewolf ? -1 : 0);
    bool isMale = (mNpc->mBaseConfig.flags & 0x000001) == 0; // 0x1 means female
    if (isMale)
    {
        static const int Flag_Female      = 1<<0;
        flags |= Flag_Female;
    }
    if(mViewMode == VM_FirstPerson)
    {
        static const int Flag_FirstPerson = 1<<1;
        flags |= Flag_FirstPerson;
    }

    std::string race = "";//(isWerewolf ? "werewolf" : Misc::StringUtils::lowerCase(mNpc->mRace));
    std::pair<std::string, int> thisCombination = std::make_pair(race, flags);
    if (sRaceMapping.find(thisCombination) == sRaceMapping.end())
    {
        typedef std::multimap<ESM::BodyPart::MeshPart,ESM::PartReferenceType> BodyPartMapType;
        static BodyPartMapType sBodyPartMap;
        if(sBodyPartMap.empty())
        {
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Neck, ESM::PRT_Neck));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Chest, ESM::PRT_Cuirass));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Groin, ESM::PRT_Groin));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Hand, ESM::PRT_RHand));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Hand, ESM::PRT_LHand));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Wrist, ESM::PRT_RWrist));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Wrist, ESM::PRT_LWrist));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Forearm, ESM::PRT_RForearm));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Forearm, ESM::PRT_LForearm));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Upperarm, ESM::PRT_RUpperarm));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Upperarm, ESM::PRT_LUpperarm));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Foot, ESM::PRT_RFoot));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Foot, ESM::PRT_LFoot));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Ankle, ESM::PRT_RAnkle));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Ankle, ESM::PRT_LAnkle));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Knee, ESM::PRT_RKnee));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Knee, ESM::PRT_LKnee));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Upperleg, ESM::PRT_RLeg));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Upperleg, ESM::PRT_LLeg));
            sBodyPartMap.insert(std::make_pair(ESM::BodyPart::MP_Tail, ESM::PRT_Tail));
        }

        std::vector<const ESM::BodyPart*> &parts = sRaceMapping[thisCombination];
        parts.resize(ESM::PRT_Count, NULL);

        const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
        const MWWorld::Store<ESM::BodyPart> &partStore = store.get<ESM::BodyPart>();
        for(MWWorld::Store<ESM::BodyPart>::iterator it = partStore.begin(); it != partStore.end(); ++it)
        {
            if(isWerewolf)
                break;
            const ESM::BodyPart& bodypart = *it;
            if (bodypart.mData.mFlags & ESM::BodyPart::BPF_NotPlayable)
                continue;
            if (bodypart.mData.mType != ESM::BodyPart::MT_Skin)
                continue;

            if (!Misc::StringUtils::ciEqual(bodypart.mRace, ""/*mNpc->mRace*/))
                continue;

            bool firstPerson = (bodypart.mId.size() >= 3)
                    && bodypart.mId[bodypart.mId.size()-3] == '1'
                    && bodypart.mId[bodypart.mId.size()-2] == 's'
                    && bodypart.mId[bodypart.mId.size()-1] == 't';
            if(firstPerson != (mViewMode == VM_FirstPerson))
            {
                if(mViewMode == VM_FirstPerson && (bodypart.mData.mPart == ESM::BodyPart::MP_Hand ||
                                                   bodypart.mData.mPart == ESM::BodyPart::MP_Wrist ||
                                                   bodypart.mData.mPart == ESM::BodyPart::MP_Forearm ||
                                                   bodypart.mData.mPart == ESM::BodyPart::MP_Upperarm))
                {
                    /* Allow 3rd person skins as a fallback for the arms if 1st person is missing. */
                    BodyPartMapType::const_iterator bIt = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
                    while(bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
                    {
                        if(!parts[bIt->second])
                            parts[bIt->second] = &*it;
                        ++bIt;
                    }
                }
                continue;
            }

            bool isFemale = (mNpc->mBaseConfig.flags & 0x000001) != 0; // 0x1 means female
            if (isFemale != (bodypart.mData.mFlags & ESM::BodyPart::BPF_Female))
            {
                // Allow opposite gender's parts as fallback if parts for our gender are missing
                BodyPartMapType::const_iterator bIt = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
                while(bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
                {
                    if(!parts[bIt->second])
                        parts[bIt->second] = &*it;
                    ++bIt;
                }
                continue;
            }

            BodyPartMapType::const_iterator bIt = sBodyPartMap.lower_bound(BodyPartMapType::key_type(bodypart.mData.mPart));
            while(bIt != sBodyPartMap.end() && bIt->first == bodypart.mData.mPart)
            {
                parts[bIt->second] = &*it;
                ++bIt;
            }
        }
    }

    const std::vector<const ESM::BodyPart*> &parts = sRaceMapping[thisCombination];
    for(int part = ESM::PRT_Neck; part < ESM::PRT_Count; ++part)
    {
        if(mPartPriorities[part] < 1)
        {
            const ESM::BodyPart* bodypart = parts[part];
            if(bodypart)
                addOrReplaceIndividualPart((ESM::PartReferenceType)part, -1, 1,
                                           "meshes\\"+bodypart->mModel);
        }
    }

    if (wasArrowAttached)
        attachArrow();
}

// TES4 animation is rather different, so a different implementation is required
//
// FIXME: Initially groupname is ignored, since only 'idle' is supported.  Not sure how to
// select which anim groups to load (each one would be a separate kf file).
//
// FIXME: The parameter 'groups' is also ignored, since TES4 groups are different. There might
// be ways to combine both, however.  Something to think about.
//
// FIXME: Just to make the initial implementation easier, 'priority', 'autodisable',
// 'startpoint', 'loops' and 'loopfallback' are also ignored.
// (always have highest priority for testing?)
void ForeignNpcAnimation::play(const std::string &groupname, int priority, int groups, bool autodisable,
              float speedmult, const std::string &start, const std::string &stop,
              float startpoint, size_t loops, bool loopfallback)
{
    //Animation::play(groupname, priority, groups, true/*autodisable*/, speedmult, start, stop, startpoint, 3/*loops*/, false/*loopfallback*/);
    Animation::play(groupname, priority, groups, autodisable, speedmult, start, stop, startpoint, loops, loopfallback);
}

void ForeignNpcAnimation::addFirstPersonOffset(const Ogre::Vector3 &offset)
{
    mFirstPersonOffset += offset;
}

class SetObjectGroup {
    int mGroup;

public:
    SetObjectGroup(int group) : mGroup(group) { }

    void operator()(Ogre::MovableObject *obj) const
    {
        obj->getUserObjectBindings().setUserAny(Ogre::Any(mGroup));
    }
};

NifOgre::ObjectScenePtr ForeignNpcAnimation::insertBoundedPart(const std::string &model, int group, const std::string &bonename, const std::string &bonefilter, bool enchantedGlow, Ogre::Vector3* glowColor)
{
    NifOgre::ObjectScenePtr objects = NifOgre::Loader::createObjects(mSkelBase, bonename, bonefilter, mInsert, model);
    setRenderProperties(objects, (mViewMode == VM_FirstPerson) ? RV_FirstPerson : mVisibilityFlags, RQG_Main, RQG_Alpha, 0,
                        enchantedGlow, glowColor);

    std::for_each(objects->mEntities.begin(), objects->mEntities.end(), SetObjectGroup(group));
    std::for_each(objects->mParticles.begin(), objects->mParticles.end(), SetObjectGroup(group));

    if(objects->mSkelBase)
    {
        Ogre::AnimationStateSet *aset = objects->mSkelBase->getAllAnimationStates();
        Ogre::AnimationStateIterator asiter = aset->getAnimationStateIterator();
        while(asiter.hasMoreElements())
        {
            Ogre::AnimationState *state = asiter.getNext();
            state->setEnabled(false);
            state->setLoop(false);
        }
        Ogre::SkeletonInstance *skelinst = objects->mSkelBase->getSkeleton();
        Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
        while(boneiter.hasMoreElements())
            boneiter.getNext()->setManuallyControlled(true);
    }

    return objects;
}

Ogre::Vector3 ForeignNpcAnimation::runAnimation(float timepassed)
{
    Ogre::Vector3 ret = Animation::runAnimation(timepassed);

    mHeadAnimationTime->update(timepassed);
//#if 0
    if (mSkelBase)
    {
        Ogre::SkeletonInstance *baseinst = mSkelBase->getSkeleton();
        if(mViewMode == VM_FirstPerson)
        {
            float pitch = mPtr.getRefData().getPosition().rot[0];
            Ogre::Node *node = baseinst->getBone("Bip01 Neck");
            node->pitch(Ogre::Radian(-pitch), Ogre::Node::TS_WORLD);

            // This has to be done before this function ends;
            // updateSkeletonInstance, below, touches the hands.
            node->translate(mFirstPersonOffset, Ogre::Node::TS_WORLD);
        }
        else
        {
            // In third person mode we may still need pitch for ranged weapon targeting
            pitchSkeleton(mPtr.getRefData().getPosition().rot[0], baseinst);

            if (0)//mHeadYaw != Ogre::Radian() || mHeadPitch != Ogre::Radian())
            {
                Ogre::Radian r = mHeadYaw;
                std::cout << "yaw " << r.valueDegrees() << std::endl;
            }

            Ogre::Node* node = baseinst->getBone("Bip01 Head");
            if (node)
                node->rotate(Ogre::Quaternion(mHeadYaw, Ogre::Vector3::UNIT_Z)
                           * Ogre::Quaternion(mHeadPitch, Ogre::Vector3::UNIT_X)
                           //* Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_Y) // fix helmet issue
                             ,Ogre::Node::TS_WORLD);
        }
    }
//#endif
    mFirstPersonOffset = 0.f; // reset the X, Y, Z offset for the next frame.

    for(size_t i = 0; i < ESM::PRT_Count; ++i)
    {
        if (!mObjectParts[i])
            continue;

        std::vector<Ogre::Controller<Ogre::Real> >::iterator ctrl(mObjectParts[i]->mControllers.begin());
        for(;ctrl != mObjectParts[i]->mControllers.end();++ctrl)
            ctrl->update();

        if (!isSkinned(mObjectParts[i])) // FIXME: maybe cache the result of isSkinned()?
            continue;

        if (mSkelBase)
            updateSkeletonInstance(mSkelBase->getSkeleton(), mObjectParts[i]->mSkelBase->getSkeleton());

        mObjectParts[i]->mSkelBase->getAllAnimationStates()->_notifyDirty();
    }

    return ret;
}

void ForeignNpcAnimation::removeIndividualPart(ESM::PartReferenceType type)
{
    mPartPriorities[type] = 0;
    mPartslots[type] = -1;

    mObjectParts[type].reset();
    if (!mSoundIds[type].empty() && !mSoundsDisabled)
    {
        MWBase::Environment::get().getSoundManager()->stopSound3D(mPtr, mSoundIds[type]);
        mSoundIds[type].clear();
    }
}

void ForeignNpcAnimation::reserveIndividualPart(ESM::PartReferenceType type, int group, int priority)
{
    if(priority > mPartPriorities[type])
    {
        removeIndividualPart(type);
        mPartPriorities[type] = priority;
        mPartslots[type] = group;
    }
}

void ForeignNpcAnimation::removePartGroup(int group)
{
    for(int i = 0; i < ESM::PRT_Count; i++)
    {
        if(mPartslots[i] == group)
            removeIndividualPart((ESM::PartReferenceType)i);
    }
}

// It seems TES5 allows lots of body part slots:
//   http://wiki.tesnexus.com/index.php/Skyrim_bodyparts_number
//   https://github.com/amorilia/nifxml/blob/master/nif.xml#L631
//
// Not sure how TES4 works.
bool ForeignNpcAnimation::addOrReplaceIndividualPart(ESM::PartReferenceType type, int group, int priority, const std::string &mesh, bool enchantedGlow, Ogre::Vector3* glowColor)
{
    if(priority <= mPartPriorities[type])
        return false;

    removeIndividualPart(type);
    mPartslots[type] = group;
    mPartPriorities[type] = priority;
    try
    {
        const std::string& bonename = "Bip01";//sPartList.at(type); // FIXME
        // PRT_Hair seems to be the only type that breaks consistency and uses a filter that's different from the attachment bone
        const std::string bonefilter = "";//(type == ESM::PRT_Hair) ? "hair" : bonename; //FIXME
        mObjectParts[type] = insertBoundedPart(mesh, group, bonename, bonefilter, enchantedGlow, glowColor);
    }
    catch (std::exception& e)
    {
        std::cerr << "Error adding NPC part: " << e.what() << std::endl;
        return false;
    }

    if (!mSoundsDisabled)
    {
        MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ContainerStoreIterator csi = inv.getSlot(group < 0 ? MWWorld::InventoryStore::Slot_Helmet : group);
        if (csi != inv.end())
        {
            mSoundIds[type] = csi->getClass().getSound(*csi);
            if (!mSoundIds[type].empty())
            {
                MWBase::Environment::get().getSoundManager()->playSound3D(mPtr, mSoundIds[type], 1.0f, 1.0f, MWBase::SoundManager::Play_TypeSfx,
                    MWBase::SoundManager::Play_Loop);
            }
        }
    }

    if(mObjectParts[type]->mSkelBase)
    {
        Ogre::SkeletonInstance *skel = mObjectParts[type]->mSkelBase->getSkeleton();

        if(mObjectParts[type]->mSkelBase->isParentTagPoint())
        {
            Ogre::Node *root = mObjectParts[type]->mSkelBase->getParentNode();
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

        if (isSkinned(mObjectParts[type]))
            updateSkeletonInstance(mSkelBase->getSkeleton(), skel);
    }

    std::vector<Ogre::Controller<Ogre::Real> >::iterator ctrl(mObjectParts[type]->mControllers.begin());
    for(;ctrl != mObjectParts[type]->mControllers.end();++ctrl)
    {
        if(!ctrl->getSource())
        {
            ctrl->setSource(mNullAnimationTimePtr);
//#if 0
            if (type == ESM::PRT_Head)
            {
                ctrl->setSource(mHeadAnimationTime);
                const NifOgre::TextKeyMap& keys = mObjectParts[type]->mTextKeys;
                for (NifOgre::TextKeyMap::const_iterator it = keys.begin(); it != keys.end(); ++it)
                {
                    if (Misc::StringUtils::ciEqual(it->second, "talk: start"))
                        mHeadAnimationTime->setTalkStart(it->first);
                    if (Misc::StringUtils::ciEqual(it->second, "talk: stop"))
                        mHeadAnimationTime->setTalkStop(it->first);
                    if (Misc::StringUtils::ciEqual(it->second, "blink: start"))
                        mHeadAnimationTime->setBlinkStart(it->first);
                    if (Misc::StringUtils::ciEqual(it->second, "blink: stop"))
                        mHeadAnimationTime->setBlinkStop(it->first);
                }
            }
            else if (type == ESM::PRT_Weapon)
                ctrl->setSource(mWeaponAnimationTime);
//#endif
        }
    }

    return true;
}

void ForeignNpcAnimation::addPartGroup(int group, int priority, const std::vector<ESM::PartReference> &parts, bool enchantedGlow, Ogre::Vector3* glowColor)
{
    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    const MWWorld::Store<ESM::BodyPart> &partStore = store.get<ESM::BodyPart>();

    const char *ext = (mViewMode == VM_FirstPerson) ? ".1st" : "";
    std::vector<ESM::PartReference>::const_iterator part(parts.begin());
    for(;part != parts.end();++part)
    {
        const ESM::BodyPart *bodypart = 0;
        bool isFemale = (mNpc->mBaseConfig.flags & 0x000001) != 0; // 0x1 means female
        if(isFemale && !part->mFemale.empty())
        {
            bodypart = partStore.search(part->mFemale+ext);
            if(!bodypart && mViewMode == VM_FirstPerson)
            {
                bodypart = partStore.search(part->mFemale);
                if(bodypart && !(bodypart->mData.mPart == ESM::BodyPart::MP_Hand ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Wrist ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Forearm ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Upperarm))
                    bodypart = NULL;
            }
            else if (!bodypart)
                std::cerr << "Failed to find body part '" << part->mFemale << "'" << std::endl;
        }
        if(!bodypart && !part->mMale.empty())
        {
            bodypart = partStore.search(part->mMale+ext);
            if(!bodypart && mViewMode == VM_FirstPerson)
            {
                bodypart = partStore.search(part->mMale);
                if(bodypart && !(bodypart->mData.mPart == ESM::BodyPart::MP_Hand ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Wrist ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Forearm ||
                                 bodypart->mData.mPart == ESM::BodyPart::MP_Upperarm))
                    bodypart = NULL;
            }
            else if (!bodypart)
                std::cerr << "Failed to find body part '" << part->mMale << "'" << std::endl;
        }

        if(bodypart)
            addOrReplaceIndividualPart((ESM::PartReferenceType)part->mPart, group, priority, "meshes\\"+bodypart->mModel, enchantedGlow, glowColor);
        else
            reserveIndividualPart((ESM::PartReferenceType)part->mPart, group, priority);
    }
}

void ForeignNpcAnimation::showWeapons(bool showWeapon)
{
    mShowWeapons = showWeapon;
    if(showWeapon)
    {
        MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
        MWWorld::ContainerStoreIterator weapon = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedRight);
        if(weapon != inv.end())
        {
            Ogre::Vector3 glowColor = getEnchantmentColor(*weapon);
            std::string mesh = weapon->getClass().getModel(*weapon);
            addOrReplaceIndividualPart(ESM::PRT_Weapon, MWWorld::InventoryStore::Slot_CarriedRight, 1,
                                       mesh, !weapon->getClass().getEnchantment(*weapon).empty(), &glowColor);

            // Crossbows start out with a bolt attached
            if (weapon->getTypeName() == typeid(ESM::Weapon).name() &&
                    weapon->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::MarksmanCrossbow)
            {
                MWWorld::ContainerStoreIterator ammo = inv.getSlot(MWWorld::InventoryStore::Slot_Ammunition);
                if (ammo != inv.end() && ammo->get<ESM::Weapon>()->mBase->mData.mType == ESM::Weapon::Bolt)
                    attachArrow();
                else
                    mAmmunition.reset();
            }
            else
                mAmmunition.reset();
        }
    }
    else
    {
        removeIndividualPart(ESM::PRT_Weapon);
    }
    mAlpha = 1.f;
}

void ForeignNpcAnimation::showCarriedLeft(bool show)
{
    mShowCarriedLeft = show;
    MWWorld::InventoryStore& inv = mPtr.getClass().getInventoryStore(mPtr);
    MWWorld::ContainerStoreIterator iter = inv.getSlot(MWWorld::InventoryStore::Slot_CarriedLeft);
    if(show && iter != inv.end())
    {
        Ogre::Vector3 glowColor = getEnchantmentColor(*iter);
        std::string mesh = iter->getClass().getModel(*iter);
        if (addOrReplaceIndividualPart(ESM::PRT_Shield, MWWorld::InventoryStore::Slot_CarriedLeft, 1,
                                   mesh, !iter->getClass().getEnchantment(*iter).empty(), &glowColor))
        {
            if (iter->getTypeName() == typeid(ESM::Light).name())
                addExtraLight(mInsert->getCreator(), mObjectParts[ESM::PRT_Shield], iter->get<ESM::Light>()->mBase);
        }
    }
    else
        removeIndividualPart(ESM::PRT_Shield);
}

void ForeignNpcAnimation::configureAddedObject(NifOgre::ObjectScenePtr object, MWWorld::Ptr ptr, int slot)
{
    Ogre::Vector3 glowColor = getEnchantmentColor(ptr);
    setRenderProperties(object, (mViewMode == VM_FirstPerson) ? RV_FirstPerson : mVisibilityFlags, RQG_Main, RQG_Alpha, 0,
                        !ptr.getClass().getEnchantment(ptr).empty(), &glowColor);

    std::for_each(object->mEntities.begin(), object->mEntities.end(), SetObjectGroup(slot));
    std::for_each(object->mParticles.begin(), object->mParticles.end(), SetObjectGroup(slot));
}

void ForeignNpcAnimation::attachArrow()
{
    WeaponAnimation::attachArrow(mPtr);
}

void ForeignNpcAnimation::releaseArrow()
{
    WeaponAnimation::releaseArrow(mPtr);
}

void ForeignNpcAnimation::permanentEffectAdded(const ESM::MagicEffect *magicEffect, bool isNew, bool playSound)
{
    // During first auto equip, we don't play any sounds.
    // Basically we don't want sounds when the actor is first loaded,
    // the items should appear as if they'd always been equipped.
    if (playSound)
    {
        static const std::string schools[] = {
            "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration"
        };

        MWBase::SoundManager *sndMgr = MWBase::Environment::get().getSoundManager();
        if(!magicEffect->mHitSound.empty())
            sndMgr->playSound3D(mPtr, magicEffect->mHitSound, 1.0f, 1.0f);
        else
            sndMgr->playSound3D(mPtr, schools[magicEffect->mData.mSchool]+" hit", 1.0f, 1.0f);
    }

    if (!magicEffect->mHit.empty())
    {
        const ESM::Static* castStatic = MWBase::Environment::get().getWorld()->getStore().get<ESM::Static>().find (magicEffect->mHit);
        bool loop = (magicEffect->mData.mFlags & ESM::MagicEffect::ContinuousVfx) != 0;
        // Don't play particle VFX unless the effect is new or it should be looping.
        if (isNew || loop)
            addEffect("meshes\\" + castStatic->mModel, magicEffect->mIndex, loop, "");
    }
}

void ForeignNpcAnimation::setAlpha(float alpha)
{
    if (alpha == mAlpha)
        return;
    mAlpha = alpha;

    for (int i=0; i<ESM::PRT_Count; ++i)
    {
        if (!mObjectParts[i])
            continue;

        for (unsigned int j=0; j<mObjectParts[i]->mEntities.size(); ++j)
        {
            Ogre::Entity* ent = mObjectParts[i]->mEntities[j];
            if (ent != mObjectParts[i]->mSkelBase)
                applyAlpha(alpha, ent, mObjectParts[i]);
        }
    }
}

void ForeignNpcAnimation::enableHeadAnimation(bool enable)
{
    mHeadAnimationTime->setEnabled(enable);
}

void ForeignNpcAnimation::preRender(Ogre::Camera *camera)
{
    Animation::preRender(camera);
    for (int i=0; i<ESM::PRT_Count; ++i)
    {
        if (!mObjectParts[i])
            continue;
        mObjectParts[i]->rotateBillboardNodes(camera);
    }
}

void ForeignNpcAnimation::applyAlpha(float alpha, Ogre::Entity *ent, NifOgre::ObjectScenePtr scene)
{
    sh::Factory::getInstance()._ensureMaterial(ent->getSubEntity(0)->getMaterial()->getName(), "Default");
    ent->getSubEntity(0)->setRenderQueueGroup(alpha != 1.f || ent->getSubEntity(0)->getMaterial()->isTransparent()
            ? RQG_Alpha : RQG_Main);


    Ogre::MaterialPtr mat = scene->mMaterialControllerMgr.getWritableMaterial(ent);
    if (mAlpha == 1.f)
    {
        // Don't bother remembering what the original values were. Just remove the techniques and let the factory restore them.
        mat->removeAllTechniques();
        sh::Factory::getInstance()._ensureMaterial(mat->getName(), "Default");
        return;
    }

    Ogre::Material::TechniqueIterator techs = mat->getTechniqueIterator();
    while(techs.hasMoreElements())
    {
        Ogre::Technique *tech = techs.getNext();
        Ogre::Technique::PassIterator passes = tech->getPassIterator();
        while(passes.hasMoreElements())
        {
            Ogre::Pass *pass = passes.getNext();
            pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
            Ogre::ColourValue diffuse = pass->getDiffuse();
            diffuse.a = alpha;
            pass->setDiffuse(diffuse);
            pass->setVertexColourTracking(pass->getVertexColourTracking() &~Ogre::TVC_DIFFUSE);
        }
    }
}

void ForeignNpcAnimation::equipmentChanged()
{
    updateParts();
}

void ForeignNpcAnimation::setVampire(bool vampire)
{
    if (mNpcType == Type_Werewolf) // we can't have werewolf vampires, can we
        return;
    if ((mNpcType == Type_Vampire) != vampire)
    {
        if (mPtr == MWBase::Environment::get().getWorld()->getPlayerPtr())
            MWBase::Environment::get().getWorld()->reattachPlayerCamera();
        else
            rebuild();
    }
}

void ForeignNpcAnimation::setHeadPitch(Ogre::Radian pitch)
{
    mHeadPitch = pitch;
}

void ForeignNpcAnimation::setHeadYaw(Ogre::Radian yaw)
{
    mHeadYaw = yaw;
}

Ogre::Radian ForeignNpcAnimation::getHeadPitch() const
{
    return mHeadPitch;
}

Ogre::Radian ForeignNpcAnimation::getHeadYaw() const
{
    return mHeadYaw;
}

}

// characters/_male/skeleton.nif has:
//                                    {{{
// Bip01
// Bip01 Tail03
// Bip01 Tail06
// Bip01 L Finger41
// Bip01 R Finger42
// Bip01 R Finger01
// Bip01 L Finger0
// Bip01 L Finger22
// Bip01 R Toe0
// Bip01 L Toe0
// Bip01 L Finger12
// Bip01 Tail07
// Bip01 L Finger02
// Bip01 L Finger01
// Bip01 R Finger2
// magicNode
// Bip01 R Finger31
// Bip01 R Finger21
// Quiver
// Bip01 L Finger31
// Bip01 Tail04
// Bip01 R Finger3
// Camera01
// Bip01 R Finger11
// Bip01 Tail08
// Bip01 Tail02
// Bip01 Tail01
// Bip01 L Finger2
// Bip01 TailRoot
// Bip01 R Finger22
// Bip01 L Finger3
// Bip01 Tail05
// Bip01 L Finger32
// Bip01 R Finger0
// Bip01 L Finger42
// Bip01 R Finger4
// Bip01 R Finger32
// Bip01 R Finger1
// Bip01 R Finger41
// Bip01 R Finger12
// Bip01 L Finger4
// Bip01 R Finger02
// Bip01 L Finger21
// Bip01 L Finger11
// BackWeapon
// Bip01 L Finger1
// Bip01 L Shoulder Helper
// Bip01 R Shoulder Helper
// Bip01 R Forearm
// Torch
// Weapon
// Bip01 L Wrist Helper
// Bip01 L ForearmTwist
// Bip01 L Hand
// Bip01 Neck1
// Bip01 R Hand
// Bip01 R ForearmTwist
// Bip01 L Forearm
// Bip01 Head
// Bip01 R Wrist Helper
// Bip01 R UpperArmTwist
// Bip01 Neck
// Bip01 R Foot
// Bip01 R Clavicle
// Bip01 L Clavicle
// Bip01 R UpperArm
// Bip01 L UpperArm
// Bip01 L Foot
// Bip01 L UpperArmTwist
// Bip01 L Calf
// Bip01 Spine2
// Bip01 R Calf
// Bip01 Spine1
// Bip01 Pelvis
// Bip01 R Thigh
// SideWeapon
// Bip01 Spine
// Bip01 L Thigh
//
// characters/_male/skeleton.nif xbase_anim.kf
// -- sorted Bip01
//
// BackWeapon
// Bip01                        Bip01
// Bip01 Head                   Bip01 Head
// Bip01 L Calf                 Bip01 L Calf
// Bip01 L Clavicle             Bip01 L Clavicle
// Bip01 L Finger0              Bip01 L Finger0
// Bip01 L Finger01             Bip01 L Finger01
// Bip01 L Finger02
// Bip01 L Finger1              Bip01 L Finger1
// Bip01 L Finger11             Bip01 L Finger11
// Bip01 L Finger12
// Bip01 L Finger2              Bip01 L Finger2
// Bip01 L Finger21             Bip01 L Finger21
// Bip01 L Finger22
// Bip01 L Finger3
// Bip01 L Finger31
// Bip01 L Finger32
// Bip01 L Finger4
// Bip01 L Finger41
// Bip01 L Finger42
// Bip01 L Foot                 Bip01 L Foot
// Bip01 L Forearm              Bip01 L Forearm
// Bip01 L ForearmTwist
// Bip01 L Hand                 Bip01 L Hand
// Bip01 L Shoulder Helper
// Bip01 L Thigh                Bip01 L Thigh
// Bip01 L Toe0
// Bip01 L UpperArm             Bip01 L UpperArm
// Bip01 L UpperArmTwist
// Bip01 L Wrist Helper
// Bip01 Neck                   Bip01 Neck
// Bip01 Neck1
// Bip01 Pelvis                 Bip01 Pelvis
// Bip01 R Calf                 Bip01 R Calf
// Bip01 R Clavicle             Bip01 R Clavicle
// Bip01 R Finger0              Bip01 R Finger0
// Bip01 R Finger01             Bip01 R Finger01
// Bip01 R Finger02
// Bip01 R Finger1              Bip01 R Finger1
// Bip01 R Finger11             Bip01 R Finger11
// Bip01 R Finger12
// Bip01 R Finger2              Bip01 R Finger2
// Bip01 R Finger21             Bip01 R Finger21
// Bip01 R Finger22
// Bip01 R Finger3
// Bip01 R Finger31
// Bip01 R Finger32
// Bip01 R Finger4
// Bip01 R Finger41
// Bip01 R Finger42
// Bip01 R Foot                 Bip01 R Foot
// Bip01 R Forearm              Bip01 R Forearm
// Bip01 R ForearmTwist
// Bip01 R Hand                 Bip01 R Hand
// Bip01 R Shoulder Helper
// Bip01 R Thigh                Bip01 R Thigh
// Bip01 R Toe0
// Bip01 R UpperArm             Bip01 R UpperArm
// Bip01 R UpperArmTwist
// Bip01 R Wrist Helper
// Bip01 Spine                  Bip01 Spine
// Bip01 Spine1                 Bip01 Spine1
// Bip01 Spine2                 Bip01 Spine2
// Bip01 Tail01
// Bip01 Tail02
// Bip01 Tail03
// Bip01 Tail04
// Bip01 Tail05
// Bip01 Tail06
// Bip01 Tail07
// Bip01 Tail08
// Bip01 TailRoot
// Camera01
// Quiver
// SideWeapon
// Torch
// Weapon                       Weapon Bone
// magicNode

// -- sorted Bip01 NonAccum
//
// Bip01 Pelvis
// Bip01 Spine
// Bip01 L Thigh
// Bip01 R Thigh
// SideWeapon
// Bip01 TailRoot
// ... etc
//                                    }}}
/* vim: set tw=100 fen fdm=marker fdl=0: */
