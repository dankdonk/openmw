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

  Much of the information on the NIF file structures are based on the NifSkope
  documenation.  See http://niftools.sourceforge.net/wiki/NifSkope for details.

*/
#ifndef NIBTOGRE_NINODE_H
#define NIBTOGRE_NINODE_H

#include <OgreVector4.h>

#include "niavobject.hpp"

namespace Ogre
{
    class Skeleton;
    class Bone;
}

// Based on NifTools/NifSkope/doc/index.html
//
// NiObjectNET
//     NiAVObject
//         NiNode
//             AvoidNode <------------------ /* typedef NiNode */
//             BSBlastNode
//             BSDamageStage
//             BSFadeNode <----------------- /* typedef NiNode */
//             BSLeafAnimNode <------------- /* typedef NiNode */
//             BSMultiBoundNode
//             BSOrderedNode
//             BSTreeNode
//             BSValueNode
//             NiBillboardNode
//             NiBSAnimationNode <--------- /* typedef NiNode */
//             NiBSParticleNode <---------- /* typedef NiNode */
//             NiSwitchNode
//             RootCollisionNode <--------- /* typedef NiNode */
namespace NiBtOgre
{
    class NiStream;

    class NiNode : public NiAVObject
    {
        std::string mNodeName; // cached since used frequently

        std::vector<NiNodeRef> mChildBoneNodes; // FIXME: experimental for building a skeleton

        void buildTES3(Ogre::SceneNode *sceneNode, BtOgreInst *inst, NiObject *parentNiNode = nullptr);

    protected:
        std::vector<NiAVObjectRef>      mChildren;
        std::vector<NiDynamicEffectRef> mEffects;

    public:
        NiNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data);
        virtual ~NiNode() {};

        // It seems that for TES4 only NiNodes/NiBillboardNode are root nodes?
        virtual void build(BtOgreInst *inst, ModelData *data, NiObject *parentNiNode = nullptr);

        // For NiGeometry children (e.g. NiTriStrips)
        virtual inline const std::string& getNodeName() const { return mNodeName; }

        // Returns true if skeleton root is found
        virtual void findBones(const NiNodeRef skeletonRoot, const NiNodeRef childNode); // FIXME: experimental
        virtual void findBones(std::int32_t rootIndex); // FIXME: experimental

        virtual void addBones(Ogre::Skeleton *skeleton,
                Ogre::Bone *parentBone, std::map<std::uint32_t, std::uint16_t>& indexToHandle);

        virtual NiNode *getParentNode();

        //const std::vector<NiNodeRef>& getChildBones() { return mChildBoneNodes; }
        //void clearSkeleton(); // FIXME: experimental
        //void buildSkeleton(BtOgreInst *inst, NiSkinInstanceRef skinInstanceIndex); // FIXME: experimental
    };

    typedef NiNode AvoidNode;

    // Seen in NIF version 20.2.0.7
    class BSBlastNode : public NiNode
    {
        char mUnknown1;;
        short mUnknown2;

    public:
        BSBlastNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data);
        virtual ~BSBlastNode() {};

        //void build(BtOgreInst *inst, NiObject *parentNiNode = nullptr);
    };

    // Seen in NIF version 20.2.0.7
    class BSDamageStage : public NiNode
    {
        char mUnknown1;;
        std::int16_t mUnknown2;

    public:
        BSDamageStage(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data);
        virtual ~BSDamageStage() {};

        //void build(BtOgreInst *inst, NiObject *parentNiNode = nullptr);
    };

    typedef NiNode BSFadeNode; // Seen in NIF version 20.2.0.7
    typedef NiNode BSLeafAnimNode; // Seen in NIF version 20.2.0.7

    // Seen in NIF version 20.2.0.7
    class BSMasterParticleSystem : public NiNode
    {
    public:
        std::uint16_t mMaxEmitterObjects;
        std::int32_t  mNumParticleSystems;
        NiAVObjectRef mParticleSystemsIndex;

        BSMasterParticleSystem(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data);
    };

    // Seen in NIF version 20.2.0.7
    class BSMultiBoundNode : public NiNode
    {
        BSMultiBoundRef mMultiBoundIndex;
        std::uint32_t mUnknown; // from 20.2.0.7

    public:
        BSMultiBoundNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data);
        virtual ~BSMultiBoundNode() {};

        //void build(BtOgreInst *inst, NiObject *parentNiNode = nullptr);
    };

    // Seen in NIF version 20.2.0.7
    class BSOrderedNode : public NiNode
    {
        Ogre::Vector4 mAlphaSortBound;
        unsigned char mIsStaticBound;

    public:
        BSOrderedNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data);
        virtual ~BSOrderedNode() {};

        //virtual void build(BtOgreInst *inst, NiObject *parentNiNode = nullptr);
    };

    // Seen in NIF version 20.2.0.7
    class BSTreeNode : public NiNode
    {
        std::vector<NiNodeRef> mBones1;
        std::vector<NiNodeRef> mBones2;

    public:
        BSTreeNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data);
        virtual ~BSTreeNode() {};

        //void build(BtOgreInst *inst, NiObject *parentNiNode = nullptr);
    };

    // Seen in NIF version 20.2.0.7
    class BSValueNode : public NiNode
    {
        std::int32_t mValue;

    public:
        BSValueNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data);
        virtual ~BSValueNode() {};

        //void build(BtOgreInst *inst, NiObject *parentNiNode = nullptr);
    };

    struct NiBillboardNode : public NiNode
    {
        std::uint16_t mBillboardMode;

    public:
        NiBillboardNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data);
        virtual ~NiBillboardNode() {};

        //void build(BtOgreInst *inst, NiObject *parentNiNode = nullptr);
    };

    typedef NiNode NiBSAnimationNode;
    typedef NiNode NiBSParticleNode;

    // Seen in NIF version 20.2.0.7
    class NiSwitchNode : public NiNode
    {
        std::uint16_t mUnknownFlags;
        std::int32_t mUnknownInt;

    public:
        NiSwitchNode(uint32_t index, NiStream& stream, const NiModel& model, ModelData& data);

        //void build(BtOgreInst *inst, NiObject *parentNiNode = nullptr);
    };

    typedef NiNode RootCollisionNode;
}

#endif // NIBTOGRE_NINODE_H
