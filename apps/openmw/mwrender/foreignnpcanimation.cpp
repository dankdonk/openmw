#include "foreignnpcanimation.hpp"

#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreParticleSystem.h>
#include <OgreSubEntity.h>
#include <OgreSkeleton.h>
#include <OgreSkeletonInstance.h>
#include <OgreSceneNode.h>
#include <OgreBone.h>
#include <OgreTechnique.h>

#include <extern/shiny/Main/Factory.hpp>

#include <extern/esm4/lvlc.hpp>
#include <extern/esm4/formid.hpp> // mainly for debugging

#include <openengine/misc/rng.hpp>

#include <components/misc/resourcehelpers.hpp>

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
    for(size_t j = 0; j < scene->mEntities.size(); j++)
    {
        Ogre::Entity *ent = scene->mEntities[j];
        if(scene->mSkelBase != ent && ent->hasSkeleton())
        {
            return true;
        }
    }
    return false;
}

}


namespace MWRender
{

    // FIXME: need differnt bone names and parts for TES4
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
    result.insert(std::make_pair(ESM::PRT_RPauldron, "Right Clavicle"));
    result.insert(std::make_pair(ESM::PRT_LPauldron, "Left Clavicle"));
    result.insert(std::make_pair(ESM::PRT_Weapon, "Weapon Bone"));
    result.insert(std::make_pair(ESM::PRT_Tail, "Tail"));
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

    //mHeadAnimationTime = Ogre::SharedPtr<HeadAnimationTime>(new HeadAnimationTime(mPtr));
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
void ForeignNpcAnimation::updateNpcBase()
{
    clearAnimSources(); // clears *all* animations

    // FIXME: what to do with head and hand?

    // find hair and eyes
    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();

    if (store.find(mNpc->mHair) == MKTAG('R','H','A','I'))
    {
        const ESM4::Hair* hair = store.getForeign<ESM4::Hair>().search(mNpc->mHair);
        if (hair)
            std::cout << "Hair " << hair->mEditorId << std::endl;
    }
    else
        std::cerr << "Hair " + ESM4::formIdToString(mNpc->mHair) + " not found!\n";

    if (store.find(mNpc->mEyes) == MKTAG('S','E','Y','E'))
    {
        const ESM4::Eyes* eyes = store.getForeign<ESM4::Eyes>().search(mNpc->mEyes);
        if (eyes)
            std::cout << "Eyes " << eyes->mEditorId << std::endl;
    }
    else
        std::cerr << "Eyes " + ESM4::formIdToString(mNpc->mHair) + " not found!\n";

    // check inventory
    for (unsigned int i = 0; i < mNpc->mInventory.size(); ++i)
    {
        switch (store.find(mNpc->mInventory[i].item))
        {
            case MKTAG('A','A','P','P'): std::cout << "Apparatus" << std::endl; break;
            case MKTAG('O','A','R','M'):
            {
                const ESM4::Armor* armor
                    = store.getForeign<ESM4::Armor>().search(mNpc->mInventory[i].item);
                if (armor)
                {
                    std::cout << "Inventory " << armor->mEditorId << std::endl;
                }
                break;
            }
            case MKTAG('K','B','O','O'): std::cout << "Books" << std::endl; break;
            case MKTAG('T','C','L','O'): std::cout << "Clothes" << std::endl; break;
            case MKTAG('R','I','N','G'): std::cout << "Ingredients" << std::endl; break;
            case MKTAG('C','M','I','S'): std::cout << "MiscItems" << std::endl; break;
            case MKTAG('P','W','E','A'):
            {
                const ESM4::Weapon* weap
                    = store.getForeign<ESM4::Weapon>().search(mNpc->mInventory[i].item);
                if (weap)
                {
                    std::cout << "Inventory " << weap->mEditorId << std::endl;
                }
                break;
            }
            case MKTAG('O','A','M','M'): std::cout << "Ammos" << std::endl; break;
            case MKTAG('M','S','L','G'): std::cout << "SoulGems" << std::endl; break;
            case MKTAG('M','K','E','Y'): std::cout << "Keys" << std::endl; break;
            case MKTAG('H','A','L','C'): std::cout << "Potions" << std::endl; break;
            case MKTAG('T','S','G','S'): std::cout << "SigilStones" << std::endl; break;
            case MKTAG('I','L','V','L'):
            {
                const ESM4::LeveledItem* lvli
                    = store.getForeign<ESM4::LeveledItem>().search(mNpc->mInventory[i].item);
                if (!lvli)
                {
                    std::cout << "LvlItems not found" << std::endl;
                    break; // FIXME
                }

                for (size_t j = lvli->mLvlObject.size()-1; j < lvli->mLvlObject.size(); ++j)
                {
                    //std::cout << "LVLI " << lvli->mEditorId << " LVLO lev "
                        //<< lvli->mLvlObject[j].level << ", item " << lvli->mLvlObject[j].item
                                  //<< ", count " << lvli->mLvlObject[j].count << std::endl;
                    switch (store.find(lvli->mLvlObject[lvli->mLvlObject.size()-1].item)) // FIXME
                    {
                        case MKTAG('A','A','P','P'): std::cout << "lvl Apparatus" << std::endl; break;
                        case MKTAG('O','A','R','M'): std::cout << "lvl Armors" << std::endl; break;
                        case MKTAG('K','B','O','O'): std::cout << "lvl Books" << std::endl; break;
                        case MKTAG('T','C','L','O'):
                        {
                            const ESM4::Clothing* clot
                                = store.getForeign<ESM4::Clothing>().search(lvli->mLvlObject[lvli->mLvlObject.size()-1].item);
                            if (clot)
                            {
                                std::cout << "LVLI " << lvli->mEditorId << " LVLO lev "
                                    << lvli->mLvlObject[j].level << std::endl;
                            }
                            break;
                        }
                        case MKTAG('R','I','N','G'): std::cout << "lvl Ingredients" << std::endl; break;
                        case MKTAG('C','M','I','S'):
                        {
                            const ESM4::MiscItem* misc
                                = store.getForeign<ESM4::MiscItem>().search(lvli->mLvlObject[lvli->mLvlObject.size()-1].item);
                            if (misc)
                            {
                                std::cout << "LVLI " << lvli->mEditorId << " LVLO lev "
                                    << lvli->mLvlObject[j].level << std::endl;
                            }
                            break;
                        }
                        case MKTAG('P','W','E','A'): std::cout << "lvl Weapons" << std::endl; break;
                        case MKTAG('O','A','M','M'): std::cout << "lvl Ammos" << std::endl; break;
                        case MKTAG('M','S','L','G'): std::cout << "lvl SoulGems" << std::endl; break;
                        case MKTAG('M','K','E','Y'): std::cout << "lvl Keys" << std::endl; break;
                        case MKTAG('H','A','L','C'): std::cout << "lvl Potions" << std::endl; break;
                        case MKTAG('T','S','G','S'): std::cout << "lvl SigilStones" << std::endl; break;
                        default: break;
                    }
                }
                break;
            }
            default: std::cout << "unknown inventory" << std::endl; break; // FIXME
        }
    }







#if 0
    const MWWorld::ESMStore &store = MWBase::Environment::get().getWorld()->getStore();
    const ESM::Race *race = store.get<ESM::Race>().find(mNpc->mRace);
    bool isWerewolf = (mNpcType == Type_Werewolf);
    bool isVampire = (mNpcType == Type_Vampire);

    if (isWerewolf)
    {
        mHeadModel = "meshes\\" + store.get<ESM::BodyPart>().find("WerewolfHead")->mModel;
        mHairModel = "meshes\\" + store.get<ESM::BodyPart>().find("WerewolfHair")->mModel;
    }
    else
    {
        mHeadModel = "";
        if (isVampire) // FIXME: fall back to regular head when getVampireHead fails?
            mHeadModel = getVampireHead(mNpc->mRace, mNpc->mFlags & ESM::NPC::Female);
        else if (!mNpc->mHead.empty())
        {
            const ESM::BodyPart* bp = store.get<ESM::BodyPart>().search(mNpc->mHead);
            if (bp)
                mHeadModel = "meshes\\" + bp->mModel;
            else
                std::cerr << "Failed to load body part '" << mNpc->mHead << "'" << std::endl;
        }

        mHairModel = "";
        if (!mNpc->mHair.empty())
        {
            const ESM::BodyPart* bp = store.get<ESM::BodyPart>().search(mNpc->mHair);
            if (bp)
                mHairModel = "meshes\\" + bp->mModel;
            else
                std::cerr << "Failed to load body part '" << mNpc->mHair << "'" << std::endl;
        }
    }
    bool isBeast = (race->mData.mFlags & ESM::Race::Beast) != 0;
    std::string smodel = (mViewMode != VM_FirstPerson) ?
                         (!isWerewolf ? !isBeast ? "meshes\\base_anim.nif"
                                                 : "meshes\\base_animkna.nif"
                                      : "meshes\\wolf\\skin.nif") :
                         (!isWerewolf ? !isBeast ? "meshes\\base_anim.1st.nif"
                                                 : "meshes\\base_animkna.1st.nif"
                                      : "meshes\\wolf\\skin.1st.nif");
    smodel = Misc::ResourceHelpers::correctActorModelPath(smodel);
#endif
    std::string smodel = "meshes\\" + mNpc->mModel;
    smodel = Misc::ResourceHelpers::correctActorModelPath(smodel);
    setObjectRoot(smodel, true); // this call should also create mSkelBase
    mSkelBase->setDisplaySkeleton(true); // FIXME for debugging (doesn't work...)

    if(mViewMode != VM_FirstPerson)
    {
        addAnimSource(smodel);
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
            addAnimSource(smodel);
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

    // Assume that the body parts come from the same directory as the chosen skeleton.  However
    // some races have their own parts, e.g. khajiit
    //
    // meshes/characters/_male/skeleton.nif
    // meshes/characters/_male/skeletonbeast.nif
    // meshes/characters/_male/skeletonsesheogorath.nif
    //
    // meshes/characters/_male/femalefoot.nif
    // meshes/characters/_male/femalehand.nif
    // meshes/characters/_male/femalelowerbody.nif
    // meshes/characters/_male/femaleupperbody.nif
    // meshes/characters/_male/femaleupperbodynude.nif
    // meshes/characters/_male/foot.nif
    // meshes/characters/_male/hand.nif
    // meshes/characters/_male/lowerbody.nif
    // meshes/characters/_male/upperbody.nif

    // Assume foot/hand models share the same slot with boots/shoes/gloves/gauntlets
    // Similarly upperbody uses the cuirass/shirt slot and lowerbody pants/skirt/greaves slot
    //
    // However head, hair and eyes should have permanent slots.

    //MWRender::Animation
    //Ogre::Entity    *mSkelBase
    //Ogre::SceneNode *mInsert

    NifOgre::ObjectScenePtr objectH
        = NifOgre::Loader::createObjects(mSkelBase,
                                         "Bip01", // not used for skinned
                                         /*"Bip01"*/"", // bonefilter??
                                         mInsert,
                                         "meshes\\characters\\imperial\\headhuman.nif");
    Ogre::Vector3 glowColor;
    setRenderProperties(objectH,
                        (mViewMode == VM_FirstPerson) ? RV_FirstPerson : mVisibilityFlags,
                        RQG_Main, RQG_Alpha,
                        0,
                        false, /*enchantedGlow*/
                        &glowColor);

    Ogre::SceneNode *nodeHand = mInsert->createChildSceneNode();
    NifOgre::ObjectScenePtr objectHand
        = NifOgre::Loader::createObjects(mSkelBase,
                                         "Bip01", // not used for skinned
                                         /*"Bip01"*/"", // bonefilter??
                                         mInsert,
                                         "meshes\\characters\\_male\\hand.nif");

    setRenderProperties(objectHand,
                        (mViewMode == VM_FirstPerson) ? RV_FirstPerson : mVisibilityFlags,
                        RQG_Main, RQG_Alpha,
                        0,
                        false, /*enchantedGlow*/
                        &glowColor);


    for(size_t i = 0;i < ESM::PRT_Count;i++)
        removeIndividualPart((ESM::PartReferenceType)i);
    updateParts();

    // ESM::PRT_Count is 27, see loadarmo.hpp
    // NifOgre::ObjectScenePtr mObjectParts[ESM::PRT_Count];
    mObjectParts[ESM::PRT_Head] = objectH;

    mWeaponAnimationTime->updateStartTime();
}

void ForeignNpcAnimation::addAnimSource(const std::string &model)
{
    OgreAssert(mInsert, "Object is missing a root!");
    if(!mSkelBase)
        return;

    // First find the kf file.  For TES3 the kf file has the same name as the nif file.
    // For TES4, different animations (e.g. idle, block) have different kf files.
    std::string kfName = model;
#if 0
    Misc::StringUtils::lowerCaseInPlace(kfname);

    if(kfname.size() > 4 && kfname.compare(kfname.size()-4, 4, ".nif") == 0)
        kfname.replace(kfname.size()-4, 4, ".kf");
#endif

    // Check whether the kf file exists
    if(!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(kfName))
        return;

    // Animation::AnimSource : public Ogre::AnimationAlloc
    //   (has a) std::multimap<float, std::string> mTextKeys
    //   (also has a vector of 4 Ogre real controllers)  TODO: check if 4 is enough
    Ogre::SharedPtr<AnimSource> animSource(OGRE_NEW AnimSource);
    std::vector<Ogre::Controller<Ogre::Real> > controllers;
    NifOgre::Loader::createKfControllers(mSkelBase, kfName, animSource->mTextKeys, controllers);
    if(animSource->mTextKeys.empty() || controllers.empty())
        return;

    mAnimSources.push_back(animSource);

    std::vector<Ogre::Controller<Ogre::Real> > *grpctrls = animSource->mControllers;
    for(size_t i = 0;i < controllers.size();i++)
    {
        NifOgre::NodeTargetValue<Ogre::Real> *dstval;
        dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(controllers[i].getDestination().getPointer());

        size_t grp = detectAnimGroup(dstval->getNode());

        if(!mAccumRoot && grp == 0)
        {
            mNonAccumRoot = dstval->getNode();
            mAccumRoot = mNonAccumRoot->getParent();
            if(!mAccumRoot)
            {
                std::cerr<< "Non-Accum root for "<<mPtr.getCellRef().getRefId()<<" is skeleton root??" <<std::endl;
                mNonAccumRoot = NULL;
            }
        }

        if (grp == 0 && (dstval->getNode()->getName() == "Bip01" || dstval->getNode()->getName() == "Root Bone"))
        {
            mNonAccumRoot = dstval->getNode();
            mAccumRoot = mNonAccumRoot->getParent();
            if(!mAccumRoot)
            {
                std::cerr<< "Non-Accum root for "<<mPtr.getCellRef().getRefId()<<" is skeleton root??" <<std::endl;
                mNonAccumRoot = NULL;
            }
        }

        controllers[i].setSource(mAnimationTimePtr[grp]);
        grpctrls[grp].push_back(controllers[i]);
    }

    for (unsigned int i = 0; i < mObjectRoot->mControllers.size(); ++i)
    {
        if (mObjectRoot->mControllers[i].getSource().isNull())
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
        { MWWorld::InventoryStore::Slot_ForeignHelmet,      0 },
        { MWWorld::InventoryStore::Slot_ForeignUpperBody,   0 },
        { MWWorld::InventoryStore::Slot_ForeignLowerBody,   0 },
        { MWWorld::InventoryStore::Slot_ForeignLeftHand,    0 },
        { MWWorld::InventoryStore::Slot_ForeignRightHand,   0 },
        { MWWorld::InventoryStore::Slot_ForeignBoots,       0 },
        { MWWorld::InventoryStore::Slot_ForeignCarriedRight,0 },
        { MWWorld::InventoryStore::Slot_ForeignCarriedLeft, 0 }
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
    std::cout << "anim play" << std::endl;
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

    //mHeadAnimationTime->update(timepassed);

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

            Ogre::Node* node = baseinst->getBone("Bip01 Head");
            if (node)
                node->rotate(Ogre::Quaternion(mHeadYaw, Ogre::Vector3::UNIT_Z) * Ogre::Quaternion(mHeadPitch, Ogre::Vector3::UNIT_X), Ogre::Node::TS_WORLD);
        }
    }
    mFirstPersonOffset = 0.f; // reset the X, Y, Z offset for the next frame.

    for(size_t i = 0;i < ESM::PRT_Count;i++)
    {
        if (mObjectParts[i].isNull())
            continue;
        std::vector<Ogre::Controller<Ogre::Real> >::iterator ctrl(mObjectParts[i]->mControllers.begin());
        for(;ctrl != mObjectParts[i]->mControllers.end();++ctrl)
            ctrl->update();

        if (!isSkinned(mObjectParts[i]))
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

    mObjectParts[type].setNull();
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
        const std::string& bonename = sPartList.at(type);
        // PRT_Hair seems to be the only type that breaks consistency and uses a filter that's different from the attachment bone
        const std::string bonefilter = (type == ESM::PRT_Hair) ? "hair" : bonename;
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
        if(ctrl->getSource().isNull())
        {
            ctrl->setSource(mNullAnimationTimePtr);
#if 0
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
#endif
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
                    mAmmunition.setNull();
            }
            else
                mAmmunition.setNull();
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
        if (mObjectParts[i].isNull())
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
    //mHeadAnimationTime->setEnabled(enable);
}

void ForeignNpcAnimation::preRender(Ogre::Camera *camera)
{
    Animation::preRender(camera);
    for (int i=0; i<ESM::PRT_Count; ++i)
    {
        if (mObjectParts[i].isNull())
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
//
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
