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
#include <components/nif/collision.hpp>
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

    // assumes starting with 'Scene Root'
    //
    // - if the NiNode has a controller, add to the ctrls vector (but skip some controllers)
    // - search the children recursively
    //
    // FIXME: is there a way to optimize by setting up the collisions at the same time, i.e. in
    // a single pass scan?
    void findController(Ogre::Skeleton *skeleton, Nif::NIFFilePtr nif,
            const Nif::Node *node, std::vector<Ogre::Controller<Ogre::Real> > &ctrls)
    {
        // TES4 starts witth the NiBSBoneLODController
        if(!node->controller.empty())
        {
            Nif::ControllerPtr ctrl = node->controller;
            std::string boneName = node->name; // remember the node name

            // follow the controller chain until we hit an active NiTransformController
            // (i.e. skip NiBSBoneLODController, bhkBlendController, etc)
            do {
                if((ctrl->recType == Nif::RC_NiTransformController) &&
                   ((ctrl->flags & Nif::NiNode::ControllerFlag_Active) != 0)) // 0x1000
                {
                    // check that the controller has the correct target node
                    if (static_cast<Nif::Named*>(ctrl->target.getPtr())->name == boneName)
                    {
                        // source value
                        Ogre::ControllerValueRealPtr srcval;

                        // destination value
                        const Nif::NiTransformController *c
                            = static_cast<const Nif::NiTransformController*>(ctrl.getPtr());
                        const Nif::NiTransformInterpolator *interp
                            = static_cast<const Nif::NiTransformInterpolator*>(c->interpolator.getPtr());
                        // FIXME: should create a dummy interpolator rather than skipping?
                        if (!interp)
                            break; // Bip01 NonAccum does not have an interpolator

                        Ogre::Bone *targetBone = skeleton->getBone(boneName);
                        Ogre::ControllerValueRealPtr
                            dstval(OGRE_NEW NifOgre::KeyframeController::Value(targetBone,
                                                                               nif, interp->transformData.getPtr()));
                        // control function
                        Ogre::ControllerFunctionRealPtr
                            func(OGRE_NEW NifOgre::KeyframeController::Function(ctrl.getPtr(), false));

                        // store the controller
                        ctrls.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));

                        break;
                    }
                    else
                        std::cout << "target bone mismatch" << std::endl; // FIXME: throw?
                }
            } while(!(ctrl = ctrl->next).empty());
        }

        const Nif::NiNode *ninode = static_cast<const Nif::NiNode*>(node);
        const Nif::NodeList &children = ninode->children;
        for(size_t i = 0;i < children.length();i++)
        {
            if(!children[i].empty())
            {
                findController(skeleton, nif, children[i].getPtr(), ctrls);
            }
        }


#if 0



        // FIXME: Not sure why "AttachLight" implies a skeleton is needed
        //        Possibley because MWRender::Animation::addExtraLight() uses attachObjectToBone()?
        if (node->name.find("AttachLight") != std::string::npos || node->name == "ArrowBone")
            return true;

        if(node->recType == Nif::RC_NiNode ||
                node->recType == Nif::RC_BSFadeNode ||
                node->recType == Nif::RC_BSValueNode ||
                node->recType == Nif::RC_NiSwitchNode || // probably has NiNode children
                node->recType == Nif::RC_NiBillboardNode || // NiNode that faces the camera
                node->recType == Nif::RC_RootCollisionNode)
        {
            const Nif::NiNode *ninode = static_cast<const Nif::NiNode*>(node);
            const Nif::NodeList &children = ninode->children;
            for(size_t i = 0;i < children.length();i++)
            {
                if(!children[i].empty())
                {
                    if(needSkeleton(children[i].getPtr()))
                        return true;
                }
            }
            return false;
        }
#endif
    }

    // FIXME: this is a duplicate copy of the same in bulletnifloader.cpp
    // NOTE: the check for controller only works at the root node
    bool isRagdoll(const Nif::Node *node, unsigned int bsxFlags)
    {
        if (node->nifVer >= 0x14020007) // TES5
            return (bsxFlags & 0x2) != 0 && (bsxFlags & 0x1) != 0 && node->controller.empty();
        else                            // TES4
            return (bsxFlags & 0x8) != 0 && (bsxFlags & 0x1) != 0 && node->controller.empty();
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

// Flag_Hidden        = 0x0001,
// Flag_MeshCollision = 0x0002,
// Flag_BBoxCollision = 0x0004
//
// both CathedralCryptLight02 and wizard\shirt_gnd have 0x0E (0000 1110), i.e. makes no sense
// arrow marker has 0000 1010
// sound marker has 0000 1110
// looks like the flags only apply to older NIF files?
//
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
            int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, (int)recIndex);
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

    int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, (int)partnode->recIndex);
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
                int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, (int)partctrl->emitter->recIndex);
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
                                                NIFSkeletonLoader::lookupOgreBoneHandle(name, (int)nodes[i]->recIndex)));
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

            int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, (int)ctrl->target->recIndex);
            if (trgtid != -1)
            {
                Ogre::Bone *trgtbone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
                Ogre::ControllerValueRealPtr srcval(isAnimationAutoPlay ?
                                            Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                            Ogre::ControllerValueRealPtr());

                if (ctrl->nifVer >= 0x0a020000) // from 10.2.0.0
                {
                    const Nif::NiInterpolator *interpolator = vis->interpolator.getPtr();
                    if (interpolator)
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
            int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, (int)ctrl->target->recIndex);
            if (trgtid != -1)
            {
                Ogre::Bone *trgtbone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
                // The keyframe controller will control this bone manually
                trgtbone->setManuallyControlled(true);
                Ogre::ControllerValueRealPtr srcval(isAnimationAutoPlay ?
                                        Ogre::ControllerManager::getSingleton().getFrameTimeSource() :
                                        Ogre::ControllerValueRealPtr());

                if (ctrl->nifVer <= 0x0a010000 && !key->data.empty()) // up to 10.1.0.0 (TES3/4)
                {
                    Ogre::ControllerValueRealPtr dstval(
                            OGRE_NEW KeyframeController::Value(trgtbone, nif, key->data.getPtr()));

                    KeyframeController::Function* function
                        = OGRE_NEW KeyframeController::Function(key, isAnimationAutoPlay);

                    scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                    Ogre::ControllerFunctionRealPtr func(function);

                    scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                }
                else if (ctrl->nifVer >= 0x0a020000 && !key->interpolator.empty()) // from 10.2.0.0 (TES4/5)
                {
                    const Nif::NiInterpolator *interpolator = key->interpolator.getPtr();
                    if (interpolator->recType == Nif::RC_NiBlendTransformInterpolator)
                    {
                        const Nif::NiBlendTransformInterpolator *bt
                            = static_cast<const Nif::NiBlendTransformInterpolator*>(interpolator);
                        std::cout << "NiBlendTransformInterpolator::unknown1 short " << bt->unknown1 << std::endl;
                        std::cout << "NiBlendTransformInterpolator::unknown2 int " << bt->unknown2 << std::endl;
                    }
                    else if (interpolator->recType == Nif::RC_NiTransformInterpolator)
                    {
                        // has traslation, rotation, scale and NiTransformData (same as NiKeyframeData)
                        //
                        // FIXME: code duplication with above, not sure what they do, simply copied
                        const Nif::NiTransformData *data
                            = static_cast<const Nif::NiTransformInterpolator*>(interpolator)->transformData.getPtr();
                        if (data)
                        {
                            Ogre::ControllerValueRealPtr dstval(
                                OGRE_NEW KeyframeController::Value(trgtbone, nif, data));

                            KeyframeController::Function* function
                                = OGRE_NEW KeyframeController::Function(key, isAnimationAutoPlay);

                            scene->mMaxControllerLength = std::max(function->mStopTime, scene->mMaxControllerLength);
                            Ogre::ControllerFunctionRealPtr func(function);

                            scene->mControllers.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
                        }
                    }
                    else
                        std::cout << "interpolator not supported " << interpolator->recName << std::endl;
#if 0
#endif
                    ctrl = ctrl->next;
                    continue;
                }
                }
            else
                // FIXME: why would there be no bones?
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
        else if (ctrl->recType == Nif::RC_NiBSBoneLODController) // FIXME used for animations
        {
            // has node (and skin shape groups in some versions) as well as some unknown ints
            // not sure what the node groups are used for?
            ctrl = ctrl->next;
            continue;
        }
        else if (ctrl->recType == Nif::RC_bhkBlendController) // FIXME prob used for animations
        {
            // same Nif::Controller except another unknown int which is usually 0
            const Nif::bhkBlendController *bc = static_cast<const Nif::bhkBlendController*>(ctrl.getPtr());
            if (bc->unknown)
                std::cout << "bhkBlendController::unknown int " << bc->unknown << std::endl;
            ctrl = ctrl->next;
            continue;
        }
        else
            std::cout << "Unsupported controller " << ctrl->recName << std::endl;

        ctrl = ctrl->next;
    } while (!ctrl.empty());
}


// See extra.cpp and extra.hpp
// both new and old versions of .kf files have NiTextKeyExtraData
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

// Call Path
// =========
//
// Scene::insertCell                                              | scene.cpp
//   ::InsertFunctor::operator()                                  | scene.cpp
//     ::addObject                                                | scene.cpp
//       MWRender::RenderingManager::addObject                    | renderingmanager.cpp
//         MWClass::ForeignBook::insertObjectRendering            | foreignbook.cpp
//           MWRender::Objects::insertModel                       | objects.cpp
//             MWRender::Animation::setObjectRoot                 | animation.cpp
//               NifOgre::Loader::createObjects                   | ogrenifloader.cpp
//                 NifOgre::NIFObjectLoader::load                 | nifobjectloader.cpp
//                   NifOgre::NIFObjectLoader::handleNode  <--- this method
//
// More detail
// ===========
//
//   MWRender::Objects::insertModel:
//     create Ogre::SceneNode as a child to the cell scene node (update Ptr with the results)
//     create ObjectAnimation (whose ctor calls setObjectRoot)
//     ... (other stuff)
//
//   Animation::setObjectRoot:
//     create mObjectRoot which is a NifOgre::ObjectScene that has the Ogre entities, controllers, etc
//     by calling either NifOgre::Loader::createObjectBase or NifOgre::Loader::createObjects
//     ... (other stuff)
//
//   NifOgre::NIFObjectLoader::load:
//     either get a copy from the cache or load the NIF file, then call this method to create the
//     NIF objects in Ogre
//
// TODO:
// Find out if it is feasible to create both Ogre and Bullet objects at the same time.
//
// TODO:
// Does it make sence to serialise the created objects to save loading time in subsequent
// executions?
//
//
// About objects
// =============
//
//   ESM defines object ref, its template (incl. NIF files if appropriate), its world
//   position/scale/rotation, etc.
//
//   NIF files defines how to put together the objects - they define any skeletons, rendering
//   meshes, collision meshes, constraints and other object properties. (KF files define
//   animations)
//
//   Ogre provides the rendering (all the visible stuff) as well as resource management
//   (including physics objects and animation controllers).
//
//   Bullet provides the physics.
//
//   In the OpenMW engine, a Ptr represents the object reference (i.e. an instance of the
//   object).  It is put together from the ESM/ESP files, the BSA and NIF files using Ogre and
//   Bullet classes.
//
//   Ideally, the engine should be able to handle more than one object format (i.e. not NIF).
//   However the current attempt at supporting two NIF versions shows that it is not a trivial task.
//
// Loading process (at a high, conceptual level):
//
//    cell     object (Ptr)    ESM/P      NIF loader   NIF file resource mgr   NIF file loader
//      |            |           |            |                 |                  |
//      |----------->|           |            |                 |                  |
//      |            |---------->|            |                 |                  |
//      |            |    ...    |            |                 |                  |
//      |            |<----------|            |                 |                  |
//      |            |           |            |                 |                  |
//      |            |----------------------->|                 |                  |
//      |            |           |            |---------------->|                  |
//      |            |           |            |                 |----------------->|  (only if not
//      |            |           |            |                 |<-----------------|   in cache)
//      |            |           |            |<----------------|                  |
//      |            |<-----------------------|                 |                  |
//      |<-----------|           |            |                 |                  |
//      |            |           |            |                 |                  |
//                                        should NIF loader
//                                     create object templates
//                                     and store them in resource
//                                     manager(s)?
//
//
//   Rather than having a resource manager for NIF files, perhaps the NIF file loader should
//   create Ogre and Bullet objects directly.
//
//   Currently the NIF file is read and its contents interpreted as "Nif" namespace classes
//   (e.g.  Nif::NiNode) These are then cached key'ed on the file's path string.
//
//   Another loader then interprets the "Nif" classes and produces Ogre usable classes.
//   (see NifOgre::NIFObjectLoader)
//
//   Yet another loader interprets the same "Nif" classes (twice, once for collision and another
//   for raycasting) and produces Bullet classes (see NifBullet::ManualBulletShapeLoader)
//   The "templates" or BulletShapePtr are stored in BulletShapeManager.  The physics engine then
//   creates the RigidBody's using the BulletShape's. [NOTE: the same NIF path with different
//   scales are treated as distinct shapes]
//
//   It may be that there are negligible performance concerns (unfortunately we won't know until
//   an alternative has been coded and compared, which may well be quite a wasted effort) and
//   perhaps even necessary (e.g. maybe ESM/ESP specified scale can affect the created object?)
//
// Requirements:
//
//   What are the things expected by Ptr, Ogre and Bullet?
//
// Bullet:
//
//   btDiscreteDynamicsWorld needs btRigidBody's and btTypedConstraint's configured and registered.
//
//   Some btRigidBody's need access to the Ogre::SceneNode of the objects so that the dynamic
//   movements such as ragdolls can be simulated (via btMotionState).
//
//   MWWorld::PhysicsSystem::stepSimulation updates the physics objects during animation. To
//   enable this pointers to the btRigidBody's are stored in OEngine::Physic::PhysicEngine.
//
//   MWWorld::PhysicsSystem::moveObject and MWWorld::PhysicsSystem::rotateObject also updates
//   btRigidBody's.  To do so it calls OEngine::Physic::PhysicEngine:getRigidBody - i.e. the
//   engine keeps the pointers to btRigidBody's keyed by its unique name (i.e. Ogre::SceneNode
//   string handles).
//   (TODO: Why doesn't the Ptr keep a copy of the pointer directly instead of searching a map
//   each time? Perhaps the searching can be limited to when the Ptr is first created?)
//
//   TODO: We want to be able to load and unload bullet objects independently to the Ptr's
//   which may need to be kept around for the game logic.
//
//   TODO: Not sure why raycast objects and collision objects are created separately. In some
//   cases raycast needs to be done on an object without collision, but that might be better
//   handled as exceptional cases rather than creating two objects each for every Ptr.
//
// Ogre:
//
//   Animation requires controllers
//
//   Mesh
//
//   Bone
//
//   In order to be able to identify the complete ragdoll object as a single entity, it is
//   probably best to have ... TODO
//
//     NifOgre::NIFObjectLoader::createEntity creates an Ogre::Entity* with a name that includes
//     the recIndex in the string and adds to the ObjectScene's mEntities. // FIXME: false
//
//     i.e. if the recIndex is known and have access to the ObjectScene (i.e. mObjectRoot) then
//     we can figure get to that entity by searching (inefficient, but will do for now)
//
//     Loader::createObjects attaches the entities to the parent SceneNode.  For a ragdoll,
//     each of the movabe entities need to be detached and re-attached to the child SceneNode
//     created for the physics object.
//
// FIXME: unused parameters
void NifOgre::NIFObjectLoader::handleNode (const Nif::NIFFilePtr& nif, const std::string &name,
            const std::string &group, Ogre::SceneNode *sceneNode, const Nif::Node *node,
            ObjectScenePtr scene, int bsxFlags, int animflags, int partflags, bool isRagdollFlag)
{
    // BSX flags are needed to detect for Ragdolls
    // FIXME: this boolean 'hasExtras' may change in the Nif class in future cleanups
    if (bsxFlags == 0 && node && node->hasExtras) // don't check bsxFlags if recursing
    {
        Nif::NiExtraDataPtr extraData;
        for (unsigned int i = 0; i < node->extras.length(); ++i)
        {
            extraData = node->extras[i]; // get the next extra data in the list
            assert(extra.getPtr() != NULL);

            if (!extraData.empty() && extraData->name == "BSX")
            {
                bsxFlags = static_cast<Nif::BSXFlags*>(extraData.getPtr())->integerData;
                break; // don't care about other NiExtraData (for now)
            }
            else if (!extraData.empty() && extraData.getPtr()->recType == Nif::RC_NiStringExtraData)
            {
                // String markers may contain important information
                // affecting the entire subtree of this node
                Nif::NiStringExtraData *sd = (Nif::NiStringExtraData*)extraData.getPtr();

                // FIXME: what to do here?

            }
        }

#if 0
        if (node->nifVer >= 0x14020007 /*TES5*/ && bsxFlags == 0) // FIXME: not sure which bits apply here
            return;
        else if ((bsxFlags & 0xf) == 0) // TES4  0x1: havok, 0x2: collision, 0x4: skeleton, 0x8: animated
            return;
#endif
    }

    if (!node->parent) // check only at the root node
        isRagdollFlag = isRagdoll(node, bsxFlags);

    if (node->recType == Nif::RC_NiTriStrips || node->recType == Nif::RC_NiTriShape)
    {
        //handleNiTriStrips(name, group, sceneNode, scene, node, bsxFlags);
        //createEntity(name, group, sceneNode->getCreator(), scene, node, flags, animflags);





    size_t recIndex = node->recIndex;
    std::string fullname;

    fullname = name+"@index="+Ogre::StringConverter::toString(recIndex);
    if (node->name.length() > 0)
        fullname += "@shape="+node->name;

    Misc::StringUtils::lowerCaseInPlace(fullname);

    Ogre::MeshManager &meshMgr = Ogre::MeshManager::getSingleton();
    if (meshMgr.getByName(fullname).isNull())
        NIFMeshLoader::createMesh(name, fullname, group, recIndex, (node->recType == Nif::RC_NiTriStrips));

    Ogre::Entity *entity = sceneNode->getCreator()->createEntity(fullname);

    // A NiTriStips node name may be something like CathedralCryptChain09:36
    // It is related to a NiNode that contains the bhkCollisionObject, and its name is CathedralCryptChain09
    //
    // There may be more than one NiTriStrips per physics object (e.g. the lamp CathedralCryptLight:35
    // and CathedralCryptLight:36 which is the chain link)
    //
    // There may be no physics object for the NiTriStrips (e.g. CathedralCryptLight02:35 and
    // CathedralCryptLight02:36 which are associated with the root NiNode CathedralCryptLight02)
    //
    // There may be no NiTriStrips for a physics object (e.g. CathedralCryptChain, recIndex 14)
    //
    // So we probably need a multi map with the node names (excluding :35, etc) as the key to
    // store the entity pointers.
    //
    // Alternatively, it seems that a NiTriStrips node is a sibling of the collision object -
    // maybe this can be used.
    //
    if (isRagdollFlag)
    {
        // FIXME hack
        // find the recIndex of the matching bhkRigidBody
        const Nif::NiCollisionObjectPtr collObj = node->parent->collision;
        if (!collObj.empty() && collObj->recType == Nif::RC_bhkCollisionObject)
        {
            const Nif::bhkCollisionObject *bhkCollObj = static_cast<const Nif::bhkCollisionObject*>(collObj.getPtr());
            const Nif::bhkRigidBody *rigidBody = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());
            size_t recIndex = rigidBody->recIndex;
            scene->mRagdollEntities.insert(std::make_pair(recIndex, entity));
        }
    }

#if OGRE_VERSION >= (1 << 16 | 10 << 8 | 0)
    // Enable skeleton-based bounding boxes. With the static bounding box,
    // the animation may cause parts to go outside the box and cause culling problems.
    if (entity->hasSkeleton() || (bsxFlags & 0xc) != 0) // BSX 0x4: skeleton, 0x8: animated
        entity->setUpdateBoundingBoxFromSkeleton(true);
#endif

    entity->setVisible(true /*!(flags&Nif::NiNode::Flag_Hidden)*/); // FIXME not for newer NIF

    scene->mEntities.push_back(entity);

    if (scene->mSkelBase)
    {
        if (entity->hasSkeleton())
            entity->shareSkeletonInstanceWith(scene->mSkelBase);
        else
        {
            int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, (int)recIndex);
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
        createMaterialControllers(static_cast<const Nif::NiTriStrips*>(node), entity, animflags, scene);
    else
        createMaterialControllers(static_cast<const Nif::NiTriShape*>(node), entity, animflags, scene);





    }
     else if (node->recType != Nif::RC_NiNode)
        std::cout << "NIFObjectLoader::handleNode: unhandled record type " << node->name << std::endl;





     // FIXME: this block does not belong here
    if (node->name == "AttachLight")
    {
        scene->mLights.push_back(sceneNode->getCreator()->createLight());
        Ogre::Light *light = scene->mLights.back();
        light->setType(Ogre::Light::LT_POINT);
        light->setDiffuseColour(0.9f, 0.8f, 0.f);
        //http://www.ogre3d.org/tikiwiki/tiki-index.php?page=-Point%20Light%20Attenuation
        light->setSpecularColour(1.0, 1.0, 0.0);
        //light->setCastShadows(true);

        float radius = 512;

        // copied from MWRender::Animation
        float threshold = 0.03f;
        float linearAttenuation = /*linearValue*/3.0f / radius;
        float quadraticAttenuation = /*quadraticValue*/16.0f / std::pow(radius, 2);
        float activationRange = std::max(activationRange, 1.0f / (threshold * linearAttenuation));
        //float activationRange = std::sqrt(1.0f / (threshold * quadraticAttenuation));
        light->setAttenuation(activationRange, 0.5, linearAttenuation, quadraticAttenuation);

        sceneNode->attachObject(light);
    }






    // loop through the children, only NiNode has NiAVObject as children
    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList& children = ninode->children;
        for(size_t i = 0; i < children.length(); i++)
        {
            if(!children[i].empty())
                handleNode(nif, name, group, sceneNode, children[i].getPtr(),
                              scene, bsxFlags, animflags, partflags, isRagdollFlag);
        }
    }
}

// Recursively traverse NIF tree to:
// - Handle animation/particle/billboard nodes
// - Extract tex keys (for animation) to scene->mTextKeys
// - Check for markers
// - Create node controllers
// - Create entity (incl. mesh)
// - Create particle systems
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
        int trgtid = NIFSkeletonLoader::lookupOgreBoneHandle(name, (int)node->recIndex);
        if (trgtid != -1)
        {
            Ogre::Bone* bone = scene->mSkelBase->getSkeleton()->getBone(trgtid);
            bone->setManuallyControlled(true);
            scene->mBillboardNodes.push_back(bone);
        }
        // Maybe only NiBillboardNode's that are fires have bones?  (possibly for attaching lights)
        // meshes\dungeons\misc\fx\fxcloudsmall01.nif does not have any bones, for example
        // (meshes\dungeons\misc\fx\fxcloudthick01.nif should not have any bones either, but has 5)
        //else
            //std::cout << "createObjects: no bone " << node->recIndex << ", " << name << std::endl;
    }

    // FIXME: duplicated code
    if (node->nifVer >= 0x0a000100) // TES4 style, i.e. from 10.0.1.0
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

// Call Path
// =========
//
// Scene::insertCell                                              | scene.cpp
//   ::InsertFunctor::operator()                                  | scene.cpp
//     ::addObject                                                | scene.cpp
//       MWRender::RenderingManager::addObject                    | renderingmanager.cpp
//         MWClass::ForeignBook::insertObjectRendering            | foreignbook.cpp
//           MWRender::Objects::insertModel                       | objects.cpp
//             MWRender::Animation::setObjectRoot                 | animation.cpp
//               NifOgre::Loader::createObjects                   | ogrenifloader.cpp
//                 NifOgre::NIFObjectLoader::load           <- this method
//                   NifOgre::NIFSkeletonLoader::createSkeleton   | skeleton.cpp
//                     Ogre::SkeletonManager::create
//                                :
//                            (callback)
//                                :
//                     NifOgre::NIFSkeletonLoader::loadResource   | skeleton.cpp
//                       NifOgre::NIFSkeletonLoader::buildBones   | skeleton.cpp

// 1. Load the NIF file where 'name' is model (i.e. a NIF file) specified in the ESM.
// 2. Check that the NIF has a root node.
// 3. Create a skeleton if required (e.g. for animation)
// 4. Create objects by traversing the NIF tree TODO: better explanation
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
    if (r->nifVer >= 0x0a000100) // TES4 style, i.e. from 10.0.1.0
        handleNode(nif, name, group, sceneNode, node, scene, flags, 0, 0); // flags is 0 by default
    else
        createObjects(nif, name, group, sceneNode, node, scene, flags, 0, 0);
}

void NifOgre::NIFObjectLoader::loadKf (Ogre::Skeleton *skel, const std::string &name,
            TextKeyMap &textKeys, std::vector<Ogre::Controller<Ogre::Real> > &ctrls)
{
    Nif::NIFFilePtr nif = Nif::Cache::getInstance().load(name);
    if (nif->numRoots() < 1)
    {
        nif->warn("Found no root nodes in "+name+".");
        return;
    }

    if (nif->getVersion() >= 0x0a000100) // from 10.0.1.0 (TES4)
    {
        //std::cout << "NIF version = 0x" << std::hex << nif->getVersion() << std::endl;
        NIFObjectLoader::loadTES4Kf(skel, nif, textKeys, ctrls);

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

    Nif::NiExtraDataPtr extra = seq->extra;

    if (extra.empty() || extra->recType != Nif::RC_NiTextKeyExtraData)
    {
        nif->warn("First extra data was not a NiTextKeyExtraData, but a "+
                  (extra.empty() ? std::string("nil") : extra->recName)+".");
        return;
    }

    extractTextKeys(static_cast<const Nif::NiTextKeyExtraData*>(extra.getPtr()), textKeys);

    extra = extra->next;

    Nif::ControllerPtr ctrl = seq->controller;
    for (;!extra.empty() && !ctrl.empty();(extra=extra->next),(ctrl=ctrl->next))
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

// NifOgre::KeyframeController
//
// Given 'skeleton' and 'name', populate textKeys and ctrls
//
// For TES3:
//
// textKeys and ctrls are extracted from the nif file (which is derived from 'name') and 'skeleton'
// is used to confirm/match the bone name in the string extra data
//
// Each .kf file starts with a NiSequenceStreamHelper block with NiTextKeyExtraData
// following it.  These are then followed by the blocks NiStringExtraData (to get bone name)
// and NiKeyframeController (and the keyframe data).
//
// For TES4/5:
//
// Newer .kf files are different.  It starts with NiControllerSequence which points to
// NiTextKeyExtraData and  has a number of Controlled Blocks.  Each Controlled Block has a node
// name string, and points to a NiTransformInterpolator which in turn points to
// NiTransformData.
//
// Comparison of old and new kf files.
//
//    NiSequenceStreamHelper                 NiControllerSequence
//                                            ControlledBocks
//                                             controllerLink
//                                            frequency
//                                            starttime
//                                            stoptime
//                                            target
//
//     NiStringExtraData (linked list)        NiStringPalette (one)
//      stringData       (string)              palette        (null separated strings)
//
//     NiKeyframeController                   NiTransformInterpolator
//      frequency                              translation
//      phase                                  rotation
//      starttime                              scale
//      stoptime
//      target
//      data (NiKeyframeData)                  data (NiKeyframeData)
//       nRotations                             nRotations
//       rotationType                           rotationType
//       quaternionkeys                         quaternionkeys
//       xyzRotations                           xyzRotations
//       translations                           translations
//       scales                                 scales
//
// MWRender::Animation::addAnimSource() /* add animation to a model (a nif file in MW/OpenMW) */
//   --> NifOgre::Loader::createKfControllers()   /* just an interface, doesn't do anything */
//         --> NifOgre::NIFObjectLoader::loadKf() /* this method */
//
// addAnimSource() assumes that 'name' has a corresponding '.kf' file. (e.g. abc.nif <-> abc.kf)
// For TES4/5 the nif file tends to be 'skeleton.nif' so we need a different algorithm.
//
// addAnimSource() may need to pass additional info (e.g. from KFFZ subrecord for additional
// special animations).  FIXME: need to find out what are the 'hard coded' animations are
//
// Perhaps best to have another method loadTES4Kf or something else
// (use nif version to determine)
//
// Maybe for new .kf always use skeleton.nif? That means either the caller needs to use the
// correct 'name' and/or we need to check version info.
//
// Can't always assume 'name' will be ending skeleton.nif to detect the new version, since
// meshes\r\skeleton.nif is a valid old version.
//
void NifOgre::NIFObjectLoader::loadTES4Kf (Ogre::Skeleton *skel, Nif::NIFFilePtr nif,
            TextKeyMap &textKeys, std::vector<Ogre::Controller<Ogre::Real> > &ctrls)
{
    // NOTE: the model (i.e. skeleton.nif) is also used to generate the skeleton in
    // Animation::setObjectRoot()

    if (nif->numRoots() < 1)
    {
        nif->warn("Found no root nodes in "+nif->getFilename()+".");
        return;
    }

    const Nif::Named *sk = static_cast<Nif::Named*>(nif->getRoot(0));
    assert(sk != NULL);

    // a simple check of the expected file structure
    if (sk->recType != Nif::RC_NiNode)
    {
        nif->warn("First root was not a NiNode, but a "+sk->recName+".");
        return;
    }
    else if (sk->name != "Scene Root")
    {
        nif->warn("First root had an unexpected name "+sk->name+".");
        return;
    }

    // FIXME: how to stop scanning the same skeleton.nif each time?
    // now traverse the children and get controllers for each of the bone nodes
    // FIXME: the current assumption is that idle.kf and others provide different interpolators
    // than those in skeleton.nif - how to switch them?
    // FIXME: it appears that skeleton.nif is not the right file for the animations, after all.
    // Back to scanning idle.kf
    //findController(skel, nif, static_cast<const Nif::Node*>(sk), ctrls);





    // FIXME: hard coded for testing, there's probably some algorithm for selecting an anim file
    Nif::NIFFilePtr kf = Nif::Cache::getInstance().load("meshes\\characters\\_male\\idle.kf");
    //Nif::NIFFilePtr kf = Nif::Cache::getInstance().load("meshes\\characters\\_male\\casttarget.kf");
    //Nif::NIFFilePtr kf = Nif::Cache::getInstance().load("meshes\\characters\\_male\\walkforward.kf");
    if (kf->numRoots() < 1)
    {
        kf->warn("Found no root nodes in "+kf->getFilename()+".");
        return;
    }

    const Nif::Record *r = kf->getRoot(0);
    assert(r != NULL);

    // a simple check of the expected file structure
    if (r->recType != Nif::RC_NiControllerSequence)
    {
        kf->warn("First root was not a NiControllerSequence, but a "+ r->recName+".");
        return;
    }

    // get the text keys (usually start stop times)
    const Nif::NiControllerSequence *seq = static_cast<const Nif::NiControllerSequence*>(r);
    std::cout << "Anim name " << seq->name << std::endl; // FIXME for testing only
    extractTextKeys(static_cast<const Nif::NiTextKeyExtraData*>(seq->textKeys.getPtr()), textKeys);

    for (unsigned int i = 0; i < seq->controlledBlocks.size(); ++i)
    {
        Nif::ControllerLink ctrl = seq->controlledBlocks[i];

// FIXME for testing string extraction only, note the use of '-1' offset to indicate 'none'
#if 0
        char* str = &(ctrl.stringPalette->buffer[0]);
        std::cout << "controlled block [" << i << "] "
            << ((ctrl.nodeNameOffset == -1) ? "" : std::string(str+ctrl.nodeNameOffset)) << ", "
            << ((ctrl.controllerTypeOffset == -1) ? "" : std::string(str+ctrl.controllerTypeOffset)) << std::endl;
#endif
        // check if we support the specified controller/interpolator and data
        // (idle.kf has NiBSplineCompTransformInterpolator, NiFloatInterpolator and NiTransformInterpolator)
        // FIXME currently not checking data type
        const Nif::NiInterpolator *key = static_cast<const Nif::NiInterpolator*>(ctrl.interpolator.getPtr());

        if (key->recType != Nif::RC_NiBSplineCompTransformInterpolator &&
            key->recType != Nif::RC_NiTransformInterpolator &&
            key->recType != Nif::RC_NiFloatInterpolator)
        {
            nif->warn("Unexpected interpolator "+key->recName);
            continue;
        }

        //if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
            //continue;
#if 0
        if (key->recType == Nif::RC_NiTransformInterpolator)
        {
            if (static_cast<const Nif::NiTransformInterpolator*>(key)->transformData.empty())
                continue;
        }
        else if (key->recType == Nif::RC_NiFloatInterpolator)
        {
            if (static_cast<const Nif::NiFloatInterpolator*>(key)->floatData.empty())
                continue;
        }
        else if (key->recType == Nif::RC_NiBSplineCompTransformInterpolator)
        {
            continue;
        }
        else
            continue;
#endif

        std::string boneName(std::string(&(ctrl.stringPalette->buffer[0]) + ctrl.nodeNameOffset));
        if (ctrl.nodeNameOffset == -1 || !skel->hasBone(boneName))
        {
            std::cout << seq->name << ": No bone named " << boneName << " found." << std::endl;
            continue;
        }

        // Animation is implemented using Ogre controllers.  According to the Ogre manual,
        // calling the controller's update() method "Tells this controller to map it's input
        // controller value to it's output controller value, via the controller function."
        //
        //   defined in OgreController.h
        //     Ogre::Controller<T>, Ogre::ControllerValue<T> and Ogre::ControllerFunction<T>
        //
        //   defined in OgreControllerManager.h
        //     typedef SharedPtr<ControllerFunction<Real> > Ogre::ControllerFunctionRealPtr
        //
        // So, we apply the values and functions defined in the kf files and apply them to the
        // matching bones in skeleton.nif (or skeletonbeast.nif, etc).
        //
        // Subclasses of the Ogre classes are used for the implementation:
        //
        // (for TES3):
        //
        //   defined in NifOgre namespace
        //     class DefaultFunction : public Ogre::ControllerFunction<Ogre::Real>
        //       and
        //     template<typename T> class NodeTargetValue : public Ogre::ControllerValue<T>
        //       and
        //     class KeyframeController
        //       and
        //     class KeyframeController::Value : public Ogre::ControllerValue<Ogre::Real>, public ValueInterpolator
        //
        //   NiStringExtraData::String Data is used to identify the bone name.
        //
        //   NiKeyframeController and NiKeyframeData are used to define the control function
        //   and its output value.
        //
        //   NiTextKeyExtraData identifies all kinds of different animations and time keys,
        //   e.g. Idle: Start (0.0000), Torch: Start (97.2000), etc.
        //
        //   The input value is probably time value in float (i.e. Ogre::Real).
        //
        //   For TES3 there are only 4 groups.  See Animation::detectAnimGroup()
        //
        //      "",                 /* Lower body / character root */
        //      "Bip01 Spine1",     /* Torso */
        //      "Bip01 L Clavicle", /* Left arm */
        //      "Bip01 R Clavicle", /* Right arm */
        //
        //   Animation::play() accepts animation group as its first parameter (e.g. idle, hit).
        //
        // (for TES4):
        //
        //   NiKeyframeController is replaced by NiTransformController (I think?) which is a
        //   NiTimeController and is found in skeleton.nif.
        //
        //   Different kinds of animations are found in different kf files, e.g. idle.kf,
        //   torchidle.kf, etc.
        //
        //   Not sure what NiBSBoneLODController does, nor what its node groups are meant to
        //   do.
        //
        //   Each of the bones in skeleton.nif specifies a NiTimeController... maybe these
        //   should be used to the Ogre functions?
        //
        //   A partial trail of the bone links in skeleton.nif, following the children...
        //
        //   Bip01
        //     |
        //   Bip01 NonAccum
        //     |
        //   Bip01 Pelvis
        //     |
        //   Bip01 Spine, Bip01 L Thigh, Bip01 R Thigh, SideWeapon, Bip01 TailRoot, Bip01 Tail72,...
        //     |
        //   Bip01 Spine1
        //     |
        //   Bip01 Spine2, Bip01 Wing 01 R, Bip01 Wing 01 L
        //     |
        //   Bip01 Neck, BackWeapon, Quiver, magicNode, ...
        //      |
        //   Bip01 Neck1, Bip01 L Clavicle, Bip01 R Clavicle
        //      |
        //   Bip01 Head
        //      |
        //   Camera01, Bip01 pony5 no motion, Bip01 pony chub, Bip01 pony1
        //                |
        //             Bip01 pony6
        //
        //
        Ogre::Bone *trgtbone = skel->getBone(boneName);
        // srcval is set in Animation::addAnimSource()
        //   ctrls[i].setSource(mAnimationTimePtr[grp]);
        Ogre::ControllerValueRealPtr srcval;
        Ogre::ControllerValueRealPtr
            dstval(OGRE_NEW TransformController::Value(trgtbone, kf, key));
        // when deltainput is false, DefaultFunction calculates:
        //   value = std::min(mStopTime, std::max(mStartTime, value+mPhase));
        // where mStopTime = &ctlr->timeStart, mStartTime = &ctlr->timeStop, mPhase = &ctlr->phase
        // (all floats, also see Nif::Controller (base.hpp))
        Nif::Controller ctlr; // same as NiTimeController
        ctlr.frequency = seq->frequency;
        ctlr.phase = 0;
        ctlr.timeStart = seq->startTime;
        ctlr.timeStop = seq->stopTime;
        Ogre::ControllerFunctionRealPtr func(OGRE_NEW TransformController::Function(&ctlr, false));

        ctrls.push_back(Ogre::Controller<Ogre::Real>(srcval, dstval, func));
    }
}

