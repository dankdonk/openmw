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
#ifndef OPENMW_COMPONENTS_NIFOGRE_OBJECTSCENE_HPP
#define OPENMW_COMPONENTS_NIFOGRE_OBJECTSCENE_HPP

#include <OgreMaterial.h>
#include <OgreController.h>

#include <vector>
#include <string>
#include <map>

namespace Ogre
{
    class Entity;
    class ParticleSystem;
    class Light;
    class Node;
    class SceneManager;
    class MovableObject;
}

namespace NifOgre
{
    typedef std::multimap<float, std::string> TextKeyMap;

    /**
     * @brief Clones materials as necessary to not make controllers affect other objects (that
     * share the original material).
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

    struct ObjectScene
    {
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

        // The maximum length on any of the controllers. For animations with controllers, but
        // no text keys, consider this the animation length.
        float mMaxControllerLength;

        TextKeyMap mTextKeys;

        MaterialControllerManager mMaterialControllerMgr;

        std::vector<Ogre::Controller<Ogre::Real> > mControllers;

        ObjectScene (Ogre::SceneManager* sceneMgr);

        ~ObjectScene ();

        // Rotate nodes in mBillboardNodes so they face the given camera
        void rotateBillboardNodes (Ogre::Camera* camera);

        void setVisibilityFlags (unsigned int flags);

        // This is called internally by the OgreNifLoader once all elements of the
        // scene have been attached to their respective nodes.
        void _notifyAttached ();
    };

    typedef Ogre::SharedPtr<ObjectScene> ObjectScenePtr;
}

#endif // OPENMW_COMPONENTS_NIFOGRE_OBJECTSCENE_HPP
