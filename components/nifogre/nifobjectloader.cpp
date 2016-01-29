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
#include "nifobjectloader.hpp"

#include <algorithm>
#include <iostream>

#include <OgreMovableObject.h>
#include <OgreParticleSystem.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreBone.h>
#include <OgreEntity.h>
#include <OgreControllerManager.h>
#include <OgreCamera.h>
#include <OgreSkeletonInstance.h>
#include <OgreMeshManager.h>
#include <OgreParticleAffector.h>
#include <OgreSkeletonManager.h>

#include <components/nif/node.hpp>
#include <components/nif/controller.hpp>
#include <components/nif/controlled.hpp>
#include <components/nif/extra.hpp>
#include <components/nif/data.hpp>
#include <components/nif/property.hpp>

#include <components/nifcache/nifcache.hpp>

#include <components/misc/stringops.hpp>

#include "mesh.hpp"
#include "skeleton.hpp"
#include "controller.hpp"
#include "particles.hpp" // NiNodeHolder
#include "material.hpp"  // NIFMaterialLoader

namespace
{

    void getAllNiNodes(const Nif::Node* node, std::vector<const Nif::NiNode*>& out)
    {
        const Nif::NiNode* ninode = dynamic_cast<const Nif::NiNode*>(node);
        if (ninode)
        {
            out.push_back(ninode);
            for (unsigned int i=0; i<ninode->children.length(); ++i)
                if (!ninode->children[i].empty())
                    getAllNiNodes(ninode->children[i].getPtr(), out);
        }
    }

}

void NifOgre::NIFObjectLoader::setShowMarkers (bool show)
{
    sShowMarkers = show;
}

void NifOgre::NIFObjectLoader::warn (const std::string &msg)
{
    std::cerr << "NIFObjectLoader: Warn: " << msg << std::endl;
}

// 1. NIFMeshLoader::createMesh() - see mesh.cpp
// 2. Ogre::SceneManager::createEntity()
// 3. Make the entity visible and add to the scene
// 4. Handle skeleton - not so sure about this
// 5. Process Nif::Controller's
//    - seems to only support Nif::RC_NiUVController and Nif::RC_NiGeomMorpherController
//    - need to add more here, especially for the new NIF's
// 6. Create material controller
//    - see notes below in createMaterialControllers()
void NifOgre::NIFObjectLoader::createEntity (const std::string &name, const std::string &group,
                 Ogre::SceneManager *sceneMgr, ObjectScenePtr scene, const Nif::Node *node, int flags, int animflags)
{
    size_t recIndex = 0;
    std::string fullname;
    const Nif::NiTriStrips *strips = static_cast<const Nif::NiTriStrips*>(node);
    const Nif::NiTriShape *shape = static_cast<const Nif::NiTriShape*>(node);

    bool isStrips = false; // FIXME not needed
    if (node->recType == Nif::RC_NiTriStrips)
    {
        recIndex = strips->recIndex;

        fullname = name+"@index="+Ogre::StringConverter::toString(recIndex);
        if (strips->name.length() > 0)
            fullname += "@shape="+strips->name; // FIXME: is this for filtering?
        isStrips = true;
    }
    else
    {
        recIndex = shape->recIndex;

        fullname = name+"@index="+Ogre::StringConverter::toString(recIndex);
        if (shape->name.length() > 0)
            fullname += "@shape="+shape->name;
    }
    Misc::StringUtils::lowerCaseInPlace(fullname);

    Ogre::MeshManager &meshMgr = Ogre::MeshManager::getSingleton();
    if (meshMgr.getByName(fullname).isNull())
        NIFMeshLoader::createMesh(name, fullname, group, recIndex, isStrips);

    Ogre::Entity *entity = sceneMgr->createEntity(fullname);

#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
    // Enable skeleton-based bounding boxes. With the static bounding box,
    // the animation may cause parts to go outside the box and cause culling problems.
    if (entity->hasSkeleton())
        entity->setUpdateBoundingBoxFromSkeleton(true);
#endif

    entity->setVisible(!(flags&Nif::NiNode::Flag_Hidden));

    scene->mEntities.push_back(entity);
    if (scene->mSkelBase)
    {
        if (entity->hasSkeleton())
            entity->shareSkeletonInstanceWith(scene->mSkelBase);
        else
        {
            int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, recIndex);
            if (trgtid != -1)
            {
                Ogre::Bone *trgtbone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
                trgtbone->getUserObjectBindings().setUserAny(Ogre::Any(static_cast<Ogre::MovableObject*>(entity)));

                scene->mSkelBase->attachObjectToBone(trgtbone->getName(), entity);
            }
        }
    }

    Nif::ControllerPtr ctrl = node->controller;
    while (!ctrl.empty())
    {
        if (ctrl->flags & Nif::NiNode::ControllerFlag_Active)
        {
            bool isAnimationAutoPlay = (animflags & Nif::NiNode::AnimFlag_AutoPlay) != 0;
            if (ctrl->recType == Nif::RC_NiUVController)
            {
                const Nif::NiUVController *uv = static_cast<const Nif::NiUVController*>(ctrl.getPtr());

                Ogre::ControllerValueRealPtr srcval(isAnimationAutoPlay ?
                                                    Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                    Ogre::ControllerValueRealPtr());
                Ogre::ControllerValueRealPtr dstval(
                        OGRE_NEW UVController::Value(entity, uv->data.getPtr(), &scene->mMaterialControllerMgr));

                UVController::Function* function = OGRE_NEW UVController::Function(uv, isAnimationAutoPlay);
                scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                Ogre::ControllerFunctionRealPtr func(function);

                scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
            }
            else if (ctrl->recType == Nif::RC_NiGeomMorpherController)
            {
                const Nif::NiGeomMorpherController *geom
                    = static_cast<const Nif::NiGeomMorpherController*>(ctrl.getPtr());

                Ogre::ControllerValueRealPtr srcval(isAnimationAutoPlay ?
                                                    Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                    Ogre::ControllerValueRealPtr());
                Ogre::ControllerValueRealPtr dstval(OGRE_NEW GeomMorpherController::Value(
                        entity, geom->data.getPtr(), geom->recIndex));

                GeomMorpherController::Function* function
                    = OGRE_NEW GeomMorpherController::Function(geom, isAnimationAutoPlay);

                scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                Ogre::ControllerFunctionRealPtr func(function);

                scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
            }
        }
        ctrl = ctrl->next;
    }

    if (node->recType == Nif::RC_NiTriStrips)
        createMaterialControllers(strips, entity, animflags, scene);
    else
        createMaterialControllers(shape, entity, animflags, scene);
}

// 1. Nif::Node::getProperties()
//    - handle texprop and matprop
//    - process the controllers
//      - for material Nif::NiAlphaController and Nif::RC_NiMaterialColorController
//      - for texture Nif::RC_NiFlipController
void NifOgre::NIFObjectLoader::createMaterialControllers (const Nif::Node* node,
            Ogre::MovableObject* movable, int animflags, ObjectScenePtr scene)
{
    const Nif::NiTexturingProperty *texprop = NULL;
    const Nif::NiMaterialProperty *matprop = NULL;
    const Nif::NiAlphaProperty *alphaprop = NULL;
    const Nif::NiVertexColorProperty *vertprop = NULL;
    const Nif::NiZBufferProperty *zprop = NULL;
    const Nif::NiSpecularProperty *specprop = NULL;
    const Nif::NiWireframeProperty *wireprop = NULL;
    const Nif::NiStencilProperty *stencilprop = NULL;
    const Nif::BSLightingShaderProperty *bsprop = NULL;
    const Nif::BSEffectShaderProperty *effectprop = NULL;
    const Nif::BSWaterShaderProperty *waterprop = NULL;

    node->getProperties(texprop, matprop, alphaprop, vertprop, zprop, specprop, wireprop, stencilprop);

    if (node->nifVer >= 0x14020007 && node->userVer == 12)
    {
        const Nif::NiGeometry *geom = static_cast<const Nif::NiGeometry*>(node);
        if (geom)
        {
            bool hasAlphaprop = alphaprop != 0;
            alphaprop = NULL;
            geom->getBSProperties(bsprop, alphaprop, effectprop, waterprop);

            // FIXME: what happens if both have alphaprop?
            if (hasAlphaprop && alphaprop)
                std::cout << "alphaprop over written" << std::endl;
        }
    }

    bool isAnimationAutoPlay = (animflags & Nif::NiNode::AnimFlag_AutoPlay) != 0;
    Ogre::ControllerValueRealPtr srcval(isAnimationAutoPlay ?
                                        Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                        Ogre::ControllerValueRealPtr());

    if (matprop)
    {
        Nif::ControllerPtr ctrls = matprop->controller;
        while (!ctrls.empty())
        {
            // FIXME: Data exists only up to NIF version 10.1.0.0, need to implement an interpolator instead
            if (ctrls->nifVer <= 0x0a010000 && ctrls->recType == Nif::RC_NiAlphaController)
            {
                const Nif::NiAlphaController *alphaCtrl = static_cast<const Nif::NiAlphaController*>(ctrls.getPtr());
                Ogre::ControllerValueRealPtr dstval(
                    OGRE_NEW AlphaController::Value(movable, alphaCtrl->data.getPtr(), &scene->mMaterialControllerMgr));
                AlphaController::Function* function
                    = OGRE_NEW AlphaController::Function(alphaCtrl, isAnimationAutoPlay);
                scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                Ogre::ControllerFunctionRealPtr func(function);
                scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
            }
            // FIXME: interpolator not yet implemented for newer nif versions
            else if (ctrls->nifVer <= 0x0a010000 && ctrls->recType == Nif::RC_NiMaterialColorController)
            {
                const Nif::NiMaterialColorController *matCtrl
                    = static_cast<const Nif::NiMaterialColorController*>(ctrls.getPtr());
                Ogre::ControllerValueRealPtr dstval(
                    OGRE_NEW MaterialColorController::Value(movable, matCtrl->data.getPtr(), &scene->mMaterialControllerMgr));
                MaterialColorController::Function* function
                    = OGRE_NEW MaterialColorController::Function(matCtrl, isAnimationAutoPlay);
                scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                Ogre::ControllerFunctionRealPtr func(function);
                scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
            }

            ctrls = ctrls->next;
        }
    }
    if (texprop)
    {
        Nif::ControllerPtr ctrls = texprop->controller;
        while (!ctrls.empty())
        {
            if (ctrls->recType == Nif::RC_NiFlipController)
            {
                const Nif::NiFlipController *flipCtrl = static_cast<const Nif::NiFlipController*>(ctrls.getPtr());


                Ogre::ControllerValueRealPtr dstval(OGRE_NEW FlipController::Value(
                    movable, flipCtrl, &scene->mMaterialControllerMgr));
                FlipController::Function* function = OGRE_NEW FlipController::Function(flipCtrl, isAnimationAutoPlay);
                scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                Ogre::ControllerFunctionRealPtr func(function);
                scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
            }

            ctrls = ctrls->next;
        }
    }
}

void NifOgre::NIFObjectLoader::createParticleEmitterAffectors (Ogre::ParticleSystem *partsys,
               const Nif::NiParticleSystemController *partctrl, Ogre::Bone* bone, const std::string& skelBaseName)
{
    Ogre::ParticleEmitter *emitter = partsys->addEmitter("Nif");
    emitter->setParticleVelocity(partctrl->velocity - partctrl->velocityRandom*0.5f,
                                 partctrl->velocity + partctrl->velocityRandom*0.5f);

    if (partctrl->emitFlags & Nif::NiParticleSystemController::NoAutoAdjust)
        emitter->setEmissionRate(partctrl->emitRate);
    else
        emitter->setEmissionRate(partctrl->numParticles / (partctrl->lifetime + partctrl->lifetimeRandom/2));

    emitter->setTimeToLive(std::max(0.f, partctrl->lifetime),
                           std::max(0.f, partctrl->lifetime + partctrl->lifetimeRandom));
    emitter->setParameter("width", Ogre::StringConverter::toString(partctrl->offsetRandom.x));
    emitter->setParameter("height", Ogre::StringConverter::toString(partctrl->offsetRandom.y));
    emitter->setParameter("depth", Ogre::StringConverter::toString(partctrl->offsetRandom.z));
    emitter->setParameter("vertical_direction", Ogre::StringConverter::toString(Ogre::Radian(partctrl->verticalDir).valueDegrees()));
    emitter->setParameter("vertical_angle", Ogre::StringConverter::toString(Ogre::Radian(partctrl->verticalAngle).valueDegrees()));
    emitter->setParameter("horizontal_direction", Ogre::StringConverter::toString(Ogre::Radian(partctrl->horizontalDir).valueDegrees()));
    emitter->setParameter("horizontal_angle", Ogre::StringConverter::toString(Ogre::Radian(partctrl->horizontalAngle).valueDegrees()));

    Nif::NiParticleModifierPtr e = partctrl->extra;
    while (!e.empty())
    {
        if (e->recType == Nif::RC_NiParticleGrowFade)
        {
            const Nif::NiParticleGrowFade *gf = static_cast<const Nif::NiParticleGrowFade*>(e.getPtr());

            Ogre::ParticleAffector *affector = partsys->addAffector("GrowFade");
            affector->setParameter("grow_time", Ogre::StringConverter::toString(gf->growTime));
            affector->setParameter("fade_time", Ogre::StringConverter::toString(gf->fadeTime));
        }
        else if (e->recType == Nif::RC_NiGravity)
        {
            const Nif::NiGravity *gr = static_cast<const Nif::NiGravity*>(e.getPtr());

            Ogre::ParticleAffector *affector = partsys->addAffector("Gravity");
            affector->setParameter("force", Ogre::StringConverter::toString(gr->mForce));
            affector->setParameter("force_type", (gr->mType==0) ? "wind" : "point");
            affector->setParameter("direction", Ogre::StringConverter::toString(gr->mDirection));
            affector->setParameter("position", Ogre::StringConverter::toString(gr->mPosition));
            affector->setParameter("skelbase", skelBaseName);
            affector->setParameter("bone", bone->getName());
        }
        else if (e->recType == Nif::RC_NiParticleColorModifier)
        {
            const Nif::NiParticleColorModifier *cl = static_cast<const Nif::NiParticleColorModifier*>(e.getPtr());
            const Nif::NiColorData *clrdata = cl->data.getPtr();

            Ogre::ParticleAffector *affector = partsys->addAffector("ColourInterpolator");
            size_t num_colors = std::min<size_t>(6, clrdata->mKeyMap.mKeys.size());
            unsigned int i=0;
            for (Nif::Vector4KeyMap::MapType::const_iterator it = clrdata->mKeyMap.mKeys.begin(); it != clrdata->mKeyMap.mKeys.end() && i < num_colors; ++it,++i)
            {
                Ogre::ColourValue color;
                color.r = it->second.mValue[0];
                color.g = it->second.mValue[1];
                color.b = it->second.mValue[2];
                color.a = it->second.mValue[3];
                affector->setParameter("colour"+Ogre::StringConverter::toString(i),
                                       Ogre::StringConverter::toString(color));
                affector->setParameter("time"+Ogre::StringConverter::toString(i),
                                       Ogre::StringConverter::toString(it->first));
            }
        }
        else if (e->recType == Nif::RC_NiParticleRotation)
        {
            // TODO: Implement (Ogre::RotationAffector?)
        }
        else
            warn("Unhandled particle modifier "+e->recName);
        e = e->extra;
    }
}

void NifOgre::NIFObjectLoader::createParticleSystem (const std::string &name,
            const std::string &group, Ogre::SceneNode *sceneNode, ObjectScenePtr scene,
            const Nif::Node *partnode, int flags, int partflags, int animflags)
{
    const Nif::NiAutoNormalParticlesData *particledata = NULL;
    if (partnode->recType == Nif::RC_NiAutoNormalParticles)
        particledata = static_cast<const Nif::NiAutoNormalParticlesData*>(
            static_cast<const Nif::NiAutoNormalParticles*>(partnode)->data.getPtr());
    else if (partnode->recType == Nif::RC_NiRotatingParticles)
        particledata = static_cast<const Nif::NiAutoNormalParticlesData*>(
            static_cast<const Nif::NiRotatingParticles*>(partnode)->data.getPtr());
    else
        throw std::runtime_error("Unexpected particle node type");

    std::string fullname = name+"@index="+Ogre::StringConverter::toString(partnode->recIndex);
    if (partnode->name.length() > 0)
        fullname += "@type="+partnode->name;
    Misc::StringUtils::lowerCaseInPlace(fullname);

    Ogre::ParticleSystem *partsys = sceneNode->getCreator()->createParticleSystem();

    const Nif::NiTexturingProperty *texprop = NULL;
    const Nif::NiMaterialProperty *matprop = NULL;
    const Nif::NiAlphaProperty *alphaprop = NULL;
    const Nif::NiVertexColorProperty *vertprop = NULL;
    const Nif::NiZBufferProperty *zprop = NULL;
    const Nif::NiSpecularProperty *specprop = NULL;
    const Nif::NiWireframeProperty *wireprop = NULL;
    const Nif::NiStencilProperty *stencilprop = NULL;
    const Nif::BSLightingShaderProperty *bsprop = NULL;
    const Nif::BSEffectShaderProperty *effectprop = NULL;
    const Nif::BSWaterShaderProperty *waterprop = NULL;
    bool needTangents = false;

    partnode->getProperties(texprop, matprop, alphaprop, vertprop, zprop, specprop, wireprop, stencilprop);

    if (partnode->nifVer >= 0x14020007 && partnode->userVer == 12)
    {
        const Nif::NiGeometry *geom = static_cast<const Nif::NiGeometry*>(partnode);
        if (geom)
            geom->getBSProperties(bsprop, alphaprop, effectprop, waterprop);
    }

    partsys->setMaterialName(NIFMaterialLoader::getMaterial(particledata, fullname, group,
                                                            texprop, matprop, alphaprop,
                                                            vertprop, zprop, specprop,
                                                            wireprop, stencilprop,
                                                            bsprop, effectprop, waterprop,
                                                            needTangents,
                                                            // MW doesn't light particles, but the MaterialProperty
                                                            // used still has lighting, so that must be ignored.
                                                            true));

    partsys->setCullIndividually(false);
    partsys->setParticleQuota(particledata->numParticles);
    partsys->setKeepParticlesInLocalSpace((partflags & Nif::NiNode::ParticleFlag_LocalSpace) != 0);

    int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, partnode->recIndex);
    if (trgtid != -1)
    {
        Ogre::Bone *trgtbone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
        scene->mSkelBase->attachObjectToBone(trgtbone->getName(), partsys);
    }
    else
        std::cout << "createParticleSystem: no bone " << partnode->recIndex << ", " << name << std::endl;

    Nif::ControllerPtr ctrl = partnode->controller;
    while (!ctrl.empty())
    {
        if ((ctrl->recType == Nif::RC_NiParticleSystemController || ctrl->recType == Nif::RC_NiBSPArrayController)
                && ctrl->flags & Nif::NiNode::ControllerFlag_Active)
        {
            const Nif::NiParticleSystemController *partctrl
                = static_cast<const Nif::NiParticleSystemController*>(ctrl.getPtr());

            float size = partctrl->size*2;
            // HACK: don't allow zero-sized particles which can rarely cause an AABB assertion in Ogre to fail
            size = std::max(size, 0.00001f);
            partsys->setDefaultDimensions(size, size);

            if (!partctrl->emitter.empty())
            {
                int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, partctrl->emitter->recIndex);
                if (trgtid != -1)
                {
                    Ogre::Bone *trgtbone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
                    // Set the emitter bone(s) as user data on the particle system
                    // so the emitters/affectors can access it easily.
                    std::vector<Ogre::Bone*> bones;
                    if (partctrl->recType == Nif::RC_NiBSPArrayController)
                    {
                        std::vector<const Nif::NiNode*> nodes;
                        getAllNiNodes(partctrl->emitter.getPtr(), nodes);
                        if (nodes.empty())
                            throw std::runtime_error("Emitter for NiBSPArrayController must be a NiNode");
                        for (unsigned int i=0; i<nodes.size(); ++i)
                        {
                            // FIXME: check handle != -1
                            bones.push_back(scene->mSkelBase->getSkeleton()->getBone(
                                                NIFSkeletonLoader::lookupOgreBoneHandle(name, nodes[i]->recIndex)));
                        }
                    }
                    else
                    {
                        bones.push_back(trgtbone);
                    }
                    NiNodeHolder holder;
                    holder.mBones = bones;
                    partsys->getUserObjectBindings().setUserAny(Ogre::Any(holder));
                    createParticleEmitterAffectors(partsys, partctrl, trgtbone, scene->mSkelBase->getName());
                }
                else
                    std::cout << "createParticleSystem: ctlr : no bone "
                              << partctrl->recIndex << ", " << name << std::endl;
            }

            createParticleInitialState(partsys, particledata, partctrl);

            bool isParticleAutoPlay = (partflags&Nif::NiNode::ParticleFlag_AutoPlay) != 0;
            Ogre::ControllerValueRealPtr srcval(isParticleAutoPlay ?
                                                Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                                Ogre::ControllerValueRealPtr());
            Ogre::ControllerValueRealPtr dstval(OGRE_NEW ParticleSystemController::Value(partsys, partctrl));

            ParticleSystemController::Function* function =
                    OGRE_NEW ParticleSystemController::Function(partctrl, isParticleAutoPlay);
            scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
            Ogre::ControllerFunctionRealPtr func(function);

            scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));

            // Emitting state will be overwritten on frame update by the ParticleSystemController,
            // but set up an initial value anyway so the user can fast-forward particle systems
            // immediately after creation if desired.
            partsys->setEmitting(isParticleAutoPlay);
        }
        ctrl = ctrl->next;
    }

    partsys->setVisible(!(flags&Nif::NiNode::Flag_Hidden));
    scene->mParticles.push_back(partsys);

    createMaterialControllers(partnode, partsys, animflags, scene);
}

void NifOgre::NIFObjectLoader::createParticleInitialState (Ogre::ParticleSystem* partsys,
            const Nif::NiAutoNormalParticlesData* particledata, const Nif::NiParticleSystemController* partctrl)
{
    partsys->_update(0.f); // seems to be required to allocate mFreeParticles. TODO: patch Ogre to handle this better
    int i=0;
    for (std::vector<Nif::NiParticleSystemController::Particle>::const_iterator it = partctrl->particles.begin();
         i<particledata->activeCount && it != partctrl->particles.end(); ++it, ++i)
    {
        const Nif::NiParticleSystemController::Particle& particle = *it;

        Ogre::Particle* created = partsys->createParticle();
        if (!created)
            break;

#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
        Ogre::Vector3& position = created->mPosition;
        Ogre::Vector3& direction = created->mDirection;
        Ogre::ColourValue& colour = created->mColour;
        float& totalTimeToLive = created->mTotalTimeToLive;
        float& timeToLive = created->mTimeToLive;
#else
        Ogre::Vector3& position = created->position;
        Ogre::Vector3& direction = created->direction;
        Ogre::ColourValue& colour = created->colour;
        float& totalTimeToLive = created->totalTimeToLive;
        float& timeToLive = created->timeToLive;
#endif

        direction = particle.velocity;
        position = particledata->vertices.at(particle.vertex);

        if (particle.vertex < int(particledata->colors.size()))
        {
            Ogre::Vector4 partcolour = particledata->colors.at(particle.vertex);
            colour = Ogre::ColourValue(partcolour.x, partcolour.y, partcolour.z, partcolour.w);
        }
        else
            colour = Ogre::ColourValue(1.f, 1.f, 1.f, 1.f);
        float size = particledata->sizes.at(particle.vertex);
        created->setDimensions(size, size);
        totalTimeToLive = std::max(0.f, particle.lifespan);
        timeToLive = std::max(0.f, particle.lifespan - particle.lifetime);
    }
    // now apparently needs another update, otherwise it won't render in the first frame.
    // TODO: patch Ogre to handle this better
    partsys->_update(0.f);
}

// create an Ogre::Controller<Ogre::Real> and add to scene->mControllers
// (see struct ObjectScene in ogrenifloader.hpp)
void NifOgre::NIFObjectLoader::createNodeControllers (const Nif::NIFFilePtr& nif,
            const std::string &name, Nif::ControllerPtr ctrl, ObjectScenePtr scene, int animflags)
{
    do {
        if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
        {
            ctrl = ctrl->next;
            continue;
        }

        bool isAnimationAutoPlay = (animflags & Nif::NiNode::AnimFlag_AutoPlay) != 0;
        if (ctrl->recType == Nif::RC_NiVisController)
        {
            const Nif::NiVisController *vis = static_cast<const Nif::NiVisController*>(ctrl.getPtr());

            int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, ctrl->target->recIndex);
            if (trgtid != -1)
            {
                Ogre::Bone *trgtbone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
                Ogre::ControllerValueRealPtr srcval(isAnimationAutoPlay ?
                                            Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                            Ogre::ControllerValueRealPtr());

                if (ctrl->nifVer >= 0x0a020000) // from 10.2.0.0
                {
                    const Nif::NiInterpolator *interpolator = vis->interpolator.getPtr();
                    std::cout << "interpolator not supported" << interpolator->recName << std::endl;
#if 0
                    if (ctrl->interpolator.getPtr()->recType == Nif::RC_NiFloatInterpolator)
                    {



                        const Nif::NiFloatInterpolator* fi
                            = static_cast<const Nif::NiFloatInterpolator*>(ctrl->interpolator.getPtr());
                        // FIXME: this key is probably not the right one to use
                        float key = fi->value;
                        // use 0.5f as the default, not sure what it should be
                        mDelta = interpKey(fi->floatData.getPtr()->mKeyList.mKeys, key, 0.5f);





                        Ogre::ControllerValueRealPtr
                            dstval(OGRE_NEW VisController::Value(trgtbone, vis->data.getPtr()));
                    }
                    else
                        std::cout << "interpolator not supported" << std::endl;
#endif
                    ctrl = ctrl->next;
                    continue;
                }
                else if (ctrl->nifVer <= 0x0a010000) // up to 10.1.0.0
                {
                    Ogre::ControllerValueRealPtr dstval(
                            OGRE_NEW VisController::Value(trgtbone, vis->data.getPtr()));

                    VisController::Function* function
                        = OGRE_NEW VisController::Function(vis, isAnimationAutoPlay);

                    scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                    Ogre::ControllerFunctionRealPtr func(function);

                    scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                }
            }
            else
                std::cout << "createNodeControllers: Vis: no bone " << ctrl->recIndex << ", " << name << std::endl;
        }
        else if (ctrl->recType == Nif::RC_NiKeyframeController || ctrl->recType == Nif::RC_NiTransformController)
        {
            // NiTransformController replaces NiKeyframeController
            const Nif::NiKeyframeController *key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());
            int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, ctrl->target->recIndex);
            if (trgtid != -1)
            {
                Ogre::Bone *trgtbone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
                // The keyframe controller will control this bone manually
                trgtbone->setManuallyControlled(true);
                Ogre::ControllerValueRealPtr srcval(isAnimationAutoPlay ?
                                        Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                        Ogre::ControllerValueRealPtr());

                if (ctrl->nifVer >= 0x0a020000&& !key->interpolator.empty()) // from 10.2.0.0
                {
                    const Nif::NiInterpolator *interpolator = key->interpolator.getPtr();
                    if (interpolator->recType == Nif::RC_NiBlendTransformInterpolator)
                    {
                        const Nif::NiBlendTransformInterpolator *bt
                            = static_cast<const Nif::NiBlendTransformInterpolator*>(interpolator);
                        std::cout << "unknown1 short " << bt->unknown1 << std::endl;
                        std::cout << "unknown2 int " << bt->unknown2 << std::endl;
                    }
                    else
                        std::cout << "interpolator not supported" << interpolator->recName << std::endl;
#if 0
#endif
                    ctrl = ctrl->next;
                    continue;
                }
                else if (ctrl->nifVer <= 0x0a010000 && !key->data.empty()) // up to 10.1.0.0
                {
                    Ogre::ControllerValueRealPtr dstval(
                            OGRE_NEW KeyframeController::Value(trgtbone, nif, key->data.getPtr()));

                    KeyframeController::Function* function
                        = OGRE_NEW KeyframeController::Function(key, isAnimationAutoPlay);

                    scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                    Ogre::ControllerFunctionRealPtr func(function);

                    scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                }
            }
            else
                std::cout << "createNodeControllers: KeyFrame : no bone "
                          << ctrl->recIndex << ", " << name << std::endl;
        }
        else if (ctrl->recType == Nif::RC_NiControllerManager) // FIXME
        {
            ctrl = ctrl->next;
            continue;
        }
        else if (ctrl->recType == Nif::RC_NiMultiTargetTransformController) // FIXME
        {
            ctrl = ctrl->next;
            continue;
        }
        else if (ctrl->recType == Nif::RC_NiPSysEmitterCtlr) // FIXME
        {
            ctrl = ctrl->next;
            continue;
        }
        else if (ctrl->recType == Nif::RC_NiPSysUpdateCtlr) // FIXME
        {
            ctrl = ctrl->next;
            continue;
        }
        else
            std::cout << "Unsupported controller " << ctrl->recName << std::endl;

        ctrl = ctrl->next;
    } while (!ctrl.empty());
}


void NifOgre::NIFObjectLoader::extractTextKeys (const Nif::NiTextKeyExtraData *tk, TextKeyMap &textkeys)
{
    for (size_t i = 0;i < tk->list.size();i++)
    {
        const std::string &str = tk->list[i].text;
        std::string::size_type pos = 0;
        while (pos < str.length())
        {
            if (::isspace(str[pos]))
            {
                pos++;
                continue;
            }

            std::string::size_type nextpos = std::min(str.find('\r', pos), str.find('\n', pos));
            if (nextpos != std::string::npos)
            {
                do {
                    nextpos--;
                } while (nextpos > pos && ::isspace(str[nextpos]));
                nextpos++;
            }
            else if (::isspace(*str.rbegin()))
            {
                std::string::const_iterator last = str.end();
                do {
                    --last;
                } while (last != str.begin() && ::isspace(*last));
                nextpos = std::distance(str.begin(), ++last);
            }
            std::string result = str.substr(pos, nextpos-pos);
            textkeys.insert(std::make_pair(tk->list[i].time, Misc::StringUtils::lowerCase(result)));

            pos = nextpos;
        }
    }
}

void NifOgre::NIFObjectLoader::createObjects (const Nif::NIFFilePtr& nif, const std::string &name,
            const std::string &group, Ogre::SceneNode *sceneNode, const Nif::Node *node,
            ObjectScenePtr scene, int flags, int animflags, int partflags, bool isRootCollisionNode)
{
    // Do not create objects for the collision shape (includes all children)
    if (node->recType == Nif::RC_RootCollisionNode)
        isRootCollisionNode = true;

    if (node->recType == Nif::RC_NiBSAnimationNode)
        animflags |= node->flags;
    else if (node->recType == Nif::RC_NiBSParticleNode)
        partflags |= node->flags;
    else
        flags |= node->flags;

    if (node->recType == Nif::RC_NiBillboardNode)
    {
        // TODO: figure out what the flags mean.
        // NifSkope has names for them, but doesn't implement them.
        // Change mBillboardNodes to map <Bone, billboard type>
        int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, node->recIndex);
        if (trgtid != -1)
        {
            Ogre::Bone* bone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
            bone->setManuallyControlled(true);
            scene->mBillboardNodes.push_back(bone);
        }
        else
            std::cout << "createObjects: no bone " << node->recIndex << ", " << name << std::endl;
    }

    // FIXME: should be able to handle this using nifVer, rather than boolean hasExtras
    // FIXME: duplicated code
    if (node->hasExtras)
    {
        for (unsigned int i = 0; i < node->extras.length(); ++i)
        {
            Nif::NiExtraDataPtr e = node->extras[i];

            if (!e.empty() && e->recType == Nif::RC_NiTextKeyExtraData)
            {
                const Nif::NiTextKeyExtraData *tk = static_cast<const Nif::NiTextKeyExtraData*>(e.getPtr());

                extractTextKeys(tk, scene->mTextKeys);
            }
            else if (!e.empty() && e->recType == Nif::RC_NiStringExtraData)
            {
                const Nif::NiStringExtraData *sd = static_cast<const Nif::NiStringExtraData*>(e.getPtr());
                // String markers may contain important information
                // affecting the entire subtree of this obj
                if (sd->stringData == "MRK" && !sShowMarkers)
                {
                    // Marker objects. These meshes are only visible in the
                    // editor.
                    flags |= 0x80000000;
                }
            }
        }
    }
    else
    {
        Nif::NiExtraDataPtr e = node->extra;
        while (!e.empty())
        {
            if (e->recType == Nif::RC_NiTextKeyExtraData)
            {
                const Nif::NiTextKeyExtraData *tk = static_cast<const Nif::NiTextKeyExtraData*>(e.getPtr());

                extractTextKeys(tk, scene->mTextKeys);
            }
            else if (e->recType == Nif::RC_NiStringExtraData)
            {
                const Nif::NiStringExtraData *sd = static_cast<const Nif::NiStringExtraData*>(e.getPtr());
                // String markers may contain important information
                // affecting the entire subtree of this obj
                if (sd->stringData == "MRK" && !sShowMarkers)
                {
                    // Marker objects. These meshes are only visible in the
                    // editor.
                    flags |= 0x80000000;
                }
            }

            e = e->next;
        }
    }

    if (!node->controller.empty())
        createNodeControllers(nif, name, node->controller, scene, animflags);

    if (!isRootCollisionNode)
    {
        if (node->recType == Nif::RC_NiCamera)
        {
            /* Ignored */
        }

        if ((node->recType == Nif::RC_NiTriShape ||
            node->recType == Nif::RC_NiTriStrips) && !(flags&0x80000000)) // ignore marker objects
        {
            createEntity(name, group, sceneNode->getCreator(), scene, node, flags, animflags);
        }

        if ((node->recType == Nif::RC_NiAutoNormalParticles ||
            node->recType == Nif::RC_NiRotatingParticles) && !(flags&0x40000000))
        {
            createParticleSystem(name, group, sceneNode, scene, node, flags, partflags, animflags);
        }
    }

    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if (ninode)
    {
        const Nif::NodeList &children = ninode->children;
        for (size_t i = 0;i < children.length();i++)
        {
            if (!children[i].empty())
                createObjects(nif, name, group, sceneNode, children[i].getPtr(),
                              scene, flags, animflags, partflags, isRootCollisionNode);
        }
    }
}

void NifOgre::NIFObjectLoader::createSkelBase (const std::string &name, const std::string &group,
                           Ogre::SceneManager *sceneMgr, const Nif::Node *node, ObjectScenePtr scene)
{
    bool isStrips = node->recType == Nif::RC_NiTriStrips;

    /* This creates an empty mesh to which a skeleton gets attached. This
     * is to ensure we have an entity with a skeleton instance, even if all
     * other entities are attached to bones and not skinned. */
    Ogre::MeshManager &meshMgr = Ogre::MeshManager::getSingleton();
    if (meshMgr.getByName(name).isNull())
        NIFMeshLoader::createMesh(name, name, group, ~(size_t)0, isStrips);

    scene->mSkelBase = sceneMgr->createEntity(name);
    scene->mEntities.push_back(scene->mSkelBase);
}

void NifOgre::NIFObjectLoader::load (Ogre::SceneNode *sceneNode,
            ObjectScenePtr scene, const std::string &name, const std::string &group, int flags)
{
    Nif::NIFFilePtr nif = Nif::Cache::getInstance().load(name);
    if (nif->numRoots() < 1)
    {
        nif->warn("Found no root nodes in "+name+".");
        return;
    }

    const Nif::Record *r = nif->getRoot(0);
    assert(r != NULL);

    const Nif::Node *node = dynamic_cast<const Nif::Node*>(r);
    if (node == NULL)
    {
        nif->warn("First root in "+name+" was not a node, but a "+
                  r->recName+".");
        return;
    }

    if (Ogre::SkeletonManager::getSingleton().resourceExists(name) ||
       !NIFSkeletonLoader::createSkeleton(name, group, node).isNull())
    {
        // Create a base skeleton entity if this NIF needs one
        createSkelBase(name, group, sceneNode->getCreator(), node, scene);
    }
    //std::cout << "creating object "<< name << ", root " << node->name << std::endl; // FIXME
    createObjects(nif, name, group, sceneNode, node, scene, flags, 0, 0);
}

// given 'skeleton' and 'name', populate textKeys and ctrls
//
// textKeys and ctrls are extracted from the nif file (which is derived from 'name') and 'skeleton'
// is used to confirm/match the bone name in the string extra data
//
// This method assumes that 'name' has a corresponding '.kf' file. (e.g. abc.nif <-> abc.kf)
// For TES4/5 the nif file tends to be 'skeleton.nif' so we need a different algorithm.
// addAnimSource() may need to pass additional info (e.g. from KFFZ subrecord for additional
// special animations).  FIXME: need to find out what are the 'hard coded' animations are
//
// MWRender::Animation::addAnimSource() /* add animation to a model (a nif file in MW/OpenMW) */
//   --> NifOgre::Loader::createKfControllers()   /* just an interface, doesn't do anything */
//         --> NifOgre::NIFObjectLoader::loadKf() /* this method */
//
// Each .kf file is started with a NiSequenceStreamHelper block with NiTextKeyExtraData
// following it.  These are then followed by NiStringExtraData and NiKeyframeController blocks.
//
// Newer .kf files are different.  It is started with NiControllerSequence which points to
// NiTextKeyExtraData and  has a number of Controlled Blocks.  Each Controlled Block has a node
// name string, and points to a NiTransformInterpolator which in turn points to
// NiTransformData.
void NifOgre::NIFObjectLoader::loadKf (Ogre::Skeleton *skel, const std::string &name,
            TextKeyMap &textKeys, std::vector<Ogre::Controller<Ogre::Real> > &ctrls)
{
    Nif::NIFFilePtr nif = Nif::Cache::getInstance().load(name);
    if (nif->numRoots() < 1)
    {
        nif->warn("Found no root nodes in "+name+".");
        return;
    }

    const Nif::Record *r = nif->getRoot(0);
    assert(r != NULL);

    if (r->recType != Nif::RC_NiSequenceStreamHelper)
    {
        nif->warn("First root was not a NiSequenceStreamHelper, but a "+
                  r->recName+".");
        return;
    }
    const Nif::NiSequenceStreamHelper *seq = static_cast<const Nif::NiSequenceStreamHelper*>(r);

    Nif::NiExtraDataPtr extra;
    // FIXME: should be handled with nifVer instead
    if (seq->hasExtras)
        extra = seq->extras[0];
    else
        extra = seq->extra;

    if (extra.empty() || extra->recType != Nif::RC_NiTextKeyExtraData)
    {
        nif->warn("First extra data was not a NiTextKeyExtraData, but a "+
                  (extra.empty() ? std::string("nil") : extra->recName)+".");
        return;
    }

    extractTextKeys(static_cast<const Nif::NiTextKeyExtraData*>(extra.getPtr()), textKeys);

    if (seq->hasExtras)
        if (seq->extras.length() > 1)
            extra = seq->extras[1];
        else
            return;
    else
        extra = extra->next;

    Nif::ControllerPtr ctrl = seq->controller;
    for (;!extra.empty() && !ctrl.empty();(extra=extra->next/*FIXME*/),(ctrl=ctrl->next))
    {
        if (extra->recType != Nif::RC_NiStringExtraData || ctrl->recType != Nif::RC_NiKeyframeController)
        {
            nif->warn("Unexpected extra data "+extra->recName+" with controller "+ctrl->recName);
            continue;
        }

        if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
            continue;

        const Nif::NiStringExtraData *strdata = static_cast<const Nif::NiStringExtraData*>(extra.getPtr());
        const Nif::NiKeyframeController *key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());

        if (key->data.empty())
            continue;
        if (!skel->hasBone(strdata->stringData))
            continue;

        Ogre::Bone *trgtbone = skel->getBone(strdata->stringData);
        Ogre::ControllerValueRealPtr srcval;
        Ogre::ControllerValueRealPtr dstval(OGRE_NEW KeyframeController::Value(trgtbone, nif, key->data.getPtr()));
        Ogre::ControllerFunctionRealPtr func(OGRE_NEW KeyframeController::Function(key, false));

        ctrls.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
    }
}
