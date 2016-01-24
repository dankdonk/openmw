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
#include "ogrenifloader.hpp"

#include <OgreSceneNode.h>
#include <OgreEntity.h>
#include <OgreTagPoint.h>
#include <OgreMesh.h>
#include <OgreSkeletonInstance.h>

#include <components/misc/stringops.hpp>

#include "nifobjectloader.hpp"

namespace NifOgre
{
// entry point from either CSVRender::Object or MWRender::Animation or others
ObjectScenePtr Loader::createObjects (Ogre::SceneNode *parentNode, std::string name, const std::string &group)
{
    ObjectScenePtr scene = ObjectScenePtr (new ObjectScene(parentNode->getCreator()));

    Misc::StringUtils::lowerCaseInPlace(name);
    NIFObjectLoader::load(parentNode, scene, name, group);

    for(size_t i = 0;i < scene->mEntities.size();i++)
    {
        Ogre::Entity *entity = scene->mEntities[i];
        if(!entity->isAttached())
            parentNode->attachObject(entity);
    }

    scene->_notifyAttached();

    return scene;
}

ObjectScenePtr Loader::createObjects (Ogre::Entity *parent, const std::string &bonename,
                                      const std::string& bonefilter,
                                      Ogre::SceneNode *parentNode,
                                      std::string name, const std::string &group)
{
    ObjectScenePtr scene = ObjectScenePtr (new ObjectScene(parentNode->getCreator()));

    Misc::StringUtils::lowerCaseInPlace(name);
    NIFObjectLoader::load(parentNode, scene, name, group);

    bool isskinned = false;
    for(size_t i = 0;i < scene->mEntities.size();i++)
    {
        Ogre::Entity *ent = scene->mEntities[i];
        if(scene->mSkelBase != ent && ent->hasSkeleton())
        {
            isskinned = true;
            break;
        }
    }

    Ogre::Vector3 scale(1.0f);
    if(bonename.find("Left") != std::string::npos)
        scale.x *= -1.0f;

    if(isskinned)
    {
        // accepts anything named "filter*" or "tri filter*"
        std::string filter = "@shape=tri "+bonefilter;
        std::string filter2 = "@shape="+bonefilter;
        Misc::StringUtils::lowerCaseInPlace(filter);
        Misc::StringUtils::lowerCaseInPlace(filter2);
        for(size_t i = 0;i < scene->mEntities.size();i++)
        {
            Ogre::Entity *entity = scene->mEntities[i];
            if(entity->hasSkeleton())
            {
                if(entity == scene->mSkelBase ||
                   entity->getMesh()->getName().find(filter) != std::string::npos
                   || entity->getMesh()->getName().find(filter2) != std::string::npos)
                    parentNode->attachObject(entity);
            }
            else
            {
                if(entity->getMesh()->getName().find(filter) == std::string::npos
                        || entity->getMesh()->getName().find(filter2) == std::string::npos)
                    entity->detachFromParent();
            }
        }
    }
    else
    {
        for(size_t i = 0;i < scene->mEntities.size();i++)
        {
            Ogre::Entity *entity = scene->mEntities[i];
            if(!entity->isAttached())
            {
                Ogre::TagPoint *tag = parent->attachObjectToBone(bonename, entity);
                tag->setScale(scale);
            }
        }
    }

    scene->_notifyAttached();

    return scene;
}


ObjectScenePtr Loader::createObjectBase (Ogre::SceneNode *parentNode, std::string name, const std::string &group)
{
    ObjectScenePtr scene = ObjectScenePtr (new ObjectScene(parentNode->getCreator()));

    Misc::StringUtils::lowerCaseInPlace(name);
    NIFObjectLoader::load(parentNode, scene, name, group, 0xC0000000);

    if(scene->mSkelBase)
        parentNode->attachObject(scene->mSkelBase);

    return scene;
}


void Loader::createKfControllers (Ogre::Entity *skelBase,
                                  const std::string &name,
                                  TextKeyMap &textKeys,
                                  std::vector<Ogre::Controller<Ogre::Real> > &ctrls)
{
    NIFObjectLoader::loadKf(skelBase->getSkeleton(), name, textKeys, ctrls);
}

bool Loader::sShowMarkers = false;
bool NIFObjectLoader::sShowMarkers = false;

void Loader::setShowMarkers (bool show)
{
    sShowMarkers = show;
    NIFObjectLoader::setShowMarkers(show);
}


} // namespace NifOgre
