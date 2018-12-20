/*
  Copyright (C) 2015-2018 cc9cii

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

  Much of the information on the NIF file structures are based on the NifSkope
  documenation.  See http://niftools.sourceforge.net/wiki/NifSkope for details.

*/
#include "nigeommorphercontroller.hpp"

#include <cassert>
#include <stdexcept>

#include <OgreMesh.h>
#include <OgreAnimation.h>
#include <OgreAnimationTrack.h>
#include <OgreKeyframe.h>

#include "nistream.hpp"
#include "nimodel.hpp"
#include "niobject.hpp" // NiTimeControllerRef
#include "nidata.hpp"
#include "btogreinst.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

// Some TES4 examples:
//
//   architecture/quests/se01waitingroomwalls.nif
//   architecture/quests/se07ardensulalter.nif
//   architecture/ships/mainmast02.nif
//   architecture/statue/nightmotherstatue.nif
//   architecture/statue/nightmotherstatuebase.nif
//
//   plus lots of others, including most creatures
//
// Some TES3 examples:
//
//   b/b_v_wood elf_f_head_01.nif
//   b/b_v_wood elf_m_head_01.nif
//   base_anim_female.1st.nif
//   e/magic_area.nif
//   e/magic_area_conjure.nif
//   ...
//   w/shadowmarksmancrossbow.nif
//   w/w_art_longbow_shade.nif
//   w/w_crossbow.nif
//   w/w_crossbow_dwemer.nif
//   w/w_crossbow_steel.nif
//   w/w_longbow.nif
//   w/w_longbow_ariel.nif
//
NiBtOgre::NiGeomMorpherController::NiGeomMorpherController(uint32_t index, NiStream& stream, const NiModel& model)
    : NiTimeController(index, stream, model), mNiControllerSequenceBuilt(false)
{
    if (stream.nifVer() >= 0x0a000102) // from 10.0.1.2
        stream.read(mExtraFlags);

    if (stream.nifVer() == 0x0a01006a) // 10.1.0.106
        stream.skip(sizeof(char)); // Unknown 2

    stream.read(mDataIndex);
    stream.read(mAlwaysUpdate);

    if (stream.nifVer() >= 0x0a01006a) // from 10.1.0.106
    {
        std::uint32_t numInterpolators;
        stream.read(numInterpolators);
        if (stream.nifVer() >= 0x0a020000 && stream.nifVer() <= 0x14000005)
        {
            mInterpolators.resize(numInterpolators);
            for (unsigned int i = 0; i < numInterpolators; ++i)
                stream.read(mInterpolators.at(i));
        }

        if (stream.nifVer() >= 0x14010003) // from 20.1.0.3
        {
            mInterpolatorWeights.resize(numInterpolators);
            for (unsigned int i = 0; i < numInterpolators; ++i)
            {
                stream.read(mInterpolatorWeights[i].interpolatorIndex);
                stream.read(mInterpolatorWeights[i].weight);
            }
        }

        if (stream.nifVer() >= 0x14000004 && stream.nifVer() <= 0x14000005 && stream.userVer() >= 10)
        {
            std::uint32_t numUnknownInts;
            stream.read(numUnknownInts);
            mUnknownInts.resize(numUnknownInts);
            for (unsigned int i = 0; i < numUnknownInts; ++i)
                stream.read(mUnknownInts.at(i));
        }
    }
}

NiBtOgre::NiTimeControllerRef NiBtOgre::NiGeomMorpherController::build(BtOgreInst *inst, Ogre::Mesh *mesh)
{
    if (NiObject::mModel.nifVer() <= 0x0a010000)
        return setupTES3Animation(inst, mesh);

    if ((NiTimeController::mFlags & 0x1000) == 0) // not active
        return mNextControllerIndex;

    std::string animationId = "NiGeomMorph" + std::to_string(NiObject::index()); // mSelfIndex
    float totalAnimationLength = NiTimeController::mStopTime - NiTimeController::mStartTime;

    Ogre::Animation *animation = mesh->createAnimation(animationId, totalAnimationLength);

    assert(mesh->getNumSubMeshes() != 0); // should be at least 1
    std::uint32_t subMeshIndex = mesh->getNumSubMeshes()-1;
    unsigned short poseIndex = (unsigned short)mesh->getPoseCount();

    // NOTE: assume that NiControllerManager and NiControllerSequence is built already
    //assert(mNiControllerSequeceBuilt == true); // throw instead?

    // For later versions the keyframe interpolators are not part of NiMorphData::Morph but instead
    // found in the ControllerBlock.
    //
    //   NiControllerSequence = animation, e.g. "Idle"
    //   Controlled Blocks  = track/pose
    //   Interpolator->Data = keyframe
    //
    // In order to find above they need to be stored in 'inst' with the index of the controller
    // as the key. e.g.
    //
    //   std::map<int, std::vector<int> > mGeomMorpherControllerMap
    //             ^                ^
    //             :                :
    //             :             index of controlled block (corresponds to a NiMorphData::Morph)
    //             :
    //          block index of NiGeomMorpherController

    // create a pose & track for each Morph
    NiMorphData* morphData = mModel.getRef<NiMorphData>(mDataIndex);
    const std::vector<NiMorphData::Morph>& morphs = morphData->mMorphs;
    // FIXME: ignore base? or create a base pose with a keyframe at time 0 and PoseRef influence of 1.f?
    for (unsigned int i = 1; i < morphs.size(); ++i)
    {
//        const std::vector<int>& controlledBlocks = mGeomMorpherControllerMap[mSelfIndex];
//        NiControllerSequence* seq = mModel.getRef<NiControllerSequence>(inst->seqIndex);

//      if (morphs[i].mKeys.size() == 0)
//          continue; // NOTE: ignore morphs that have no keys (i.e. tracks with no keyframes)

        // NOTE multiple poses are created for the same 'target' (i.e. sub-mesh)
        Ogre::Pose* pose = mesh->createPose(subMeshIndex+1); // target is user defined (for us subMeshIndex+1)
        for (unsigned int j = 0; j < morphs[i].mVectors.size(); ++j)
            pose->addVertex(j, morphs[i].mVectors[j]);

        // NOTE: 'handle' is set to subMeshIndex+1 to locate the correct sub-entity vertices when
        //       Ogre::Animation::apply() is called
        Ogre::VertexAnimationTrack* track = animation->createVertexTrack(subMeshIndex+1, Ogre::VAT_POSE);
//      for (unsigned int k = 0; k < morphs[i].mKeys.size(); ++k)
//      {
//          Ogre::VertexPoseKeyFrame* keyframe = track->createVertexPoseKeyFrame(morphs[i].mKeys[k].time);
//          keyframe->addPoseReference(poseIndex + i, morphs[i].mKeys[k].value);

//          // FIXME: set custom interpolation code for a derived track here?
//          // But VertexAnimationTrack doesn't use listener! :-(
//          // Maybe use a modified version of Ogre so that VertexAminationTrack::getInterpolatedKeyFrame
//          // uses a listener?  Unmodified Ogre should also work, just that we won't get quadratic
//          // interpolation.
//          AnimTrackInterpolator<float> *listner
//              = OGRE_NEW AnimTrackInterpolator<float>(/*quadratic*/2, morphs[i].mKeys[k]);
//          track->setListener(listner); // custom getInterpolatedKeyFrame()

//          inst->mInterpolators.push_back(listner); // plug potential memory leak
//      }
    }

    return mNextControllerIndex;
}

// FIXME: implement NiTimeController::mFlags
//
// Controller flags (usually 0x000C). Probably controls loops.
// Bit 0 : Anim type, 0=APP_TIME 1=APP_INIT
// Bit 1-2 : Cycle type 00=Loop 01=Reverse 10=Loop
// Bit 3 : Active
// Bit 4 : Play backwards

// FIXME: how to call this method in a generic way via a base class interface?
//
// FIXME: may need to have 2 versions, one for TES3 and one for TES4 (with interpolators).
// Later versions of NiMorphData don't have keys... But there seems to be some relationship
// with NiControllerSequence.  Check examples e.g. mainmast02.nif which has idle animation
//
// See Ogre::Animation::apply() regarding the value of AnimationTrack::mHandle for sub-meshes.
//
// FIXME: how to apply NiTimeController's frequency, start/stop time, etc?
//
NiBtOgre::NiTimeControllerRef NiBtOgre::NiGeomMorpherController::setupTES3Animation(BtOgreInst *inst, Ogre::Mesh *mesh)
{
    // Newer verions of NiMorphData::Morph don't have keys, use NiControllerSequence instead
    // (in TES4 only creatures/minotaur/minotaurold.nif is older version, 10.0.1.2, and it
    // isn't referred in any of the official ESM/ESP; also doesn't have a NiGeomMorpherController)
    if (NiObject::mModel.nifVer() > 0x0a010000) // 10.1.0.0
        return mNextControllerIndex; // TODO: maybe assert or throw rather than silently returning?

    if ((NiTimeController::mFlags & 0x1000) == 0) // not active
        return mNextControllerIndex;

    // An animation has a number of tracks (i.e. poses/morphs in our case).
    //
    // A track is a sequence of keyframes.  The keyframes in a track have the same pose index.
    //
    // (I think) the keyframes have different amount of morphing (i.e. influence) from the base.
    // There can be more than one PoseRef per keyframe, but not in this implementation below.
    //
    //   Ogre::Mesh
    //     o    o
    //     |    |            (NOTE: Animation needs a unique name within the Mesh)
    //     |    |   N
    //     |    +---- Ogre::Animation
    //     |  mAnimationList    o             Ogre::AnimationTrack
    //     |                    |                      ^
    //     |                    |        N             |
    //     |                    +--------- Ogre::VertexAnimationTrack (one per Morph)
    //     |                 mVertexTrackList       o     o
    //     |    N                                   |     |   1
    //     +---- Ogre::Pose                         |     +---- handle (subMeshIndex+1)
    //   mPoseList    o                             |   mHandle
    //        :       |   1                         |
    //        :       +---- target (subMeshIndex+1) |   N
    //        :     mTarget                         +---- Ogre::VertexPoseKeyFrame (one per Key)
    //        :                                   mKeyFrames       o         o
    //        :                                                    |         |
    //        :                                                    |         +---- time
    //        :                                                    |   1
    //        :                                                    +---- PoseRef
    //        :                                                           o  o
    //        :                                                           |  |
    //        :                                                           |  +---- influence
    //        :                                                           |         (value)
    //        :....................................................... poseIndex
    //

    std::string animationId = "NiGeomMorph" + std::to_string(NiObject::index()); // mSelfIndex
    float totalAnimationLength = NiTimeController::mStopTime - NiTimeController::mStartTime;

    Ogre::Animation *animation = mesh->createAnimation(animationId, totalAnimationLength);

    assert(mesh->getNumSubMeshes() != 0); // should be at least 1
    std::uint32_t subMeshIndex = mesh->getNumSubMeshes()-1;
    unsigned short poseIndex = (unsigned short)mesh->getPoseCount();

    // create a pose & track for each Morph
    NiMorphData* morphData = mModel.getRef<NiMorphData>(mDataIndex);
    const std::vector<NiMorphData::Morph>& morphs = morphData->mMorphs;
    // FIXME: ignore base? or create a base pose with a keyframe at time 0 and PoseRef influence of 1.f?
    for (unsigned int i = 1; i < morphs.size(); ++i)
    {
        if (morphs[i].mKeys.size() == 0)
            continue; // NOTE: ignore morphs that have no keys (i.e. tracks with no keyframes)

        // NOTE multiple poses are created for the same 'target' (i.e. sub-mesh)
        Ogre::Pose* pose = mesh->createPose(subMeshIndex+1); // target is user defined (for us subMeshIndex+1)
        for (unsigned int j = 0; j < morphs[i].mVectors.size(); ++j)
            pose->addVertex(j, morphs[i].mVectors[j]);

        // NOTE: 'handle' is set to subMeshIndex+1 to locate the correct sub-entity vertices when
        //       Ogre::Animation::apply() is called
        Ogre::VertexAnimationTrack* track = animation->createVertexTrack(subMeshIndex+1, Ogre::VAT_POSE);
        for (unsigned int k = 0; k < morphs[i].mKeys.size(); ++k)
        {
            Ogre::VertexPoseKeyFrame* keyframe = track->createVertexPoseKeyFrame(morphs[i].mKeys[k].time);
            keyframe->addPoseReference(poseIndex + i, morphs[i].mKeys[k].value);

            // FIXME: set custom interpolation code for a derived track here?
            // But VertexAnimationTrack doesn't use listener! :-(
            // Maybe use a modified version of Ogre so that VertexAminationTrack::getInterpolatedKeyFrame
            // uses a listener?  Unmodified Ogre should also work, just that we won't get quadratic
            // interpolation.
            AnimTrackInterpolator<float> *listner
                = OGRE_NEW AnimTrackInterpolator<float>(/*quadratic*/2, morphs[i].mKeys[k]);
            track->setListener(listner); // custom getInterpolatedKeyFrame()

            inst->mInterpolators.push_back(listner); // plug potential memory leak
        }
    }

    return mNextControllerIndex;
}

#if 0
// Ogre::Controller<T>::update
//
//   mDest->setValue(mFunc->calculate(mSource->getValue()));
//
// For NiGeomMorpherController, setValue changes the Entity's animation state.
//
const Ogre::Controller<Ogre::Real>& NiBtOgre::NiGeomMorpherController::createController(BtOgreInst *inst)
{
    Ogre::ControllerValueRealPtr src(isAnimationAutoPlay ?
                                        Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                        Ogre::ControllerValueRealPtr());

    Ogre::ControllerValueRealPtr dest(OGRE_NEW GeomMorpherController::Value(
        entity, geom->data.getPtr(), geom->recIndex));



    GeomMorpherController::Function* function = OGRE_NEW GeomMorpherController::Function(geom, isAnimationAutoPlay);
    scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
    Ogre::ControllerFunctionRealPtr func(function);

    inst->mControllers.push_back(Ogre::Controller<Ogre::Real>(src, dest, func));
}
#endif
