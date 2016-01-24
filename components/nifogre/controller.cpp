#include "controller.hpp"

#include <OgreTechnique.h>
#include <OgreNode.h>
#include <OgreTagPoint.h>
#include <OgreParticleSystem.h>
#include <OgreEntity.h>
#include <OgreAnimationState.h>

#include <components/nif/base.hpp>
#include <components/nif/controller.hpp>
#include <components/nif/controlled.hpp>
#include <components/nif/property.hpp>

#include <components/misc/resourcehelpers.hpp>

#include "objectscene.hpp" // MaterialControllerManager

float NifOgre::ValueInterpolator::interpKey (const Nif::FloatKeyMap::MapType &keys, float time, float def) const
{
    if (keys.size() == 0)
        return def;

    if(time <= keys.begin()->first)
        return keys.begin()->second.mValue;

    Nif::FloatKeyMap::MapType::const_iterator it = keys.lower_bound(time);
    if (it != keys.end())
    {
        float aTime = it->first;
        const Nif::FloatKey* aKey = &it->second;

        assert (it != keys.begin()); // Shouldn't happen, was checked at beginning of this function

        Nif::FloatKeyMap::MapType::const_iterator last = --it;
        float aLastTime = last->first;
        const Nif::FloatKey* aLastKey = &last->second;

        float a = (time - aLastTime) / (aTime - aLastTime);
        return aLastKey->mValue + ((aKey->mValue - aLastKey->mValue) * a);
    }
    else
        return keys.rbegin()->second.mValue;
}

Ogre::Vector3 NifOgre::ValueInterpolator::interpKey (const Nif::Vector3KeyMap::MapType &keys, float time) const
{
    if(time <= keys.begin()->first)
        return keys.begin()->second.mValue;

    Nif::Vector3KeyMap::MapType::const_iterator it = keys.lower_bound(time);
    if (it != keys.end())
    {
        float aTime = it->first;
        const Nif::Vector3Key* aKey = &it->second;

        assert (it != keys.begin()); // Shouldn't happen, was checked at beginning of this function

        Nif::Vector3KeyMap::MapType::const_iterator last = --it;
        float aLastTime = last->first;
        const Nif::Vector3Key* aLastKey = &last->second;

        float a = (time - aLastTime) / (aTime - aLastTime);
        return aLastKey->mValue + ((aKey->mValue - aLastKey->mValue) * a);
    }
    else
        return keys.rbegin()->second.mValue;
}

NifOgre::DefaultFunction::DefaultFunction (const Nif::Controller *ctrl, bool deltaInput)
: Ogre::ControllerFunction<Ogre::Real>(deltaInput)
, mFrequency(ctrl->frequency)
, mPhase(ctrl->phase)
, mStartTime(ctrl->timeStart)
, mStopTime(ctrl->timeStop)
{
if(mDeltaInput)
    mDeltaCount = mPhase;
}

Ogre::Real NifOgre::DefaultFunction::calculate (Ogre::Real value)
{
    if(mDeltaInput)
    {
        if (mStopTime - mStartTime == 0.f)
            return 0.f;

        mDeltaCount += value*mFrequency;
        if(mDeltaCount < mStartTime)
            mDeltaCount = mStopTime - std::fmod(mStartTime - mDeltaCount,
                                                mStopTime - mStartTime);
        mDeltaCount = std::fmod(mDeltaCount - mStartTime,
                                mStopTime - mStartTime) + mStartTime;
        return mDeltaCount;
    }

    value = std::min(mStopTime, std::max(mStartTime, value+mPhase));
    return value;
}

NifOgre::FlipController::Value::Value (Ogre::MovableObject *movable,
        const Nif::NiFlipController *ctrl, MaterialControllerManager* materialControllerMgr)
  : mMovable(movable)
  , mMaterialControllerMgr(materialControllerMgr)
{
    mTexSlot = ctrl->mTexSlot;
    if (ctrl->nifVer >= 0x0a020000) // from 10.2.0.0
    {
        if (ctrl->interpolator.getPtr()->recType == Nif::RC_NiFloatInterpolator)
        {
            const Nif::NiFloatInterpolator* fi
                = static_cast<const Nif::NiFloatInterpolator*>(ctrl->interpolator.getPtr());
            // FIXME: this key is probably not the right one to use
            float key = fi->value;
            // use 0.5f as the default, not sure what it should be
            mDelta = interpKey(fi->floatData.getPtr()->mKeyList.mKeys, key, 0.5f);
        }
    }
    else if (ctrl->nifVer <= 0x0a010000) // up to 10.1.0.0
        mDelta = ctrl->mDelta;
    for (unsigned int i=0; i<ctrl->mSources.length(); ++i)
    {
        const Nif::NiSourceTexture* tex = ctrl->mSources[i].getPtr();
        if (!tex->external)
            std::cerr << "Warning: Found internal texture, ignoring." << std::endl;
        mTextures.push_back(Misc::ResourceHelpers::correctTexturePath(tex->filename));
    }
}

Ogre::Real NifOgre::FlipController::Value::getValue () const
{
    // Should not be called
    return 0.0f;
}

void NifOgre::FlipController::Value::setValue (Ogre::Real time)
{
    if (mDelta == 0)
        return;
    int curTexture = int(time / mDelta) % mTextures.size(); // FIXME: maybe the interpolator is used here?

    Ogre::MaterialPtr mat = mMaterialControllerMgr->getWritableMaterial(mMovable);
    Ogre::Material::TechniqueIterator techs = mat->getTechniqueIterator();
    while(techs.hasMoreElements())
    {
        Ogre::Technique *tech = techs.getNext();
        Ogre::Technique::PassIterator passes = tech->getPassIterator();
        while(passes.hasMoreElements())
        {
            Ogre::Pass *pass = passes.getNext();
            Ogre::Pass::TextureUnitStateIterator textures = pass->getTextureUnitStateIterator();
            while (textures.hasMoreElements())
            {
                Ogre::TextureUnitState *texture = textures.getNext();
                if ((texture->getName() == "diffuseMap" && mTexSlot == Nif::NiTexturingProperty::BaseTexture)
                        || (texture->getName() == "normalMap" && mTexSlot == Nif::NiTexturingProperty::BumpTexture)
                        || (texture->getName() == "detailMap" && mTexSlot == Nif::NiTexturingProperty::DetailTexture)
                        || (texture->getName() == "darkMap" && mTexSlot == Nif::NiTexturingProperty::DarkTexture)
                        || (texture->getName() == "emissiveMap" && mTexSlot == Nif::NiTexturingProperty::GlowTexture))
                    texture->setTextureName(mTextures[curTexture]);
            }
        }
    }
}

NifOgre::AlphaController::Value::Value (Ogre::MovableObject *movable,
        const Nif::NiFloatData *data, MaterialControllerManager* materialControllerMgr)
  : mMovable(movable)
  , mData(data->mKeyList)
  , mMaterialControllerMgr(materialControllerMgr)
{
}

Ogre::Real NifOgre::AlphaController::Value::getValue () const
{
    // Should not be called
    return 0.0f;
}

void NifOgre::AlphaController::Value::setValue (Ogre::Real time)
{
    float value = interpKey(mData.mKeys, time);
    Ogre::MaterialPtr mat = mMaterialControllerMgr->getWritableMaterial(mMovable);
    Ogre::Material::TechniqueIterator techs = mat->getTechniqueIterator();
    while(techs.hasMoreElements())
    {
        Ogre::Technique *tech = techs.getNext();
        Ogre::Technique::PassIterator passes = tech->getPassIterator();
        while(passes.hasMoreElements())
        {
            Ogre::Pass *pass = passes.getNext();
            Ogre::ColourValue diffuse = pass->getDiffuse();
            diffuse.a = value;
            pass->setDiffuse(diffuse);
        }
    }
}

NifOgre::MaterialColorController::Value::Value (Ogre::MovableObject *movable,
        const Nif::NiPosData *data, MaterialControllerManager* materialControllerMgr)
  : mMovable(movable)
  , mData(data->mKeyList)
  , mMaterialControllerMgr(materialControllerMgr)
{
}

Ogre::Real NifOgre::MaterialColorController::Value::getValue () const
{
    // Should not be called
    return 0.0f;
}

void NifOgre::MaterialColorController::Value::setValue (Ogre::Real time)
{
    Ogre::Vector3 value = interpKey(mData.mKeys, time);
    Ogre::MaterialPtr mat = mMaterialControllerMgr->getWritableMaterial(mMovable);
    Ogre::Material::TechniqueIterator techs = mat->getTechniqueIterator();
    while(techs.hasMoreElements())
    {
        Ogre::Technique *tech = techs.getNext();
        Ogre::Technique::PassIterator passes = tech->getPassIterator();
        while(passes.hasMoreElements())
        {
            Ogre::Pass *pass = passes.getNext();
            Ogre::ColourValue diffuse = pass->getDiffuse();
            diffuse.r = value.x;
            diffuse.g = value.y;
            diffuse.b = value.z;
            pass->setDiffuse(diffuse);
        }
    }
}

bool NifOgre::VisController::Value::calculate (Ogre::Real time) const
{
    if(mData.size() == 0)
        return true;

    for(size_t i = 1;i < mData.size();i++)
    {
        if(mData[i].time > time)
            return mData[i-1].isSet;
    }
    return mData.back().isSet;
}

void NifOgre::VisController::Value::setVisible (Ogre::Node *node, bool vis)
{
    // Skinned meshes are attached to the scene node, not the bone.
    // We use the Node's user data to connect it with the mesh.
    Ogre::Any customData = node->getUserObjectBindings().getUserAny();

    if (!customData.isEmpty())
        Ogre::any_cast<Ogre::MovableObject*>(customData)->setVisible(vis);

    Ogre::TagPoint *tag = dynamic_cast<Ogre::TagPoint*>(node);
    if(tag != NULL)
    {
        Ogre::MovableObject *obj = tag->getChildObject();
        if(obj != NULL)
            obj->setVisible(vis);
    }

    Ogre::Node::ChildNodeIterator iter = node->getChildIterator();
    while(iter.hasMoreElements())
    {
        node = iter.getNext();
        setVisible(node, vis);
    }
}

NifOgre::VisController::Value::Value (Ogre::Node *target, const Nif::NiVisData *data)
  : NodeTargetValue<Ogre::Real>(target)
  , mData(data->mVis)
{ }

Ogre::Quaternion NifOgre::VisController::Value::getRotation (float time) const
{
    return Ogre::Quaternion();
}

Ogre::Vector3 NifOgre::VisController::Value::getTranslation (float time) const
{
    return Ogre::Vector3(0.0f);
}

Ogre::Vector3 NifOgre::VisController::Value::getScale (float time) const
{
    return Ogre::Vector3(1.0f);
}

Ogre::Real NifOgre::VisController::Value::getValue () const
{
    // Should not be called
    return 0.0f;
}

void NifOgre::VisController::Value::setValue (Ogre::Real time)
{
    bool vis = calculate(time);
    setVisible(mNode, vis);
}

Ogre::Quaternion NifOgre::TransformController::Value::interpKey (const Nif::QuaternionKeyMap::MapType &keys, float time)
{
    if(time <= keys.begin()->first)
        return keys.begin()->second.mValue;

    Nif::QuaternionKeyMap::MapType::const_iterator it = keys.lower_bound(time);
    if (it != keys.end())
    {
        float aTime = it->first;
        const Nif::QuaternionKey* aKey = &it->second;

        assert (it != keys.begin()); // Shouldn't happen, was checked at beginning of this function

        Nif::QuaternionKeyMap::MapType::const_iterator last = --it;
        float aLastTime = last->first;
        const Nif::QuaternionKey* aLastKey = &last->second;

        float a = (time - aLastTime) / (aTime - aLastTime);
        return Ogre::Quaternion::nlerp(a, aLastKey->mValue, aKey->mValue);
    }
    else
        return keys.rbegin()->second.mValue;
}

Ogre::Quaternion NifOgre::TransformController::Value::getXYZRotation (float time) const
{
    float xrot = interpKey(mXRotations->mKeys, time);
    float yrot = interpKey(mYRotations->mKeys, time);
    float zrot = interpKey(mZRotations->mKeys, time);
    Ogre::Quaternion xr(Ogre::Radian(xrot), Ogre::Vector3::UNIT_X);
    Ogre::Quaternion yr(Ogre::Radian(yrot), Ogre::Vector3::UNIT_Y);
    Ogre::Quaternion zr(Ogre::Radian(zrot), Ogre::Vector3::UNIT_Z);
    return (zr*yr*xr);
}

// data may be:
//   BSTreadTransfInterpolator
//   NiBSplineInterpolator
//     NiBSplineFloatInterpolator
//     NiBSplinePoint3Interpolator
//     NiBSplineTransformInterpolator
//   NiBlendInterpolator
//     NiBlendBoolInterpolator
//     NiBlendFloatInterpolator
//     NiBlendPoint3Interpolator
//     NiBlendTransformInterpolator
//   NiKeyBasedInterpolator
//     NiBoolInterpolator
//     NiFloatInterpolator
//     NiPathInterpolator
//     NiPoint3Interpolator
//     NiTransformInterpolator
//   NiLookAtInterpolator
NifOgre::TransformController::Value::Value (Ogre::Node *target,
        const Nif::NIFFilePtr& nif, const Nif::NiInterpolator *data)
  : NodeTargetValue<Ogre::Real>(target)
  , mRotations(NULL)
  , mXRotations(NULL)
  , mYRotations(NULL)
  , mZRotations(NULL)
  , mTranslations(NULL)
  , mScales(NULL)
  , mData(data)
  , mNif(nif)
{ }

Ogre::Quaternion NifOgre::TransformController::Value::getRotation (float time) const
{
    if(mRotations->mKeys.size() > 0)
        return interpKey(mRotations->mKeys, time);
    else if (!mXRotations->mKeys.empty() || !mYRotations->mKeys.empty() || !mZRotations->mKeys.empty())
        return getXYZRotation(time);
    return mNode->getOrientation();
}

Ogre::Vector3 NifOgre::TransformController::Value::getTranslation (float time) const
{
    if(mTranslations->mKeys.size() > 0)
        return interpKey(mTranslations->mKeys, time);
    return mNode->getPosition();
}

Ogre::Vector3 NifOgre::TransformController::Value::getScale (float time) const
{
    if(mScales->mKeys.size() > 0)
        return Ogre::Vector3(interpKey(mScales->mKeys, time));
    return mNode->getScale();
}

Ogre::Real NifOgre::TransformController::Value::getValue () const
{
    // Should not be called
    return 0.0f;
}

void NifOgre::TransformController::Value::setValue (Ogre::Real time)
{
    if(mRotations->mKeys.size() > 0)
        mNode->setOrientation(interpKey(mRotations->mKeys, time));
    else if (!mXRotations->mKeys.empty() || !mYRotations->mKeys.empty() || !mZRotations->mKeys.empty())
        mNode->setOrientation(getXYZRotation(time));
    if(mTranslations->mKeys.size() > 0)
        mNode->setPosition(interpKey(mTranslations->mKeys, time));
    if(mScales->mKeys.size() > 0)
        mNode->setScale(Ogre::Vector3(interpKey(mScales->mKeys, time)));
}

Ogre::Quaternion NifOgre::KeyframeController::Value::interpKey (const Nif::QuaternionKeyMap::MapType &keys, float time)
{
    if(time <= keys.begin()->first)
        return keys.begin()->second.mValue;

    Nif::QuaternionKeyMap::MapType::const_iterator it = keys.lower_bound(time);
    if (it != keys.end())
    {
        float aTime = it->first;
        const Nif::QuaternionKey* aKey = &it->second;

        assert (it != keys.begin()); // Shouldn't happen, was checked at beginning of this function

        Nif::QuaternionKeyMap::MapType::const_iterator last = --it;
        float aLastTime = last->first;
        const Nif::QuaternionKey* aLastKey = &last->second;

        float a = (time - aLastTime) / (aTime - aLastTime);
        return Ogre::Quaternion::nlerp(a, aLastKey->mValue, aKey->mValue);
    }
    else
        return keys.rbegin()->second.mValue;
}

Ogre::Quaternion NifOgre::KeyframeController::Value::getXYZRotation (float time) const
{
    float xrot = interpKey(mXRotations->mKeys, time);
    float yrot = interpKey(mYRotations->mKeys, time);
    float zrot = interpKey(mZRotations->mKeys, time);
    Ogre::Quaternion xr(Ogre::Radian(xrot), Ogre::Vector3::UNIT_X);
    Ogre::Quaternion yr(Ogre::Radian(yrot), Ogre::Vector3::UNIT_Y);
    Ogre::Quaternion zr(Ogre::Radian(zrot), Ogre::Vector3::UNIT_Z);
    return (zr*yr*xr);
}

NifOgre::KeyframeController::Value::Value (Ogre::Node *target,
        const Nif::NIFFilePtr& nif, const Nif::NiKeyframeData *data)
  : NodeTargetValue<Ogre::Real>(target)
  , mRotations(&data->mRotations)
  , mXRotations(&data->mXRotations)
  , mYRotations(&data->mYRotations)
  , mZRotations(&data->mZRotations)
  , mTranslations(&data->mTranslations)
  , mScales(&data->mScales)
  , mNif(nif)
{ }

Ogre::Quaternion NifOgre::KeyframeController::Value::getRotation (float time) const
{
    if(mRotations->mKeys.size() > 0)
        return interpKey(mRotations->mKeys, time);
    else if (!mXRotations->mKeys.empty() || !mYRotations->mKeys.empty() || !mZRotations->mKeys.empty())
        return getXYZRotation(time);
    return mNode->getOrientation();
}

Ogre::Vector3 NifOgre::KeyframeController::Value::getTranslation (float time) const
{
    if(mTranslations->mKeys.size() > 0)
        return interpKey(mTranslations->mKeys, time);
    return mNode->getPosition();
}

Ogre::Vector3 NifOgre::KeyframeController::Value::getScale (float time) const
{
    if(mScales->mKeys.size() > 0)
        return Ogre::Vector3(interpKey(mScales->mKeys, time));
    return mNode->getScale();
}

Ogre::Real NifOgre::KeyframeController::Value::getValue () const
{
    // Should not be called
    return 0.0f;
}

void NifOgre::KeyframeController::Value::setValue (Ogre::Real time)
{
    if(mRotations->mKeys.size() > 0)
        mNode->setOrientation(interpKey(mRotations->mKeys, time));
    else if (!mXRotations->mKeys.empty() || !mYRotations->mKeys.empty() || !mZRotations->mKeys.empty())
        mNode->setOrientation(getXYZRotation(time));
    if(mTranslations->mKeys.size() > 0)
        mNode->setPosition(interpKey(mTranslations->mKeys, time));
    if(mScales->mKeys.size() > 0)
        mNode->setScale(Ogre::Vector3(interpKey(mScales->mKeys, time)));
}

NifOgre::UVController::Value::Value (Ogre::MovableObject* movable,
        const Nif::NiUVData *data, MaterialControllerManager* materialControllerMgr)
  : mMovable(movable)
  , mUTrans(data->mKeyList[0])
  , mVTrans(data->mKeyList[1])
  , mUScale(data->mKeyList[2])
  , mVScale(data->mKeyList[3])
  , mMaterialControllerMgr(materialControllerMgr)
{ }

Ogre::Real NifOgre::UVController::Value::getValue () const
{
    // Should not be called
    return 1.0f;
}

void NifOgre::UVController::Value::setValue (Ogre::Real value)
{
    float uTrans = interpKey(mUTrans.mKeys, value, 0.0f);
    float vTrans = interpKey(mVTrans.mKeys, value, 0.0f);
    float uScale = interpKey(mUScale.mKeys, value, 1.0f);
    float vScale = interpKey(mVScale.mKeys, value, 1.0f);

    Ogre::MaterialPtr material = mMaterialControllerMgr->getWritableMaterial(mMovable);

    Ogre::Material::TechniqueIterator techs = material->getTechniqueIterator();
    while(techs.hasMoreElements())
    {
        Ogre::Technique *tech = techs.getNext();
        Ogre::Technique::PassIterator passes = tech->getPassIterator();
        while(passes.hasMoreElements())
        {
            Ogre::Pass *pass = passes.getNext();
            Ogre::TextureUnitState *tex = pass->getTextureUnitState(0);
            tex->setTextureScroll(uTrans, vTrans);
            tex->setTextureScale(uScale, vScale);
        }
    }
}

NifOgre::ParticleSystemController::Value::Value (Ogre::ParticleSystem *psys,
        const Nif::NiParticleSystemController *pctrl)
  : mParticleSys(psys)
  , mEmitStart(pctrl->startTime)
  , mEmitStop(pctrl->stopTime)
{
}

Ogre::Real NifOgre::ParticleSystemController::Value::getValue () const
{
    return 0.0f;
}

void NifOgre::ParticleSystemController::Value::setValue (Ogre::Real value)
{
    mParticleSys->setEmitting(value >= mEmitStart && value < mEmitStop);
}

NifOgre::GeomMorpherController::Value::Value (Ogre::Entity *ent, const Nif::NiMorphData *data, size_t controllerIndex)
  : mEntity(ent)
  , mMorphs(data->mMorphs)
  , mControllerIndex(controllerIndex)
{
}

Ogre::Real NifOgre::GeomMorpherController::Value::getValue () const
{
    // Should not be called
    return 0.0f;
}

void NifOgre::GeomMorpherController::Value::setValue (Ogre::Real time)
{
    if (mMorphs.size() <= 1)
        return;
    int i = 1;
    for (std::vector<Nif::NiMorphData::MorphData>::iterator it = mMorphs.begin()+1; it != mMorphs.end(); ++it,++i)
    {
        float val = 0;
        if (!it->mData.mKeys.empty())
            val = interpKey(it->mData.mKeys, time);
        val = std::max(0.f, std::min(1.f, val));

        Ogre::String animationID = Ogre::StringConverter::toString(mControllerIndex)
                + "_" + Ogre::StringConverter::toString(i);

        Ogre::AnimationState* state = mEntity->getAnimationState(animationID);
        state->setEnabled(val > 0);
        state->setWeight(val);
    }
}
