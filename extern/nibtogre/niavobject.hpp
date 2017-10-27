/*
  Copyright (C) 2015-2017 cc9cii

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
#ifndef NIBTOGRE_NIAVOBJECT_H
#define NIBTOGRE_NIAVOBJECT_H

#include <string>
#include <vector>
#include <cstdint>

#include <OgreVector3.h>
#include <OgreVector4.h>
#include <OgreMatrix3.h>

#include "niobjectnet.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiObjectNET
//     NiAVObject
//         NiCamera
//         NiDynamicEffect
//             NiLight
//                 NiAmbientLight <--------- /* typedef NiLight */
//                 NiDirectionalLight <----- /* typedef NiLight */
//             NiTextureEffect
//         NiGeometry
//             NiParticles <---------------- /* typedef NiGeometry */
//                 NiAutoNormalParticles <-- /* typedef NiParticles */
//                 NiParticleSystem
//                     BSStripParticleSystem /* typedef NiParticleSystem */
//                 NiRotatingParticles <---- /* typedef NiParticles */
//             NiTriBasedGeom <------------- /* typedef NiGeometry */
//                 BSLODTriShape
//                 NiTriShape <------------- /* typedef NiTriBasedGeom */
//                 NiTriStrips <------------ /* typedef NiTriBasedGeom */
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
    class Header;

    class NiAVObject : public NiObjectNET
    {
    public:
        struct BoundingBox
        {
            std::uint32_t unknownInt;
            Ogre::Vector3 translation;
            Ogre::Matrix3 rotation;
            Ogre::Vector3 radius; // per direction
        };

        NiAVObject(NiStream& stream, const NiModel& model);
        virtual ~NiAVObject() {}

        virtual void build(const RecordBlocks& objects, const Header& header,
                           Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);

    protected:
        std::uint16_t mFlags;

        Ogre::Vector3 mTranslation;
        Ogre::Matrix3 mRotation;
        float         mScale; // only uniform scaling

        Ogre::Vector3 mVelocity; // unknown, to 4.2.2.0

        std::vector<NiPropertyRef> mProperty;

        bool mHasBoundingBox;       // to 4.2.2.0
        BoundingBox   mBoundingBox; // to 4.2.2.0

        NiCollisionObjectRef mCollisionObjectIndex; // from 10.0.1.0
    };

    class NiCamera : public NiAVObject
    {
        float mFrustumLeft, mFrustumRight, mFrustumTop, mFrustumBottom, mFrustumNear, mFrustumFar;
        bool  mUseOrthographicProjection;
        float mViewportLeft, mViewportRight, mViewportTop, mViewportBottom;
        float mLODAdjust;

    public:
        NiCamera(NiStream& stream, const NiModel& model);
        virtual ~NiCamera() {}

        virtual void build(const RecordBlocks& objects, const Header& header,
                           Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    struct NiDynamicEffect : public NiAVObject
    {
        bool mSwitchState;
        std::vector<NiAVObjectRef> mAffectedNodes;

        NiDynamicEffect(NiStream& stream, const NiModel& model);

        //virtual void build(const RecordBlocks& objects, const Header& header,
                           //Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    struct NiLight : public NiDynamicEffect
    {
        float mDimmer;
        Ogre::Vector3 mAmbientColor;
        Ogre::Vector3 mDiffuseColor;
        Ogre::Vector3 mSpecularColor;

        NiLight(NiStream& stream, const NiModel& model);

        //virtual void build(const RecordBlocks& objects, const Header& header,
                           //Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    typedef NiLight NiAmbientLight;

    typedef NiLight NiDirectionalLight;

    struct NiTextureEffect : public NiDynamicEffect
    {
        Ogre::Matrix3 mModelProjectionMatrix;
        Ogre::Vector3 mModelProjectionTransform;

        std::uint32_t mTextureFiltering;
        std::uint32_t mTextureClamping;
        std::uint32_t mTextureType;
        std::uint32_t mCoordGenType;

        NiSourceTextureRef mSourceTexture;

        char mClippingPlane;

        NiTextureEffect(NiStream& stream, const NiModel& model);

        //virtual void build(const RecordBlocks& objects, const Header& header,
                           //Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    /* ------------------------------- NiGeometry --------------------------- */
    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiGeometry : public NiAVObject
    {
        // FIXME: probably not grouped together, need to check real examples
        struct MaterialExtraData
        {
            StringIndex  materialName;
            std::int32_t materialExtraData;
        };

        NiGeometryDataRef mDataIndex; // subclass of NiGeometryData includes NiTriShapeData
        NiSkinInstanceRef mSkinInstanceIndex;

        std::vector<StringIndex> mMaterialName;
        std::vector<std::int32_t> mMaterialExtraData;
        //std::vector<MaterialExtraData> mMaterials;

        bool mHasShader;
        StringIndex mShaderName;

        bool mDirtyFlag;
        std::vector<NiPropertyRef> mBSProperties;

        NiGeometry(NiStream& stream, const NiModel& model);

        virtual void build(const RecordBlocks& objects, const Header& header,
                           Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    typedef NiGeometry NiParticles;
    typedef NiParticles NiAutoNormalParticles;

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiParticleSystem : public NiParticles
    {
        std::uint16_t mUnknownS2;
        std::uint16_t mUnknownS3;
        std::uint32_t mUnknownI1;
        bool mWorldSpace;                          // from 10.1.0.0
        std::vector<NiPSysModifierRef> mModifiers; // from 10.1.0.0

        NiParticleSystem(NiStream& stream, const NiModel& model);

        virtual void build(const RecordBlocks& objects, const Header& header,
                           Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    typedef NiParticleSystem BSStripParticleSystem; // Seen in NIF version 20.2.0.7
    typedef NiParticles NiRotatingParticles;
    typedef NiGeometry NiTriBasedGeom;
    typedef NiTriBasedGeom NiTriShape;
    typedef NiTriBasedGeom NiTriStrips; // Seen in NIF version 20.0.0.4, 20.0.0.5


    // Seen in NIF version 20.2.0.7
    struct BSLODTriShape : public NiGeometry
    {
        std::uint32_t mLevel0Size;
        std::uint32_t mLevel1Size;
        std::uint32_t mLevel2Size;

        BSLODTriShape(NiStream& stream, const NiModel& model);

        virtual void build(const RecordBlocks& objects, const Header& header,
                           Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    /* --------------------------------- NiNode ----------------------------- */
    struct NiDynamicEffect;
    class BSMultiBound;

    class NiNode : public NiAVObject
    {
    protected:
        std::vector<NiAVObjectRef>      mChildren;
        std::vector<NiDynamicEffectRef> mEffects;

    public:
        NiNode(NiStream& stream, const NiModel& model);
        virtual ~NiNode() {};

        // It seems that for TES4 only NiNodes are root nodes?
        virtual void build(const RecordBlocks& objects, const Header& header,
                           Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    typedef NiNode AvoidNode;

    // Seen in NIF version 20.2.0.7
    class BSBlastNode : public NiNode
    {
        char mUnknown1;;
        short mUnknown2;

    public:
        BSBlastNode(NiStream& stream, const NiModel& model);
        virtual ~BSBlastNode() {};

        //virtual void build(const RecordBlocks& objects, const Header& header,
                           //Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    // Seen in NIF version 20.2.0.7
    class BSDamageStage : public NiNode
    {
        char mUnknown1;;
        std::int16_t mUnknown2;

    public:
        BSDamageStage(NiStream& stream, const NiModel& model);
        virtual ~BSDamageStage() {};

        //virtual void build(const RecordBlocks& objects, const Header& header,
                           //Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
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

        BSMasterParticleSystem(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.2.0.7
    class BSMultiBoundNode : public NiNode
    {
        BSMultiBoundRef mMultiBoundIndex;
        std::uint32_t mUnknown; // from 20.2.0.7

    public:
        BSMultiBoundNode(NiStream& stream, const NiModel& model);
        virtual ~BSMultiBoundNode() {};

        //virtual void build(const RecordBlocks& objects, const Header& header,
                           //Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    // Seen in NIF version 20.2.0.7
    class BSOrderedNode : public NiNode
    {
        Ogre::Vector4 mAlphaSortBound;
        unsigned char mIsStaticBound;

    public:
        BSOrderedNode(NiStream& stream, const NiModel& model);
        virtual ~BSOrderedNode() {};

        //virtual void build(const RecordBlocks& objects, const Header& header,
                           //Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    // Seen in NIF version 20.2.0.7
    class BSTreeNode : public NiNode
    {
        std::vector<NiNodeRef> mBones1;
        std::vector<NiNodeRef> mBones2;

    public:
        BSTreeNode(NiStream& stream, const NiModel& model);
        virtual ~BSTreeNode() {};

        //virtual void build(const RecordBlocks& objects, const Header& header,
                           //Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    // Seen in NIF version 20.2.0.7
    class BSValueNode : public NiNode
    {
        std::int32_t mValue;

    public:
        BSValueNode(NiStream& stream, const NiModel& model);
        virtual ~BSValueNode() {};

        //virtual void build(const RecordBlocks& objects, const Header& header,
                           //Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    struct NiBillboardNode : public NiNode
    {
        std::uint16_t mBillboardMode;

    public:
        NiBillboardNode(NiStream& stream, const NiModel& model);
        virtual ~NiBillboardNode() {};

        //virtual void build(const RecordBlocks& objects, const Header& header,
                           //Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    typedef NiNode NiBSAnimationNode;
    typedef NiNode NiBSParticleNode;

    // Seen in NIF version 20.2.0.7
    class NiSwitchNode : public NiNode
    {
        std::uint16_t mUnknownFlags;
        std::int32_t mUnknownInt;

    public:
        NiSwitchNode(NiStream& stream, const NiModel& model);

        //virtual void build(const RecordBlocks& objects, const Header& header,
                           //Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene);
    };

    typedef NiNode RootCollisionNode;
}

#endif // NIBTOGRE_NIOAVOBJECT_H
