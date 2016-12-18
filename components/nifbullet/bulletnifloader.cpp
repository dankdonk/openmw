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

#include "bulletnifloader.hpp"

#include <cstdio>
#include <iostream> // FIXME

#include <components/misc/stringops.hpp>

#include <components/nifcache/nifcache.hpp>

#include "../nif/niffile.hpp"
#include "../nif/node.hpp"
#include "../nif/data.hpp"
#include "../nif/property.hpp"
#include "../nif/controller.hpp"
#include "../nif/extra.hpp"
#include "../nif/collision.hpp"
#include <libs/platform/strings.h>

#include <vector>
#include <list>
// For warning messages
#include <iostream>

// float infinity
#include <limits>

namespace
{

typedef unsigned char ubyte;

// Extract a list of keyframe-controlled nodes from a .kf file
// FIXME: this is a similar copy of OgreNifLoader::loadKf
void extractControlledNodes(Nif::NIFFilePtr kfFile, std::set<std::string>& controlled)
{
    if(kfFile->numRoots() < 1)
    {
        kfFile->warn("Found no root nodes in "+kfFile->getFilename()+".");
        return;
    }

    const Nif::Record *r = kfFile->getRoot(0);
    assert(r != NULL);

    if(r->recType != Nif::RC_NiSequenceStreamHelper)
    {
        kfFile->warn("First root was not a NiSequenceStreamHelper, but a "+
                  r->recName+".");
        return;
    }
    const Nif::NiSequenceStreamHelper *seq = static_cast<const Nif::NiSequenceStreamHelper*>(r);

    Nif::NiExtraDataPtr extra;
    if (seq->hasExtras)
        extra = seq->extras[0];
    else
        extra = seq->extra;

    if(extra.empty() || extra->recType != Nif::RC_NiTextKeyExtraData)
    {
        kfFile->warn("First extra data was not a NiTextKeyExtraData, but a "+
                  (extra.empty() ? std::string("nil") : extra->recName)+".");
        return;
    }

    if (seq->hasExtras)
        if (seq->extras.length() > 1)
            extra = seq->extras[1];
        else
            return;
    else
        extra = extra->next;

    Nif::ControllerPtr ctrl = seq->controller;
    for(;!extra.empty() && !ctrl.empty();(extra=extra->next)/*FIXME*/,(ctrl=ctrl->next))
    {
        if(extra->recType != Nif::RC_NiStringExtraData || ctrl->recType != Nif::RC_NiKeyframeController)
        {
            kfFile->warn("Unexpected extra data "+extra->recName+" with controller "+ctrl->recName);
            continue;
        }

        if (!(ctrl->flags & Nif::NiNode::ControllerFlag_Active))
            continue;

        const Nif::NiStringExtraData *strdata = static_cast<const Nif::NiStringExtraData*>(extra.getPtr());
        const Nif::NiKeyframeController *key = static_cast<const Nif::NiKeyframeController*>(ctrl.getPtr());

        if(key->data.empty())
            continue;
        controlled.insert(strdata->stringData);
    }
}

// NOTE: calls new, delete is done elsewhere
// FIXME: tested only when called from a list shape
btTriangleMesh *createBhkNiTriStripsShape(const Nif::Node *node,
        Ogre::Vector3& translation, Ogre::Quaternion& rotation, const Nif::bhkShape *bhkShape)
{
    const Nif::bhkNiTriStripsShape* triShape
        = static_cast<const Nif::bhkNiTriStripsShape*>(bhkShape);

    btTriangleMesh *staticMesh = new btTriangleMesh();

    Ogre::Matrix4 t;
    t.makeTransform(translation, Ogre::Vector3(1.f), rotation); // assume uniform scale
    const Nif::NiTriStrips *triStrips = dynamic_cast<const Nif::NiTriStrips*>(node);
    if (triStrips)
        t = triStrips->getWorldTransform() * t;

    for (unsigned int s = 0; s < triShape->stripsData.size(); ++s)
    {
        const Nif::NiTriStripsData* triData
            = static_cast<const Nif::NiTriStripsData*>(triShape->stripsData[s].getPtr());

        const std::vector<Ogre::Vector3> &vertices = triData->vertices;
        const std::vector<short> &triangles = triData->triangles;

        for(size_t i = 0; i < triData->triangles.size(); i += 3)
        {
            Ogre::Vector3 b1 = t*vertices[triangles[i+0]];
            Ogre::Vector3 b2 = t*vertices[triangles[i+1]];
            Ogre::Vector3 b3 = t*vertices[triangles[i+2]];
            staticMesh->addTriangle(btVector3(b1.x,b1.y,b1.z),
                                    btVector3(b2.x,b2.y,b2.z),
                                    btVector3(b3.x,b3.y,b3.z));
        }
    }

    // Since this is assumed to be called only from bhkListShape, we need to pass on the
    // transform so that it can be added to addChildShape()
    return staticMesh;
}

// NOTE: calls new, delete is done elsewhere
btTriangleMesh *createBhkPackedNiTriStripsShape(const Nif::Node *node,
        Ogre::Vector3& translation, Ogre::Quaternion& rotation, const Nif::bhkShape *bhkShape)
{
    const Nif::bhkPackedNiTriStripsShape* triShape
        = static_cast<const Nif::bhkPackedNiTriStripsShape*>(bhkShape);
    const Nif::hkPackedNiTriStripsData* triData
        = static_cast<const Nif::hkPackedNiTriStripsData*>(triShape->data.getPtr());

    btTriangleMesh *staticMesh = new btTriangleMesh();

    Ogre::Matrix4 t;
    // note the difference (no factor of 7 on translation)
    t.makeTransform(translation, Ogre::Vector3(1.f), rotation); // assume uniform scale
    t = node->getWorldTransform() * t;

    for(size_t i = 0; i < triData->triangles.size(); ++i)
    {
        Ogre::Vector3 b1 = t*triData->vertices[triData->triangles[i].triangle[0]]*7; // NOTE: havok scale
        Ogre::Vector3 b2 = t*triData->vertices[triData->triangles[i].triangle[1]]*7; // NOTE: havok scale
        Ogre::Vector3 b3 = t*triData->vertices[triData->triangles[i].triangle[2]]*7; // NOTE: havok scale
        staticMesh->addTriangle((btVector3(b1.x,b1.y,b1.z)),
                                (btVector3(b2.x,b2.y,b2.z)),
                                (btVector3(b3.x,b3.y,b3.z)));
    }

    translation = Ogre::Vector3::ZERO; // transform was applied here, do not apply again later
    rotation = Ogre::Quaternion::IDENTITY;

    return staticMesh;
}

// NOTE: calls new, delete is done elsewhere
btConvexHullShape *createBhkConvexVerticesShape(const Nif::Node *node,
        Ogre::Vector3& translation, Ogre::Quaternion& rotation, const Nif::bhkShape *bhkShape)
{
    const Nif::bhkConvexVerticesShape *shape = static_cast<const Nif::bhkConvexVerticesShape*>(bhkShape);

    btConvexHullShape *convexHull = new btConvexHullShape();

    Ogre::Matrix4 n = node->getWorldTransform();
    Ogre::Matrix4 t; // NOTE: havok scale of 7
    t.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale
    t = n * t;

    for (unsigned int i = 0; i < shape->vertices.size(); ++i)
    {
        btVector3 v = shape->vertices[i]*7; // NOTE: havok scale
        Ogre::Vector3 point = t * Ogre::Vector3(v.getX(), v.getY(), v.getZ());
        convexHull->addPoint(btVector3(point.x, point.y, point.z));
    }

    translation = Ogre::Vector3::ZERO; // transform was applied here, do not apply again later
    rotation = Ogre::Quaternion::IDENTITY;

    return convexHull;
}

btCollisionShape *createBtPrimitive(const Nif::Node *node, const Nif::bhkShape *bhkShape)
{
    if (!bhkShape) // assumes node is valid
        return nullptr;

    switch (bhkShape->recType)
    {
        case Nif::RC_bhkBoxShape:
        {
            const Nif::bhkBoxShape* boxShape = static_cast<const Nif::bhkBoxShape*>(bhkShape);
            return new btBoxShape(boxShape->dimensions*7); // NOTE: havok scale
        }
        case Nif::RC_bhkSphereShape:
        {
            const Nif::bhkSphereShape* shape = static_cast<const Nif::bhkSphereShape*>(bhkShape);
            float radius = shape->radius*7; // NOTE: havok scale
            return new btSphereShape(radius);
        }
        case Nif::RC_bhkMultiSphereShape:
        {
            const Nif::bhkMultiSphereShape *multiSphereShape
                = static_cast<const Nif::bhkMultiSphereShape*>(bhkShape);

            unsigned int numSpheres = multiSphereShape->spheres.size();
            btVector3 *centers = new btVector3[numSpheres];
            btScalar *radii = new btScalar[numSpheres];

            for (unsigned int i = 0; i < numSpheres; ++i)
            {
                Ogre::Vector3 c = multiSphereShape->spheres[i].center*7; // NOTE: havok scale
                centers[i] = btVector3(c.x, c.y, c.z);
                radii[i] = multiSphereShape->spheres[i].radius*7; // NOTE: havok scale
            }

            btMultiSphereShape *res = new btMultiSphereShape(centers, radii, numSpheres);

            delete[] radii;
            radii = nullptr;
            delete[] centers;
            centers = nullptr;

            return res;
        }
        default:
            return nullptr;
    }
}

// NOTE: ICColArc01.NIF doesn't have any collision defined for the roof parts
btCollisionShape *createBhkShape(const Nif::Node *node,
        Ogre::Vector3& translation, Ogre::Quaternion& rotation, const Nif::bhkShape *bhkShape)
{
    if (!bhkShape) // assumes node is valid
        return nullptr;

    switch (bhkShape->recType)
    {
        case Nif::RC_bhkNiTriStripsShape: // meshes\\Furniture\\MiddleClass\\BearSkinRug01.NIF
        {
            const Nif::bhkNiTriStripsShape* shape = static_cast<const Nif::bhkNiTriStripsShape*>(bhkShape);

            return new NifBullet::TriangleMeshShape(
                createBhkNiTriStripsShape(node, translation, rotation, shape), true);
        }
        case Nif::RC_bhkMoppBvTreeShape: // e.g. ICColArc01.NIF
        {
            // FIXME: TODO get some info before moving to the next shape in a link

            return createBhkShape(node, translation, rotation,
                    static_cast<const Nif::bhkMoppBvTreeShape*>(bhkShape)->shape.getPtr());
        }
        case Nif::RC_bhkPackedNiTriStripsShape: // usually from bhkMoppBvTreeShape
        {
            const Nif::bhkPackedNiTriStripsShape* shape
                = static_cast<const Nif::bhkPackedNiTriStripsShape*>(bhkShape);

            return new NifBullet::TriangleMeshShape(
                createBhkPackedNiTriStripsShape(node, translation, rotation, shape), true);
        }
        case Nif::RC_bhkConvexVerticesShape: // e.g. ICSignCopious01.NIF, "CollisionICDoor04"
        {
            const Nif::bhkConvexVerticesShape *shape
                = static_cast<const Nif::bhkConvexVerticesShape*>(bhkShape);

            return createBhkConvexVerticesShape(node, translation, rotation, shape);
        }
        case Nif::RC_bhkListShape:
        {
            const Nif::bhkListShape *shape = static_cast<const Nif::bhkListShape*>(bhkShape);

            btCompoundShape *compoundShape = new btCompoundShape();

            Ogre::Vector3 v;
            Ogre::Quaternion q;
            for (unsigned int i = 0; i < shape->numSubShapes; ++i)
            {
                const Nif::bhkShape *subShape = shape->subShapes[i].getPtr();

                v = translation; // keep a copy in case it gets modified by createBhkShape
                q = rotation;

                btCollisionShape *collisionShape = createBhkShape(node, v, q, subShape);
                if (!collisionShape)
                {
                    std::cerr << "createBhkShape: returned nullptr " << node->name << std::endl;
                    continue;
                }
                Ogre::Matrix4 shapeTrans;
                shapeTrans.makeTransform(v, Ogre::Vector3(1.f), q); // assume uniform scale

                Ogre::Matrix4 t;
                const Nif::bhkNiTriStripsShape* triShape = dynamic_cast<const Nif::bhkNiTriStripsShape*>(subShape);
                if (triShape) // FIXME: horrible hack
                    t = shapeTrans; // already applied rigid body and world transform
                else
                {
                    // NOTE: havok scale of 7
                    t.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale
                    t = node->getWorldTransform() * t * shapeTrans;
                }

                v = t.getTrans();          // NOTE: reusing the variables for a different purpose
                q = t.extractQuaternion(); // NOTE: reusing the variables for a different purpose

                btTransform transform(btQuaternion(q.x, q.y, q.z, q.w), btVector3(v.x, v.y, v.z));
                compoundShape->addChildShape(transform, collisionShape);
            }

            translation = Ogre::Vector3::ZERO; // transform was applied here, do not apply again later
            rotation = Ogre::Quaternion::IDENTITY;

            return compoundShape;
        }
        case Nif::RC_bhkTransformShape:
        case Nif::RC_bhkConvexTransformShape:
        {
            const Nif::bhkTransformShape *shape = dynamic_cast<const Nif::bhkTransformShape*>(bhkShape);

            // first get the shape's transformations (if any), it is done this way because
            // sometimes the shape needs to apply the world transform (i.e. when it is not
            // called from this node) - TODO refactor to make it consistent
            // FIXME: is there a way to do this without converting to 4x4 matrix each time?
            Ogre::Vector3 v = Ogre::Vector3::ZERO;
            Ogre::Quaternion q = Ogre::Quaternion::IDENTITY;

            btCollisionShape *colShape = createBhkShape(node, v, q, shape->shape.getPtr());

            Ogre::Matrix4 shapeTrans;
            shapeTrans.makeTransform(v, Ogre::Vector3(1.f), q); // assume uniform scale

            // now apply this node's transform
            Ogre::Vector3 localTranslation = shape->transform.getTrans();
            Ogre::Quaternion localRotation = shape->transform.extractQuaternion();

            Ogre::Matrix4 localTrans; // NOTE: havok scale of 7
            localTrans.makeTransform(localTranslation*7, Ogre::Vector3(1.f), localRotation); // assume uniform scale
            localTrans = localTrans * shapeTrans;

            translation = localTrans.getTrans(); // update the caller's transform
            rotation = localTrans.extractQuaternion();

            return colShape;

        }
        case Nif::RC_bhkBoxShape: // e.g. Clutter\\Books\\WantedPoster02Static.NIF
        case Nif::RC_bhkSphereShape: // e.g. Blackberry01 in ICMarketDistrictJensinesGoodasUpstairs
        case Nif::RC_bhkMultiSphereShape: // e.g. "meshes\\Clutter\\MagesGuild\\ApparatusAlembicNovice.NIF"
        {
            // more examples of box shape:
            // MiddleCrate06, MiddleCrate01, MiddleCrate02, MiddleCrate04, MiddlePaintingForest03d
            return createBtPrimitive(node, bhkShape);
        }
        case Nif::RC_bhkCapsuleShape: // e.g. "meshes\\Clutter\\MagesGuild\\ApparatusAlembicNovice.NIF"
        {
            // more examples of capsule shape:
            // meshes\clutter\farm\yarn01.nif,    meshes\lights\candlefat01.nif,
            // meshes\lights\candlefat02fake.nif, meshes\lights\candleskinny01fake.nif
            const Nif::bhkCapsuleShape *shape
                = static_cast<const Nif::bhkCapsuleShape*>(bhkShape);

            // The btCapsuleShape represents a capsule around the Y axis so it needs to be
            // rotated.  Make use of Ogre's convenient functions to do this.
            Ogre::Vector3 firstPoint = shape->firstPoint*7;   // NOTE: havok scale
            Ogre::Vector3 secondPoint = shape->secondPoint*7; // NOTE: havok scale
            Ogre::Vector3 axis = secondPoint - firstPoint;

            float height = firstPoint.distance(secondPoint);
            float radius = shape->radius; // FIXME: what is radius1 and radius2 ?

            // update the caller's transform
            translation = firstPoint.midPoint(secondPoint);

            // FIXME: horrible hack - upright capsule shapes don't get the rotations right for some reason
            if (firstPoint.x == secondPoint.x && firstPoint.y == secondPoint.y && firstPoint.z != secondPoint.z)
            {
                rotation = axis.getRotationTo(Ogre::Vector3::NEGATIVE_UNIT_Z);
                return new btCapsuleShapeZ(radius*7, height); // NOTE: havok scale
            }
            else
            {
                rotation = axis.getRotationTo(Ogre::Vector3::UNIT_Y); // should this be NEGATIVE_UNIT_Y?
                return new btCapsuleShape(radius*7, height); // NOTE: havok scale
            }
        }
        default:
            return nullptr;
    }
}

}

namespace NifBullet
{

ManualBulletShapeLoader::~ManualBulletShapeLoader()
{
}


btVector3 ManualBulletShapeLoader::getbtVector(Ogre::Vector3 const &v)
{
    return btVector3(v[0], v[1], v[2]);
}

// Notes on collision for newer Nif files:
// =======================================
//
// The root node is a NiNode.  If the Nif file has collision, it should have a pointer to a
// NiCollisionObject.  The NiStringsExtraData should indicate the number of collision groups.
// Extra Data List BSX also has informtion about collision.  From niflib:
//
//         Bit 0 : enable havok,       bAnimated(Skyrim)
//         Bit 1 : enable collision,   bHavok(Skyrim)
//         Bit 2 : is skeleton nif?,   bRagdoll(Skyrim)
//         Bit 3 : enable animation,   bComplex(Skyrim)
//         Bit 4 : FlameNodes present, bAddon(Skyrim)
//         Bit 5 : EditorMarkers present
//         Bit 6 :                     bDynamic(Skyrim)
//         Bit 7 :                     bArticulated(Skyrim)
//         Bit 8 :                     bIKTarget(Skyrim)
//         Bit 9 :                     Unknown(Skyrim)
//
// The collision object has a flag and couple of pointers, to the target and to the body which
// is usually a bhkRigidBodyT.
//
// The body points to a collision shape:
//   bhkConvexVerticesShape (e.g. shield/bow)
//   bhkListShape with sub shapes (swords/waraxe/warhammer)
//   bhkMoppBvTreeShape which in turn points to the shape itself e.g. bhkPackedNiTriStripsShape (architecture)
//   (TODO: figure out the decision tree logic)
//
// Note: NiTriStrips block may also specify a collision object (but don't have an example yet)
//
// Notes on this method:
// =====================
//
// This method is a callback function, etc, etc TODO
//
void ManualBulletShapeLoader::loadResource(Ogre::Resource *resource)
{
    mShape = static_cast<OEngine::Physic::BulletShape *>(resource);
    mResourceName = mShape->getName();
    mShape->mCollide = false;
    mBoundingBox = NULL;
    mShape->mBoxTranslation = Ogre::Vector3(0,0,0);
    mShape->mBoxRotation = Ogre::Quaternion::IDENTITY;
    mCompoundShape = NULL;
    mStaticMesh = NULL;

    Nif::NIFFilePtr pnif (Nif::Cache::getInstance().load(mResourceName.substr(0, mResourceName.length()-7)));
    Nif::NIFFile & nif = *pnif.get ();
    if (nif.numRoots() < 1)
    {
        warn("Found no root nodes in NIF.");
        return;
    }

    Nif::Record *r = nif.getRoot(0);
    assert(r != NULL);

    Nif::Node *node = dynamic_cast<Nif::Node*>(r);
    if (node == NULL)
    {
        warn("First root in file was not a node, but a " +
             r->recName + ". Skipping file.");
        return;
    }

    mControlledNodes.clear();

    if (r->nifVer >= 0x0a000100)
    {
        mShape->mAutogenerated = false; // should not autogenerate collision, use bhk* shapes instead
#if 0
        // TODO: should also check BSX flags and NiStringsExtraData
        if (ninode->collision.empty())
            mShape->mAutogenerated = true;
        else
            mShape->mAutogenerated = false;
#endif
    }
    else // check controlled nodes only for older style TES3 Nif's
    {
        // Have to load controlled nodes from the .kf
        // FIXME: the .kf has to be loaded both for rendering and physics, ideally it should be opened once and then reused
        std::string kfname = mResourceName.substr(0, mResourceName.length()-7);
        Misc::StringUtils::lowerCaseInPlace(kfname);
        if(kfname.size() > 4 && kfname.compare(kfname.size()-4, 4, ".nif") == 0)
            kfname.replace(kfname.size()-4, 4, ".kf");
        if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(kfname))
        {
            Nif::NIFFilePtr kf (Nif::Cache::getInstance().load(kfname));
            extractControlledNodes(kf, mControlledNodes);
        }

        mShape->mAutogenerated = hasAutoGeneratedCollision(node);
    }

    //do a first pass
    if (r->nifVer >= 0x0a000100) // TES4 style, i.e. from 10.0.1.0
    {
        // for newer Nif's we should know the flags up front without going through all the nodes
        // except collision which may be indicated in each of NiNode or NiTriStrips?
        bool isAnimated = false;
        int flags = 0;
        Nif::Extra const *e = dynamic_cast<Nif::Extra*>(r);
        if (e && e->hasExtras)
        {
            Nif::NiExtraDataPtr extra;
            for (unsigned int i = 0; i < node->extras.length(); ++i)
            {
                extra = node->extras[i]; // get the next extra data in the list
                assert(extra.getPtr() != NULL);

                if (!extra.empty() && extra->name == "BSX")
                {
                    if (r->nifVer >= 0x14020007) // TES5
                    {
                        isAnimated = (static_cast<Nif::BSXFlags*>(extra.getPtr())->integerData & 0x1) != 0;
                        // FIXME: which bits indicate collision?
                    }
                    else
                    {
                        isAnimated = (static_cast<Nif::BSXFlags*>(extra.getPtr())->integerData & 0x8) != 0;
                        //if ((extra->data & 0x11) == 0) // 0x1 is havok, 0x10 is collision
                            //flags |= 0x800; // no collision
                    }

                    break;
                }
            }
            // FIXME what if there are no extras or BSX?
        }

        handleNiNode(node);
    }
    else
        handleNode(node, 0/*flags*/, false/*isCollsionNode*/, false/*raycasting*/); // isAnimated=false by default

    // FIXME: TES4 skeleton.nif has a bounding box as well as havok collision shapes
    // We need a way to deal with having both (one for fast check and another for detailed check)
    if(mBoundingBox != NULL) // this is set in handleNode if hasBounds
    {
       mShape->mCollisionShape = mBoundingBox;
       delete mStaticMesh;
       if (mCompoundShape) // this is set if animated shape
       {
           int n = mCompoundShape->getNumChildShapes();
           for(int i=0; i <n;i++)
               delete (mCompoundShape->getChildShape(i));
           delete mCompoundShape;
           mShape->mAnimatedShapes.clear();
       }
    }
    else
    {
        if (mCompoundShape)
        {
            mShape->mCollisionShape = mCompoundShape;
            if (mStaticMesh)
            {
                btTransform trans;
                trans.setIdentity();
                mCompoundShape->addChildShape(trans, new TriangleMeshShape(mStaticMesh,true));
            }
        }
        else if (mStaticMesh)
            mShape->mCollisionShape = new TriangleMeshShape(mStaticMesh,true);
    }

    if (r->nifVer >= 0x0a000100) // TES4 style, i.e. from 10.0.1.0
        return; // FIXME: skip second pass for now

    //second pass which create a shape for raycasting.
    mResourceName = mShape->getName();
    mShape->mCollide = false;
    mBoundingBox = NULL;
    mStaticMesh = NULL;
    mCompoundShape = NULL;
    if (r->nifVer >= 0x0a000100) // TES4 style, i.e. from 10.0.1.0
    {
        // for newer Nif's we should know the flags up front without going through all the nodes
        // except collision which may be indicated in each of NiNode or NiTriStrips?
        bool isAnimated = false;
        int flags = 0;
        Nif::Extra const *e = dynamic_cast<Nif::Extra*>(r);
        if (e && e->hasExtras)
        {
            Nif::NiExtraDataPtr extra;
            for (unsigned int i = 0; i < node->extras.length(); ++i)
            {
                extra = node->extras[i]; // get the next extra data in the list
                assert(extra.getPtr() != NULL);

                if (!extra.empty() && extra->name == "BSX")
                {
                    if (r->nifVer >= 0x14020007) // TES5
                    {
                        isAnimated = (static_cast<Nif::BSXFlags*>(extra.getPtr())->integerData & 0x1) != 0;
                        // FIXME: which bits indicate collision?
                    }
                    else
                    {
                        isAnimated = (static_cast<Nif::BSXFlags*>(extra.getPtr())->integerData & 0x8) != 0;
                        //if ((extra->data & 0x11) == 0) // 0x1 is havok, 0x10 is collision
                            //flags |= 0x800; // no collision
                    }

                    break;
                }
            }
            // FIXME what if there are no extras or BSX?
        }

        handleNiNode(node);
    }
    else
        handleNode(node,0,true,true,false);

    if (mCompoundShape)
    {
        mShape->mRaycastingShape = mCompoundShape;
        if (mStaticMesh)
        {
            btTransform trans;
            trans.setIdentity();
            mCompoundShape->addChildShape(trans, new TriangleMeshShape(mStaticMesh,true));
        }
    }
    else if (mStaticMesh)
        mShape->mRaycastingShape = new TriangleMeshShape(mStaticMesh,true);
}

bool ManualBulletShapeLoader::hasAutoGeneratedCollision(Nif::Node const * rootNode)
{
    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(rootNode);
    if(ninode)
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(!list[i].empty())
            {
                if(list[i].getPtr()->recType == Nif::RC_RootCollisionNode)
                    return false;
            }
        }
    }
    return true;
}

// Method for bridging with the existing code.
//
// mShape->mBoxTranslation and mShape->mBoxRotation are used in PhysicEngine::adjustRigidBody()
// and PhysicEngine::createAndAdjustRigidBody()
//
// FIXME: createBhkShape may return a nullptr
void ManualBulletShapeLoader::handleBhkShape(const Nif::Node *node,
        const Ogre::Vector3& trans, const Ogre::Quaternion& rot, const Nif::bhkShape *bhkShape)
{
    if (!bhkShape) // node is already checked upstream
        return;

    switch (bhkShape->recType)
    {
        case Nif::RC_bhkMoppBvTreeShape:
        case Nif::RC_bhkPackedNiTriStripsShape:
        case Nif::RC_bhkListShape:
        {
            if (!mShape->mCollide)
            {
                mShape->mCollide = true;

                if (mShape->mCollisionShape) // this shouldn't happen...
                {
                    std::cerr << "bhkShape: mCollisionShape already exists, skipping " << node->name << std::endl;
                    break;
                }

                Ogre::Vector3 translation(trans);
                Ogre::Quaternion rotation(rot);
                mShape->mCollisionShape = createBhkShape(node, translation, rotation, bhkShape);

                mShape->mRaycastingShape = mShape->mCollisionShape;
            }

            break;
        }
        case Nif::RC_bhkConvexVerticesShape:
        {
            if (!mShape->mCollide) // we're the first one
            {
                mShape->mCollide = true;

                if (mShape->mCollisionShape) // this shouldn't happen...
                {
                    std::cerr << "bhkShape: mCollisionShape already exists, skipping " << node->name << std::endl;
                    break;
                }

                Ogre::Vector3 translation(trans);
                Ogre::Quaternion rotation(rot);
                mShape->mCollisionShape = createBhkShape(node, translation, rotation, bhkShape);

                mShape->mRaycastingShape = mShape->mCollisionShape;
            }
            else
            {
                // for this shape the transform was already applied
                btTransform transform;
                transform.setIdentity();

                if (!mShape->mCollisionShape->isCompound()) // convert
                {
                    if (mShape->mCollisionShape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE)
                    {
                        std::cerr << "bhkShape: can't convert to a compound shape, skipping "
                                  << node->name << std::endl;
                        break;
                    }

                    btCompoundShape *compoundShape = new btCompoundShape();

                    compoundShape->addChildShape(transform, mShape->mCollisionShape);
                    mShape->mCollisionShape = compoundShape;
                }

                // now add
                Ogre::Vector3 translation(trans);
                Ogre::Quaternion rotation(rot);
                btCollisionShape *subShape = createBhkShape(node, translation, rotation, bhkShape);

                static_cast<btCompoundShape*>(mShape->mCollisionShape)->addChildShape(transform, subShape);
            }

            break;
        }
        case Nif::RC_bhkBoxShape:
        case Nif::RC_bhkSphereShape:
        case Nif::RC_bhkCapsuleShape:
        {
            if (!mShape->mCollide) // we're the first one
            {
                mShape->mCollide = true;

                if (mShape->mCollisionShape) // this shouldn't happen...
                {
                    std::cerr << "bhkShape: mCollisionShape already exists, skipping " << node->name << std::endl;
                    break;
                }

                Ogre::Vector3 v = Ogre::Vector3::ZERO;
                Ogre::Quaternion q = Ogre::Quaternion::IDENTITY;
                mShape->mCollisionShape = createBhkShape(node, v, q, bhkShape);

                mShape->mRaycastingShape = mShape->mCollisionShape;

                Ogre::Matrix4 shapeTrans;
                shapeTrans.makeTransform(v, Ogre::Vector3(1.f), q); // assume uniform scale

                Ogre::Matrix4 t = Ogre::Matrix4::IDENTITY; // NOTE: havok scale of 7
                t.makeTransform(trans*7, Ogre::Vector3(1.f), rot); // assume uniform scale
                t = node->getWorldTransform() * t * shapeTrans;

                mShape->mBoxTranslation = t.getTrans();
                mShape->mBoxRotation = t.extractQuaternion();

                // FIXME: hack to keep a copy in case this becomes a compound shape
                // FIXME: is this hack still needed?
                const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
                if (ninode && ninode->children.length() > 0)
                {
                    Ogre::Vector3 v = t.getTrans();
                    mFirstTransform = btTransform(btQuaternion(mShape->mBoxRotation.x,
                                                               mShape->mBoxRotation.y,
                                                               mShape->mBoxRotation.z,
                                                               mShape->mBoxRotation.w),
                                                  btVector3(v.x, v.y, v.z));
                }
            }
            else
            {
                // FIXME: may need to flag as an animated shape?
                if (!mShape->mCollisionShape->isCompound()) // convert
                {
                    if (mShape->mCollisionShape->getShapeType() == TRIANGLE_MESH_SHAPE_PROXYTYPE)
                    {
                        std::cerr << "bhkShape: can't convert to a compound shape, skipping "
                                  << node->name << std::endl;
                        break;
                    }

                    btCompoundShape *compoundShape = new btCompoundShape();

                    compoundShape->addChildShape(mFirstTransform, mShape->mCollisionShape);
                    mShape->mCollisionShape = compoundShape;

                    mShape->mBoxTranslation = Ogre::Vector3::ZERO;
                    mShape->mBoxRotation = Ogre::Quaternion::IDENTITY;
                }

                // now add
                Ogre::Vector3 translation(trans);
                Ogre::Quaternion rotation(rot);
                btCollisionShape *subShape = createBhkShape(node, translation, rotation, bhkShape);

                Ogre::Matrix4 t = node->getWorldTransform();
                translation = t * translation;
                rotation = t.extractQuaternion() * rotation;

                btTransform transform(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w),
                                      btVector3(translation.x, translation.y, translation.z));
                static_cast<btCompoundShape*>(mShape->mCollisionShape)->addChildShape(transform, subShape);
            }

            break;
        }
        default:
            std::cerr << "bhkShape: unsupported shape " << node->name << std::endl;
    }
}

// Ideally, the object being created (at the end of the branch) will apply all the transforms.
// However, some of the bullet primitives does not allow transforms to be applied (at least I
// don't know how).  Compound shapes have the transforms applied when calling addChildShape().
//
// Current solution is a mixture:
//
// Transforms are applied for btConvexHullShape, btTriangleMesh (used for creating
// btBvhTriangleMeshShape).
//
// Trasforms are not applied for btBoxShape, btSphereShape and btCapsuleShape.
//
// i.e:
//
//  - the primitives return its own transform (if any) to the caller
//  - the caller applies the primitive shapes's transform to its own then return to is caller
//  - a list shape
//
// NOTE: the Havok scale factor of 7 is not applied here - deferred till when the transforms
// are applied.
//
// NOTE: assumed that this won't be recused i.e. rigidBodyTransform  occurs only once in a branch
void ManualBulletShapeLoader::handleBhkCollisionObject(const Nif::Node *node,
        const Nif::NiCollisionObject *collObj)
{
    if (!node || !collObj)
        return;

    const Nif::bhkCollisionObject *bhkCollObj = static_cast<const Nif::bhkCollisionObject*>(collObj);
    const Nif::bhkRigidBody *rigidBody = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());

    if(bhkCollObj->body.getPtr()->recType == Nif::RC_bhkRigidBodyT)
    {
        Ogre::Quaternion rotation = Ogre::Quaternion(rigidBody->rotation.w,
                rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);
        Ogre::Vector3 translation = Ogre::Vector3(rigidBody->translation.x,
                rigidBody->translation.y, rigidBody->translation.z);

        handleBhkShape(node, translation, rotation, rigidBody->shape.getPtr());
    }
    else
        handleBhkShape(node, Ogre::Vector3::ZERO, Ogre::Quaternion::IDENTITY, rigidBody->shape.getPtr());

    Ogre::Quaternion rotation = Ogre::Quaternion(rigidBody->rotation.w,
            rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);

    return;
}

// NOTE: this method gets recursed
//
// At the root node we don't know if any of the children will have collision shapes.
// If there are more than one we will need to use btCompoundShape instead.
//
// e.g. Meshes\Architecture\Cathedral\Crypt\CathedralCryptLight02.NIF
//
// Note that mShape->mCollide is set to false by loadResource() before calling handleNiNode()
//
// Strategy: see handleBhkShape()
//
// - check if we have a collision object for this node
//
// - if mShape->mCollide is false, we're the first one with a collision; either create a
//   compound shape (for bhkListShape) or another collision shape and assign it to
//   mShape->mCollisionShape (save the local transform to mShape->mBoxTranslation and
//   mShape->mBoxRotation as required)
//
//   FIXME: some can't do this, e.g. bhkPackedNiTriStripsShape and bhkConvexVerticesShape
//          because the transforms are already applied; for now just log an error
//
// - if mShape->mCollide is true, either add to the compund shape or convert the existing one
//   as a compound shape
//
// - loop though the children (recurse)
//
void ManualBulletShapeLoader::handleNiNode(const Nif::Node *node)
{
    // Check for extra data
    Nif::Extra const *e = node;
    if (e->hasExtras) // FIXME: this boolean may change in the Nif class in future cleanups
    {
        Nif::NiExtraDataPtr extra;
        for (unsigned int i = 0; i < node->extras.length(); ++i)
        {
            // Get the next extra data in the list
            extra = node->extras[i];
            assert(extra.getPtr() != NULL);

            if (!extra.empty() && extra.getPtr()->recType == Nif::RC_NiStringExtraData)
            {
                // String markers may contain important information
                // affecting the entire subtree of this node
                Nif::NiStringExtraData *sd = (Nif::NiStringExtraData*)extra.getPtr();

                // FIXME: what to do here?

            }
        }
    }

    // NOTE: nifVer > 4.2.2.0 do not have hasBounds indicator but may have NiExtraData BBX
    // (e.g. skeleton.nif) // FIXME

    if (!node->collision.empty())
    {
        // node->collision is a NiCollisionObject and may be NiCollisionData or bhkNiCollisionObject
        // (or its child, bhkCollisionObject)
        //
        // bhkCollisionObject refers to a body (NiObject) usually bhkRigidBodyT (or bhkRigidBody)
        // which then refers to a bhkShape.
        const Nif::NiCollisionObjectPtr collObj = node->collision;
        if (collObj->recType == Nif::RC_bhkCollisionObject)
        {
            handleBhkCollisionObject(node, collObj.getPtr());
        }
        else if (collObj->recType == Nif::RC_NiCollisionData)
        {
            // FIXME TODO
            // FIXME: examples?
            std::cout << "NiNode: unhandled NiCollisionData " << node->name << std::endl;
        }
        else
            std::cout << "NiNode: unhandled NiCollisionObject " << node->name << std::endl;
    }

    // loop through the children
    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0; i < list.length(); i++)
        {
            if(!list[i].empty())
                handleNiNode(list[i].getPtr());
        }
    }
}

void ManualBulletShapeLoader::handleNode(const Nif::Node *node, int flags,
                                         bool isCollisionNode,
                                         bool raycasting, bool isAnimated)
{
    // Accumulate the flags from all the child nodes. This works for all
    // the flags we currently use, at least.
    flags |= node->flags;

    if (!node->controller.empty() && node->controller->recType == Nif::RC_NiKeyframeController
            && (node->controller->flags & Nif::NiNode::ControllerFlag_Active))
        isAnimated = true;

    if (mControlledNodes.find(node->name) != mControlledNodes.end())
        isAnimated = true;

    if (!raycasting)
        isCollisionNode = isCollisionNode || (node->recType == Nif::RC_RootCollisionNode);
    else
        isCollisionNode = isCollisionNode && (node->recType != Nif::RC_RootCollisionNode);

    // Don't collide with AvoidNode shapes
    if(node->recType == Nif::RC_AvoidNode)
        flags |= 0x800;

    // Check for extra data
    Nif::Extra const *e = node;
    Nif::NiExtraDataPtr extra = node->extra;
    while (!extra.empty() && !extra->next.empty()) // FIXME: a bit ugly
    {
        // Get the next extra data in the list
        extra = extra->next;
        assert(extra.getPtr() != NULL);

        if (extra.getPtr()->recType == Nif::RC_NiStringExtraData)
        {
            // String markers may contain important information
            // affecting the entire subtree of this node
            Nif::NiStringExtraData *sd = (Nif::NiStringExtraData*)extra.getPtr();

            // not sure what the difference between NCO and NCC is, or if there even is one
            if (sd->stringData == "NCO" || sd->stringData == "NCC")
            {
                // No collision. Use an internal flag setting to mark this.
                flags |= 0x800;
            }
            else if (sd->stringData == "MRK" && !mShowMarkers && (raycasting || mShape->mAutogenerated))
            {
                // Marker objects should be invisible, but can still have collision if the model explicitely specifies it via a RootCollisionNode.
                // Except in the editor, the marker objects are visible.
                return;
            }
        }
    }

    if (isCollisionNode || (mShape->mAutogenerated && !raycasting))
    {
        // NOTE: a trishape with hasBounds=true, but no BBoxCollision flag should NOT go through handleNiTriShape!
        // It must be ignored completely.
        // (occurs in tr_ex_imp_wall_arch_04.nif)
        if(node->hasBounds)
        {
            if (flags & Nif::NiNode::Flag_BBoxCollision && !raycasting)
            {
                mShape->mBoxTranslation = node->boundPos;
                mShape->mBoxRotation = node->boundRot;
                mBoundingBox = new btBoxShape(getbtVector(node->boundXYZ));
            }
        }
        else if(node->recType == Nif::RC_NiTriShape)
        {
            mShape->mCollide = !(flags&0x800);
            handleNiTriShape(static_cast<const Nif::NiTriShape*>(node), flags, node->getWorldTransform(), raycasting, isAnimated);
        }
    }

    // For NiNodes, loop through children
    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(!list[i].empty())
                handleNode(list[i].getPtr(), flags, isCollisionNode, raycasting, isAnimated);
        }
    }
}

void ManualBulletShapeLoader::handleNiTriShape(const Nif::NiTriShape *shape,
        int flags, const Ogre::Matrix4 &transform, bool raycasting, bool isAnimated)
{
    assert(shape != NULL);

    // Interpret flags
    bool hidden    = (flags&Nif::NiNode::Flag_Hidden) != 0;
    bool collide   = (flags&Nif::NiNode::Flag_MeshCollision) != 0;
    bool bbcollide = (flags&Nif::NiNode::Flag_BBoxCollision) != 0;

    // If the object was marked "NCO" earlier, it shouldn't collide with
    // anything. So don't do anything.
    if ((flags & 0x800) && !raycasting)
    {
        return;
    }

    if (!collide && !bbcollide && hidden && !raycasting)
        // This mesh apparently isn't being used for anything, so don't
        // bother setting it up.
        return;

    if (!shape->skin.empty())
        isAnimated = false;

    if (isAnimated)
    {
        if (!mCompoundShape)
            mCompoundShape = new btCompoundShape();

        btTriangleMesh* childMesh = new btTriangleMesh();

        const Nif::NiTriShapeData *data = static_cast<const Nif::NiTriShapeData*>(shape->data.getPtr());

        childMesh->preallocateVertices(data->vertices.size());
        childMesh->preallocateIndices(data->triangles.size());

        const std::vector<Ogre::Vector3> &vertices = data->vertices;
        const std::vector<short> &triangles = data->triangles;

        for(size_t i = 0;i < data->triangles.size();i+=3)
        {
            Ogre::Vector3 b1 = vertices[triangles[i+0]];
            Ogre::Vector3 b2 = vertices[triangles[i+1]];
            Ogre::Vector3 b3 = vertices[triangles[i+2]];
            childMesh->addTriangle(btVector3(b1.x,b1.y,b1.z),btVector3(b2.x,b2.y,b2.z),btVector3(b3.x,b3.y,b3.z));
        }

        TriangleMeshShape* childShape = new TriangleMeshShape(childMesh,true);

        float scale = shape->trafo.scale;
        const Nif::Node* parent = shape;
        while (parent->parent)
        {
            parent = parent->parent;
            scale *= parent->trafo.scale;
        }
        Ogre::Quaternion q = transform.extractQuaternion();
        Ogre::Vector3 v = transform.getTrans();
        childShape->setLocalScaling(btVector3(scale, scale, scale));

        btTransform trans(btQuaternion(q.x, q.y, q.z, q.w), btVector3(v.x, v.y, v.z));

        if (raycasting)
            mShape->mAnimatedRaycastingShapes.insert(std::make_pair(shape->recIndex, mCompoundShape->getNumChildShapes()));
        else
            mShape->mAnimatedShapes.insert(std::make_pair(shape->recIndex, mCompoundShape->getNumChildShapes()));

        mCompoundShape->addChildShape(trans, childShape);
    }
    else
    {
        if (!mStaticMesh)
            mStaticMesh = new btTriangleMesh();

        // Static shape, just transform all vertices into position
        const Nif::NiTriShapeData *data = static_cast<const Nif::NiTriShapeData*>(shape->data.getPtr());
        const std::vector<Ogre::Vector3> &vertices = data->vertices;
        const std::vector<short> &triangles = data->triangles;

        for(size_t i = 0;i < data->triangles.size();i+=3)
        {
            Ogre::Vector3 b1 = transform*vertices[triangles[i+0]];
            Ogre::Vector3 b2 = transform*vertices[triangles[i+1]];
            Ogre::Vector3 b3 = transform*vertices[triangles[i+2]];
            mStaticMesh->addTriangle(btVector3(b1.x,b1.y,b1.z),btVector3(b2.x,b2.y,b2.z),btVector3(b3.x,b3.y,b3.z));
        }
    }
}

void ManualBulletShapeLoader::handleNiTriStrips(const Nif::NiTriStrips *shape,
        int flags, const Ogre::Matrix4 &transform, bool raycasting, bool isAnimated)
{
    assert(shape != NULL);

    // Interpret flags
    bool hidden    = (flags&Nif::NiNode::Flag_Hidden) != 0;
    bool collide   = (flags&Nif::NiNode::Flag_MeshCollision) != 0;
    bool bbcollide = (flags&Nif::NiNode::Flag_BBoxCollision) != 0;

    // If the object was marked "NCO" earlier, it shouldn't collide with
    // anything. So don't do anything.
    if ((flags & 0x800) && !raycasting)
    {
        return;
    }

    if (!collide && !bbcollide && hidden && !raycasting)
        // This mesh apparently isn't being used for anything, so don't
        // bother setting it up.
        return;

    if (!shape->skin.empty())
        isAnimated = false;

    if (isAnimated)
    {
        if (!mCompoundShape)
            mCompoundShape = new btCompoundShape();

        btTriangleMesh* childMesh = new btTriangleMesh();

        const Nif::NiTriStripsData *data = static_cast<const Nif::NiTriStripsData*>(shape->data.getPtr());

        childMesh->preallocateVertices(data->vertices.size());
        childMesh->preallocateIndices(data->triangles.size());

        const std::vector<Ogre::Vector3> &vertices = data->vertices;
        const std::vector<short> &triangles = data->triangles;

        for(size_t i = 0;i < data->triangles.size();i+=3)
        {
            Ogre::Vector3 b1 = vertices[triangles[i+0]];
            Ogre::Vector3 b2 = vertices[triangles[i+1]];
            Ogre::Vector3 b3 = vertices[triangles[i+2]];
            childMesh->addTriangle(btVector3(b1.x,b1.y,b1.z),btVector3(b2.x,b2.y,b2.z),btVector3(b3.x,b3.y,b3.z));
        }

        TriangleMeshShape* childShape = new TriangleMeshShape(childMesh,true);

        float scale = shape->trafo.scale;
        const Nif::Node* parent = shape;
        while (parent->parent)
        {
            parent = parent->parent;
            scale *= parent->trafo.scale;
        }
        Ogre::Quaternion q = transform.extractQuaternion();
        Ogre::Vector3 v = transform.getTrans();
        childShape->setLocalScaling(btVector3(scale, scale, scale));

        btTransform trans(btQuaternion(q.x, q.y, q.z, q.w), btVector3(v.x, v.y, v.z));

        if (raycasting)
            mShape->mAnimatedRaycastingShapes.insert(std::make_pair(shape->recIndex, mCompoundShape->getNumChildShapes()));
        else
            mShape->mAnimatedShapes.insert(std::make_pair(shape->recIndex, mCompoundShape->getNumChildShapes()));

        mCompoundShape->addChildShape(trans, childShape);
    }
    else
    {
        if (!mStaticMesh)
            mStaticMesh = new btTriangleMesh();

        // Static shape, just transform all vertices into position
        const Nif::NiTriStripsData *data = static_cast<const Nif::NiTriStripsData*>(shape->data.getPtr());
        const std::vector<Ogre::Vector3> &vertices = data->vertices;
        const std::vector<short> &triangles = data->triangles;

        for(size_t i = 0;i < data->triangles.size();i+=3)
        {
            Ogre::Vector3 b1 = transform*vertices[triangles[i+0]];
            Ogre::Vector3 b2 = transform*vertices[triangles[i+1]];
            Ogre::Vector3 b3 = transform*vertices[triangles[i+2]];
            mStaticMesh->addTriangle(btVector3(b1.x,b1.y,b1.z),btVector3(b2.x,b2.y,b2.z),btVector3(b3.x,b3.y,b3.z));
        }
    }
}

void ManualBulletShapeLoader::load(const std::string &name,const std::string &group)
{
    // Check if the resource already exists
    Ogre::ResourcePtr ptr = OEngine::Physic::BulletShapeManager::getSingleton().getByName(name, group);
    if (!ptr.isNull())
        return;
    OEngine::Physic::BulletShapeManager::getSingleton().create(name,group,true,this);
}

bool findBoundingBox (const Nif::Node* node, Ogre::Vector3& halfExtents, Ogre::Vector3& translation, Ogre::Quaternion& orientation)
{
    if(node->hasBounds)
    {
        if (!(node->flags & Nif::NiNode::Flag_Hidden))
        {
            translation = node->boundPos;
            orientation = node->boundRot;
            halfExtents = node->boundXYZ;
            return true;
        }
    }

    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(!list[i].empty())
                if (findBoundingBox(list[i].getPtr(), halfExtents, translation, orientation))
                    return true;
        }
    }
    return false;
}

bool getBoundingBox(const std::string& nifFile, Ogre::Vector3& halfExtents, Ogre::Vector3& translation, Ogre::Quaternion& orientation)
{
    Nif::NIFFilePtr pnif (Nif::Cache::getInstance().load(nifFile));
    Nif::NIFFile & nif = *pnif.get ();

    if (nif.numRoots() < 1)
    {
        return false;
    }

    Nif::Record *r = nif.getRoot(0);
    assert(r != NULL);

    Nif::Node *node = dynamic_cast<Nif::Node*>(r);
    if (node == NULL)
    {
        return false;
    }

    return findBoundingBox(node, halfExtents, translation, orientation);
}

} // namespace NifBullet
