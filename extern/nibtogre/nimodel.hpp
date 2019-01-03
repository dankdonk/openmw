/*
  Copyright (C) 2015-2019 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

*/
#ifndef NIBTOGRE_NIMODEL_H
#define NIBTOGRE_NIMODEL_H

#include <vector>
#include <map>
#include <string>
#include <cstdint>
#include <memory>

#include <btBulletCollisionCommon.h>

#include <OgreResource.h>

#include "nistream.hpp"
#include "niheader.hpp"
#include "meshloader.hpp" // not sure why this is needed
#include "skeletonloader.hpp"


namespace Ogre
{
    class ResourceManager;
    class ManualResourceLoader;
    class SceneNode;
}

namespace NiBtOgre
{
    class NiObject;
//    class SkeletonLoader;
//    class MeshLoader;
    struct NiTriBasedGeom;
    struct BtOgreInst;
    class NiModel;

    struct ModelData
    {
    private:
        const NiModel& mModel;

    public:

        bool mEditorMarkerPresent;

        typedef std::int32_t NiAVObjectRef;
        typedef std::int32_t NiNodeRef;
        typedef std::int32_t bhkEntityRef;

        std::map<NiAVObjectRef, NiNodeRef> mNiNodeMap; // keep track of parent NiNode index
        void setNiNodeParent(NiAVObjectRef child, NiNodeRef parent);

        std::vector<NiNodeRef> mSkelLeafIndicies; // tempoarily used to find the bones
        void addSkelLeafIndex(NiNodeRef leaf) { mSkelLeafIndicies.push_back(leaf); }

        std::unique_ptr<SkeletonLoader> mSkeletonLoader;

        //      index = parent NiNode's block index
        //        |
        //        |                  name  = concatenation of model, "@" and parent NiNode name
        //        |                    |
        //        v                    v
        std::map<NiNodeRef, std::pair<std::string, std::unique_ptr<MeshLoader> > > mMeshLoaders;

        // nodeIndex = parent NiNode's block index
        // name      = concatenation of model, "@", parent NiNode name
        //             (e.g. meshes\\architecture\\imperialcity\\icwalltower01.nif@ICWallTower01)
        void registerNiTriBasedGeom(std::uint32_t nodeIndex, const std::string& name, NiTriBasedGeom* geometry);

        // The btCollisionShape for btRigidBody corresponds to an Ogre::Entity whose Ogre::SceneNode
        // may be controlled for Ragdoll animations.  So we just really need the Model name,
        // NiNode name (and maybe the NiNode block index) to be able to load the required info.
        //
        // Maybe we only need one manual loader (per thread?) as long as we feed it the inputs?
        // (but how to do that?)
        //
        // In any case, once the NiModel is "built", all the required info are ready to be
        // simply collected and used.
        //
        //      index = parent NiNode's block index
        //        |
        //        |                  name  = concatenation of model, "@" and parent NiNode name
        //        |                    |
        //        v                    v
        std::map<NiNodeRef, std::pair<std::string, int32_t> > mBhkRigidBodyMap;

        //std::map<NiNodeRef, int32_t> mBtShapeMap; // bhkRigidBody for the NiNode

        ModelData(const NiModel& model) : mModel(model), mEditorMarkerPresent(false) {}
    };

    //
    //   +--& NiModel
    //   |     o o o
    //   |     | | |
    //   |     | | +--- NiStream
    //   |     | |        o o
    //   |     | |        | |
    //   |     | |        | +-- NIF version numbers (local copy)
    //   |     | |        *
    //   |     | +----- Header
    //   |     |         o o
    //   |     |         | |
    //   |     |         | +--- NIF version numbers
    //   |     |         |
    //   |     |         +----- vector<string>
    //   |     |
    //   |     +------- vector<NiObject*>
    //   |                       o
    //   |                       |
    //   +-----------------------+
    //
    // NOTE: NiStream has a pointer to Header for the appending long strings (TES3/TES4)
    //       NiObject has a reference to NiModel for getting strings and other NiObject Ptrs/Refs
    //
    class NiModel : public Ogre::Resource
    {
        // NOTE the initialisation order
        NiStream mNiStream;
        NiHeader mHeader;

        const std::string mGroup;  // Ogre group

        std::vector<std::unique_ptr<NiObject> > mObjects;
        std::vector<std::uint32_t> mRoots;

        int mCurrIndex; // FIXME: for debugging Ptr
        std::string mModelName;

        ModelData mModelData;

        bool mShowEditorMarkers; // usually only for OpenCS

        // default, copy and assignment not allowed
        NiModel();
        NiModel(const NiModel& other);
        NiModel& operator=(const NiModel& other);

    protected:
        void loadImpl();
        void unloadImpl() {}

    public:
        // The parameter 'name' refers to those in the Ogre::ResourceGroupManager
        // e.g. full path from the directory/BSA added in Bsa::registerResources(), etc
        //
        // NOTE: the constructor may throw
#if 0
        NiModel(const std::string& name, const std::string& group, bool showEditorMarker=false);
#else
        NiModel(Ogre::ResourceManager *creator, const Ogre::String& name, Ogre::ResourceHandle handle,
                const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
                bool showEditorMarker=false);
#endif
        ~NiModel();

        const std::string& getOgreGroup() const { return mGroup; }
        const std::string& getModelName() const { return mModelName; }

        template<class T>
        inline T *getRef(std::int32_t index) const {

            if (index >= 0 && (index > mCurrIndex)) // FIXME: for debugging Ptr
                throw std::runtime_error("Ptr");

            return (index < 0) ? nullptr : static_cast<T*>(mObjects[index].get());
        }

        // WARNING: SceneNode in inst should have the scale (assumed uniform)
        void build(BtOgreInst *inst);

        // returns NiObject type name
        const std::string& blockType(std::uint32_t index) const {
            return mHeader.blockType(index);
        }

        // needed for the NIF version and strings (TES5)
        inline std::uint32_t nifVer() const { return mHeader.nifVer(); }

        inline const std::string& indexToString(std::int32_t index) const {
            return mHeader.indexToString(index);
        }

        inline bool showEditorMarkers() const { return mShowEditorMarkers; }

        std::int32_t getNiNodeParent(std::int32_t child) const;
        std::uint32_t getRootIndex() const { return mRoots[0]; } // assumes only one root

        typedef std::int32_t NiNodeRef;
        const std::map<NiNodeRef, std::pair<std::string, int32_t> >&
        getBhkRigidBodyMap() const { return mModelData.mBhkRigidBodyMap; }

        //const std::map<NiNodeRef, int32_t>& getBhkRigidBodyMap() const { return mModelData.mBhkRigidBocyMap; }

        void buildMeshAndEntity(BtOgreInst *inst);
    };
}

#endif // NIBTOGRE_NIMODEL_H
