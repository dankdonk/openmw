#include "foreignactivatoranimation.hpp"

//#include <iostream> // FIXME: testing only

#include <OgreSceneNode.h>
#include <OgreParticleSystem.h>
#include <OgreEntity.h>
#include <OgreSkeleton.h>
#include <OgreSkeletonInstance.h>
#include <OgreBone.h>
#include <OgreMesh.h>

#include <components/esm/loadacti.hpp>

#include "renderconst.hpp"

namespace MWRender
{

    ForeignActivatorAnimation::~ForeignActivatorAnimation()
    {
    }

    // Dungeons\Caves\Traps\CTrapLog01.NIF (SCPT 00051AC2)
    //
    ForeignActivatorAnimation::ForeignActivatorAnimation(const MWWorld::Ptr &ptr, const std::string& model)
        : ObjectAnimation(ptr, model)
    {
        if (!model.empty())
        {
            // Unlike NPC animation, activator animations are built into the NIF model.  The
            // NiModel has its own skeleton as well.  So the high level build steps are:
            //
            // 1. load the NIF Model
            // 2. build the skeleton and animation controllers
            // 3. build the meshes and link to the skeleton
            // 4. instantiate the entities
            //setObjectRoot(model, false);
            //setRenderProperties(mObjectRoot, RV_Misc, RQG_Main, RQG_Alpha);

            Ogre::SkeletonInstance *skelinst;
            if (!mObjectRoot->mSkelBase)
                return;

            //if (mObjectRoot->mForeignObj->mModel->getModelName().find("geardoor") == std::string::npos)
                //return;

            skelinst = mObjectRoot->mSkelBase->getSkeleton();
            if (!skelinst)
                return;

            Ogre::Skeleton::BoneIterator boneiter = skelinst->getBoneIterator();
            while(boneiter.hasMoreElements())
                boneiter.getNext()->setManuallyControlled(true);

            std::map<std::string, std::vector<Ogre::Entity*> >::iterator iter
                = mObjectRoot->mForeignObj->mSkeletonAnimEntities.begin();
            for (;iter != mObjectRoot->mForeignObj->mSkeletonAnimEntities.end(); ++iter)
            {
                for (size_t i = 0; i < iter->second.size(); ++i)
                {
                    if (iter->second[i]->isAttached())
                        //iter->second[i]->detachFromParent();
                        continue;

                    std::string meshName = iter->second[i]->getMesh()->getName();
                    //size_t pos = meshName.find_first_of('#');
                    size_t pos = meshName.find_last_of('@');
                    meshName = meshName.substr(pos+1); // same as bone name

                    mSkelBase->attachObjectToBone(meshName, iter->second[i]);
                }
            }

            //addAnimSource(model);

            Ogre::SharedPtr<AnimSource> animSource(OGRE_NEW AnimSource);
            std::vector<Ogre::Controller<Ogre::Real> >& controllers = mObjectRoot->mForeignObj->mControllers;

            mAnimSources.push_back(animSource);

            std::vector<Ogre::Controller<Ogre::Real> > *grpctrls = animSource->mControllers;
            for (size_t i = 0; i < controllers.size(); i++)
            {
                NifOgre::NodeTargetValue<Ogre::Real> *dstval;
                dstval = static_cast<NifOgre::NodeTargetValue<Ogre::Real>*>(controllers[i].getDestination().get());

                controllers[i].setSource(mAnimationTimePtr[0]);
                grpctrls[0].push_back(controllers[i]);
            }
        }
//      else
//      {
//          // No model given. Create an object root anyway, so that lights can be added to it if needed.
//          mObjectRoot = NifOgre::ObjectScenePtr(new NifOgre::ObjectScene(mInsert->getCreator()));
//      }
    }
}
