/*
  Copyright (C) 2015-2020 cc9cii

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
#include <OgreController.h>
#include <OgreQuaternion.h>

#include "nistream.hpp"
#include "niheader.hpp"
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
    struct NiTriBasedGeom;
    struct BtOgreInst;
    class NiModel;
    class NiNode;

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
        NiNodeRef mSkeletonRoot;
        //bool mFlameNodesPresent;
        //bool mEditorMarkerPresent;

        int32_t mBuildFlags; // mainly from BSX flags
        inline bool havokEnabled()          const { return (mBuildFlags & Flag_EnableHavok)         != 0; }
        inline bool animEnabled()           const { return (mBuildFlags & Flag_EnableAnimation)     != 0; }
        inline bool isSkeletonTES4()        const { return (mBuildFlags & Flag_IsSkeleton)          != 0; }
        inline bool flameNodesPresentTES4() const { return (mBuildFlags & Flag_FlameNodesPresent)   != 0; }
        inline bool editorMarkerPresent()   const { return (mBuildFlags & Flag_EditorMarkerPresent) != 0; }

        // helper to get pointer to parent NiNode
        std::map<NiAVObjectRef, NiNode*> mNiNodeMap;
        void setNiNodeParent(NiAVObjectRef child, NiNode *parent);
        /*const*/ NiNode& getNiNodeParent(NiAVObjectRef child) const;

        std::map<NiNodeRef, NiNode*> mMeshBuildList;

        // during construction various NiObjects may indicate that it has bones
        // these are then used as the starting points for NiNode::findBones which recursively
        // traverses till a skeleton root is found - the main objective is to filter out any
        // NiNodes that are not needed as bones (to minimise the number of bones)
        std::vector<NiNodeRef> mSkelLeafIndicies; // tempoarily used to find the bones

        // adds without checking
        // NiSkinInstance - bone refs
        // NiNode - flame nodes, attach light
        // NiKeyframeController - target refs
        // NiMultiTargetTransformController - extra target refs (what is this?)
        // NiTriBasedGeom - hack for testing animation of sub-mesh
        void addSkelLeafIndex(NiNodeRef leaf) { mSkelLeafIndicies.push_back(leaf); }

        // only adds if none found
        void addNewSkelLeafIndex(NiNodeRef leaf); // FIXME: not used?
        bool hasBoneLeaf(NiNodeRef leaf) const; // FIXME: not used?

        //bool mIsSkeleton; // npc/creature

        // The btCollisionShape for btRigidBody corresponds to an Ogre::Entity whose Ogre::SceneNode
        // may be controlled for Ragdoll animations.  So we just really need the NiModel name,
        // NiNode name (and maybe the NiNode block index) to be able to load the required info.
        //
        // Once the NiModel is "built", all the required info are ready to be simply collected
        // and used.
        //
        //      index = parent NiNode's block index
        //        |
        //        |                  name  = concatenation of model, "@" and parent NiNode name
        //        |                    |
        //        v                    v
        std::map<NiNodeRef, /*std::pair<std::string,*/ int32_t/*>*/ > mBhkRigidBodyMap;
        //std::vector<std::pair<bhkConstraint*, bhkEntity*> > mBhkConstraints;
        bool mHasBhkConstraint;
        inline bool hasBhkConstraint() const { return mHasBhkConstraint; }

        // FIXME: these needs to be vectors (there can be multiple) and the flame node editor
        // id needs to be extracted (without '@#N' where N is a number)
        std::vector<NiNode*> mFlameNodes;
        std::vector<NiNode*> mAttachLights;

        std::map<NiTimeControllerRef, std::vector<int> > mGeomMorpherControllerMap;

        //       animation name  bone name
        //            |            |
        //            v            v
        std::map<std::string, std::vector<std::string> > mMovingBoneNameMap;

        void setAnimBoneName(const std::string& anim, const std::string& bone)
        {
            std::map<std::string, std::vector<std::string> >::iterator lb
                = mMovingBoneNameMap.lower_bound(anim);

            if (lb != mMovingBoneNameMap.end() && !(mMovingBoneNameMap.key_comp()(anim, lb->first)))
            {
                lb->second.push_back(bone);
            }
            else // None found, create one
            {
                mMovingBoneNameMap.insert(lb, std::make_pair(anim, std::vector<std::string> { bone }));
            }
        }

        std::multimap<float, std::string> mTextKeys;
        std::vector<Ogre::Controller<Ogre::Real> > mControllers;

        int mFlags; // some global properties

        BuildData(const NiModel& model)
            : mModel(model), mIsSkinned(false), mSkeletonRoot(0)
            , /*mFlameNodesPresent(false), mEditorMarkerPresent(false) ,*/ mBuildFlags(0)
            , mHasBhkConstraint(false)
            , mFlags(0)
        {
        }
    };

    //
    //   +--& NiModel o--- NiStream
    //   |    o o o o        o o
    //   |    | | | |        | +-- NIF version numbers (local copy)
    //   |    | | | |        *
    //   |    | | | +----- Header
    //   |    | | |         o o
    //   |    | | |         | +--- NIF version numbers
    //   |    | | |         +----- vector<string>
    //   |    | | |
    //   |    | | +--- ObjectPalatte
    //   |    | |
    //   |    | +--- BuildData &--+   (access during construction)
    //   |    |                   o
    //   |    +------- vector<NiObject*>
    //   |                      o
    //   +----------------------+
    //
    // NOTE: NiStream has a pointer to Header for the appending long strings (TES3/TES4)
    //       NiObject has a reference to NiModel for getting strings and other NiObject Ptrs/Refs
    //
    //       BuildData is passed to NiObject to capture useful data during construction
    //       (this reduces the need to scan the objects later)
    //
    //       FIXME: skeleton should be here along with ObjectPalatte?
    //
    //       ObjectPalatte is used by another NiModel to build animation based on targets
    //       (bones) in this NiModel
    //
    class NiModel : public Ogre::Resource
    {
        std::unique_ptr<NiStream> mNiStream; // NOTE: the initialisation order of NiStream and NiHeader
        std::unique_ptr<NiHeader> mHeader;

        const std::string mGroup;  // Ogre group

        std::vector<std::unique_ptr<NiObject> > mObjects;
        std::vector<std::uint32_t> mRoots;

        int mCurrIndex; // FIXME: for debugging Ptr

        std::string mModelName;
        std::string mNif; // store NIF path for prepareImpl (may be different to mModelName)

        NiNode *mRootNode; // convenience copy (NOTE: only valid if there is only one)
        NiNode *mBoneRootNode; // convenience copy

        BuildData mBuildData;
        Ogre::SkeletonPtr mSkeleton;
        std::vector<std::pair<Ogre::MeshPtr, NiNode*> > mMeshes;

        //      target NiNode ref     NiNode world transform
        //              |                   |
        //              v                   v
        std::map<std::int32_t, std::pair<Ogre::Matrix4, btCollisionShape *> > mBtCollisionShapeMap;

        bool mShowEditorMarkers; // usually only for OpenCS

        // for skeleton.nif, etc, only
        std::map<std::string, NiAVObjectRef> mObjectPalette;

        // default, copy and assignment not allowed
        NiModel();
        NiModel(const NiModel& other);
        NiModel& operator=(const NiModel& other);

    protected:
        virtual void prepareImpl();
        virtual void unprepareImpl();
        virtual void loadImpl();   // called by Ogre::Resource::load()
        virtual void unloadImpl(); // called by Ogre::Resource::unload()
        virtual void preLoadImpl();   // called by Ogre::Resource::load()
        virtual void preUnloadImpl(); // called by Ogre::Resource::unload()

    public:
        // The parameter 'nif' refers to those in the Ogre::ResourceGroupManager
        // e.g. full path from the directory/BSA added in Bsa::registerResources(), etc
        NiModel(Ogre::ResourceManager *creator, const Ogre::String& name, Ogre::ResourceHandle handle,
                const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
                const Ogre::String& nif/*, bool showEditorMarkers=false*/);
        ~NiModel();

        const std::string& getOgreGroup() const { return mGroup; }
        const std::string& getModelName() const { return mModelName; }

        template<class T>
        inline T *getRef(std::int32_t index) const {
#if 0
            try {
                return static_cast<T*>(mObjects[index].get());
            }
            catch (...) {
                return nullptr;
            }
#else
            if (index >= 0 && (index > mCurrIndex)) // FIXME: for debugging Ptr
                throw std::runtime_error("Ptr");

            return (index < 0) ? nullptr : static_cast<T*>(mObjects[index].get());
#endif
        }

        // returns NiObject type name
        const std::string& blockType(std::uint32_t index) const {
            return mHeader->blockType(index);
        }

        // needed for the NIF version and strings (TES5)
        inline std::uint32_t nifVer() const { return mHeader->nifVer(); }

        inline const std::string& indexToString(std::int32_t index) const {
            return mHeader->indexToString(index);
        }

        inline std::int32_t searchStrings(const std::string& str) const {
            return mHeader->searchStrings(str);
        }

        inline bool hideEditorMarkers() const { return !mShowEditorMarkers; }

        NiNode *skeletonRoot();          // returns nullptr if none found

        inline const NiNode *rootNode() const { return mRootNode; }; // returns the root NiNode of the model
        std::uint32_t rootIndex() const; // WARN: will throw if there are more than one
        inline std::size_t numRootNodes() const { return mRoots.size(); }

        typedef std::int32_t NiNodeRef;
        const std::map<NiNodeRef, /*std::pair<std::string,*/ int32_t/*>*/ >&
            getBhkRigidBodyMap() const { return mBuildData.mBhkRigidBodyMap; }

        const std::multimap<float, std::string>& textKeys() const { return mBuildData.mTextKeys; }
        const std::vector<Ogre::Controller<Ogre::Real> >& controllers() const { return mBuildData.mControllers; }

        //const std::map<NiNodeRef, int32_t>& getBhkRigidBodyMap() const { return mBuildData.mBhkRigidBocyMap; }

        void buildSkinnedModel(Ogre::SkeletonPtr skeleton = Ogre::SkeletonPtr());

        void buildModel();

        //void createCollisionshapes();

        void buildAnimation(Ogre::Entity *skelBase, NiModelPtr anim,
                std::multimap<float, std::string>& textKeys,
                std::vector<Ogre::Controller<Ogre::Real> >& controllers,
                NiModel *skeleton,
                NiModel *bow = nullptr);
        const std::map<std::string, NiAVObjectRef>& getObjectPalette() const { return mObjectPalette; }

        Ogre::SkeletonPtr getSkeleton() const { return mSkeleton; }
        inline bool hasSkeleton() const { return !mSkeleton.isNull(); }
        void buildSkeleton(bool load = false);

        inline const std::vector<std::pair<Ogre::MeshPtr, NiNode*> >& meshes() const { return mMeshes; }

        void createNiObjects();
        // for skeleton.nif, etc, that requires an Entity for ObjectScene
        void createDummyMesh();
        // supply skeleton for skinned objects
        void createMesh(bool isMorphed = false, Ogre::SkeletonPtr skeleton = Ogre::SkeletonPtr());

        // FIXME: move out?
        //void buildEntities(BtOgreInst *inst);
        //void buildEntities(BtOgreInst *inst, const std::string& npcName,
                //std::unique_ptr<std::vector<Ogre::Vector3> > morphedVertices);

        inline const BuildData& buildData() const { return mBuildData; }

        // used to generate a FaceGen TRI file, returns the vertices of the first NiTriBasedGeom
        const std::vector<Ogre::Vector3>& fgVertices() const;

        // used by FgSam to populate the morphed vertices
        std::vector<Ogre::Vector3>& fgMorphVertices();

        void useFgMorphVertices();

        std::string targetBone() const;

        void findBoneNodes(bool buildObjectPalette = false, std::size_t rootIndex = 0);

        const Ogre::Quaternion getBaseRotation() const;

    private:

        // access to NiGeometryData for generating a FaceGen TRI file or to populate morphed vertices
        // WARN: may throw
        NiTriBasedGeom *fgGeometry() const;
    };
}

#endif // NIBTOGRE_NIMODEL_H
