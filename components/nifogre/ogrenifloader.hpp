/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (ogre_nif_loader.h) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIFOGRE_OGRENIFLOADER_HPP
#define OPENMW_COMPONENTS_NIFOGRE_OGRENIFLOADER_HPP

#include<boost/shared_ptr.hpp>

#include <OgreResource.h>
#include <OgreMaterial.h>
#include <OgreController.h>

#include <vector>
#include <string>
#include <map>

#include "controller.hpp" // ValueInterpolator

namespace Nif
{
    class Controller;
    class NiInterpolator;

    typedef boost::shared_ptr<Nif::NIFFile> NIFFilePtr;
}

// FIXME: This namespace really doesn't do anything Nif-specific. Any supportable
// model format should go through this.
namespace NifOgre
{

/**
 * @brief Clones materials as necessary to not make controllers affect other objects (that share the original material).
 */
class MaterialControllerManager
{
public:
    ~MaterialControllerManager();

    /// @attention if \a movable is an Entity, it needs to have *one* SubEntity
    Ogre::MaterialPtr getWritableMaterial (Ogre::MovableObject* movable);

private:
    std::map<Ogre::MovableObject*, Ogre::MaterialPtr> mClonedMaterials;
};

typedef std::multimap<float,std::string> TextKeyMap;
static const char sTextKeyExtraDataID[] = "TextKeyExtraData";
struct ObjectScene {
    Ogre::Entity *mSkelBase;
    std::vector<Ogre::Entity*> mEntities;
    std::vector<Ogre::ParticleSystem*> mParticles;
    std::vector<Ogre::Light*> mLights;

    // FIXME: this doesn't really belong here but since we create Ogre::Entities separately
    // to physics objects we need some way of tying those together for ragdoll movements
    std::unordered_multimap<size_t, Ogre::Entity*> mRagdollEntities;

    // Nodes that should always face the camera when rendering
    std::vector<Ogre::Node*> mBillboardNodes;

    Ogre::SceneManager* mSceneMgr;

    // The maximum length on any of the controllers. For animations with controllers, but no text keys, consider this the animation length.
    float mMaxControllerLength;

    TextKeyMap mTextKeys;

    MaterialControllerManager mMaterialControllerMgr;

    std::vector<Ogre::Controller<Ogre::Real> > mControllers;

    ObjectScene(Ogre::SceneManager* sceneMgr) : mSkelBase(0), mSceneMgr(sceneMgr), mMaxControllerLength(0)
    { }

    ~ObjectScene();

    // Rotate nodes in mBillboardNodes so they face the given camera
    void rotateBillboardNodes(Ogre::Camera* camera);

    void setVisibilityFlags (unsigned int flags);

    // This is called internally by the OgreNifLoader once all elements of the
    // scene have been attached to their respective nodes.
    void _notifyAttached();
};

typedef Ogre::SharedPtr<ObjectScene> ObjectScenePtr;


class Loader
{
public:
    static ObjectScenePtr createObjects(Ogre::Entity *parent, const std::string &bonename,
                                        const std::string& filter,
                                    Ogre::SceneNode *parentNode,
                                    std::string name,
                                    const std::string &group="General");

    static ObjectScenePtr createObjects(Ogre::SceneNode *parentNode,
                                    std::string name,
                                    const std::string &group="General");

    static ObjectScenePtr createObjectBase(Ogre::SceneNode *parentNode,
                                       std::string name,
                                       const std::string &group="General");

    /// Set whether or not nodes marked as "MRK" should be shown.
    /// These should be hidden ingame, but visible in the editior.
    /// Default: false.
    static void setShowMarkers(bool show);

    static void createKfControllers(Ogre::Entity *skelBase,
                                    const std::string &name,
                                    TextKeyMap &textKeys,
                                    std::vector<Ogre::Controller<Ogre::Real> > &ctrls);

private:
    static bool sShowMarkers;
};

// FIXME: Should be with other general Ogre extensions.
template<typename T>
class NodeTargetValue : public Ogre::ControllerValue<T>
{
protected:
    Ogre::Node *mNode;

public:
    NodeTargetValue(Ogre::Node *target) : mNode(target)
    { }

    virtual Ogre::Quaternion getRotation(T value) const = 0;
    virtual Ogre::Vector3 getTranslation(T value) const = 0;
    virtual Ogre::Vector3 getScale(T value) const = 0;

    void setNode(Ogre::Node *target)
    { mNode = target; }
    Ogre::Node *getNode() const
    { return mNode; }
};
typedef Ogre::SharedPtr<NodeTargetValue<Ogre::Real> > NodeTargetValueRealPtr;

// FIXME: not sure whether to replace Default Function
#if 0
class InterpolateFunction : public Ogre::ControllerFunction<Ogre::Real>
{
    const Nif::NiInterpolator* mInterpolator;

public:
    InterpolateFunction (const Nif::NiInterpolator *interp, bool deltaInput);

    virtual Ogre::Real calculate(Ogre::Real value);
};
#endif
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
        const Nif::NiInterpolator* mInterpolator;
        Nif::NIFFilePtr mNif; // Hold a SharedPtr to make sure key lists stay valid
        // FIXME: are these used?
        int mLastRotate;
        int mLastTranslate;
        int mLastScale;

        using ValueInterpolator::interpKey;

        static Ogre::Quaternion interpKey(const Nif::QuaternionKeyMap::MapType &keys, float time);

        Ogre::Quaternion getXYZRotation(float time) const;

    public:
        Value (Ogre::Node *target, const Nif::NIFFilePtr& nif, const Nif::NiInterpolator *interp);

        virtual Ogre::Quaternion getRotation(float time) const;

        virtual Ogre::Vector3 getTranslation(float time) const;

        virtual Ogre::Vector3 getScale(float time) const;

        virtual Ogre::Real getValue() const;

        virtual void setValue(Ogre::Real time);
    };

    typedef DefaultFunction Function;
};

}

#endif
