#ifndef COMPONENTS_NIFOGRE_CONTROLLER_H
#define COMPONENTS_NIFOGRE_CONTROLLER_H

#include <vector>
#include <string>

#include <boost/shared_ptr.hpp>

#include <OgreController.h>

#include <components/nif/niffile.hpp>
#include <components/nif/nifkey.hpp>
#include <components/nif/data.hpp>

#include "nodetargetvalue.hpp"

namespace Ogre
{
    class MovableObject;
    class ParticleSystem;
    class Entity;
    class Node;
}

namespace Nif
{
    class Controller;
    class NiFlipController;
    class NiInterpolator;
    class NiParticleSystemController;

    typedef boost::shared_ptr<Nif::NIFFile> NIFFilePtr;
}

namespace NifOgre
{
    class MaterialControllerManager;

    // FIXME: Should not be here.
    class DefaultFunction : public Ogre::ControllerFunction<Ogre::Real>
    {
        float mFrequency;
        float mPhase;
        float mStartTime;
    public:
        float mStopTime;

    public:
        DefaultFunction (const Nif::Controller *ctrl, bool deltaInput);

        virtual Ogre::Real calculate(Ogre::Real value);
    };

    class ValueInterpolator
    {
    protected:
        float interpKey(const Nif::FloatKeyMap::MapType &keys, float time, float def=0.f) const;

        Ogre::Vector3 interpKey(const Nif::Vector3KeyMap::MapType &keys, float time) const;
    };

    // Animates a texture
    class FlipController
    {
    public:
        class Value : public Ogre::ControllerValue<Ogre::Real>, public ValueInterpolator
        {
            Ogre::MovableObject* mMovable;
            int mTexSlot;
            float mDelta;
            std::vector<std::string> mTextures;
            MaterialControllerManager* mMaterialControllerMgr;

            using ValueInterpolator::interpKey;

        public:
            Value (Ogre::MovableObject *movable,
                    const Nif::NiFlipController *ctrl, MaterialControllerManager* materialControllerMgr);

            virtual Ogre::Real getValue() const;

            virtual void setValue(Ogre::Real time);
        };

        typedef DefaultFunction Function;
    };

    class AlphaController
    {
    public:
        class Value : public Ogre::ControllerValue<Ogre::Real>, public ValueInterpolator
        {
            Ogre::MovableObject* mMovable;
            Nif::FloatKeyMap mData;
            MaterialControllerManager* mMaterialControllerMgr;

        public:
            Value (Ogre::MovableObject *movable,
                    const Nif::NiFloatData *data, MaterialControllerManager* materialControllerMgr);

            virtual Ogre::Real getValue() const;

            virtual void setValue(Ogre::Real time);
        };

        typedef DefaultFunction Function;
    };

    class MaterialColorController
    {
    public:
        class Value : public Ogre::ControllerValue<Ogre::Real>, public ValueInterpolator
        {
            Ogre::MovableObject* mMovable;
            Nif::Vector3KeyMap mData;
            MaterialControllerManager* mMaterialControllerMgr;

        public:
            Value (Ogre::MovableObject *movable,
                    const Nif::NiPosData *data, MaterialControllerManager* materialControllerMgr);

            virtual Ogre::Real getValue() const;

            virtual void setValue(Ogre::Real time);
        };

        typedef DefaultFunction Function;
    };

    class VisController
    {
    public:
        class Value : public NodeTargetValue<Ogre::Real>
        {
            std::vector<Nif::NiVisData::VisData> mData;

            bool calculate(Ogre::Real time) const;

            static void setVisible(Ogre::Node *node, bool vis);

        public:
            Value (Ogre::Node *target, const Nif::NiVisData *data);

            virtual Ogre::Quaternion getRotation(float time) const;

            virtual Ogre::Vector3 getTranslation(float time) const;

            virtual Ogre::Vector3 getScale(float time) const;

            virtual Ogre::Real getValue() const;

            virtual void setValue(Ogre::Real time);
        };

        typedef DefaultFunction Function;
    };

    class TransformController
    {
    public:
        class Value : public NodeTargetValue<Ogre::Real>, public ValueInterpolator
        {
            const Nif::QuaternionKeyMap* mRotations;
            const Nif::FloatKeyMap* mXRotations;
            const Nif::FloatKeyMap* mYRotations;
            const Nif::FloatKeyMap* mZRotations;
            const Nif::Vector3KeyMap* mTranslations;
            const Nif::FloatKeyMap* mScales;
            const Nif::NiInterpolator* mData;
            Nif::NIFFilePtr mNif; // Hold a SharedPtr to make sure key lists stay valid

            using ValueInterpolator::interpKey;

            static Ogre::Quaternion interpKey(const Nif::QuaternionKeyMap::MapType &keys, float time);

            Ogre::Quaternion getXYZRotation(float time) const;

        public:
            Value (Ogre::Node *target, const Nif::NIFFilePtr& nif, const Nif::NiInterpolator *data);

            virtual Ogre::Quaternion getRotation(float time) const;

            virtual Ogre::Vector3 getTranslation(float time) const;

            virtual Ogre::Vector3 getScale(float time) const;

            virtual Ogre::Real getValue() const;

            virtual void setValue(Ogre::Real time);
        };

        typedef DefaultFunction Function;
    };

    class KeyframeController
    {
    public:
        class Value : public NodeTargetValue<Ogre::Real>, public ValueInterpolator
        {
            const Nif::QuaternionKeyMap* mRotations;
            const Nif::FloatKeyMap* mXRotations;
            const Nif::FloatKeyMap* mYRotations;
            const Nif::FloatKeyMap* mZRotations;
            const Nif::Vector3KeyMap* mTranslations;
            const Nif::FloatKeyMap* mScales;
            Nif::NIFFilePtr mNif; // Hold a SharedPtr to make sure key lists stay valid

            using ValueInterpolator::interpKey;

            static Ogre::Quaternion interpKey(const Nif::QuaternionKeyMap::MapType &keys, float time);

            Ogre::Quaternion getXYZRotation(float time) const;

        public:
            /// @note The NiKeyFrameData must be valid as long as this KeyframeController exists.
            Value (Ogre::Node *target, const Nif::NIFFilePtr& nif, const Nif::NiKeyframeData *data);

            virtual Ogre::Quaternion getRotation(float time) const;

            virtual Ogre::Vector3 getTranslation(float time) const;

            virtual Ogre::Vector3 getScale(float time) const;

            virtual Ogre::Real getValue() const;

            virtual void setValue(Ogre::Real time);
        };

        typedef DefaultFunction Function;
    };

    class UVController
    {
    public:
        class Value : public Ogre::ControllerValue<Ogre::Real>, public ValueInterpolator
        {
            Ogre::MovableObject* mMovable;
            Nif::FloatKeyMap mUTrans;
            Nif::FloatKeyMap mVTrans;
            Nif::FloatKeyMap mUScale;
            Nif::FloatKeyMap mVScale;
            MaterialControllerManager* mMaterialControllerMgr;

        public:
            Value (Ogre::MovableObject* movable,
                    const Nif::NiUVData *data, MaterialControllerManager* materialControllerMgr);

            virtual Ogre::Real getValue() const;

            virtual void setValue(Ogre::Real value);
        };

        typedef DefaultFunction Function;
    };

    class ParticleSystemController
    {
    public:
        class Value : public Ogre::ControllerValue<Ogre::Real>
        {
            Ogre::ParticleSystem *mParticleSys;
            float mEmitStart;
            float mEmitStop;

        public:
            Value (Ogre::ParticleSystem *psys, const Nif::NiParticleSystemController *pctrl);

            Ogre::Real getValue() const;

            void setValue(Ogre::Real value);
        };

        typedef DefaultFunction Function;
    };

    class GeomMorpherController
    {
    public:
        class Value : public Ogre::ControllerValue<Ogre::Real>, public ValueInterpolator
        {
            Ogre::Entity *mEntity;
            std::vector<Nif::NiMorphData::MorphData> mMorphs;
            size_t mControllerIndex;

            std::vector<Ogre::Vector3> mVertices;

        public:
            Value (Ogre::Entity *ent, const Nif::NiMorphData *data, size_t controllerIndex);

            virtual Ogre::Real getValue() const;

            virtual void setValue(Ogre::Real time);
        };

        typedef DefaultFunction Function;
    };
}

#endif
