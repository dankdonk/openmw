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
#ifndef OPENMW_COMPONENTS_NIFOGRE_NODETARGETVALUE_HPP
#define OPENMW_COMPONENTS_NIFOGRE_NODETARGETVALUE_HPP

#include <OgreController.h>
#include <OgreQuaternion.h>
#include <OgreVector3.h>

namespace Ogre
{
    class Node;
}

namespace NifOgre
{
    // FIXME: Should be with other general Ogre extensions.
    template<typename T>
    class NodeTargetValue : public Ogre::ControllerValue<T>
    {
    protected:
        Ogre::Node *mNode;

    public:
        NodeTargetValue (Ogre::Node *target) : mNode(target) {}

        virtual Ogre::Quaternion getRotation (T value) const = 0;
        virtual Ogre::Vector3 getTranslation (T value) const = 0;
        virtual Ogre::Vector3 getScale (T value) const = 0;

        void setNode (Ogre::Node *target) { mNode = target; }

        Ogre::Node *getNode () const { return mNode; }
    };

    typedef Ogre::SharedPtr<NodeTargetValue<Ogre::Real> > NodeTargetValueRealPtr;
}

#endif
