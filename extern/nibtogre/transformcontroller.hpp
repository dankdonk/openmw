#ifndef NIBTOGRE_TRANSFORMCONTROLLER_H
#define NIBTOGRE_TRANSFORMCONTROLLER_H

//#include <OgrePrerequisites.h>
#include <OgreControllerManager.h>
#include <OgreController.h>
#include <OgreNode.h> // FIXME: debugging
#include <OgreBone.h> // FIXME: debugging

//#include <components/nifogre/controller.hpp>    // ValueInterpolator, DefaultFunction
#include <components/nifogre/ogrenifloader.hpp> // NodeTargetValue

#include "nimodelmanager.hpp"
#include "nitimecontroller.hpp"
#include "niinterpolator.hpp"
#include "nidata.hpp"
#include "nimodel.hpp"

namespace NiBtOgre
{
    class DefaultFunction : public Ogre::ControllerFunction<Ogre::Real>
    {
    private:
        float mFrequency;
        float mPhase;
        float mStartTime;
    public:
        float mStopTime;

    public:
        DefaultFunction(const NiTimeController *controller, bool deltaInput)
            : Ogre::ControllerFunction<Ogre::Real>(deltaInput)
            , mFrequency(controller->mFrequency)
            , mPhase(controller->mPhase)
            , mStartTime(controller->mStartTime)
            , mStopTime(controller->mStopTime)
        {
            if(mDeltaInput)
                mDeltaCount = mPhase;
        }

        virtual Ogre::Real calculate(Ogre::Real value)
        {
            if(mDeltaInput)
            {
                if (mStopTime - mStartTime == 0.f)
                    return 0.f;

                mDeltaCount += value*mFrequency;
                if(mDeltaCount < mStartTime)
                    mDeltaCount = mStopTime - std::fmod(mStartTime - mDeltaCount,
                                                        mStopTime - mStartTime);
                mDeltaCount = std::fmod(mDeltaCount - mStartTime,
                                        mStopTime - mStartTime) + mStartTime;
                return mDeltaCount;
            }

            value = std::min(mStopTime, std::max(mStartTime, value+mPhase));
            return value;
        }
    };

    class TransformController
    {
    public:
        class Value : public NifOgre::NodeTargetValue<Ogre::Real>
        {
    //      const Nif::QuaternionKeyMap* mRotations;
    //      const Nif::FloatKeyMap* mXRotations;
    //      const Nif::FloatKeyMap* mYRotations;
    //      const Nif::FloatKeyMap* mZRotations;
    //      const Nif::Vector3KeyMap* mTranslations;
    //      const Nif::FloatKeyMap* mScales;

    //      const std::vector<QuatKey<Ogre::Quaternion> >& mQuaternionKeys;

    //      const KeyGroup<float>& mXRotations;
    //      const KeyGroup<float>& mYRotations;
    //      const KeyGroup<float>& mZRotations;

    //      const KeyGroup<Ogre::Vector3>& mTranslations;
    //      const KeyGroup<float>& mScales;

            const NiInterpolator* mInterpolator;
            const NiTransformData* mTransformData;
            NiModelPtr mModel; // Hold a SharedPtr to make sure key lists stay valid

            std::string boneName; // FIXME: debugging
            bool isManual;

            // FIXME: are these used?
            int mLastRotate;
            int mLastTranslate;
            int mLastScale;

            static Ogre::Quaternion interpQuatKey(const KeyGroup<Ogre::Quaternion>& keyGroup, uint32_t cycleType, float time);

            Ogre::Quaternion getXYZRotation(float time) const;

        public:
            Value (Ogre::Node *target, const NiModelPtr& model, const NiInterpolator *interpolator)
              : NodeTargetValue(target) , mInterpolator(interpolator) , mModel(model)
              , boneName(target->getName()), isManual(static_cast<Ogre::Bone*>(target)->isManuallyControlled())
              , mLastRotate(0) , mLastTranslate(0) , mLastScale(0)
            {
                const NiTransformInterpolator* transInterp
                    = dynamic_cast<const NiTransformInterpolator*>(interpolator);
                if (transInterp)
                {
                    mTransformData = model->getRef<NiTransformData>(transInterp->mDataIndex);
                }

                if (boneName == "" || !isManual)
                    throw std::runtime_error("empty bone name or not manual controlled");
            }

            virtual Ogre::Quaternion getRotation(Ogre::Real time) const;// { return Ogre::Quaternion(); }

            virtual Ogre::Vector3 getTranslation(Ogre::Real time) const;// { return Ogre::Vector3(); }

            virtual Ogre::Vector3 getScale(Ogre::Real time) const;// { return Ogre::Vector3(); }

            virtual Ogre::Real getValue() const;// { return 0.f; }

            virtual void setValue(Ogre::Real time);// {}
        };

        typedef DefaultFunction Function;
    };
}
#endif // NIBTOGRE_TRANSFORMCONTROLLER_H
