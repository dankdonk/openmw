/*
 * =====================================================================================
 *
 *       Filename:  BtOgrePG.h
 *
 *    Description:  The part of BtOgre that handles information transfer from Bullet to
 *                  Ogre (like updating graphics object positions).
 *
 *        Version:  1.0
 *        Created:  27/12/2008 03:40:56 AM
 *
 *         Author:  Nikhilesh (nikki)
 *
 * =====================================================================================
 */

#ifndef BtOgreGP_H_
#define BtOgreGP_H_

#include "btBulletDynamicsCommon.h"
#include "OgreSceneNode.h"
#include "BtOgreExtras.h"

namespace BtOgre {

//A MotionState is Bullet's way of informing you about updates to an object.
//Pass this MotionState to a btRigidBody to have your SceneNode updated automaticaly.
class RigidBodyState : public btMotionState
{
    protected:
        btTransform mTransform;
        btTransform mCenterOfMassOffset;

        Ogre::SceneNode *mSceneNode;

    public:
        RigidBodyState(Ogre::SceneNode *node, const btTransform &transform, const btTransform &offset = btTransform::getIdentity())
            : mTransform(transform),
              mCenterOfMassOffset(offset),
              mSceneNode(node)
        {
        }
#if 0
        RigidBodyState(Ogre::SceneNode *node)
            : mTransform(((node != NULL) ? BtOgre::Convert::toBullet(node->getOrientation()) : btQuaternion(0,0,0,1)),
                         ((node != NULL) ? BtOgre::Convert::toBullet(node->getPosition())    : btVector3(0,0,0))),
              mCenterOfMassOffset(btTransform::getIdentity()),
              mSceneNode(node)
        {
        }
#endif
        // called by btRigidBody::setupRigidBody during construction to set its m_worldTransform,
        // ignoring btRigidBodyConstructionInfo::m_startWorldTransform
        //
        // also called by btRigidBody::predictIntegratedTransform
        virtual void getWorldTransform(btTransform &ret) const
        {
            // return the previously transform saved from setWorldTransform
            ret = mCenterOfMassOffset.inverse() * mTransform;
#if 0
            Ogre::Vector3 v = mSceneNode->_getDerivedPosition();
            Ogre::Quaternion q = mSceneNode->_getDerivedOrientation();
            ret.setOrigin(btVector3(v.x, v.y, v.z));
            ret.setRotation(btQuaternion(q.x, q.y, q.z, q.w));
#endif
        }

        // called by btDiscreteDynamicsWorld::synchronizeSingleMotionState
        virtual void setWorldTransform(const btTransform &in)
        {
            if (mSceneNode == nullptr)
				return; // silently return before we set a node

            mTransform = in;
            btTransform transform = in * mCenterOfMassOffset;
//#if 0
            Ogre::SceneNode *parent = mSceneNode->getParentSceneNode();
            Ogre::Vector3 pv = parent->getPosition();
            Ogre::Quaternion pq = parent->getOrientation();
            btTransform parentTrans(btQuaternion(pq.x, pq.y, pq.z, pq.w), btVector3(pv.x, pv.y, pv.z));

            transform = parentTrans.inverse() * transform;
//#endif
            btQuaternion rot = transform.getRotation();
            btVector3 pos = transform.getOrigin();
            //std::cout << "in " << mSceneNode->getName() << ", x " << pos.x() << ", y " << pos.y() << ", z " << pos.z() << std::endl;
            mSceneNode->setOrientation(rot.w(), rot.x(), rot.y(), rot.z());
            mSceneNode->setPosition(pos.x(), pos.y(), pos.z());
            //Ogre::Vector3 cv = mSceneNode->_getDerivedPosition();
            //std::cout << "derived " << mSceneNode->getName() << ", x " << cv.x << ", y " << cv.y << ", z " << cv.z << std::endl;

            //mSceneNode->rotate(Ogre::Quaternion(rot.w(), rot.x(), rot.y(), rot.z()), Ogre::SceneNode::TS_LOCAL);
            //mSceneNode->translate(Ogre::Vector3(pos.x(), pos.y(), pos.z()), Ogre::SceneNode::TS_LOCAL);
            //std::cout << "in " << mSceneNode->getName() << ", x " << pos.x() << ", y " << pos.y() << ", z " << pos.z() << std::endl;
            //Ogre::Vector3 v = mSceneNode->getPosition();
            //std::cout << "x delta " << pos.x() - v.x << " y delta " << pos.y() - v.y << " z delta " << pos.z() - v.z << std::endl;
            //Ogre::Vector3 cv = mSceneNode->_getDerivedPosition();
            //std::cout << "derived " << mSceneNode->getName() << ", x " << cv.x << ", y " << cv.y << ", z " << cv.z << std::endl;
        }

        void setNode(Ogre::SceneNode *node)
        {
            mSceneNode = node;
        }
};

//Softbody-Ogre connection goes here!

}

#endif
