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
#include "skeletonloader.hpp" // not sure why this is needed
#include "niobject.hpp"
#include "nimodelmanager.hpp"

namespace Ogre
{
    class ResourceManager;
    class ManualResourceLoader;
    class SceneNode;
    class Skeleton;
    class SkeletonInstance;
}

namespace NiBtOgre
{
    class NiObject;
//    class SkeletonLoader;
    struct NiTriBasedGeom;
    struct BtOgreInst;
    class NiModel;
    class NiNode;
    class MeshLoader;

    enum BuildFlags {
        Flag_EnableHavok         = 0x0001,
        Flag_EnableCollision     = 0x0002, // TES4 only?
        Flag_IsSkeleton          = 0x0004, // TES4 only?
        Flag_EnableAnimation     = 0x0008,
        Flag_FlameNodesPresent   = 0x0010, // TES4 only?
        Flag_EditorMarkerPresent = 0x0020,
        Flag_IgnoreEditorMarker  = 0x0040,
        Flag_HasSkin             = 0x0080,
        Flag_None                = 0xFFFF
    };

    struct BuildData
    {
    private:
        const NiModel& mModel;

    public:

        bool mIsSkinned;
        //bool mFlameNodesPresent;
        //bool mEditorMarkerPresent;

        int32_t mBuildFlags;
        inline bool havokEnabled()        const { return (mBuildFlags & Flag_EnableHavok)         != 0; }
        inline bool isSkeleton()          const { return (mBuildFlags & Flag_IsSkeleton)          != 0; }
        inline bool flameNodesPresent()   const { return (mBuildFlags & Flag_FlameNodesPresent)   != 0; }
        inline bool editorMarkerPresent() const { return (mBuildFlags & Flag_EditorMarkerPresent) != 0; }

        // helper to get pointer to parent NiNode
        std::map<NiAVObjectRef, NiNode*> mNiNodeMap;
        void setNiNodeParent(NiAVObjectRef child, NiNode *parent);
        /*const*/ NiNode& getNiNodeParent(NiAVObjectRef child) const;

        std::map<NiNodeRef, NiNode*> mMeshBuildList;

        std::vector<NiNodeRef> mSkelLeafIndicies; // tempoarily used to find the bones
        void addSkelLeafIndex(NiNodeRef leaf) { mSkelLeafIndicies.push_back(leaf); }
        void addNewSkelLeafIndex(NiNodeRef leaf);
        inline bool hasSkeleton() const { return !mSkeleton.isNull(); }
        bool hasBoneLeaf(NiNodeRef leaf) const;

        //bool mIsSkeleton; // npc/creature

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
        std::map<NiNodeRef, /*std::pair<std::string,*/ int32_t/*>*/ > mBhkRigidBodyMap;

        Ogre::SkeletonPtr mSkeleton;

        // FIXME: these needs to be vectors (there can be multiple) and the flame node editor
        // id needs to be extracted (without '@#N' where N is a number)
        std::vector<NiNode*> mFlameNodes;
        std::vector<NiNode*> mAttachLights;

        std::map<NiTimeControllerRef, std::vector<int> > mGeomMorpherControllerMap;

        //       animation name  bone name
        //            |            |
        //            v            v
        std::map<std::string, std::vector<std::string> > mMovingBoneNameMap;
        void setAnimBoneName(const std::string& anim, const std::string& bone) {
            std::map<std::string, std::vector<std::string> >::iterator lb
                = mMovingBoneNameMap.lower_bound(anim);

            if (lb != mMovingBoneNameMap.end() && !(mMovingBoneNameMap.key_comp()(anim, lb->first)))
            {
                lb->second.push_back(bone);
            }
            else // None found, create one
            {
                //std::vector<std::string> tmp;
                //tmp.push_back(bone);
                mMovingBoneNameMap.insert(lb, std::make_pair(anim, std::vector<std::string> { bone }));
            }
        }

        BuildData(const NiModel& model) : mModel(model), mIsSkinned(false), /*mFlameNodesPresent(false), mEditorMarkerPresent(false),*/ mBuildFlags(0)
        {}
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

        std::vector<std::unique_ptr<MeshLoader> > mMeshLoaders;

        std::vector<std::unique_ptr<NiObject> > mObjects;
        std::vector<std::uint32_t> mRoots;

        int mCurrIndex; // FIXME: for debugging Ptr
        std::string mModelName;

        BuildData mBuildData;

        bool mShowEditorMarkers; // usually only for OpenCS

        std::map<std::string, NiAVObjectRef> mObjectPalette;

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
                const Ogre::NameValuePairList* createParams=nullptr/*bool showEditorMarkers=false*/);
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
        void buildBodyPart(BtOgreInst *inst, Ogre::SkeletonPtr skeleton = Ogre::SkeletonPtr());

        void buildAnimation(Ogre::Entity *skelBase, NiModelPtr anim,
                std::multimap<float, std::string>& textKeys,
                std::vector<Ogre::Controller<Ogre::Real> >& controllers,
                NiModel *skeleton,
                NiModel *bow = nullptr);
        const std::map<std::string, NiAVObjectRef>& getObjectPalette() const { return mObjectPalette; }
        Ogre::SkeletonPtr getSkeleton() const { return mBuildData.mSkeleton; }
        void buildSkeleton();

        // returns NiObject type name
        const std::string& blockType(std::uint32_t index) const {
            return mHeader.blockType(index);
        }

        // needed for the NIF version and strings (TES5)
        inline std::uint32_t nifVer() const { return mHeader.nifVer(); }

        inline const std::string& indexToString(std::int32_t index) const {
            return mHeader.indexToString(index);
        }

        inline bool hideEditorMarkers() const { return !mShowEditorMarkers; }

        std::uint32_t getRootIndex() const { return mRoots[0]; } // assumes only one root

        typedef std::int32_t NiNodeRef;
        const std::map<NiNodeRef, /*std::pair<std::string,*/ int32_t/*>*/ >&
            getBhkRigidBodyMap() const { return mBuildData.mBhkRigidBodyMap; }

        //const std::map<NiNodeRef, int32_t>& getBhkRigidBodyMap() const { return mBuildData.mBhkRigidBocyMap; }

        void buildMeshAndEntity(BtOgreInst *inst, const std::string& meshExt = "");
        void buildMeshAndEntity(BtOgreInst *inst, const std::string& npcName, std::vector<Ogre::Vector3>& vertices);

        inline const BuildData& getBuildData() const { return mBuildData; }
    };
}

#endif // NIBTOGRE_NIMODEL_H
