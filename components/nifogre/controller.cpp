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

namespace
{
// From https://github.com/niftools/nifskope

/*********************************************************************
Simple b-spline curve algorithm

Copyright 1994 by Keith Vertanen (vertankd@cda.mrs.umn.edu)

Released to the public domain (your mileage may vary)

Found at: Programmers Heaven (www.programmersheaven.com/zone3/cat415/6660.htm)
(Seems to be here now: https://www.keithv.com/software/3dpath/spline.cpp)
Modified by: Theo
- reformat and convert doubles to floats
- removed point structure in favor of arbitrary sized float array
Further modified by: cc9cii
- use std::vector with offset rather than QModelIndex
- changed Vector3 and Quat to Ogre::Vector3 and Ogre::Quaternion
**********************************************************************/
#if 0
/*! Used to enable static arrays to be members of vectors */
template <typename T>
struct qarray
{
    qarray( const QModelIndex & array, uint off = 0 )
        : array_( array ), off_( off )
    {
        nif_ = static_cast<const NifModel *>( array_.model() );
    }
    qarray( const qarray & other, uint off = 0 )
        : nif_( other.nif_ ), array_( other.array_ ), off_( other.off_ + off )
    {
    }

    T operator[]( uint index ) const
    {
        return nif_->get<T>( array_.child( index + off_, 0 ) );
    }
    const NifModel * nif_;
    const QModelIndex & array_;
    uint off_;
};
#endif

template <typename T>
struct SplineTraits
{
    // Zero data
    static T & Init( T & v )
    {
        v = T();
        return v;
    }

    // Number of control points used
    static int CountOf()
    {
        return ( sizeof(T) / sizeof(float) );
    }

    // Compute point from short array and mult/bias
    static T & Compute( T & v, const std::vector<short> & c, unsigned int off, float mult )
    {
        float * vf = (float *)&v; // assume default data is a vector of floats. specialize if necessary.

        for ( int i = 0; i < CountOf(); ++i )
            vf[i] = vf[i] + ( float(c.at(off+i)) / float(SHRT_MAX) ) * mult;

        return v;
    }
    static T & Adjust( T & v, float mult, float bias )
    {
        float * vf = (float *)&v;  // assume default data is a vector of floats. specialize if necessary.

        for ( int i = 0; i < CountOf(); ++i )
            vf[i] = vf[i] * mult + bias;

        return v;
    }
};

template <> struct SplineTraits<Ogre::Vector3>
{
    static Ogre::Vector3 & Init( Ogre::Vector3 & v )
    {
        v = Ogre::Vector3();
        return v;
    }
    static int CountOf() { return 3; }
    static Ogre::Vector3 & Compute( Ogre::Vector3 & v, const std::vector<short> & c, unsigned int off, float mult )
    {
        v.x = v.x + ( float(c.at(off+0)) / float(SHRT_MAX) ) * mult;
        v.y = v.y + ( float(c.at(off+1)) / float(SHRT_MAX) ) * mult;
        v.z = v.z + ( float(c.at(off+2)) / float(SHRT_MAX) ) * mult;

        return v;
    }
    static Ogre::Vector3 & Adjust( Ogre::Vector3 & v, float mult, float bias )
    {
        v.x = v.x * mult + bias;
        v.y = v.y * mult + bias;
        v.z = v.z * mult + bias;

        return v;
    }
};

template <> struct SplineTraits<Ogre::Quaternion>
{
    static Ogre::Quaternion & Init( Ogre::Quaternion & v )
    {
        v = Ogre::Quaternion();
        v.w = 0.0f;
        return v;
    }
    static int CountOf() { return 4; }
    // mult is from blend()
    static Ogre::Quaternion & Compute( Ogre::Quaternion & v, const std::vector<short> & c, unsigned int off, float mult )
    {
        v.w = v.w + ( float(c.at(off+0)) / float(SHRT_MAX) ) * mult;
        v.x = v.x + ( float(c.at(off+1)) / float(SHRT_MAX) ) * mult;
        v.y = v.y + ( float(c.at(off+2)) / float(SHRT_MAX) ) * mult;
        v.z = v.z + ( float(c.at(off+3)) / float(SHRT_MAX) ) * mult;

        return v;
    }
    static Ogre::Quaternion & Adjust( Ogre::Quaternion & v, float mult, float bias )
    {
        v.w = v.w * mult + bias;
        v.x = v.x * mult + bias;
        v.y = v.y * mult + bias;
        v.z = v.z * mult + bias;

        return v;
    }
};

// calculate the blending value
static float blend( int k, int t, int * u, float v )
{
    float value;

    if ( t == 1 ) {
        // base case for the recursion
        value = ( ( u[k] <= v ) && ( v < u[k + 1] ) ) ? 1.0f : 0.0f;
    } else {
        if ( ( u[k + t - 1] == u[k] ) && ( u[k + t] == u[k + 1] ) ) // check for divide by zero
            value = 0;
        else if ( u[k + t - 1] == u[k] )                            // if a term's denominator is zero,use just the other
            value = ( u[k + t] - v) / ( u[k + t] - u[k + 1] ) * blend( k + 1, t - 1, u, v );
        else if ( u[k + t] == u[k + 1] )
            value = (v - u[k]) / (u[k + t - 1] - u[k]) * blend( k, t - 1, u, v );
        else
            value = ( v - u[k] ) / ( u[k + t - 1] - u[k] ) * blend( k, t - 1, u, v )
                    + ( u[k + t] - v ) / ( u[k + t] - u[k + 1] ) * blend( k + 1, t - 1, u, v );
    }

    return value;
}

// figure out the knots
static void compute_intervals( int * u, int n, int t )
{
    for ( int j = 0; j <= n + t; j++ ) {
        if ( j < t )
            u[j] = 0;
        else if ( ( t <= j ) && ( j <= n ) )
            u[j] = j - t + 1;
        else if ( j > n )
            u[j] = n - t + 2;  // if n-t=-2 then we're screwed, everything goes to 0
    }
}

template <typename T>
static void compute_point( int * u, int n, int t, float v, const std::vector<short> & control, unsigned int off, T & output, float mult, float bias )
{
    // initialize the variables that will hold our output
    int l = SplineTraits<T>::CountOf();
    SplineTraits<T>::Init( output );

    for ( int k = 0; k <= n; k++ ) {
        SplineTraits<T>::Compute( output, control, off+k*l, blend( k, t, u, v ) );
    }

    SplineTraits<T>::Adjust( output, mult, bias );
}

template <typename T>
bool bsplineinterpolate( T & value, int degree, float interval, unsigned int nctrl, const std::vector<short> & array, unsigned int off, float mult, float bias )
{
    if ( off == USHRT_MAX )
        return false;

    int t = degree + 1;
    int n = nctrl - 1;
    int l = SplineTraits<T>::CountOf();

    if ( interval >= float(nctrl - degree) ) {
        SplineTraits<T>::Init( value );
        SplineTraits<T>::Compute( value, array, off+n*l, 1.0f );
        SplineTraits<T>::Adjust( value, mult, bias );
    } else {
        int * u = new int[ n + t + 1 ];
        compute_intervals( u, n, t );
        compute_point( u, n, t, interval, array, off, value, mult, bias );
        delete [] u;
    }

    return true;
}
} // namespace

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
#if 0
NifOgre::InterpolateFunction (const Nif::NiInterpolator *interp, bool deltaInput)
    ; mInterpolator(interp)
{
}

Ogre::Real NifOgre::InterpolateFunction::calculate(Ogre::Real value)
{
}
#endif
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
#if 0

// FIXME: is this equivalent of DefaultFunction::calculate()?
float Controller::ctrlTime( float time ) const
{
    time = frequency * time + phase;

    if ( time >= start && time <= stop )
        return time;

    switch ( extrapolation ) {
    case Cyclic:
        {
            float delta = stop - start;

            if ( delta <= 0 )
                return start;

            float x = ( time - start ) / delta;
            float y = ( x - floor( x ) ) * delta;

            return start + y;
        }
    case Reverse:
        {
            float delta = stop - start;

            if ( delta <= 0 )
                return start;

            float x = ( time - start ) / delta;
            float y = ( x - floor( x ) ) * delta;

            if ( ( ( (int)fabs( floor( x ) ) ) & 1 ) == 0 )
                return start + y;

            return stop - y;
        }
    case Constant:
    default:

        if ( time < start )
            return start;

        if ( time > stop )
            return stop;

        return time;
    }
}

void TransformController::updateTime( float time )
{
    if ( !(active && target) )
        return;

    time = ctrlTime( time );

    if ( interpolator ) {
        interpolator->updateTransform( target->local, time );
    }
}

void TransformController::setInterpolator( const QModelIndex & iBlock )
{
    const NifModel * nif = static_cast<const NifModel *>(iBlock.model());

    if ( nif && iBlock.isValid() ) {
        if ( interpolator ) {
            delete interpolator;
            interpolator = 0;
        }

        if ( nif->isNiBlock( iBlock, "NiBSplineCompTransformInterpolator" ) ) {
            iInterpolator = iBlock;
            interpolator = new BSplineTransformInterpolator( this );
        } else if ( nif->isNiBlock( iBlock, "NiTransformInterpolator" ) ) {
            iInterpolator = iBlock;
            interpolator = new TransformInterpolator( this );
        }

        if ( interpolator ) {
            interpolator->update( nif, iInterpolator );
        }
    }
}

bool Controller::timeIndex( float time, const NifModel * nif, const QModelIndex & array, int & i, int & j, float & x )
{
    int count;

    if ( array.isValid() && ( count = nif->rowCount( array ) ) > 0 ) {
        if ( time <= nif->get<float>( array.child( 0, 0 ), "Time" ) ) {
            i = j = 0;
            x = 0.0;

            return true;
        }

        if ( time >= nif->get<float>( array.child( count - 1, 0 ), "Time" ) ) {
            i = j = count - 1;
            x = 0.0;

            return true;
        }

        if ( i < 0 || i >= count )
            i = 0;

        float tI = nif->get<float>( array.child( i, 0 ), "Time" );

        if ( time > tI ) {
            j = i + 1;
            float tJ;

            while ( time >= ( tJ = nif->get<float>( array.child( j, 0 ), "Time" ) ) ) {
                i  = j++;
                tI = tJ;
            }

            x = ( time - tI ) / ( tJ - tI );

            return true;
        } else if ( time < tI ) {
            j = i - 1;
            float tJ;

            while ( time <= ( tJ = nif->get<float>( array.child( j, 0 ), "Time" ) ) ) {
                i  = j--;
                tI = tJ;
            }

            x = ( time - tI ) / ( tJ - tI );

            // Quadratic Bug Fix

            // Invert x
            //    Previously, this branch was causing x to decrement from 1.0.
            //    (This works fine for linear interpolation apparently)
            x = 1.0 - x;

            // Swap I and J
            //    With x inverted, we must swap I and J or the animation will reverse.
            auto tmpI = i;
            i = j;
            j = tmpI;

            // End Bug Fix

            return true;
        }

        j = i;
        x = 0.0;

        return true;
    }

    return false;
}

template <typename T> bool interpolate( T & value, const QModelIndex & array, float time, int & last )
{
    const NifModel * nif = static_cast<const NifModel *>( array.model() );

    if ( nif && array.isValid() ) {
        QModelIndex frames = nif->getIndex( array, "Keys" );
        int next;
        float x;

        if ( Controller::timeIndex( time, nif, frames, last, next, x ) ) {
            T v1 = nif->get<T>( frames.child( last, 0 ), "Value" );
            T v2 = nif->get<T>( frames.child( next, 0 ), "Value" );

            switch ( nif->get<int>( array, "Interpolation" ) ) {

            case 2:
            {
                // Quadratic
                /*
                    In general, for keyframe values v1 = 0, v2 = 1 it appears that
                    setting v1's corresponding "Backward" value to 1 and v2's
                    corresponding "Forward" to 1 results in a linear interpolation.
                */

                // Tangent 1
                float t1 = nif->get<float>( frames.child( last, 0 ), "Backward" );
                // Tangent 2
                float t2 = nif->get<float>( frames.child( next, 0 ), "Forward" );

                float x2 = x * x;
                float x3 = x2 * x;

                // Cubic Hermite spline
                //    x(t) = (2t^3 - 3t^2 + 1)P1  + (-2t^3 + 3t^2)P2 + (t^3 - 2t^2 + t)T1 + (t^3 - t^2)T2

                value = v1 * (2.0f * x3 - 3.0f * x2 + 1.0f) + v2 * (-2.0f * x3 + 3.0f * x2) + t1 * (x3 - 2.0f * x2 + x) + t2 * (x3 - x2);

            }    return true;

            case 5:
                // Constant
                if ( x < 0.5 )
                    value = v1;
                else
                    value = v2;

                return true;
            default:
                value = v1 + ( v2 - v1 ) * x;
                return true;
            }
        }
    }

    return false;
}
#endif
NifOgre::TransformController::Value::Value (Ogre::Node *target,
        const Nif::NIFFilePtr& nif, const Nif::NiInterpolator *interp)
  : NodeTargetValue<Ogre::Real>(target)
  , mRotations(NULL)
  , mXRotations(NULL)
  , mYRotations(NULL)
  , mZRotations(NULL)
  , mTranslations(NULL)
  , mScales(NULL)
  , mInterpolator(interp)
  , mNif(nif)
    , mLastRotate(0)
    , mLastTranslate(0)
    , mLastScale(0)
{
    const Nif::NiTransformInterpolator* transInterp = dynamic_cast<const Nif::NiTransformInterpolator*>(interp);
    if (transInterp)
    {
        const Nif::NiTransformData* transData = transInterp->transformData.getPtr();
        if (transData)
        {
            mRotations = &transData->mRotations;
            mXRotations = &transData->mXRotations;
            mYRotations = &transData->mYRotations;
            mZRotations = &transData->mZRotations;
            mTranslations = &transData->mTranslations;
            mScales = &transData->mScales;
        }
    }
}

Ogre::Quaternion NifOgre::TransformController::Value::getRotation (float time) const
{
    const Nif::NiTransformInterpolator*
        transInterp = dynamic_cast<const Nif::NiTransformInterpolator*>(mInterpolator);
    if (transInterp)
    {
        const Nif::NiTransformData* transData = transInterp->transformData.getPtr();
        if (!transData)
            return mNode->getOrientation();

        if(mRotations->mKeys.size() > 0)
            return interpKey(mRotations->mKeys, time);
        else if (!mXRotations->mKeys.empty() || !mYRotations->mKeys.empty() || !mZRotations->mKeys.empty())
            return getXYZRotation(time);

        return mNode->getOrientation();
    }

    const Nif::NiBSplineCompTransformInterpolator* bsi
        = dynamic_cast<const Nif::NiBSplineCompTransformInterpolator*>(mInterpolator);
    if (bsi)
    {
        const Nif::NiBSplineData* sd = bsi->splineData.getPtr();
        const Nif::NiBSplineBasisData* bd = bsi->basisData.getPtr();
        if (!sd || !bd)
            return mNode->getOrientation();

        unsigned int nCtrl = bd->numControlPoints;
        int degree = 3; // degree of the polynomial
        float interval
            = ( ( time - bsi->startTime ) / ( bsi->stopTime - bsi->startTime ) ) * float(nCtrl - degree);

        //Ogre::Quaternion q = Ogre::Quaternion();
        Ogre::Quaternion q = mNode->getOrientation(); // FIXME which?

        bsplineinterpolate<Ogre::Quaternion>( q, degree, interval, nCtrl, sd->shortControlPoints,
                bsi->rotationOffset, bsi->rotationMultiplier, bsi->rotationBias );

        return q;
    }
    else // float
        return mNode->getOrientation(); // FIXME float
}

Ogre::Vector3 NifOgre::TransformController::Value::getTranslation (float time) const
{
    const Nif::NiTransformInterpolator*
        transInterp = dynamic_cast<const Nif::NiTransformInterpolator*>(mInterpolator);
    if (transInterp)
    {
        const Nif::NiTransformData* transData = transInterp->transformData.getPtr();
        if (!transData)
            return mNode->getPosition();

        if(mTranslations->mKeys.size() > 0)
            return interpKey(mTranslations->mKeys, time);

        return mNode->getPosition();
    }

    const Nif::NiBSplineCompTransformInterpolator* bsi
        = dynamic_cast<const Nif::NiBSplineCompTransformInterpolator*>(mInterpolator);
    if (bsi)
    {
        const Nif::NiBSplineData* sd = bsi->splineData.getPtr();
        const Nif::NiBSplineBasisData* bd = bsi->basisData.getPtr();
        if (!sd || !bd)
            return mNode->getPosition();

        unsigned int nCtrl = bd->numControlPoints;
        int degree = 3; // degree of the polynomial
        float interval
            = ( ( time - bsi->startTime ) / ( bsi->stopTime - bsi->startTime ) ) * float(nCtrl - degree);

        //Ogre::Vector3 v = Ogre::Vector3();
        Ogre::Vector3 v = mNode->getPosition(); // FIXME which?

        bsplineinterpolate<Ogre::Vector3>( v, degree, interval, nCtrl, sd->shortControlPoints,
                bsi->translationOffset, bsi->translationMultiplier, bsi->translationBias );

        return v;
    }
    else // float
        return mNode->getPosition(); // FIXME float
}

Ogre::Vector3 NifOgre::TransformController::Value::getScale (float time) const
{
    const Nif::NiTransformInterpolator*
        transInterp = dynamic_cast<const Nif::NiTransformInterpolator*>(mInterpolator);
    if (transInterp)
    {
        const Nif::NiTransformData* transData = transInterp->transformData.getPtr();
        if (!transData)
            return mNode->getScale();

        if(mScales->mKeys.size() > 0)
            return Ogre::Vector3(interpKey(mScales->mKeys, time));

        return mNode->getScale();
    }

    const Nif::NiBSplineCompTransformInterpolator* bsi
        = dynamic_cast<const Nif::NiBSplineCompTransformInterpolator*>(mInterpolator);
    if (bsi)
    {
        const Nif::NiBSplineData* sd = bsi->splineData.getPtr();
        const Nif::NiBSplineBasisData* bd = bsi->basisData.getPtr();
        if (!sd || !bd)
            return mNode->getScale();

        unsigned int nCtrl = bd->numControlPoints;
        int degree = 3; // degree of the polynomial
        float interval
            = ((time - bsi->startTime) / (bsi->stopTime - bsi->startTime)) * float(nCtrl - degree);
        float scale;

        //Ogre::Vector3 s = Ogre::Vector3();
        Ogre::Vector3 s = mNode->getScale(); // FIXME which?
        scale = s.x; // FIXME: assume uniform scaling

        bsplineinterpolate<float>( scale, degree, interval, nCtrl, sd->shortControlPoints,
                bsi->scaleOffset, bsi->scaleMultiplier, bsi->scaleBias );

        return Ogre::Vector3(scale, scale, scale); // assume uniform scaling
    }
    else // float
        return mNode->getScale(); // FIXME float
}

Ogre::Real NifOgre::TransformController::Value::getValue () const
{
    // Should not be called
    return 0.0f;
}

// Move the target bone (mNode) based on time value
//
// Called from Animation::runAnimation() while applying "group" controllers via
// Ogre::Controller<Ogre::Real>::update()
void NifOgre::TransformController::Value::setValue (Ogre::Real time)
{
    // FIXME: do away with all this dynamic casting
    const Nif::NiTransformInterpolator*
        transInterp = dynamic_cast<const Nif::NiTransformInterpolator*>(mInterpolator);
    if (transInterp)
    {
        const Nif::NiTransformData* transData = transInterp->transformData.getPtr();
        if (!transData)
            return;

        // FIXME: try not to update rotation to Bip01 (but doesn't work...)
        if(mRotations->mKeys.size() > 0 /*&& mNode->getName() != "Bip01"*/)
            mNode->setOrientation(interpKey(mRotations->mKeys, time));
        else if (/*mNode->getName() != "Bip01" && */
                (!mXRotations->mKeys.empty() || !mYRotations->mKeys.empty() || !mZRotations->mKeys.empty()))
            mNode->setOrientation(getXYZRotation(time));

        Ogre::Vector3 old = mNode->getPosition(); // FIXME: debug only
        if(mTranslations->mKeys.size() > 0)
        {
            float dist;
            Ogre::Vector3 pos = interpKey(mTranslations->mKeys, time);
            if ((dist = old.squaredDistance(pos)) < 10)
                mNode->setPosition(pos);
            //else
                //std::cout << "tr " << mNode->getName() << " dist " << dist << ", time " << time << std::endl;
        }

        if(mScales->mKeys.size() > 0)
            mNode->setScale(Ogre::Vector3(interpKey(mScales->mKeys, time)));

        //std::cout << "ok " << time << std::endl;
        return;
    }

    const Nif::NiBSplineCompTransformInterpolator* bsi
        = dynamic_cast<const Nif::NiBSplineCompTransformInterpolator*>(mInterpolator);
    if (bsi)
    {
        const Nif::NiBSplineData* sd = bsi->splineData.getPtr();
        const Nif::NiBSplineBasisData* bd = bsi->basisData.getPtr();
        if (!sd || !bd)
            return;

        unsigned int nCtrl = bd->numControlPoints;
        int degree = 3; // degree of the polynomial TODO why is it 3?
        //if (time >= 6.f) // FIXME temporary testing
            //std::cout << "time " << time << std::endl;
        // for "Idle" Bip01 Pelvis:
        //    interval = ((time - 0) / (6 - 0) * (128 - degree) = time/6 * 125  = time * 20.833
        float interval
            = ((time - bsi->startTime) / (bsi->stopTime - bsi->startTime)) * float(nCtrl - degree);
        float scale;

        Ogre::Quaternion q = Ogre::Quaternion();
        Ogre::Vector3 v = Ogre::Vector3();
        Ogre::Vector3 s = Ogre::Vector3();

        scale = s.x; // FIXME: assume uniform scaling

        if (bsi->rotationOffset != USHRT_MAX && bsi->rotationOffset+nCtrl > sd->shortControlPoints.size())
            std::cout << "rotation overflow" << std::endl; // FIXME: debugging
        if (bsplineinterpolate<Ogre::Quaternion>( q, degree, interval, nCtrl, sd->shortControlPoints,
                bsi->rotationOffset, bsi->rotationMultiplier, bsi->rotationBias )
                /*&& mNode->getName() != "Bip01"*/) // doesn't work
        {
            mNode->setOrientation(q);
        }

        // FIXME should verify that the size of of short vector is greater than offset + nCtrl
        if (bsi->translationOffset != USHRT_MAX && bsi->translationOffset+nCtrl > sd->shortControlPoints.size())
            std::cout << "translation overflow" << std::endl; // FIXME: debugging
        Ogre::Vector3 old = mNode->getPosition(); // FIXME: debug only
        if (bsplineinterpolate<Ogre::Vector3>( v, degree, interval, nCtrl, sd->shortControlPoints,
                    bsi->translationOffset, bsi->translationMultiplier, bsi->translationBias))
        {
            float dist;
            //if ((dist = old.squaredDistance(mNode->getPosition())) < 10) // FIXME: debug only
            if ((dist = old.squaredDistance(v)) < 10) // FIXME: debug only, horrible hack
                mNode->setPosition(v);
            //else
                //std::cout << mNode->getName() << " dist " << dist << ", time " << time << std::endl;
        }

        if (bsi->scaleOffset != USHRT_MAX && bsi->scaleOffset+nCtrl > sd->shortControlPoints.size())
            std::cout << "scale overflow" << std::endl; // FIXME: debugging
        if (bsplineinterpolate<float>( scale, degree, interval, nCtrl, sd->shortControlPoints,
                    bsi->scaleOffset, bsi->scaleMultiplier, bsi->scaleBias ))
        {
            mNode->setScale(Ogre::Vector3(scale, scale, scale)); // assume uniform scaling
        }

        //std::cout << "ok " << time << std::endl;
        return;
    }

    // FIXME float
}

// from NifOgre::ValueInterpolator
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

// from NifOgre::NodeTargetValue<T>
Ogre::Quaternion NifOgre::KeyframeController::Value::getRotation (float time) const
{
    if(mRotations->mKeys.size() > 0)
        return interpKey(mRotations->mKeys, time);
    else if (!mXRotations->mKeys.empty() || !mYRotations->mKeys.empty() || !mZRotations->mKeys.empty())
        return getXYZRotation(time);
    return mNode->getOrientation();
}

// from NifOgre::NodeTargetValue<T>
Ogre::Vector3 NifOgre::KeyframeController::Value::getTranslation (float time) const
{
    if(mTranslations->mKeys.size() > 0)
        return interpKey(mTranslations->mKeys, time);
    return mNode->getPosition();
}

// from NifOgre::NodeTargetValue<T>
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
