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
#include <cmath> // abs
#include <iostream> // FIXME

#include <components/misc/stringops.hpp>

#include <components/nifcache/nifcache.hpp>

#include <components/nif/niffile.hpp>
#include <components/nif/node.hpp>
#include <components/nif/data.hpp>
#include <components/nif/property.hpp>
#include <components/nif/controller.hpp>
#include <components/nif/extra.hpp>
#include <components/nif/collision.hpp>

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

// Meshes\Architecture\ImperialCity\ICDoor04.NIF has 0xa, i.e. animated and collision
// whereas Meshes\Architecture\Cathedral\Crypt\CathedralCryptLight02.NIF has 0xb i.e. havok also
//
// TES5 0x1: animated, 0x2: havok
// TES4 0x1: havok, 0x2: collision, 0x4: skeleton, 0x8: animated
//
// NOTE: above test fails with meshes\clutter\minotaurhead01.nif (0xb but it is not a ragdoll)
// so an additional check needs to be made to see if it is animated (i.e. has a controller)
// NOTE: the check for controller only works at the root node
//
// FIXME: this is duplicated in ogrenifloader.cpp
bool isRagdoll(const Nif::Node *node, unsigned int bsxFlags)
{
    if (node->nifVer >= 0x14020007) // TES5
        return (bsxFlags & 0x2) != 0 && (bsxFlags & 0x1) != 0 && node->controller.empty();
    else                            // TES4
        return (bsxFlags & 0x8) != 0 && (bsxFlags & 0x1) != 0 && node->controller.empty();
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
                //if (node->name == "ICBasement3Way")
                if (node->name == "ICBasementCorner01")
                {
                    std::cout << "AFightingChanceBasement" << std::endl;
                }

    Ogre::Matrix4 t;
    // NOTE: havok scale of 7
    t.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale
    t = node->getWorldTransform() * t;

    for(size_t i = 0; i < triData->triangles.size(); ++i)
    {
        Ogre::Vector3 b1 = t*(triData->vertices[triData->triangles[i].triangle[0]]*7); // NOTE: havok scale
        Ogre::Vector3 b2 = t*(triData->vertices[triData->triangles[i].triangle[1]]*7); // NOTE: havok scale
        Ogre::Vector3 b3 = t*(triData->vertices[triData->triangles[i].triangle[2]]*7); // NOTE: havok scale
        staticMesh->addTriangle((btVector3(b1.x,b1.y,b1.z)),
                                (btVector3(b2.x,b2.y,b2.z)),
                                (btVector3(b3.x,b3.y,b3.z)));
    }

    // FIXME: below3 lines are experiments
    //Ogre::Matrix4 n = node->getWorldTransform();
    //translation = n.getTrans();
    //rotation = n.extractQuaternion();
    translation = Ogre::Vector3::ZERO; // transform was applied here, do not apply again later
    rotation = Ogre::Quaternion::IDENTITY;

    return staticMesh;
}

// NOTE: calls new, delete is done elsewhere
// FIXME: for TargetWeight in TargetHeavy01.NIF, we don't want to bake in the transform here
//        not sure how to fix
//
// clutter/repairhammer.nif has rotation at the node but none at rigid body
// architecture/imperialcity/icsigncopious01.nif has transform at rigid body but none at the node
btConvexHullShape *createBhkConvexVerticesShape(const Nif::Node *node,
        Ogre::Vector3& translation, Ogre::Quaternion& rotation, const Nif::bhkShape *bhkShape)
{
    const Nif::bhkConvexVerticesShape *shape = static_cast<const Nif::bhkConvexVerticesShape*>(bhkShape);
                if (node->name == "ICSignCopious01")
                {
                    std::cout << "copious" << std::endl;
                }

    btConvexHullShape *convexHull = new btConvexHullShape();

    Ogre::Matrix4 n = node->getWorldTransform();
    Ogre::Matrix4 t;
    // NOTE: havok scale of 7
    t.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale
    t = n * t;

    for (unsigned int i = 0; i < shape->vertices.size(); ++i)
    {
        btVector3 v = shape->vertices[i]*7; // NOTE: havok scale
        Ogre::Vector3 point = t * Ogre::Vector3(v.getX(), v.getY(), v.getZ());
        convexHull->addPoint(btVector3(point.x, point.y, point.z));
    }
    translation = n.getTrans();
    rotation = n.extractQuaternion();

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

            size_t numSpheres = multiSphereShape->spheres.size();
            btVector3 *centers = new btVector3[numSpheres];
            btScalar *radii = new btScalar[numSpheres];

            for (size_t i = 0; i < numSpheres; ++i)
            {
                Ogre::Vector3 c = multiSphereShape->spheres[i].center*7; // NOTE: havok scale
                centers[i] = btVector3(c.x, c.y, c.z);
                radii[i] = multiSphereShape->spheres[i].radius*7; // NOTE: havok scale
            }

            btMultiSphereShape *res = new btMultiSphereShape(centers, radii, (int)numSpheres);

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
            if (static_cast<const Nif::bhkMoppBvTreeShape*>(bhkShape)->shape->recType == Nif::RC_bhkNiTriStripsShape)
            {




    const Nif::bhkNiTriStripsShape* triShape
        = static_cast<const Nif::bhkNiTriStripsShape*>(
            static_cast<const Nif::bhkMoppBvTreeShape*>(bhkShape)->shape.getPtr());

    btTriangleMesh *staticMesh = new btTriangleMesh();

    Ogre::Matrix4 t;
    // NOTE: havok scale of 7
    t.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale
    t = node->getWorldTransform() * t;

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

    translation = Ogre::Vector3::ZERO; // transform was applied here, do not apply again later
    rotation = Ogre::Quaternion::IDENTITY;
    return new NifBullet::TriangleMeshShape(staticMesh, true);





            }
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

            float radius = shape->radius; // FIXME: what is radius1 and radius2 ?

            // update the caller's transform
            translation = firstPoint.midPoint(secondPoint);

            // FIXME: horrible hack - upright capsule shapes don't get the rotations right for some reason
            if (firstPoint.x == secondPoint.x && firstPoint.y == secondPoint.y && firstPoint.z != secondPoint.z)
            {
                float height = std::abs(firstPoint.z - secondPoint.z); // NOTE: havok scale already factored in



// FIXME: testing if capsule shape has some special effect on bullet transforms
#if 0
                if (node->name == "TargetchainLeft01" || node->name == "TargetchainLeft02")
                {
                    btVector3 dimensions(radius*7, radius*7, height/2+radius*7);
                    return new btBoxShape(dimensions);
                }
#endif





                rotation = Ogre::Quaternion::IDENTITY;
                return new btCapsuleShapeZ(radius*7, height); // NOTE: havok scale
            }
            else
            {
                float height = firstPoint.distance(secondPoint);
                rotation = axis.getRotationTo(Ogre::Vector3::UNIT_Y); // should this be NEGATIVE_UNIT_Y?
                return new btCapsuleShape(radius*7, height); // NOTE: havok scale
            }
        }
        default:
            return nullptr;
    }
}

// FIXME: not used anywhere
btCollisionShape *createConstraint(const Nif::Node *node, const Nif::bhkShape *bhkShape)
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

            size_t numSpheres = multiSphereShape->spheres.size();
            btVector3 *centers = new btVector3[numSpheres];
            btScalar *radii = new btScalar[numSpheres];

            for (size_t i = 0; i < numSpheres; ++i)
            {
                Ogre::Vector3 c = multiSphereShape->spheres[i].center*7; // NOTE: havok scale
                centers[i] = btVector3(c.x, c.y, c.z);
                radii[i] = multiSphereShape->spheres[i].radius*7; // NOTE: havok scale
            }

            btMultiSphereShape *res = new btMultiSphereShape(centers, radii, (int)numSpheres);

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

} // anon namespace

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
// The root node is a NiNode.  If the Nif file has collision, one of its nodes should have a
// pointer to a NiCollisionObject.  The NiStringExtraData should indicate the number of
// collision groups.  Extra Data List BSX also has informtion about collision.  From niflib:
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
// is usually a bhkRigidBodyT or bhkRigidBody.
//
// The body points to a collision shape:
//
//   bhkConvexVerticesShape (e.g. shield/bow)
//   bhkListShape with sub shapes (swords/waraxe/warhammer)
//   bhkMoppBvTreeShape which in turn points to the shape itself e.g. bhkPackedNiTriStripsShape (architecture)
//   (TODO: figure out the decision tree logic)
//
// Note: NiTriStrips block may also specify a collision object
// (e.g. Meshes\Furniture\MiddleClass\BearSkinRug01.NIF)
//
// Notes on this method:
// =====================
//
// This method is a callback function.  Basic call path is:
//
// MWClass::ForeignStatic::insertObject
//   MWWorld::PhysicsSystem::addObject
//     OEngine::Physic::PhysicEngine::createAndAdjustRigidBody
//       NifBullet::ManualBulletShapeLoader::load
//         OEngine::Physic::BulletShapeManager::create
//           Ogre::ResourceManager::createResource
//                           :
//                           : (callback)
//                           v
//                 NifBullet::ManualBulletShapeLoader::loadResource
//
//
// As a comparison, NPCs and Creatures are handled this way:
//
// MWClass::ForeignNpc::insertObject
//   MWWorld::PhysicsSystem::addActor
//     OEngine::Physic::PhysicEngine::addCharacter
//       OEngine::Physic::PhysicActor::PhysicActor
//         (TES3 actors only get a bounding box/cylinder in the ctor)
//
//
// TODO: Need to figure out how to handle TES4 actors differently.  Probably best to modify
// PhysicsSystem::addActor so that based on the Ptr type call a different method in PhysicEngine
// i.e. using:
//
//   if(ptr.getTypeName() == typeid(ESM::Creature).name())
//
//
// Classes:
// --------
//
//       Ogre::Resource                      Ogre::ResourceManager
//              ^                                      ^
//              |                                      |
//  OEngine::Physic::BulletShape          OEngine::Physic::BulletShapeManager
//
//
//                    Ogre::ManualResourceLoader
//                               ^
//                               |
//                 OEngine::Physic::BulletShapeLoader
//                               ^
//                               |
//                 NifBullet::ManualBulletShapeLoader
//
// FIXME: mResourceName is not normalised, so probably misses the Nif::Cache and loaded again
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
        mShape->mAutogenerated = false; // should not autogenerate collision, use bhk* shapes instead
    else // check controlled nodes only for older style TES3 Nif's
    {
        // Have to load controlled nodes from the .kf
        // FIXME: the .kf has to be loaded both for rendering and physics,
        // ideally it should be opened once and then reused
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
        handleNode(node, 0); // start with 0 bsxFlag
    else
        handleNode(node, /*flags*/0, /*isCollsionNode*/false, /*raycasting*/false); // isAnimated=false by default

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
        //std::cout << node->name << std::endl;
        return; // FIXME: skip second pass for now

    //second pass which create a shape for raycasting.
    mResourceName = mShape->getName();
    mShape->mCollide = false;
    mBoundingBox = NULL;
    mStaticMesh = NULL;
    mCompoundShape = NULL;

    if (r->nifVer >= 0x0a000100) // TES4 style, i.e. from 10.0.1.0
        handleNode(node, 0); // start with 0 bsxFlag
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
void ManualBulletShapeLoader::handleBhkShape(const Nif::Node *node, unsigned int bsxFlags,
        const Ogre::Vector3& trans, const Ogre::Quaternion& rot, const Nif::bhkShape *bhkShape)
{
    if (!bhkShape) // node is already checked upstream
        return;

    switch (bhkShape->recType)
    {
        case Nif::RC_bhkListShape:
        {
#if 0
            // an example that contains a bhkListShape: meshes\Clutter\UpperClass\UpperScales01.NIF
            if (isRagdoll(node, bsxFlags))
            {
                std::cerr << "bhkShape: bhkListShape should not be an animated shape" << node->name << std::endl;
                return;
            }
#endif
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

                mShape->mRaycastingShape = mShape->mCollisionShape; // FIXME

                mShape->mBoxTranslation = translation; // for ragdoll
                mShape->mBoxRotation = rotation; // for ragdoll
            }

            break;
        }
        case Nif::RC_bhkMoppBvTreeShape:
        //case Nif::RC_bhkPackedNiTriStripsShape:
        case Nif::RC_bhkConvexVerticesShape:
        {
            if (mShape->mCollide)
                break; // FIXME: log an error here?
#if 0
            if (isRagdoll(node, bsxFlags))
            {
                std::cout << "bhkShape: animated " << node->name << std::endl;

            }
            else
#endif
            {
                mShape->mCollide = true;

                if (mShape->mCollisionShape) // this shouldn't happen...
                {
                    std::cerr << "bhkShape: mCollisionShape already exists, skipping " << node->name << std::endl;
                    break;
                }

                Ogre::Vector3 translation(trans);
                Ogre::Quaternion rotation(rot);
#if 0
                Ogre::Vector3 translation = Ogre::Vector3::ZERO;
                Ogre::Quaternion rotation = Ogre::Quaternion::IDENTITY;
                //if (!isRagdoll(node, bsxFlags))
                {
                    translation = trans;
                    rotation = rot;
                }
#endif
                mShape->mCollisionShape = createBhkShape(node, translation, rotation, bhkShape);

                mShape->mRaycastingShape = mShape->mCollisionShape; // FIXME

                mShape->mBoxTranslation = translation; // for ragdoll
                mShape->mBoxRotation = rotation; // for ragdoll
            }

            break;
        }
        case Nif::RC_bhkBoxShape:
        case Nif::RC_bhkSphereShape:
        case Nif::RC_bhkCapsuleShape:
        {
            if (mShape->mCollide)
                break; // FIXME: log an error here?
#if 0
            if (isRagdoll(node, bsxFlags))
            {
                std::cout << "bhkShape: animated " << node->name << std::endl;

            }
            else
#endif
            {
                //if (node->name == "HeavyTargetStructure01")
                //if (node->name == "TargetchainRight01")
                    //std::cout << "break" << std::endl;




                mShape->mCollide = true;

                if (mShape->mCollisionShape) // this shouldn't happen...
                {
                    std::cerr << "bhkShape: mCollisionShape already exists, skipping " << node->name << std::endl;
                    break;
                }

                Ogre::Vector3 v = Ogre::Vector3::ZERO;
                Ogre::Quaternion q = Ogre::Quaternion::IDENTITY;
                mShape->mCollisionShape = createBhkShape(node, v, q, bhkShape);
                // for a capsule shape v is the midpoint vector (in Havok scale) at this point

                mShape->mRaycastingShape = mShape->mCollisionShape; // FIXME

                Ogre::Matrix4 shapeTrans;
                shapeTrans.makeTransform(v, Ogre::Vector3(1.f), q); // assume uniform scale

                Ogre::Matrix4 t = Ogre::Matrix4::IDENTITY; // NOTE: havok scale of 7
                t.makeTransform(trans*7, Ogre::Vector3(1.f), rot); // assume uniform scale
                t = node->getWorldTransform() * t * shapeTrans;

                // FIXME: HACK
                // check if constraints exist and if so fix pivotA and pivotB here?
                // createBhkShape will have v and q modified in case of RC_bhkCapsuleShape
                // and hence shapeTrans will be non-zero/identity (?better description?)
                //mShape->mShapeTrans.makeTransform(v, Ogre::Vector3(1.f), q); // assume uniform scale
                mShape->mShapeTrans = v;
                //mShape->mShapeTrans = t;

                mShape->mBoxTranslation = t.getTrans(); // FIXME: gets overwritten the next time!!
                mShape->mBoxRotation = t.extractQuaternion();
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
void ManualBulletShapeLoader::handleBhkCollisionObject(const Nif::Node *node, unsigned int bsxFlags,
        const Nif::NiCollisionObject *collObj)
{
    if (!node || !collObj)
        return;
    //if (node->name == "HeavyTargetStructure01")
    //if (node->name == "TargetchainLeft01")
        //std::cout << "break" << std::endl;

    const Nif::bhkCollisionObject *bhkCollObj = static_cast<const Nif::bhkCollisionObject*>(collObj);
    const Nif::bhkRigidBody *rigidBody = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());

    Ogre::Vector3 translation = Ogre::Vector3::ZERO;
    Ogre::Quaternion rotation = Ogre::Quaternion::IDENTITY;

    // apply rotation and translation only if the collision object's body is a bhkRigidBodyT type
    if(bhkCollObj->body.getPtr()->recType == Nif::RC_bhkRigidBodyT)
    {
        rotation = Ogre::Quaternion(rigidBody->rotation.w,
                rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);
        translation = Ogre::Vector3(rigidBody->translation.x,
                rigidBody->translation.y, rigidBody->translation.z);
    }
    // aftr this call mShape->mBoxTranslation and mShape->mBoxRotation are updated for the shape
    handleBhkShape(node, bsxFlags, translation, rotation, rigidBody->shape.getPtr());

    // FIXME: the logic seems to be broken here, mShape is a global one for the
    //        ManualBulletShapeLoader, not local for this bhkCollisionObject
    if (mShape->mIsRagdoll)
    {
        //std::cout << "bhkCollisionObject: ragdoll " << node->name << std::endl;

        // the way the current code is structured, OEngine::Physic::BulletShapeManager
        // expects to see a OEngine::Physic::BulletShape, which provides a challenge to
        // fit in a ragdoll class as a resource.
        //
        // A quick hack might be to subcass (even if a ragdoall is not really 'is a'
        // BulletShape)
        //
        // A better/longer term solution might be to modify BulletShapeManager
        //
        // A method similar to PhysicEngine::createAndAdjustRigidBody will be required
        // for the ragdoll object.
        //

        // NOTE: takeing the ownership of mShape->mCollisionShape
        //
        // if handleBhkShape was successful, a bullet shape would have been created
        //
        // FIXME: check mShape->mCollisionShape is not nullptr (and perhaps mShape->mCollide)
        // FIXME: should check for insert failures
        mShape->mShapes.insert(std::make_pair(rigidBody->recIndex, mShape->mCollisionShape));

        std::pair<std::map<size_t, btRigidBody::btRigidBodyConstructionInfo>::iterator, bool> res
            = mShape->mRigidBodyCI.insert(std::make_pair(rigidBody->recIndex,
                    btRigidBody::btRigidBodyConstructionInfo(rigidBody->mass, 0, mShape->mCollisionShape)));

        // FIXME: experiment
        //if (node->name == "TargetchainLeft01" || node->name == "TargetchainLeft02")
            //res.first->second.m_mass = 1.f;

        // FIXME: update CI here
        // FIXME: what to do if res.second is false, i.e. insert failed?
        // FIXME: translation and rotation may have been updated by handleBhkShape()
        //        TODO: current does doesn't do that!
        // FIXME: m_motionState should be used at a later point when btRigidBody is constructed
        //        which means m_startWorldTransform will be ignored
        res.first->second.m_startWorldTransform
            = btTransform(btQuaternion(mShape->mBoxRotation.x, mShape->mBoxRotation.y, mShape->mBoxRotation.z, mShape->mBoxRotation.w),
                          btVector3(mShape->mBoxTranslation.x, mShape->mBoxTranslation.y, mShape->mBoxTranslation.z));
        res.first->second.m_friction       = rigidBody->friction;
        res.first->second.m_restitution    = rigidBody->restitution;
        res.first->second.m_linearDamping  = rigidBody->dampingLinear;
        res.first->second.m_angularDamping = rigidBody->dampingAngular;

        res.first->second.m_collisionShape->setUserIndex(static_cast<int>(rigidBody->recIndex));

        // from btRigidBodyConstructionInfo:
        //
        //btVector3  m_localInertia // not sure what this does
        //btScalar  m_rollingFriction
        //btScalar  m_linearSleepingThreshold  // seems to be used for deactivation
        //btScalar  m_angularSleepingThreshold // seems to be used for deactivation

        // from bhkRigidbody: not sure how to map these to bullet
        //
        // unsigned char collisionResponse;
        // unsigned short callbackDelay;
        // unsigned char layerCopy;
        // unsigned char collisionFilterCopy;
        // Ogre::Vector4 velocityLinear;
        // Ogre::Vector4 velocityAngular;
        // Ogre::Real inertia[3][4];
        // Ogre::Vector4 center;
        // float gravityFactor1;             // UserVersion >= 12
        // float gravityFactor2;             // UserVersion >= 12
        // float rollingFrictionMultiplier;  // UserVersion >= 12
        // float maxVelocityLinear;
        // float maxVelocityAngular;
        // float penetrationDepth;
        // unsigned char motionSystem;       // http://niftools.sourceforge.net/doc/nif/MotionSystem.html
        // unsigned char deactivatorType;    // http://niftools.sourceforge.net/doc/nif/DeactivatorType.html
        // unsigned char solverDeactivation; // http://niftools.sourceforge.net/doc/nif/SolverDeactivation.html
        // unsigned char motionQuality;      // http://niftools.sourceforge.net/doc/nif/MotionQuality.html

        // FIXME: store temporary mShapeTrans keyed by recIndex for later retreival
        // FIXME: ignored insert failures...
        mShape->mShapeTransMap.insert(std::make_pair(rigidBody->recIndex, mShape->mShapeTrans));

        // cleanup for the next node's call to handleBhkShape()
        // NOTE: ragdoll shapes are deleted in the dtor of the BulletShape object
        mShape->mCollisionShape = nullptr;
        mShape->mCollide = false;

        // check if constraints exist, possibly need to support:
        //
        // bhkBallSocketConstraintChain
        // bhkConstraint
        // bhkLiquidAction
        // bhkOrientHingedBodyAction
        //
        // at least for the known ragdoll objects, the contraints refer only to known entities
        for (unsigned int i = 0; i < rigidBody->constraints.size(); ++i)
        {
#if 0
            for (unsigned int j = 0; j < rigidBody->constraints[i]->entities.size(); ++j)
            {
                std::cout << shapeName << std::endl;

                std::cout << "node " << node->name << ", rigidBody " << rigidBody->recIndex
                    << ", entities " << rigidBody->constraints[i]->entities[j]->recIndex << std::endl;
            }
#endif
            // FIXME: the first entry seems to be always the current RigidBody, so probably
            // don't need to store as a pair
            //
            // At least check if rigidBody->recIndex == rigidBody->constraints[i]->entities[0]->recIndex
            if (rigidBody->constraints[i]->recType == Nif::RC_bhkRagdollConstraint)
            {
#if 0
    if (node->name == "TargetchainRight02" /*|| node->name == "TargetchainRight02"*/)
    //if (node->name == "TargetHeavyTarget")
        continue;
#endif
                mShape->mJoints[rigidBody->recIndex].push_back(
                        std::make_pair(rigidBody->constraints[i]->entities[0]->recIndex,
                                       rigidBody->constraints[i]->entities[1]->recIndex));

                    Nif::RagdollDescriptor ragdollDesc;
                    const Nif::bhkRagdollConstraint *ragdoll
                        = static_cast<const Nif::bhkRagdollConstraint*>(rigidBody->constraints[i].getPtr());

                    // FIXME: try to get the NIF frame of reference so that pivotA and pivotB
                    // can be "fixed" with correct frames of references
                    const Nif::bhkConstraint *constraint = rigidBody->constraints[i].getPtr();
                    // rbA is not needed since this we already have node, translation and rotation
                    //const Nif::bhkRigidBody *rbA
                        //= static_cast<const Nif::bhkRigidBody*>(constraint->entities[0].getPtr()); // 0 HACK
                    // however it doesn't seem possible to get the node info from the rigid body
                    // maybe the shape's mbox translation needs to be used? (but how to get the
                    // second shape from the entity?)
                    const Nif::bhkRigidBody *rbB
                        = static_cast<const Nif::bhkRigidBody*>(constraint->entities[1].getPtr()); // 1 HACK

                Ogre::Matrix4 rbT = Ogre::Matrix4::IDENTITY;
                typedef std::map<size_t, btRigidBody::btRigidBodyConstructionInfo>::iterator ConstructionInfoIter;
                ConstructionInfoIter itCIA(mShape->mRigidBodyCI.find(rigidBody->recIndex));
                if (itCIA == mShape->mRigidBodyCI.end())
                    continue; // FIXME: shouldn't happen, so probably best to throw here
                btQuaternion qa = itCIA->second.m_startWorldTransform.getRotation();

    if (rigidBody->recType == Nif::RC_bhkRigidBodyT)
    {
                rbT.makeTransform(translation*7, Ogre::Vector3(1.f), rotation);
    }

    if (rigidBody->shape->recType == Nif::RC_bhkCapsuleShape)
    {
                    //ragdollDesc.pivotA = mShape->mShapeTrans * ragdoll->ragdoll.pivotA *7; // FIXME
                    Ogre::Vector3 t = mShape->mShapeTrans; // t is the midpoint vector
                    Ogre::Vector4 p = ragdoll->ragdoll.pivotA * 7;
                    ragdollDesc.pivotA = Ogre::Vector4(p.x-t.x, p.y-t.y, p.z-t.z, p.w);
                    ragdollDesc.planeA = ragdoll->ragdoll.planeA * 7;
                    ragdollDesc.twistA = ragdoll->ragdoll.twistA * 7;
    }
    else
    {
#if 0
                    Ogre::Vector4 p = ragdoll->ragdoll.pivotA * 7;
                    Ogre::Vector3 v(p.x, p.y, p.z);
                v = Ogre::Quaternion(qa.w(), qa.x(), qa.y(), qa.z()) * v; // rotate as per m_startWorldTransform
                    ragdollDesc.pivotA = Ogre::Vector4(v.x, v.y, v.z, p.w);
#endif


                    //ragdollDesc.pivotA = rbT.inverse() * ragdoll->ragdoll.pivotA * 7;
                    ragdollDesc.pivotA = ragdoll->ragdoll.pivotA * 7;
                    ragdollDesc.planeA = ragdoll->ragdoll.planeA * 7;
                    ragdollDesc.twistA = ragdoll->ragdoll.twistA * 7;
    }



                Ogre::Matrix4 rbTB = Ogre::Matrix4::IDENTITY;
                typedef std::map<size_t, btRigidBody::btRigidBodyConstructionInfo>::iterator ConstructionInfoIter;
                ConstructionInfoIter itCI(mShape->mRigidBodyCI.find(rbB->recIndex));
                if (itCI == mShape->mRigidBodyCI.end())
                    continue; // FIXME: shouldn't happen, so probably best to throw here
                btQuaternion q = itCI->second.m_startWorldTransform.getRotation();
                //btTransform qt(q, btVector3(0.f, 0.f, 0.f)); // rotate only




    if (rbB->recType == Nif::RC_bhkRigidBodyT)
    {
                Ogre::Vector3 rbBtrans = Ogre::Vector3::ZERO;
                Ogre::Quaternion rbBrot = Ogre::Quaternion::IDENTITY;

                    rbBrot = Ogre::Quaternion(rbB->rotation.w,
                            rbB->rotation.x, rbB->rotation.y, rbB->rotation.z);
                    rbBtrans = Ogre::Vector3(rbB->translation.x,
                            rbB->translation.y, rbB->translation.z);
                rbTB.makeTransform(rbBtrans*7, Ogre::Vector3(1.f), rbBrot);
    }
    if (rbB->shape->recType == Nif::RC_bhkCapsuleShape)
    {
                    Ogre::Vector3 trans = mShape->mShapeTransMap[rbB->recIndex]; // FIXME: just assume one exists!
                    //ragdollDesc.pivotB = trans * ragdoll->ragdoll.pivotB *7; // FIXME
                    Ogre::Vector3 t = trans;
                    Ogre::Vector4 p = ragdoll->ragdoll.pivotB * 7;
                    Ogre::Vector3 v(p.x, p.y, p.z);
                v = Ogre::Quaternion(q.w(), q.x(), q.y(), q.z()) * v; // rotate as per m_startWorldTransform
                    ragdollDesc.pivotB = Ogre::Vector4(v.x-t.x, v.y-t.y, v.z-t.z, p.w);
                    ragdollDesc.planeB = ragdoll->ragdoll.planeB * 7;
                    ragdollDesc.twistB = ragdoll->ragdoll.twistB * 7;
    }
    else
    {
#if 0
                    Ogre::Vector4 p = ragdoll->ragdoll.pivotB * 7;
                    Ogre::Vector3 v(p.x, p.y, p.z);
                v = Ogre::Quaternion(q.w(), q.x(), q.y(), q.z()) * v; // rotate as per m_startWorldTransform
                    ragdollDesc.pivotB = Ogre::Vector4(v.x, v.y, v.z, p.w);

#endif


                    //ragdollDesc.pivotB = rbTB.inverse() * ragdoll->ragdoll.pivotB * 7;
                    ragdollDesc.pivotB = ragdoll->ragdoll.pivotB * 7;
                    ragdollDesc.planeB = ragdoll->ragdoll.planeB * 7;
                    ragdollDesc.twistB = ragdoll->ragdoll.twistB * 7;
    }
                    ragdollDesc.coneMaxAngle = ragdoll->ragdoll.coneMaxAngle;
                    ragdollDesc.planeMinAngle = ragdoll->ragdoll.planeMinAngle;
                    ragdollDesc.planeMaxAngle = ragdoll->ragdoll.planeMaxAngle;
                    ragdollDesc.twistMinAngle = ragdoll->ragdoll.twistMinAngle;
                    ragdollDesc.twistMaxAngle = ragdoll->ragdoll.twistMaxAngle;
                    ragdollDesc.maxFriction = ragdoll->ragdoll.maxFriction;

                    mShape->mNifRagdollDesc[std::make_pair(rigidBody->recIndex, rbB->recIndex)] = ragdollDesc; // cast away compiler warning
                }
                else
                    continue; // FIXME: support other types
        }
    }

    return;
}

// NOTE: this method gets recursed
//
// At the root node we don't know if any of the children will have collision shapes.
// If there are more than one it is probably an animated shape - check the BSX flags. As far as
// I know only bhkListShape needs to use btCompoundShape.
//
// e.g. bhkListShape: Meshes\Architecture\Cathedral\Crypt\CathedralCryptLight01.NIF
// e.g. ragdoll shape: Meshes\Architecture\Cathedral\Crypt\CathedralCryptLight02.NIF
//
// Note that mShape->mCollide is set to false by loadResource() before calling handleNode()
//
// Strategy: see handleBhkShape()
//
// - check if we have a collision object for this node
//
// FIXME: below comments are no longer correct since we support ragdolls and constraints
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

// for newer Nif's we should know the flags up front without going through all the nodes
// to check for collision which may be indicated in each of NiNode or NiTriStrips
//
// examples where there are no BSX flags:
//
//     meshes\Architecture\ImperialCity\ICPalaceTower01.NIF
//     meshes\Architecture\Castle\CastleBannerPost03.NIF
//     meshes\Architecture\Castle\CastleBannerPost04.NIF
//     meshes\Architecture\Statue\Lilypad01.NIF
//     meshes\Architecture\ImperialCity\ICArenaPoster01.NIF
//     meshes\Clutter\MiddleClass\MiddleClassRugSquare01.NIF
//     meshes\Clutter\FightersGuild\PracticeMat01.NIF
//     meshes\Clutter\ChandelierHangingRod01.NIF
//     meshes\Marker_North.nif
//     meshes\MarkerXHeading.nif
//     meshes\MarkerX.nif
//
// NiExtraData is part of NiObjectNET (which we call 'Extra')
//
void ManualBulletShapeLoader::handleNode(const Nif::Node *node, unsigned int bsxFlags)
{
    // FIXME: this boolean 'hasExtras' may change in the Nif class in future cleanups
    if (bsxFlags == 0 && node && node->hasExtras) // don't check bsxFlags if recursing
    {
        Nif::NiExtraDataPtr extraData;
        for (unsigned int i = 0; i < node->extras.length(); ++i)
        {
            extraData = node->extras[i]; // get the next extra data in the list
            assert(extraData.getPtr() != NULL);

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

        if (node->nifVer >= 0x14020007 /*TES5*/ && bsxFlags == 0) // FIXME: not sure which bits apply here
            return;
        else if ((bsxFlags & 0xf) == 0) // TES4  0x1: havok, 0x2: collision, 0x4: skeleton, 0x8: animated
            return;
    }
    else if (bsxFlags == 0) // no Extras, which implies no BSX flags
    {
        std::cout << "======> No collision: no Exras " << node->name << std::endl;
        return;
    }

    //if (node->name == "CathedralCryptLight02")
        //std::cout << "stop" << std::endl;

    if (!node->parent && isRagdoll(node, bsxFlags)) // check only at the root node
        mShape->mIsRagdoll = true;

    // NOTE: nifVer > 4.2.2.0 do not have hasBounds indicator but may have NiExtraData BBX
    // (e.g. skeleton.nif) // FIXME

    if (!node->collision.empty()) // collision is part of an NiAVObject (a.k.a. 'Node')
    {
        // node->collision is a NiCollisionObject and may be NiCollisionData or bhkNiCollisionObject
        // (or its child, bhkCollisionObject)
        //
        // bhkCollisionObject refers to a body (NiObject) usually bhkRigidBodyT (or bhkRigidBody)
        // which then refers to a bhkShape.
        const Nif::NiCollisionObjectPtr collObj = node->collision;
        if (collObj->recType == Nif::RC_bhkCollisionObject)
        {
            handleBhkCollisionObject(node, bsxFlags, collObj.getPtr());
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

    // loop through the children, only NiNode has NiAVObject as children
    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList& children = ninode->children;
        for(size_t i = 0; i < children.length(); i++)
        {
            if(!children[i].empty())
                handleNode(children[i].getPtr(), bsxFlags);
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

        childMesh->preallocateVertices((int)data->vertices.size());
        childMesh->preallocateIndices((int)data->triangles.size());

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
