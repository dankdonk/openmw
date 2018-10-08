/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (ogre_nif_loader.cpp) is part of the OpenMW package.

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
#ifndef OPENMW_COMPONENTS_NIFOGRE_NIFOBJECTLOADER_HPP
#define OPENMW_COMPONENTS_NIFOGRE_NIFOBJECTLOADER_HPP

#include <boost/shared_ptr.hpp>

#include <OgreController.h>

#include <components/nif/recordptr.hpp>
#include <components/nif/niffile.hpp>

#include "ogrenifloader.hpp"

namespace Ogre
{
    class MovableObject;
    class ParticleSystem;
    class SceneManager;
    class SceneNode;
    class Bone;
}

namespace Nif
{
    class Node;
    class NiParticleSystemController;
    class NiAutoNormalParticlesData;
    class NiParticleSystemController;
    class NiTextKeyExtraData;

    typedef boost::shared_ptr<Nif::NIFFile> NIFFilePtr;
}

namespace NifOgre
{
    /** Object creator for NIFs. This is the main class responsible for creating
     * "live" Ogre objects (entities, particle systems, controllers, etc) from
     * their NIF equivalents.
     */
    class NIFObjectLoader
    {
        static bool sShowMarkers;

        static void warn (const std::string &msg);

        static void createEntity (const std::string &name,
                const std::string &group, Ogre::SceneManager *sceneMgr, ObjectScenePtr scene,
                const Nif::Node *node, int flags, int animflags);

        static void createMaterialControllers (const Nif::Node* node,
                Ogre::MovableObject* movable, int animflags, ObjectScenePtr scene);

        static void createParticleEmitterAffectors (Ogre::ParticleSystem *partsys,
                const Nif::NiParticleSystemController *partctrl, Ogre::Bone* bone, const std::string& skelBaseName);

        static void createParticleSystem (const std::string &name,
                const std::string &group, Ogre::SceneNode *sceneNode, ObjectScenePtr scene,
                const Nif::Node *partnode, int flags, int partflags, int animflags);

        static void createParticleInitialState (Ogre::ParticleSystem* partsys,
                const Nif::NiAutoNormalParticlesData* particledata, const Nif::NiParticleSystemController* partctrl);

        static void createNodeControllers (const Nif::NIFFilePtr& nif,
                const std::string &name, Nif::ControllerPtr ctrl, ObjectScenePtr scene, int animflags);

        static void extractTextKeys (const Nif::NiTextKeyExtraData *tk, TextKeyMap &textkeys);

        static void createObjects (const Nif::NIFFilePtr& nif,
                const std::string &name, const std::string &group, Ogre::SceneNode *sceneNode, const Nif::Node *node,
                ObjectScenePtr scene, int flags, int animflags, int partflags, bool isRootCollisionNode = false);
#if 0
        static void handleNode2 (const Nif::NIFFilePtr& nif,
                const std::string &name, const std::string &group, Ogre::SceneNode *sceneNode, const Nif::Node *node,
                ObjectScenePtr scene);
#endif
        static void createSkelBase (const std::string &name,
                const std::string &group, Ogre::SceneManager *sceneMgr, const Nif::Node *node, ObjectScenePtr scene);

    public:
        static void setShowMarkers (bool show);

        static void load (Ogre::SceneNode *sceneNode,
                ObjectScenePtr scene, const std::string &name, const std::string &group, int flags=0);

        static void loadKf (Ogre::Skeleton *skel,
                const std::string &name, TextKeyMap &textKeys, std::vector<Ogre::Controller<Ogre::Real> > &ctrls);
    };
}
#endif // OPENMW_COMPONENTS_NIFOGRE_NIFOBJECTLOADER_HPP
