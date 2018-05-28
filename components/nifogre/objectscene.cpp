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
#include "objectscene.hpp"

#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreParticleSystem.h>
#include <OgreSceneManager.h>
#include <OgreParticle.h>
#include <OgreNode.h>
//#include <OgreMeshManager.h>
#include <OgreMaterialManager.h>
#include <OgreCamera.h>

#include <extern/shiny/Main/Factory.hpp>

Ogre::MaterialPtr NifOgre::MaterialControllerManager::getWritableMaterial (Ogre::MovableObject *movable)
{
    if (mClonedMaterials.find(movable) != mClonedMaterials.end())
        return mClonedMaterials[movable];

    else
    {
        Ogre::MaterialPtr mat;
        if (Ogre::Entity* ent = dynamic_cast<Ogre::Entity*>(movable))
            mat = ent->getSubEntity(0)->getMaterial();
        else if (Ogre::ParticleSystem* partSys = dynamic_cast<Ogre::ParticleSystem*>(movable))
            mat = Ogre::MaterialManager::getSingleton().getByName(partSys->getMaterialName());

        static int count=0;
        Ogre::String newName = mat->getName() + Ogre::StringConverter::toString(count++);
        sh::Factory::getInstance().createMaterialInstance(newName, mat->getName());
        // Make sure techniques are created
        sh::Factory::getInstance()._ensureMaterial(newName, "Default");
        mat = Ogre::MaterialManager::getSingleton().getByName(newName);

        mClonedMaterials[movable] = mat;

        if (Ogre::Entity* ent = dynamic_cast<Ogre::Entity*>(movable))
            ent->getSubEntity(0)->setMaterial(mat);
        else if (Ogre::ParticleSystem* partSys = dynamic_cast<Ogre::ParticleSystem*>(movable))
            partSys->setMaterialName(mat->getName());

        return mat;
    }
}

NifOgre::MaterialControllerManager::~MaterialControllerManager ()
{
    for (std::map<Ogre::MovableObject*,
            Ogre::MaterialPtr>::iterator it = mClonedMaterials.begin(); it != mClonedMaterials.end(); ++it)
    {
        sh::Factory::getInstance().destroyMaterialInstance(it->second->getName());
    }
}

NifOgre::ObjectScene::ObjectScene (Ogre::SceneManager* sceneMgr)
  : mSkelBase(0), mSceneMgr(sceneMgr), mMaxControllerLength(0)
{ }

NifOgre::ObjectScene::~ObjectScene ()
{
    for(size_t i = 0;i < mLights.size();i++)
    {
        Ogre::Light *light = mLights[i];
        // If parent is a scene node, it was created specifically for this light. Destroy it now.
        if(light->isAttached() && !light->isParentTagPoint())
            mSceneMgr->destroySceneNode(light->getParentSceneNode());
        mSceneMgr->destroyLight(light);
    }
    for(size_t i = 0;i < mParticles.size();i++)
        mSceneMgr->destroyParticleSystem(mParticles[i]);
    for(size_t i = 0;i < mEntities.size();i++)
        mSceneMgr->destroyEntity(mEntities[i]);
    mControllers.clear();
    mLights.clear();
    mParticles.clear();
    mEntities.clear();
    mSkelBase = NULL;
}

void NifOgre::ObjectScene::setVisibilityFlags (unsigned int flags)
{
    for (std::vector<Ogre::Entity*>::iterator iter (mEntities.begin()); iter!=mEntities.end();
        ++iter)
        (*iter)->setVisibilityFlags (flags);

    for (std::vector<Ogre::ParticleSystem*>::iterator iter (mParticles.begin());
        iter!=mParticles.end(); ++iter)
        (*iter)->setVisibilityFlags (flags);

    for (std::vector<Ogre::Light*>::iterator iter (mLights.begin()); iter!=mLights.end();
        ++iter)
        (*iter)->setVisibilityFlags (flags);
}

void NifOgre::ObjectScene::rotateBillboardNodes (Ogre::Camera *camera)
{
    for (std::vector<Ogre::Node*>::iterator it = mBillboardNodes.begin(); it != mBillboardNodes.end(); ++it)
    {
        assert(mSkelBase);
        Ogre::Node* node = *it;
        node->_setDerivedOrientation(mSkelBase->getParentNode()->_getDerivedOrientation().Inverse() *
                                     camera->getRealOrientation());
    }
}

void NifOgre::ObjectScene::_notifyAttached ()
{
    // convert initial particle positions to world space for world-space particle systems
    // this can't be done on creation because the particle system is not in its correct world space position yet
    for (std::vector<Ogre::ParticleSystem*>::iterator it = mParticles.begin(); it != mParticles.end(); ++it)
    {
        Ogre::ParticleSystem* psys = *it;
        if (psys->getKeepParticlesInLocalSpace())
            continue;
        Ogre::ParticleIterator pi = psys->_getIterator();
        while (!pi.end())
        {
            Ogre::Particle *p = pi.getNext();

#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
            Ogre::Vector3& position = p->mPosition;
            Ogre::Vector3& direction = p->mDirection;
#else
            Ogre::Vector3& position = p->position;
            Ogre::Vector3& direction = p->direction;
#endif

            position =
                (psys->getParentNode()->_getDerivedOrientation() *
                (psys->getParentNode()->_getDerivedScale() * position))
                + psys->getParentNode()->_getDerivedPosition();
            direction =
                (psys->getParentNode()->_getDerivedOrientation() * direction);
        }
    }
}
