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

#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <OgreController.h>

#include <vector>
#include <string>

#include "objectscene.hpp"

namespace Ogre
{
    class Entity;
    class SceneNode;
    class Node;
}

// FIXME: This namespace really doesn't do anything Nif-specific. Any supportable
// model format should go through this.
namespace NifOgre
{
    //static const char sTextKeyExtraDataID[] = "TextKeyExtraData";

    class Loader
    {
        static bool sShowMarkers;

    public:
        static ObjectScenePtr createObjects (Ogre::Entity *parent,
                                             const std::string &bonename,
                                             const std::string& filter,
                                             Ogre::SceneNode *parentNode,
                                             std::string name,
                                             const std::string &group="General");

        static ObjectScenePtr createObjects (Ogre::SceneNode *parentNode,
                                             std::string name,
                                             const std::string &group="General");

        static ObjectScenePtr createObjectBase (Ogre::SceneNode *parentNode,
                                                std::string name,
                                                const std::string &group="General");

        /// Set whether or not nodes marked as "MRK" should be shown.
        /// These should be hidden ingame, but visible in the editior.
        /// Default: false.
        static void setShowMarkers(bool show);

        static void createKfControllers (Ogre::Entity *skelBase,
                                         const std::string &name,
                                         TextKeyMap &textKeys,
                                         std::vector<Ogre::Controller<Ogre::Real> > &ctrls);

    };
}

#endif
