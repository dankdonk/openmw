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

// from nifskope
#if 0
Ogre::Matrix4 fromQuat( const Ogre::Vector4& q )
{
	float fTx  = ( (float)2.0 ) * q.y;
	float fTy  = ( (float)2.0 ) * q.z;
	float fTz  = ( (float)2.0 ) * q.w;
	float fTwx = fTx * q.x;
	float fTwy = fTy * q.x;
	float fTwz = fTz * q.x;
	float fTxx = fTx * q.y;
	float fTxy = fTy * q.y;
	float fTxz = fTz * q.y;
	float fTyy = fTy * q.z;
	float fTyz = fTz * q.z;
	float fTzz = fTz * q.w;

    return Ogre::Matrix4((float)1.0 - (fTyy + fTzz), // m[0][0]
                         fTxy - fTwz,                // m[0][1]
                         fTxz + fTwy,                // m[0][2]
                         fTxy + fTwz,                // m[1][0]
                         (float)1.0 - (fTxx + fTzz), // m[1][1]
                         fTyz - fTwx,                // m[1][2]
                         fTxz - fTwy,                // m[2][0]
                         fTyz + fTwx,                // m[2][1]
                         (float)1.0 - (fTxx + fTyy)) // m[2][2]
}
#endif
//! Find the dot product of two vectors
//! Find the dot product of two vectors
static float dotproduct( const Ogre::Vector3 & v1, const Ogre::Vector3 & v2 )
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}
//! Find the cross product of two vectors
static Ogre::Vector3 crossproduct( const Ogre::Vector3 & a, const Ogre::Vector3 & b )
{
	return Ogre::Vector3( a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] );
}

//! Generate triangles for convex hull
static std::vector<Ogre::Vector3> generateTris( const std::vector<Ogre::Vector4>& vertices, float scale )
{
	if ( vertices.empty() )
		return std::vector<Ogre::Vector3>();

	Ogre::Vector3 A, B, C, N, V;
	float D;
	int L, prev, eps;
	bool good;

    L = (int)vertices.size();
    std::vector<Ogre::Vector3> P( L );
    std::vector<Ogre::Vector3> tris;

	// Convert Vector4 to Vector3
	for ( int v = 0; v < L; v++ ) {
		P[v] = Ogre::Vector3( vertices[v].x, vertices[v].y, vertices[v].z );
	}

    // 0 1 2 3 4 5 6 7 for L = 8
    // i i i i i i
    //   j j j j j j
    //     k k k k k k
	for ( int i = 0; i < L - 2; i++ ) {
		A = P[i];

		for ( int j = i + 1; j < L - 1; j++ ) {
			B = P[j];

			for ( int k = j + 1; k < L; k++ ) {
				C = P[k];

				prev = 0;
				good = true;

                //      N
                //      |
                //      |
                //      |
                //     A+-------C         A(-10, -4, 0)  C(-10, 4, 0)
                //     /
                //    /
                //   B               B(-10, -4, 1.5)
				N = crossproduct( (B - A), (C - A) );

				for ( int p = 0; p < L; p++ ) {
					V = P[p];

					if ( (V == A) || (V == B) || (V == C) ) continue;

					D = dotproduct( (V - A), N );

					if ( D == 0 ) continue;

					eps = (D > 0) ? 1 : -1;

					if ( eps + prev == 0 ) {
						good = false;
						continue;
					}

					prev = eps;
				}

				if ( good ) {
					// Append ABC
                    tris.push_back(A*scale);
                    tris.push_back(B*scale);
                    tris.push_back(C*scale);
					//tris << (A*scale) << (B*scale) << (C*scale);
                    //std::cout << "i " << i << ", j " << j << ", k " << k << std::endl;
				}
			}
		}
	}

	return tris;
}

// NOTE: calls new, delete is done elsewhere
btTriangleMesh *createBhkPackedNiTriStripsShape2(const Nif::Node *node,
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

    for(size_t i = 0; i < triData->triangles.size(); ++i)
    {
        Ogre::Vector3 b1 = t*triData->vertices[triData->triangles[i].triangle[0]]*7; // FIXME factor 7
        Ogre::Vector3 b2 = t*triData->vertices[triData->triangles[i].triangle[1]]*7; // FIXME factor 7
        Ogre::Vector3 b3 = t*triData->vertices[triData->triangles[i].triangle[2]]*7; // FIXME factor 7
        staticMesh->addTriangle((btVector3(b1.x,b1.y,b1.z)),
                                (btVector3(b2.x,b2.y,b2.z)),
                                (btVector3(b3.x,b3.y,b3.z)));
    }

    translation = Ogre::Vector3::ZERO; // transform was applied here, do not apply again later
    rotation = Ogre::Quaternion::IDENTITY;

    return staticMesh;
}

// NOTE: calls new, delete is done elsewhere
// e.g. node->name == "CollisionICDoor04"
btConvexHullShape *createBhkConvexVerticesShape2(const Nif::Node *node,
        Ogre::Vector3& translation, Ogre::Quaternion& rotation, const Nif::bhkShape *bhkShape)
{
    const Nif::bhkConvexVerticesShape *shape = static_cast<const Nif::bhkConvexVerticesShape*>(bhkShape);

    btConvexHullShape *convexHull = new btConvexHullShape();

    Ogre::Matrix4 t;
    t.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale

    for (unsigned int i = 0; i < shape->vertices.size(); ++i)
    {
        btVector3 v = shape->vertices[i]*7;
        Ogre::Vector3 point = t * Ogre::Vector3(v.getX(), v.getY(), v.getZ());
        convexHull->addPoint(btVector3(point.x, point.y, point.z));
    }

    translation = Ogre::Vector3::ZERO; // transform was applied here, do not apply again later
    rotation = Ogre::Quaternion::IDENTITY;

    return convexHull;
}

// NOTE: calls new, delete is done elsewhere
btTriangleMesh *createBhkConvexVerticesShape3(const Nif::Node *node,
        Ogre::Vector3& translation, Ogre::Quaternion& rotation, const Nif::bhkShape *bhkShape)
{
    const Nif::bhkConvexVerticesShape *shape = static_cast<const Nif::bhkConvexVerticesShape*>(bhkShape);

    btTriangleMesh *staticMesh = new btTriangleMesh();

    Ogre::Matrix4 t = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
    // note the difference (translation factor of 7)
    t.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale

    std::vector<Ogre::Vector3> triangles = generateTris(shape->vertices_old, 7.0f); // 7*10.f for Skyrim?

    // FIXME: create triangles using btVector3 instead?
    for(size_t i = 0; i < triangles.size(); i += 3)
    {
        Ogre::Vector3 b1 = t*triangles[i+0];
        Ogre::Vector3 b2 = t*triangles[i+1];
        Ogre::Vector3 b3 = t*triangles[i+2];
        staticMesh->addTriangle(btVector3(b1.x,b1.y,b1.z), btVector3(b2.x,b2.y,b2.z), btVector3(b3.x,b3.y,b3.z));
    }

    translation = Ogre::Vector3::ZERO; // transform was applied here, do not apply again later
    rotation = Ogre::Quaternion::IDENTITY;

    return staticMesh;
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
            return new btBoxShape(boxShape->dimensions*7); // FIXME
        }
        case Nif::RC_bhkSphereShape:
        {
            const Nif::bhkSphereShape* shape = static_cast<const Nif::bhkSphereShape*>(bhkShape);
            float radius = shape->radius*7; // FIXME
            return new btSphereShape(radius);
        }
        case Nif::RC_bhkMultiSphereShape: // e.g. "meshes\\Clutter\\MagesGuild\\ApparatusAlembicNovice.NIF"
        {
            const Nif::bhkMultiSphereShape *multiSphereShape
                = static_cast<const Nif::bhkMultiSphereShape*>(bhkShape);

            unsigned int numSpheres = multiSphereShape->spheres.size();
            btVector3 *centers = new btVector3[numSpheres];
            btScalar *radii = new btScalar[numSpheres];

            for (unsigned int i = 0; i < numSpheres; ++i)
            {
                Ogre::Vector3 c = multiSphereShape->spheres[i].center*7;
                centers[i] = btVector3(c.x, c.y, c.z);
                radii[i] = multiSphereShape->spheres[i].radius*7;
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
                createBhkPackedNiTriStripsShape2(node, translation, rotation, shape), true);
        }
        case Nif::RC_bhkConvexVerticesShape: // e.g. ICSignCopious01.NIF
        {
            const Nif::bhkConvexVerticesShape *shape
                = static_cast<const Nif::bhkConvexVerticesShape*>(bhkShape);

            return createBhkConvexVerticesShape2(node, translation, rotation, shape);
            //return new NifBullet::TriangleMeshShape(
                //createBhkConvexVerticesShape3(node, translation, rotation, shape), true);
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
                btTransform transform(btQuaternion(q.x, q.y, q.z, q.w), btVector3(v.x, v.y, v.z));
                compoundShape->addChildShape(transform, collisionShape);
            }

            translation = Ogre::Vector3::ZERO; // transform was applied here, do not apply again later
            rotation = Ogre::Quaternion::IDENTITY;

            return compoundShape;
        }
        case Nif::RC_bhkConvexTransformShape:
        {
            const Nif::bhkConvexTransformShape *shape
                = static_cast<const Nif::bhkConvexTransformShape*>(bhkShape);

            Ogre::Matrix4 localTrans(shape->transform[0][0], shape->transform[1][0],
                                     shape->transform[2][0], shape->transform[3][0],
                                     shape->transform[0][1], shape->transform[1][1],
                                     shape->transform[2][1], shape->transform[3][1],
                                     shape->transform[0][2], shape->transform[1][2],
                                     shape->transform[2][2], shape->transform[3][2],
                                     shape->transform[0][3], shape->transform[1][3],
                                     shape->transform[2][3], shape->transform[3][3]);

            Ogre::Quaternion q = localTrans.extractQuaternion();
            Ogre::Vector3 v = localTrans.getTrans();

            Ogre::Matrix4 l;
            l.makeTransform(v*7, Ogre::Vector3(1.f), q); // assume uniform scale

            Ogre::Matrix4 t;
            t.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale
            t = t * l;
            translation = t.getTrans(); // update the caller's transform
            rotation = t.extractQuaternion();

            return createBhkShape(node, translation, rotation, shape->shape.getPtr());
        }
        case Nif::RC_bhkBoxShape: // e.g. Clutter\\Books\\WantedPoster02Static.NIF
        {
            const Nif::bhkConvexVerticesShape *shape
                = static_cast<const Nif::bhkConvexVerticesShape*>(bhkShape);

            if (node->name == "MiddleCrate06" || node->name == "MiddleCrate01" ||
                node->name == "MiddleCrate02" || node->name == "MiddleCrate04")
            {
                //std::cout << "box " << node->name << std::endl;
                //std::cout << "x " << translation.x << ", y " << translation.y << ", z " << translation.z << std::endl;
            }

            return createBtPrimitive(node, shape);
#if 0
            btCompoundShape *compoundShape = new btCompoundShape();

            btTransform transform(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w),
                    btVector3(translation.x, translation.y, translation.z));
            compoundShape->addChildShape(transform,
                            createBtPrimitive(node, shape));
            translation = Ogre::Vector3::ZERO; // transform was applied here, do not apply again later
            rotation = Ogre::Quaternion::IDENTITY;
            return compoundShape;
#endif
        }
        case Nif::RC_bhkSphereShape:
#if 0
        {
            const Nif::bhkSphereShape* shape
                = static_cast<const Nif::bhkSphereShape*>(bhkShape);

            return createBtPrimitive(node, shape);
        }
#endif
        case Nif::RC_bhkMultiSphereShape: // e.g. "meshes\\Clutter\\MagesGuild\\ApparatusAlembicNovice.NIF"
        {
#if 0
            const Nif::bhkMultiSphereShape *multiSphereShape
                = static_cast<const Nif::bhkMultiSphereShape*>(bhkShape);

            return createBtPrimitive(node, shape);
#endif
            return createBtPrimitive(node, bhkShape);
        }
        case Nif::RC_bhkCapsuleShape: // e.g. "meshes\\Clutter\\MagesGuild\\ApparatusAlembicNovice.NIF"
        {
            const Nif::bhkCapsuleShape *shape
                = static_cast<const Nif::bhkCapsuleShape*>(bhkShape);

            // The btCapsuleShape represents a capsule around the Y axis so it needs to be
            // rotated.  Make use of Ogre's convenient functions to do this.
            Ogre::Vector3 firstPoint = shape->firstPoint*7; // FIXME scale
            Ogre::Vector3 secondPoint = shape->secondPoint*7; // FIXME scale
            Ogre::Vector3 axis = secondPoint - firstPoint;

            float height = firstPoint.distance(secondPoint);
            float radius = shape->radius1; // FIXME: what is radius2 ?

            Ogre::Quaternion q = axis.getRotationTo(Ogre::Vector3::UNIT_Y);
            Ogre::Vector3 midPoint = firstPoint.midPoint(secondPoint);

            Ogre::Matrix4 t;
            t.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale

            Ogre::Matrix4 capsuleTrans;
            capsuleTrans.makeTransform(midPoint, Ogre::Vector3(1.f), q); // assume uniform scale
            t = t * capsuleTrans;

            // update the caller's transform
            translation = t.getTrans();
            rotation = t.extractQuaternion();

            return new btCapsuleShape(radius*7, height);
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
// This method is a callback function, etc, etc
//
// FIXME
// =====
//
// Not sure for what purpose the raycasting shape is used.
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
        //handleNode2(node, flags, false/*isCollsionNode*/, false/*raycasting*/, isAnimated);
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

    //if (r->nifVer >= 0x0a000100) // TES4 style, i.e. from 10.0.1.0
        //return; // FIXME: skip second pass for now

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

        return;
        //handleNiNode(node);
        //handleNode2(node, flags, true/*isCollsionNode*/, true/*raycasting*/, isAnimated);
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
// FIXME: should log errors if mStaticMesh or mShape->mCollisionShape already exists
void ManualBulletShapeLoader::handleBhkShape(const Nif::Node *node,
        const Ogre::Vector3& trans, const Ogre::Quaternion& rot, const Nif::bhkShape *bhkShape)
{
    if (!bhkShape) // node is already checked upstream
        return;

    if (mStaticMesh)
        delete mStaticMesh;
    mStaticMesh = nullptr;

    switch (bhkShape->recType)
    {
        case Nif::RC_bhkMoppBvTreeShape:
        case Nif::RC_bhkPackedNiTriStripsShape:
        case Nif::RC_bhkConvexVerticesShape:
        {
            if (!mShape->mCollisionShape)
            {
                Ogre::Vector3 translation(trans);
                Ogre::Quaternion rotation(rot);
                mShape->mCollisionShape = createBhkShape(node, translation, rotation, bhkShape);

                mShape->mRaycastingShape = mShape->mCollisionShape;
            }

            break;
        }
        case Nif::RC_bhkBoxShape:
        {
            if (mShape->mCollisionShape)
                delete mShape->mCollisionShape;
            {
                Ogre::Vector3 translation(trans);
                Ogre::Quaternion rotation(rot);
                mShape->mCollisionShape = createBhkShape(node, translation, rotation, bhkShape);

                mShape->mRaycastingShape = mShape->mCollisionShape;

                mShape->mBoxTranslation = translation*7;
                mShape->mBoxRotation = rotation;
            }
            break;
        }
        case Nif::RC_bhkListShape:
        {
            if (!mShape->mCollisionShape)
            {
                Ogre::Vector3 translation(trans);
                Ogre::Quaternion rotation(rot);
                mShape->mCollisionShape = createBhkShape(node, translation, rotation, bhkShape);
            }

            mShape->mRaycastingShape = mShape->mCollisionShape;
            // FIXME: need to add
            //mCompoundShape = createBhkShape(node, trans, rot, bhkShape);

            break;
        }
        default:
            std::cerr << "bhkCollisionObject: unsupported shape " << node->name << std::endl;
    }
}

// NOTE: assumed that this won't be recused i.e. rigidBodyTransform  occurs only once in a branch
//
// Ideally, the object being created (at the end of the branch) will have all the transforms
// applied.  However, some of the bullet primitives does not allow transforms to be applied (at
// least I don't know how).  For example compound shapes have the transforms applied when
// calling addChildShape().
//
// Current solution is a mixture:
//
// Transforms are applied for btConvexHullShape, btTriangleMesh (used for creating
// btBvhTriangleMeshShape).
//
// Trasforms are not applied for btBoxShape, btSphereShape and btCapsuleShape.
//
void ManualBulletShapeLoader::handleBhkCollisionObject(const Nif::Node *node,
        const Nif::NiCollisionObject *collObj)
{
    if (!node || !collObj)
        return;

    const Nif::bhkCollisionObject *bhkCollObj = static_cast<const Nif::bhkCollisionObject*>(collObj);
    const Nif::bhkRigidBody *rigidBody = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());

    // FIXME: it might be that the 'T' type has translation while the other only has rotation
    //        despite what the documentations say (e.g. "Cylinder02" repair hammer)
    if(bhkCollObj->body.getPtr()->recType == Nif::RC_bhkRigidBodyT)
    {
        Ogre::Vector3 translation = Ogre::Vector3(rigidBody->translation.x,
                rigidBody->translation.y, rigidBody->translation.z);
        Ogre::Quaternion rotation = Ogre::Quaternion(rigidBody->rotation.w,
                rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);

        handleBhkShape(node, translation, rotation, rigidBody->shape.getPtr());
    }
    else
        // FIXME
        //handleBhkShape(node, Ogre::Vector3::ZERO, Ogre::Quaternion::IDENTITY, rigidBody->shape.getPtr());
    {
        Ogre::Quaternion rotation = Ogre::Quaternion(rigidBody->rotation.w,
                rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);
        handleBhkShape(node, Ogre::Vector3::ZERO, rotation, rigidBody->shape.getPtr());
    }

    return;
}

// note: this method gets recursed
void ManualBulletShapeLoader::handleNiNode(const Nif::Node *node)
{
    // Check for extra data
    Nif::Extra const *e = node;
    if (e->hasExtras) // FIXME: this boolean may change in the Nif class
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

    // Note: nifVer > 4.2.2.0 do not have hasBounds indicator
    // but may have NiExtraData BBX (e.g. skeleton.nif) // FIXME
    if (!node->collision.empty())
    {
        mShape->mCollide = true;

        // node->collision is a NiCollisionObject and may be NiCollisionData or bhkNiCollisionObject
        // (or its child, bhkCollisionObject)
        //
        // bhkCollisionObject refers to a body (NiObject) usually bhkRigidBodyT (or bhkRigidBody)
        // which then refers to a bhkShape.
        //
        // If the bhkShape is bhkMoppBvTreeShape then it refers to yet another bkhShape.
        const Nif::NiCollisionObjectPtr collObj = node->collision;
        if (collObj->recType == Nif::RC_bhkCollisionObject)
        {
            handleBhkCollisionObject(node, collObj.getPtr());
        }
        else if (collObj->recType == Nif::RC_NiCollisionData)
        {
            // FIXME TODO // FIXME: examples?
            std::cout << "coll obj rectype " << collObj->recType << std::endl;
        }
        else
            std::cout << "coll obj rectype " << collObj->recType << std::endl;
    }

    // For NiNodes, loop through children
    // FIXME: is there a case where a node has collision *and* one of its children also has its
    // own collision defined?
    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(!list[i].empty())
                handleNiNode(list[i].getPtr());
        }
    }
}

// note: this method gets recursed
void ManualBulletShapeLoader::handleNode2(const Nif::Node *node, int flags,
                                         bool isCollisionNode,
                                         bool raycasting, bool isAnimated)
{
    // Check for extra data
    Nif::Extra const *e = node;
    if (e->hasExtras)
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

    // Note: nifVer > 4.2.2.0 do not have hasBounds indicator
    // but may have NiExtraData BBX (e.g. skeleton.nif) // FIXME
    if (!node->collision.empty())
    {
                if (node->name.find("Painting") != std::string::npos)
                    std::cout << "lower" << std::endl;

        mShape->mCollide = true;

        // node->collision is a NiCollisionObject and may be NiCollisionData or bhkNiCollisionObject
        // (or its child, bhkCollisionObject)
        //
        // bhkCollisionObject refers to a body (NiObject) usually bhkRigidBodyT (or bhkRigidBody)
        // which then refers to a bhkShape.
        //
        // If the bhkShape is bhkMoppBvTreeShape then it refers to yet another bkhShape.
        const Nif::NiCollisionObjectPtr collObj = node->collision;
        if (collObj->recType == Nif::RC_bhkCollisionObject)
        {
            const Nif::bhkCollisionObject *bhkCollObj = static_cast<const Nif::bhkCollisionObject*>(collObj.getPtr());
            const Nif::bhkRigidBody *rigidBody = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());
            const Nif::bhkShape *shape = rigidBody->shape.getPtr();

            // prepare local transformation in case we have bhkRigidBodyT type
            Ogre::Vector3 translation = Ogre::Vector3();
            Ogre::Quaternion rotation = Ogre::Quaternion();
            if(bhkCollObj->body.getPtr()->recType == Nif::RC_bhkRigidBodyT)
            {
                translation = Ogre::Vector3(rigidBody->translation.x,
                                            rigidBody->translation.y,
                                            rigidBody->translation.z);
                rotation = Ogre::Quaternion(rigidBody->rotation.w,
                                            rigidBody->rotation.x,
                                            rigidBody->rotation.y,
                                            rigidBody->rotation.z);
            }

            // NOTE: ICColArc01.NIF doesn't have any collision defined for the roof parts
            if(shape->recType == Nif::RC_bhkMoppBvTreeShape) // e.g. ICColArc01.NIF
            {
                // FIXME: TODO get some info


                shape = static_cast<const Nif::bhkMoppBvTreeShape*>(shape)->shape.getPtr(); // move to the next one
                if (shape->recType != Nif::RC_bhkPackedNiTriStripsShape)
                {
                    std::cout << "unexpected shape->shape rectype " << shape->recType << std::endl;
                    return;
                }

                const Nif::bhkPackedNiTriStripsShape* bhkShape
                    = static_cast<const Nif::bhkPackedNiTriStripsShape*>(shape);

                createBhkPackedNiTriStripsShape(bhkCollObj, bhkShape);

                return;
            }
            else if(shape->recType == Nif::RC_bhkConvexVerticesShape) // e.g. ICSignCopious01.NIF
            {
                const Nif::bhkConvexVerticesShape *bhkShape
                    = static_cast<const Nif::bhkConvexVerticesShape*>(shape);

                if (!mStaticMesh)
                    mStaticMesh = createBhkConvexVerticesShape(bhkCollObj, bhkShape,
                    ((node->name == "Cylinder02") ? true : false)); // repair hammer special case

                return;
            }
            else if(shape->recType == Nif::RC_bhkBoxShape) // e.g. Clutter\\Books\\WantedPoster02Static.NIF
            {
                const Nif::bhkBoxShape* boxShape = static_cast<const Nif::bhkBoxShape*>(shape);

                mBoundingBox = createBhkBoxShape(bhkCollObj, boxShape,
                                                 mShape->mBoxTranslation, mShape->mBoxRotation);
                mShape->mCollisionShape = mBoundingBox;

                return;
            }
            else if(shape->recType == Nif::RC_bhkListShape)
            {
                std::cout << "List Shape: " << node->name << std::endl;
                //
                // List Shape: ApparatusAlembicNovice
                // List Shape: MiddleBowlRed03
                // List Shape: upperTable06
                // List Shape: ClothBolt01
                // List Shape: ClothBolt02
                // List Shape: ClothBolt03
                // List Shape: FoldedCloth05
                // List Shape: MiddlebarrelHolder01
                // List Shape: MiddleShelf02
                if (!mCompoundShape)
                    mCompoundShape = new btCompoundShape();

                if (!mShape->mCollisionShape)
                    mShape->mCollisionShape = mCompoundShape;

                const Nif::bhkListShape *listShape = static_cast<const Nif::bhkListShape*>(shape);

                for (unsigned int i = 0; i < listShape->numSubShapes; ++i)
                {
                    const Nif::bhkShape *subShape = listShape->subShapes[i].getPtr();
                    if (subShape->recType == Nif::RC_bhkConvexTransformShape)
                    {
                        const Nif::bhkConvexTransformShape *transShape
                            = static_cast<const Nif::bhkConvexTransformShape*>(subShape);

                        // get the transform
                        Ogre::Matrix4 transform(transShape->transform[0][0], transShape->transform[1][0],
                                                transShape->transform[2][0], transShape->transform[3][0],
                                                transShape->transform[0][1], transShape->transform[1][1],
                                                transShape->transform[2][1], transShape->transform[3][1],
                                                transShape->transform[0][2], transShape->transform[1][2],
                                                transShape->transform[2][2], transShape->transform[3][2],
                                                transShape->transform[0][3], transShape->transform[1][3],
                                                transShape->transform[2][3], transShape->transform[3][3]);
                        // create the shape
                        if (transShape->shape->recType == Nif::RC_bhkBoxShape)
                        {
                            const Nif::bhkBoxShape* boxShape
                                = static_cast<const Nif::bhkBoxShape*>(transShape->shape.getPtr());

                            Ogre::Quaternion q;
                            Ogre::Vector3 v;
                            Ogre::Vector3 s;
                            transform.decomposition(v, s, q);
                            v *= 7.f; // FIXME
//#if 0
                            // FIXME: duplicated code
                            Ogre::Matrix4 bodyTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
                            if(bhkCollObj->body.getPtr()->recType == Nif::RC_bhkRigidBodyT)
                            {
                                const Nif::bhkRigidBody *rigidBody
                                    = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());

                                Ogre::Vector3 translation = Ogre::Vector3();
                                Ogre::Quaternion rotation = Ogre::Quaternion();
                                translation = Ogre::Vector3(rigidBody->translation.x,
                                        rigidBody->translation.y, rigidBody->translation.z);
                                rotation = Ogre::Quaternion(rigidBody->rotation.w,
                                        rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);

                                Ogre::Matrix4 localTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
                                // FIXME: not sure if this requires the scale factor of 7
                                localTrans.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale
                                //transform = /*node->getWorldTransform() **/ localTrans;
                                //transform = /*node->getWorldTransform() **/ localTrans * transform;
                                bodyTrans = localTrans;
                            }

                            Ogre::Quaternion lq = bodyTrans.extractQuaternion();
                            Ogre::Vector3 lv = bodyTrans.getTrans();
//#endif

                            btTransform ltrans(btQuaternion(lq.x, lq.y, lq.z, lq.w), btVector3(lv.x, lv.y, lv.z));
                            btTransform trans(btQuaternion(q.x, q.y, q.z, q.w), btVector3(v.x, v.y, v.z));

                            mCompoundShape->addChildShape(ltrans*trans,
                                new btBoxShape(getbtVector(boxShape->dimensions_old*7))); // FIXME
                        }
                        else if (transShape->shape->recType == Nif::RC_bhkSphereShape)
                        {
                            // FIXME: not working for LowerJugTan01
                            const Nif::bhkSphereShape* sphereShape
                                = static_cast<const Nif::bhkSphereShape*>(subShape);
//#if 0
                            // FIXME: duplicated code
                            Ogre::Matrix4 localTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
                            if(bhkCollObj->body.getPtr()->recType == Nif::RC_bhkRigidBodyT)
                            {
                                const Nif::bhkRigidBody *rigidBody
                                    = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());

                                Ogre::Vector3 translation = Ogre::Vector3();
                                Ogre::Quaternion rotation = Ogre::Quaternion();
                                translation = Ogre::Vector3(rigidBody->translation.x,
                                        rigidBody->translation.y, rigidBody->translation.z);
                                rotation = Ogre::Quaternion(rigidBody->rotation.w,
                                        rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);

                                //Ogre::Matrix4 localTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
                                // FIXME: not sure if this requires the scale factor of 7
                                localTrans.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale
                                //transform = /*node->getWorldTransform() **/ localTrans;
                                transform = localTrans * transform;
                            }

//#endif
                            Ogre::Quaternion q;
                            Ogre::Vector3 v;
                            Ogre::Vector3 s;
                            transform.decomposition(v, s, q);
                            v *= 7.f; // FIXME
                            btTransform trans(btQuaternion(q.x, q.y, q.z, q.w), btVector3(v.x, v.y, v.z));

                            float radius = sphereShape->radius*7;

                            mCompoundShape->addChildShape(trans, new btSphereShape(radius));
                        }
                        else
                            std::cout << "unsupported transform shape " << transShape->recType << std::endl;
                    }
                    else if (subShape->recType == Nif::RC_bhkConvexVerticesShape)
                    {
                        const Nif::bhkConvexVerticesShape *bhkShape
                            = static_cast<const Nif::bhkConvexVerticesShape*>(subShape);

                        btTransform trans;
                        trans.setIdentity();
                        mCompoundShape->addChildShape(trans,
                                new TriangleMeshShape(createBhkConvexVerticesShape(bhkCollObj, bhkShape), true));
                    }
                    else if (subShape->recType == Nif::RC_bhkCapsuleShape)
                    {
                        // e.g. "meshes\\Clutter\\MagesGuild\\ApparatusAlembicNovice.NIF"
                        const Nif::bhkCapsuleShape *capsuleShape
                            = static_cast<const Nif::bhkCapsuleShape*>(subShape);

                        Ogre::Matrix4 bodyTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
                        if(bhkCollObj->body.getPtr()->recType == Nif::RC_bhkRigidBodyT)
                        {
                            const Nif::bhkRigidBody *rigidBody
                                = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());

                            Ogre::Vector3 translation = Ogre::Vector3();
                            Ogre::Quaternion rotation = Ogre::Quaternion();
                            translation = Ogre::Vector3(rigidBody->translation.x,
                                    rigidBody->translation.y, rigidBody->translation.z);
                            rotation = Ogre::Quaternion(rigidBody->rotation.w,
                                    rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);

                            Ogre::Matrix4 localTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
                            // FIXME: not sure if this requires the scale factor of 7
                            localTrans.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale
                            bodyTrans = /*node->getWorldTransform() **/ localTrans;
                        }

                        // find the direction and length
                        // FIXME: translation calculation is very messy
                        Ogre::Vector3 firstPoint = capsuleShape->firstPoint*7; // FIXME
                        Ogre::Vector3 secondPoint = capsuleShape->secondPoint*7; // FIXME
                        Ogre::Vector3 axis = secondPoint - firstPoint;
                        float height = firstPoint.distance(secondPoint);
                        float radius = capsuleShape->radius1; // FIXME: what is radius2 ?
                        Ogre::Quaternion q = axis.getRotationTo(Ogre::Vector3::UNIT_Y);
                        Ogre::Vector3 midPoint = firstPoint.midPoint(secondPoint);

                        Ogre::Matrix4 capsuleTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
                        capsuleTrans.makeTransform(midPoint, Ogre::Vector3(1.f), q);
                        bodyTrans = bodyTrans * capsuleTrans;

                        q = bodyTrans.extractQuaternion();
                        midPoint = bodyTrans.getTrans();

                        btTransform trans(btQuaternion(q.x, q.y, q.z, q.w), btVector3(midPoint.x, midPoint.y, midPoint.z));
                        mCompoundShape->addChildShape(trans, new btCapsuleShape(radius*7, height)); // FIXME

                    }
                    else if (subShape->recType == Nif::RC_bhkMultiSphereShape)
                    {
                        // e.g. "meshes\\Clutter\\MagesGuild\\ApparatusAlembicNovice.NIF"
                        const Nif::bhkMultiSphereShape *multiSphereShape
                            = static_cast<const Nif::bhkMultiSphereShape*>(subShape);

                        unsigned int numSpheres = multiSphereShape->spheres.size();
                        btVector3 *centers = new btVector3[numSpheres];
                        btScalar *radii = new btScalar[numSpheres];

                        // FIXME: transform is ignored
                        Ogre::Matrix4 transform = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
                        if(bhkCollObj->body.getPtr()->recType == Nif::RC_bhkRigidBodyT)
                        {
                            const Nif::bhkRigidBody *rigidBody
                                = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());

                            Ogre::Vector3 translation = Ogre::Vector3();
                            Ogre::Quaternion rotation = Ogre::Quaternion();
                            translation = Ogre::Vector3(rigidBody->translation.x,
                                    rigidBody->translation.y, rigidBody->translation.z);
                            rotation = Ogre::Quaternion(rigidBody->rotation.w,
                                    rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);

                            Ogre::Matrix4 localTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
                            // FIXME: not sure if this requires the scale factor of 7
                            localTrans.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale
                            transform = /*node->getWorldTransform() **/ localTrans;
                        }

                        for (unsigned int i = 0; i < numSpheres; ++i)
                        {
                            Ogre::Vector3 c = transform*multiSphereShape->spheres[i].center*7;
                            centers[i] = btVector3(c.x, c.y, c.z);
                            radii[i] = multiSphereShape->spheres[i].radius*7;
                        }

                        btTransform trans;
                        trans.setIdentity();
                        mCompoundShape->addChildShape(trans,
                            new btMultiSphereShape(centers, radii, numSpheres));

                        delete[] radii;
                        radii = nullptr;
                        delete[] centers;
                        centers = nullptr;
                    }
                    else if (subShape->recType == Nif::RC_bhkBoxShape)
                    {
                        const Nif::bhkBoxShape* boxShape = static_cast<const Nif::bhkBoxShape*>(subShape);

                        // FIXME: duplicated code
                        Ogre::Matrix4 bodyTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
                        if(bhkCollObj->body.getPtr()->recType == Nif::RC_bhkRigidBodyT)
                        {
                            const Nif::bhkRigidBody *rigidBody
                                = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());

                            Ogre::Vector3 translation = Ogre::Vector3();
                            Ogre::Quaternion rotation = Ogre::Quaternion();
                            translation = Ogre::Vector3(rigidBody->translation.x,
                                    rigidBody->translation.y, rigidBody->translation.z);
                            rotation = Ogre::Quaternion(rigidBody->rotation.w,
                                    rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);

                            Ogre::Matrix4 localTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
                            // FIXME: not sure if this requires the scale factor of 7
                            localTrans.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale
                            bodyTrans = /*node->getWorldTransform() **/ localTrans;
                        }

                        Ogre::Quaternion q = bodyTrans.extractQuaternion();
                        Ogre::Vector3 v = bodyTrans.getTrans();

                        btTransform trans(btQuaternion(q.x, q.y, q.z, q.w), btVector3(v.x, v.y, v.z));
                        mCompoundShape->addChildShape(trans,
                            new btBoxShape(getbtVector(boxShape->dimensions_old*7))); // FIXME
                    }
                    else
                        std::cout << "unsupported compound shape " << subShape->recType << std::endl;
                } // for loop

                return;
            }
            else
                std::cout << "unsupported shape rectype " << shape->recType << std::endl;
        }
        else if (collObj->recType == Nif::RC_NiCollisionData)
        {
            // FIXME TODO // FIXME: examples?
            std::cout << "coll obj rectype " << collObj->recType << std::endl;
        }
        else
            std::cout << "coll obj rectype " << collObj->recType << std::endl;

    }
    else if (mShape->mAutogenerated)
    {
        mShape->mCollide = !(flags&0x800);
        if(node->recType == Nif::RC_NiTriShape)
        {
            handleNiTriShape(static_cast<const Nif::NiTriShape*>(node),
                    flags, node->getWorldTransform(), raycasting, isAnimated);
        }
        else if (node->recType == Nif::RC_NiTriStrips)
        {
            handleNiTriStrips(static_cast<const Nif::NiTriStrips*>(node),
                    flags, node->getWorldTransform(), raycasting, isAnimated);
        }
        //else
            //std::cout << "rectype " << node->recType << std::endl;
    }

    // For NiNodes, loop through children
    const Nif::NiNode *ninode = dynamic_cast<const Nif::NiNode*>(node);
    if(ninode)
    {
        const Nif::NodeList &list = ninode->children;
        for(size_t i = 0;i < list.length();i++)
        {
            if(!list[i].empty())
                handleNode2(list[i].getPtr(), flags, isCollisionNode, raycasting, isAnimated);
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

void ManualBulletShapeLoader::handleNiTriShape(const Nif::NiTriShape *shape, int flags, const Ogre::Matrix4 &transform,
                                               bool raycasting, bool isAnimated)
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

void ManualBulletShapeLoader::handleNiTriStrips(const Nif::NiTriStrips *shape, int flags, const Ogre::Matrix4 &transform,
                                               bool raycasting, bool isAnimated)
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

Ogre::Matrix4 ManualBulletShapeLoader::getBhkRigidBodyTransform(const Nif::bhkCollisionObject *bhkCollObj)
{
    Ogre::Matrix4 transform = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
    if(bhkCollObj->body.getPtr()->recType != Nif::RC_bhkRigidBodyT)
        return transform;

    const Nif::bhkRigidBody *rigidBody = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());

    Ogre::Vector3 translation = Ogre::Vector3();
    Ogre::Quaternion rotation = Ogre::Quaternion();
    translation = Ogre::Vector3(rigidBody->translation.x, rigidBody->translation.y, rigidBody->translation.z);
    rotation = Ogre::Quaternion(rigidBody->rotation.w,
                                rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);

    Ogre::Matrix4 localTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
    // FIXME: don't know why the translation here is not the same scale as the
    // translation in bhkPackedNiTriStripsShape which does *not* require a factor of 7
    localTrans.makeTransform(translation*7, Ogre::Vector3(1.f), rotation); // assume uniform scale

    return /*node->getWorldTransform() **/ localTrans;
}

// modifies mShape and mBoundingBox
btBoxShape *ManualBulletShapeLoader::createBhkBoxShape(const Nif::bhkCollisionObject *bhkCollObj,
                                                const Nif::bhkBoxShape *bhkShape,
                                                Ogre::Vector3& boxTranslation,
                                                Ogre::Quaternion& boxRotation)
{
    if(bhkCollObj->body.getPtr()->recType == Nif::RC_bhkRigidBodyT)
    {
        const Nif::bhkRigidBody *rigidBody = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());

        Ogre::Vector3 translation = Ogre::Vector3();
        Ogre::Quaternion rotation = Ogre::Quaternion();
        translation = Ogre::Vector3(rigidBody->translation.x, rigidBody->translation.y, rigidBody->translation.z);
        rotation = Ogre::Quaternion(rigidBody->rotation.w,
                                    rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);

        // mBoxTranslation and mBoxRotation are used in PhysicEngine::adjustRigidBody()
        // FIXME: rather than using them consider creating new method to use bhk
        // transform instead (maybe less calculations?)
        boxTranslation = translation*7; // FIXME
        boxRotation = rotation;
    }
    return new btBoxShape(getbtVector(bhkShape->dimensions_old*7));
}

btTriangleMesh *ManualBulletShapeLoader::createBhkConvexVerticesShape(const Nif::bhkCollisionObject *bhkCollObj,
                                                           const Nif::bhkConvexVerticesShape *bhkShape, bool force)
{
    btTriangleMesh *staticMesh = new btTriangleMesh();

    // Havok scale issue, see: http://niftools.sourceforge.net/forum/viewtopic.php?f=24&t=1400
    Ogre::Matrix4 transform = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
    if(bhkCollObj->body.getPtr()->recType == Nif::RC_bhkRigidBodyT ||
            force) // Repair Hammer
    {
        const Nif::bhkRigidBody *rigidBody = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());

        Ogre::Vector3 translation = Ogre::Vector3();
        Ogre::Quaternion rotation = Ogre::Quaternion();
        translation = Ogre::Vector3(rigidBody->translation.x, rigidBody->translation.y, rigidBody->translation.z);
        rotation = Ogre::Quaternion(rigidBody->rotation.w,
                                    rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);

        Ogre::Matrix4 localTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
        // FIXME: don't know why the translation here is not the same scale as the
        // translation in bhkPackedNiTriStripsShape which does *not* require a factor of 7
        localTrans.makeTransform(translation*(force ? 1 : 7), Ogre::Vector3(1.f), rotation); // assume uniform scale
        transform = /*node->getWorldTransform() **/ localTrans;
    }

    std::vector<Ogre::Vector3> triangles = generateTris(bhkShape->vertices_old, 7.0f); // 7*10.f for Skyrim?

    // FIXME: create triangles using btVector3 instead?
    for(size_t i = 0; i < triangles.size(); i += 3)
    {
        Ogre::Vector3 b1 = transform*triangles[i+0];
        Ogre::Vector3 b2 = transform*triangles[i+1];
        Ogre::Vector3 b3 = transform*triangles[i+2];
        staticMesh->addTriangle(btVector3(b1.x,b1.y,b1.z), btVector3(b2.x,b2.y,b2.z), btVector3(b3.x,b3.y,b3.z));
    }

    return staticMesh;
}

// modifies mShape and mStaticMesh
void ManualBulletShapeLoader::createBhkPackedNiTriStripsShape(const Nif::bhkCollisionObject *bhkCollObj,
                                                              const Nif::bhkPackedNiTriStripsShape *bhkShape)
{
    const Nif::hkPackedNiTriStripsData* triData
        = static_cast<const Nif::hkPackedNiTriStripsData*>(bhkShape->data.getPtr());

    if (!mStaticMesh)
        mStaticMesh = new btTriangleMesh();

    // Havok scale issue, see: http://niftools.sourceforge.net/forum/viewtopic.php?f=24&t=1400
    Ogre::Matrix4 transform = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
    if(bhkCollObj->body.getPtr()->recType == Nif::RC_bhkRigidBodyT)
    {
        const Nif::bhkRigidBody *rigidBody = static_cast<const Nif::bhkRigidBody*>(bhkCollObj->body.getPtr());

        Ogre::Vector3 translation = Ogre::Vector3();
        Ogre::Quaternion rotation = Ogre::Quaternion();
        translation = Ogre::Vector3(rigidBody->translation.x, rigidBody->translation.y, rigidBody->translation.z);
        rotation = Ogre::Quaternion(rigidBody->rotation.w,
                                    rigidBody->rotation.x, rigidBody->rotation.y, rigidBody->rotation.z);

        Ogre::Matrix4 localTrans = Ogre::Matrix4(Ogre::Matrix4::IDENTITY);
        // FIXME: don't know why the translation here is not the same scale as the
        // translation in bhkConvexVerticesShape which requires a factor of 7
        localTrans.makeTransform(translation, Ogre::Vector3(1.f), rotation); // assume uniform scale
        transform = /*node->getWorldTransform() **/ localTrans;
    }

    for(size_t i = 0; i < triData->triangles.size(); ++i)
    {
        Ogre::Vector3 b1 = transform*triData->vertices[triData->triangles[i].triangle[0]]*7;
        Ogre::Vector3 b2 = transform*triData->vertices[triData->triangles[i].triangle[1]]*7;
        Ogre::Vector3 b3 = transform*triData->vertices[triData->triangles[i].triangle[2]]*7;
        mStaticMesh->addTriangle(btVector3(b1.x,b1.y,b1.z),
                                 btVector3(b2.x,b2.y,b2.z),
                                 btVector3(b3.x, b3.y, b3.z));
    if (i == 0 && bhkCollObj->target->name == "ICMarketBlock01House01")
    {
        std::cout << "x " << b1.x << ", y " << b1.y << ", z " << b1.z << std::endl;
    }

    }
}

} // namespace NifBullet
