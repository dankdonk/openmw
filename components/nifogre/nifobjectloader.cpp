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

#include <extern/nibtogre/nimodel.hpp>
#include <extern/nibtogre/btogreinst.hpp>

#include "mesh.hpp"
#include "skeleton.hpp"
#include "controller.hpp"
#include "particles.hpp" // NiNodeHolder
#include "material.hpp"  // NIFMaterialLoader

#if 0 // seems to be unused?
namespace
{
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
}
#endif

#if 0
void NifOgre::NIFObjectLoader::handleNode2 (const Nif::NIFFilePtr& nif, const std::string &name,
            const std::string &group, Ogre::SceneNode *sceneNode, const Nif::Node *node,
            ObjectScenePtr scene)
{
    // local variables for this node
    int bsxFlags = 0; // usually only at the root record?

    // first get any flags and extra data
    if (node && node->hasExtras)
    {
        Nif::NiExtraDataPtr extraData;
        for (unsigned int i = 0; i < node->extras.length(); ++i)
        {
            extraData = node->extras[i];
            assert(extraData.getPtr() != NULL);

            if (!extraData.empty() && extraData->name == "BSX")
            {
                bsxFlags = static_cast<Nif::BSXFlags*>(extraData.getPtr())->integerData;
                break; // FIXME: don't care about other NiExtraData (for now)
            }
            else if (!extraData.empty() && extraData.getPtr()->recType == Nif::RC_NiStringExtraData)
            {
                // String markers may contain important information affecting the entire
                // subtree of this node
                Nif::NiStringExtraData *sd = (Nif::NiStringExtraData*)extraData.getPtr();

                // FIXME: what to do here?

            }
        }
    }

    // next process the node
    // AvoidNode              /* Morrowind only? */
    // NiBSAnimationNode      /* Morrowind only? */
    // NiBSParticleNode       /* Morrowind only? */
    // NiBillboardNode
    // BSBlastNode            /* Skyrim only */
    // BSDamageStage          /* Skyrim only */
    // BSFadeNode             /* Skyrim only */
    // BSLeafAnimNode         /* Skyrim only */
    // BSMasterParticleSystem /* Skyrim only */
    // BSMultiBoundNode       /* Skyrim only */
    // BSOrderedNode          /* Skyrim only */
    // BSTreeNode             /* Skyrim only */
    // BSValueNode            /* Skyrim only */
    // NiSwitchNode           /* Skyrim only */
    // FxWidget               /* not used in TES? */
    //   FxButton             /* not used in TES? */
    //   FxRadioButton        /* not used in TES? */
    // BSDebrisNode           /* not used in TES? */
    // NiBone                 /* not used in TES? */
    // NiRoom                 /* not used in TES? */
    // NiRoomGroup            /* not used in TES? */
    // NiSortAdjustNode       /* not used in TES? */

    // IDEA: use c++ inheritance here?
    // e.g. maybe the Nif classes should generate the Ogre code relevant for itself?
    // - pass ObjectScenePtr and setup whatever is required there
    // - pass the current accumulated transform in case the object needs it
    if (node->recType == Nif::RC_AvoidNode) // TES3 only
    {
        // TODO
    }
    else if (node->recType == Nif::RC_NiBSAnimationNode) // TES3 only
    {
        // TODO
    }
    else if (node->recType == Nif::RC_NiBSParticleNode)  // TES3 only
    {
        // TODO
    }
    else if (node->recType == Nif::RC_RootCollisionNode) // TES3 only
    {
        // TODO: e.g. NoM/NoM_ac_pool00.nif
    }
    else if (node->recType == Nif::RC_NiBillboardNode)
    {
        // TODO
    }
    else if (node->recType == Nif::RC_NiBillboardNode)
    {
        // TODO: TES5 only
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

    if (node->recType == Nif::RC_NiTriStrips || node->recType == Nif::RC_NiTriShape)
    {
        // create an Ogre Entity
    }
    else if (node->recType == BSSegmentedTriShape) // TES5 only
    {
    }
    else if (node->recType == BSLODTriShape) // TES5 only
    {
    }
    else if (node->recType == NiCamera)
    {
        // how to implement? (e.g. magiceffects/illusion.nif)
    }
    else if (node->recType == NiAutoNormalParticles || node->recType == NiRotatingParticles
          || node->recType == NiParticleSystem || node->recType == BSStripParticleSystem)
    {
          //NiParticleSystem      e.g. architecture/anvil/lorgrenskeleton01.nif
          //BSStripParticleSystem  in Skyrim only? e.g. actors/wisp/testwisp.nif
    }
    else if (node->recType == NiNode)
    {
        // recurse into it
    }
    else
    {
        //if the child is an NiPSParticleSystem, NiPortal, NiBezierMesh and NiRenderObject
        // - ignore, not used in TES?
    }
           if the child is an NiDynamic Effect - same node also listed under effects
}
#endif

