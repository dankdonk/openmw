#include "transformcontroller.hpp"

#include <iostream> // FIXME

#include <OgreNode.h>
#include <OgreBone.h> // FIXME: debugging

#include "niinterpolator.hpp"
#include "nimodel.hpp"

// the code below is based on NifSkope gl/glcontroller.cpp
namespace
{
template <typename T>
T interpolate (const NiBtOgre::KeyGroup<T>& keyGroup, uint32_t cycleType, float time)
{
    if(time <= keyGroup.indexMap.begin()->first)
        return keyGroup.keys[keyGroup.indexMap.begin()->second].value;

    std::map<float, int>::const_iterator nextIt = keyGroup.indexMap.lower_bound(time); // >=
    if (nextIt != keyGroup.indexMap.end())
    {
        std::map<float, int>::const_iterator lastIt = nextIt;
        --lastIt; // point to prev key

        float nextTime = nextIt->first;
        float lastTime = lastIt->first;
        float x = (time - lastTime) / (nextTime - lastTime);

        if (keyGroup.interpolation == 5)      // CONST
        {
            if ( x < 0.5f ) // step up at half way (or should it be step up at 1.0f instead?)
                return keyGroup.keys[lastIt->second].value;
            else
                return keyGroup.keys[nextIt->second].value;
        }
        else if (keyGroup.interpolation == 2) // QUADRATIC
        {
            const T& t1 = keyGroup.keys[lastIt->second].backward;
            const T& t2 = keyGroup.keys[nextIt->second].forward;
            const T& v1 = keyGroup.keys[lastIt->second].value;
            const T& v2 = keyGroup.keys[nextIt->second].value;
            float x2 = x * x;
            float x3 = x2 * x;

            // Cubic Hermite spline
            //    x(t) = (2t^3 - 3t^2 + 1)P1  + (-2t^3 + 3t^2)P2 + (t^3 - 2t^2 + t)T1 + (t^3 - t^2)T2
            return  v1 * (2.0f * x3 - 3.0f * x2 + 1.0f) + v2 * (-2.0f * x3 + 3.0f * x2) + t1 * (x3 - 2.0f * x2 + x) + t2 * (x3 - x2);
        }
        else if (keyGroup.interpolation == 1) // LINEAR
        {
            const T& v1 = keyGroup.keys[lastIt->second].value;
            const T& v2 = keyGroup.keys[nextIt->second].value;

            return v1 + (v2 - v1) * x;
        }
        else if (keyGroup.interpolation == 3) // TBC
        {
            // Creatures\Goblin\Skeleton.NIF
            // FIXME: use linear for now
            const T& v1 = keyGroup.keys[lastIt->second].value;
            const T& v2 = keyGroup.keys[nextIt->second].value;

            return v1 + (v2 - v1) * x;
        }
        else
            throw std::runtime_error("Unsupported interpolation type");
    }
    else // no key has >= time; i.e. time is greater than any of the keys'
    {
        // FIXME: do we ever get here?
        if (cycleType == 2) // CYCLE_CLAMP
            return keyGroup.keys[keyGroup.indexMap.rbegin()->second].value;
        else
            return keyGroup.keys[keyGroup.indexMap.begin()->second].value; // wrap around
    }
}

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
        v = Ogre::Vector3(0.f, 0.f, 0.f);
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
        v = Ogre::Quaternion::IDENTITY; v.w = 0.f; return v;
    }
    static int CountOf() { return 4; }
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
} // anon namespace

//      lastKey    interpolate value     nextKey
//       value            :               value
//         :              :                 :
//         v              v                 v
//      lastTime         time            nextTime
//
//        0.0f .......... x .............. 1.0f
//

Ogre::Quaternion NiBtOgre::TransformController::Value::interpQuatKey (
        const NiBtOgre::KeyGroup<Ogre::Quaternion>& keyGroup, uint32_t cycleType, float time)
{
    if (time <= keyGroup.indexMap.begin()->first)
        return keyGroup.keys[keyGroup.indexMap.begin()->second].value;

    std::map<float, int>::const_iterator nextIt = keyGroup.indexMap.lower_bound(time); // >=
    if (nextIt != keyGroup.indexMap.end())
    {
        std::map<float, int>::const_iterator lastIt = nextIt;
        --lastIt; // point to prev key

        float lastTime = lastIt->first;
        float nextTime = nextIt->first;
        float x = (time - lastTime) / (nextTime - lastTime);

        if (keyGroup.interpolation == 5)      // CONST
        {
            if ( x < 0.5f ) // step up at half way (or should it be step up at 1.0f instead?)
                return keyGroup.keys[lastIt->second].value;
            else
                return keyGroup.keys[nextIt->second].value;
        }
        else if (keyGroup.interpolation == 1) // LINEAR
        {
            const Ogre::Quaternion& v1 = keyGroup.keys[lastIt->second].value;
            const Ogre::Quaternion& v2 = keyGroup.keys[nextIt->second].value;

            if (1)
            {
                Ogre::Quaternion v3 = v1;
                if (v1.Dot(v2) < 0)
                    v3 = v1.Inverse();
                return Ogre::Quaternion::nlerp(x, v3, v2, true/*shortestPath*/);
                //return Ogre::Quaternion::Slerp(x, v3, v2); // TODO: test this
            }
            else
            {
                // FIXME: just experimenting
                Ogre::Quaternion r;
                r.x = v1.x + (v2.x - v1.x) * x;
                r.y = v1.y + (v2.y - v1.y) * x;
                r.z = v1.z + (v2.z - v1.z) * x;
                r.w = v1.w + (v2.w - v1.w) * x;
                return r;
            }
        }
        else if (keyGroup.interpolation == 3) // TBC
        {
            // Slaughter Fish
            // FIXME: use linear for now
            const Ogre::Quaternion& v1 = keyGroup.keys[lastIt->second].value;
            const Ogre::Quaternion& v2 = keyGroup.keys[nextIt->second].value;

            return Ogre::Quaternion::nlerp(x, v1, v2);
        }
        else // quadratic interpolation not supported for QuatKey
            throw std::runtime_error("Unsupported interpolation type for Quaternion keys");
    }
    else // no key has >= time; i.e. time is greater than any of the keys' (TODO: should this happen?)
    {
        // FIXME: do we ever get here?
        if (cycleType == 2) // CYCLE_CLAMP
            return keyGroup.keys[keyGroup.indexMap.rbegin()->second].value;
        else
            return keyGroup.keys[keyGroup.indexMap.begin()->second].value; // wrap around
    }
}

Ogre::Quaternion NiBtOgre::TransformController::Value::getXYZRotation (float time) const
{
    if (mInterpolatorType != 1)
        return mNode->getOrientation();

    float xrot = interpolate<float>(mTransformData->mXRotations, 2/*CYCLE_CLAMP*/, time);
    float yrot = interpolate<float>(mTransformData->mYRotations, 2/*CYCLE_CLAMP*/, time);
    float zrot = interpolate<float>(mTransformData->mZRotations, 2/*CYCLE_CLAMP*/, time);
    Ogre::Quaternion xr(Ogre::Radian(xrot), Ogre::Vector3::UNIT_X);
    Ogre::Quaternion yr(Ogre::Radian(yrot), Ogre::Vector3::UNIT_Y);
    Ogre::Quaternion zr(Ogre::Radian(zrot), Ogre::Vector3::UNIT_Z);

    //if (mInterpolator->mRotation.w == -3.40282e+38 || mInterpolator->mRotation.x == -3.40282e+38 ||
    //    mInterpolator->mRotation.y == -3.40282e+38 || mInterpolator->mRotation.z == -3.40282e+38)
    //if (transInterp->mRotation.x < -5000) // some small value
        return (zr*yr*xr);
    //else
        //return transInterp->mRotation.Inverse() * (zr*yr*xr);
}
#if 0
NiBtOgre::TransformController::Value::Value (Ogre::Node *target, const NiModelPtr& model, const NiInterpolator *interpolator)
  : NodeTargetValue(target)
  , mInterpolator(interpolator)
  , mModel(model)
    , mLastRotate(0)
    , mLastTranslate(0)
    , mLastScale(0)
{
    const NiTransformInterpolator* transInterp = dynamic_cast<const NiTransformInterpolator*>(interpolator);
    if (transInterp)
    {
        mTransformData = model->getRef<NiTransformData>(transInterp->mDataRef);
    }
}
#endif
Ogre::Quaternion NiBtOgre::TransformController::Value::getRotation (Ogre::Real time) const
{
    if (mInterpolatorType == 1)
    {
        if (!mTransformData) // shouldn't happen, already checked in NiControllerSequence?!
            return mNode->getOrientation(); // FIXME: log an erorr with mNode->getName()

        if (mTransformData->mQuaternionKeys.keys.size() > 0)
        {
// TODO: test other rotation types
#if 0
            if (mTransformData->mRotationType != 1) // LINEAR_KEY
                throw std::runtime_error("TransformController: unsupported rotation type");
#endif
            return interpQuatKey(mTransformData->mQuaternionKeys, 2/*CYCLE_CLAMP*/, time);
        }
        else if (!mTransformData->mXRotations.keys.empty() ||
                 !mTransformData->mYRotations.keys.empty() || !mTransformData->mZRotations.keys.empty())
        {
            // FIXME: check mRotationType (seems mostly type 4, XYZ_ROTATION_KEY?)
            //std::cout << "TransformData.mRotationType " << mTransformData->mRotationType << std::endl;
            return getXYZRotation(time);
        }

        return mNode->getOrientation(); // no change
    }
    else if (mInterpolatorType == 2)
    {
        const NiBSplineCompTransformInterpolator* bsi
                        = static_cast<const NiBSplineCompTransformInterpolator*>(mInterpolator);

        const NiBSplineData* sd = mModel->getRef<NiBSplineData>(bsi->mSplineDataRef);
        const NiBSplineBasisData* bd = mModel->getRef<NiBSplineBasisData>(bsi->mBasisDataRef);
        if (!sd || !bd) // shouldn't happen, already checked in NiControllerSequence?!
            return mNode->getOrientation(); // FIXME: log an error with mNode->getName()

        unsigned int nCtrl = bd->mNumControlPoints;
        int degree = 3; // degree of the polynomial
        float interval
            = ( ( time - bsi->mStartTime ) / ( bsi->mStopTime - bsi->mStartTime ) ) * float(nCtrl - degree);

        Ogre::Quaternion q = mNode->getOrientation(); // use the last (i.e. current) orientation

        if (bsi->mRotationMultiplier == FLT_MAX || bsi->mRotationOffset == FLT_MAX)
            return mNode->getOrientation();

// FIXME: testing SpiderDaedra
//#if 0
        if (mModel->getModelName().find("pider") != std::string::npos
                &&
                (
                 mNode->getName() == "Bip01 R UpperArm" ||
                 mNode->getName() == "Bip01 R Forearm" ||
                 mNode->getName() == "Bip01 R Hand" ||
                 mNode->getName() == "Bip01 L UpperArm"||
                 mNode->getName() == "Bip01 L Forearm" ||
                 mNode->getName() == "Bip01 L Hand"
                )
        )
        {
            return mNode->getInitialOrientation();
        }
//#endif
        // bsplineinterpolate returns false if bsi->mRotatoinHandle == USHRT_MAX
        if (bsplineinterpolate<Ogre::Quaternion>( q, degree, interval, nCtrl, sd->mShortControlPoints,
                bsi->mRotationHandle, bsi->mRotationMultiplier, bsi->mRotationOffset ))
            return q;

        return mNode->getOrientation();
    }

    return mNode->getOrientation(); // coudn't find an iterpolator
}

Ogre::Vector3 NiBtOgre::TransformController::Value::getTranslation (float time) const
{
    if (mInterpolatorType == 1)
    {
        if (!mTransformData) // shouldn't happen, already checked in NiControllerSequence?!
            return mNode->getPosition(); // FIXME: log an erorr with mNode->getName()

        //FIXME: other translation type
        if(mTransformData->mTranslations.keys.size() > 0)
            return interpolate<Ogre::Vector3>(mTransformData->mTranslations, 2/*CYCLE_CLAMP*/, time);

        return mNode->getPosition();
    }
    else if (mInterpolatorType == 2)
    {
        const NiBSplineCompTransformInterpolator* bsi
                        = static_cast<const NiBSplineCompTransformInterpolator*>(mInterpolator);

        // FIXME: check that time is b/w NiBSplineInterpolator::mStartTime and NiBSplineInterpolator::mStopTime
        if (time > bsi->mStopTime)
            std::cout << "TransformController::Value::getTranslation time " << time
                      << " greater than mStopTime " << bsi->mStopTime << std::endl;

        const NiBSplineData* sd = mModel->getRef<NiBSplineData>(bsi->mSplineDataRef);
        const NiBSplineBasisData* bd = mModel->getRef<NiBSplineBasisData>(bsi->mBasisDataRef);

        if (!sd || !bd) // shouldn't happen, already checked in NiControllerSequence?!
            return mNode->getPosition(); // FIXME: log an error with mNode->getName()

        unsigned int nCtrl = bd->mNumControlPoints;
        int degree = 3; // degree of the polynomial
        float interval
            = ( ( time - bsi->mStartTime ) / ( bsi->mStopTime - bsi->mStartTime ) ) * float(nCtrl - degree);

        Ogre::Vector3 v = mNode->getPosition(); // use the last (i.e. current) position

        if (bsi->mTranslationMultiplier == FLT_MAX || bsi->mTranslationOffset == FLT_MAX)
            return mNode->getPosition(); // cannot interpolate, bail

        // bsplineinterpolate returns false if bsi->mTranslationHandle == USHRT_MAX
        if (bsplineinterpolate<Ogre::Vector3>( v, degree, interval, nCtrl, sd->mShortControlPoints,
                bsi->mTranslationHandle, bsi->mTranslationMultiplier, bsi->mTranslationOffset ))
            return v;

        return mNode->getPosition(); // something went wrong, stay put
    }

    return mNode->getPosition(); // don't have an interpolator
}

Ogre::Vector3 NiBtOgre::TransformController::Value::getScale (float time) const
{
    if (mInterpolatorType == 1)
    {
        const NiTransformInterpolator* transInterp
            = static_cast<const NiTransformInterpolator*>(mInterpolator);
        // shouldn't happen, already checked in NiControllerSequence?!
        if (!mTransformData || transInterp->mScale == FLT_MIN)
            return mNode->getScale(); // FIXME: log an erorr with mNode->getName()

        if(mTransformData->mScales.keys.size() > 0)
            return Ogre::Vector3(interpolate<float>(mTransformData->mScales, 2/*CYCLE_CLAMP*/, time));

        return mNode->getScale();
    }
    else if (mInterpolatorType == 2)
    {
        const NiBSplineCompTransformInterpolator* bsi
                        = static_cast<const NiBSplineCompTransformInterpolator*>(mInterpolator);

        const NiBSplineData* sd = mModel->getRef<NiBSplineData>(bsi->mSplineDataRef);
        const NiBSplineBasisData* bd = mModel->getRef<NiBSplineBasisData>(bsi->mBasisDataRef);
        if (!sd || !bd)
            return mNode->getScale(); // shouldn't happen, already checked in NiControllerSequence?!

        unsigned int nCtrl = bd->mNumControlPoints;
        int degree = 3; // degree of the polynomial
        float interval
            = ((time - bsi->mStartTime) / (bsi->mStopTime - bsi->mStartTime)) * float(nCtrl - degree);
        float scale;

        Ogre::Vector3 s = mNode->getScale();

        if (bsi->mScaleMultiplier == FLT_MAX || bsi->mScaleOffset == FLT_MAX)
            return mNode->getScale(); // cannot interpolate, bail

        scale = s.x; // FIXME: assume uniform scaling
        // bsplineinterpolate returns false if bsi->mScaleHandle == USHRT_MAX
        bsplineinterpolate<float>( scale, degree, interval, nCtrl, sd->mShortControlPoints,
                bsi->mScaleHandle, bsi->mScaleMultiplier, bsi->mScaleOffset );

        return Ogre::Vector3(scale, scale, scale); // assume uniform scaling
    }

    return mNode->getScale();
}

Ogre::Real NiBtOgre::TransformController::Value::getValue () const
{
    // Should not be called
    return 0.0f;
}

// Move the target bone (mNode) based on time value
//
// Called from Animation::runAnimation() while applying "group" controllers via
// Ogre::Controller<Ogre::Real>::update()
void NiBtOgre::TransformController::Value::setValue (Ogre::Real time)
{
#if 0
    if (0)//mNode->getName() == "Bip01 R Calf")
        //throw std::runtime_error("mNode lost name");
        std::cout << "setValue : bone handle " << static_cast<Ogre::Bone*>(mNode)->getHandle() << std::endl;
    if (0)//!static_cast<Ogre::Bone*>(mNode)->isManuallyControlled())
    {
        std::cout << "not manually controlled " << mNode->getName() << std::endl;
        static_cast<Ogre::Bone*>(mNode)->setManuallyControlled(true);
    }
    if (0)//mBoneName == "Bip01 L Hand")// || mBoneName == "Bip01 R Hand")
    {
        std::cout << mModel->getModelName() << " " << mBoneName << " time " << time << std::endl;
        //return;
    }

    if (mInterpolatorType == 1)
    {
#   if 1
        Ogre::Quaternion nonaccumq;
        // FIXME: try not to update rotation to Bip01 (but doesn't work...)
        if(mTransformData && mTransformData->mQuaternionKeys.keys.size() > 0 /*&& mNode->getName() != "Bip01"*/)
        {
            //Ogre::Quaternion q = interpQuatKey(mTransformData->mQuaternionKeys, 2, time);

            //if (transInterp->mRotation.x < -5000) // some small value
            //if (q.isNaN() || q == Ogre::Quaternion::ZERO) // FIXME: does this happen?
                //throw std::runtime_error("transInterp quaternion nan or zero");

            if (mBoneName == "Bip01 NonAccum") // HACK
            {
                mNode->setOrientation(mNode->getOrientation());
                nonaccumq = interpQuatKey(mTransformData->mQuaternionKeys, 2, time);
            }
            else
            {
#if 0
                if (0)//mBoneName == "Bip01 L Hand")// || mBoneName == "Bip01 R Hand")
                {
                    //Ogre::Quaternion h = q * Ogre::Quaternion(Ogre::Degree(90), Ogre::Vector3::UNIT_X);
                    Ogre::Quaternion h = mNode->getOrientation();
                    std::cout << "node " << time
                    << " " << h.x << " " << h.y << " " << h.z << " " << h.w << std::endl;
                    Ogre::Vector3 vv = mNode->_getDerivedPosition();
                    std::cout << "vec " << vv.x << " " << vv.y << " " << vv.z << std::endl;
                    std::cout << "time " << time
                    << " " << q.x << " " << q.y << " " << q.z << " " << q.w << std::endl;
                    mNode->setOrientation(q);
                }
                else
#endif
                    mNode->setOrientation(interpQuatKey(mTransformData->mQuaternionKeys, 2, time));


                if (0)//mBoneName == "Bip01 L Finger3")
                    std::cout << /*"parent dist^2 "*/ mNode->getParent()->getName() << " " << mNode->getParent()->_getDerivedPosition().squaredDistance(mNode->_getDerivedPosition()) << std::endl;




            }
            //else
                //mNode->setOrientation(transInterp->mRotation.Inverse() * q);
        }
        else if (mTransformData && /*mNode->getName() != "Bip01" && */
                (!mTransformData->mXRotations.keys.empty() || !mTransformData->mYRotations.keys.empty() || !mTransformData->mZRotations.keys.empty()))
        {
            if (mBoneName == "Bip01 NonAccum") // HACK
            {
                mNode->setOrientation(mNode->getOrientation());
                nonaccumq = getXYZRotation(time);
            }
            else
                mNode->setOrientation(getXYZRotation(time));
        }
        else if (!mTransformData)
        {
            //mNode->setOrientation(transInterp->mRotation);
            //mNode->setOrientation(mNode->getOrientation());
            Ogre::Node *parent = mNode->getParent();
            if (parent)
            {
                //mNode->setOrientation(transInterp->mRotation * parent->_getDerivedOrientation());
                //mNode->setOrientation(transInterp->mRotation * parent->getOrientation());
                //mNode->setOrientation(parent->getOrientation());
                //mNode->setOrientation(parent->getOrientation() * transInterp->mRotation);
                //mNode->setOrientation(Ogre::Quaternion::ZERO);//transInterp->mRotation);
                Ogre::Quaternion test = mNode->getInitialOrientation();
                mNode->setOrientation(test);
            }
        }

        //Ogre::Vector3 old = mNode->getPosition(); // FIXME: debug only
        if(mTransformData && mTransformData->mTranslations.keys.size() > 0)
        {
            //float dist;
            Ogre::Vector3 pos = interpolate<Ogre::Vector3>(mTransformData->mTranslations, 2/*CYCLE_CLAMP*/, time);
            //pos = -transInterp->mTranslation + pos;

            // FIXME: hack
            //if ((dist = old.squaredDistance(pos)) < 10)
            if (mBoneName != "Bip01 NonAccum") // HACK
                mNode->setPosition(pos);
            else
            {
                mNode->setPosition(nonaccumq.Inverse()*pos);
            }
//          {
//              Ogre::Vector3 pos2 = mNode->getPosition();
//              pos.z = pos2.z;
//              mNode->setPosition(pos);
//          }
//          //else
                //std::cout << "tr " << mNode->getName() << " dist " << dist << ", time " << time << std::endl;
        }
        else if (!mTransformData)
        {
            //mNode->setPosition(transInterp->mTranslation);
            //mNode->setPosition(mNode->getPosition());
            Ogre::Node *parent = mNode->getParent();
            if (parent)
            {
                //mNode->setPosition(parent->_getDerivedPosition() + transInterp->mTranslation);
                //mNode->setPosition(parent->getPosition());
                //mNode->setPosition(parent->getPosition() + transInterp->mTranslation);
                //mNode->setPosition(parent->_getFullTransform() * transInterp->mTranslation);
                //mNode->setPosition(Ogre::Vector3(0.f, 0.f, 0.f));//transInterp->mTranslation);
                Ogre::Vector3 test = mNode->getInitialPosition();
                mNode->setPosition(test);
            }
        }

        if(mTransformData && mTransformData->mScales.keys.size() > 0)
            mNode->setScale(Ogre::Vector3(interpolate<float>(mTransformData->mScales, 2/*CYCLE_CLAMP*/, time)));

        return;
#   else
    if (mBoneName == "Bip01 NonAccum") // HACK
    {
        mNode->setOrientation(mNode->getOrientation()); // nochange

        Ogre::Quaternion non_accum_q;
        non_accum_q = getRotation(time);
        mNode->setPosition(non_accum_q.Inverse() * getTranslation(time));
    }
    else
    {
        mNode->setOrientation(getRotation(time));
        mNode->setPosition(getTranslation(time));
    }
    mNode->setScale(getScale(time));
#   endif
    }
    else if (mInterpolatorType == 2)
    {
        const NiBSplineCompTransformInterpolator* bsi
                        = static_cast<const NiBSplineCompTransformInterpolator*>(mInterpolator);
        //if (mModel->getModelName().find("pider") != std::string::npos && mNode->getName() == "Bip01 R UpperArm")
            //std::cout << "spider" << std::endl;

        const NiBSplineData* sd = mModel->getRef<NiBSplineData>(bsi->mSplineDataRef);
        const NiBSplineBasisData* bd = mModel->getRef<NiBSplineBasisData>(bsi->mBasisDataRef);
        if (!sd || !bd)
            return;// mNode->getOrientation(); // shouldn't happen, already checked in NiControllerSequence

        unsigned int nCtrl = bd->mNumControlPoints;
        int degree = 3; // degree of the polynomial TODO why is it 3?
        //if (time >= 6.f) // FIXME temporary testing
            //std::cout << "time " << time << std::endl;
        // for "Idle" Bip01 Pelvis:
        //    interval = ((time - 0) / (6 - 0) * (128 - degree) = time/6 * 125  = time * 20.833
        float interval
            = ((time - bsi->mStartTime) / (bsi->mStopTime - bsi->mStartTime)) * (float)(nCtrl - degree);
        float scale;

        Ogre::Quaternion q = mNode->getOrientation();//Ogre::Quaternion();
        Ogre::Vector3 v = mNode->getPosition();//Ogre::Vector3();
        Ogre::Vector3 s = mNode->getScale();//Ogre::Vector3();

        scale = s.x; // FIXME: assume uniform scaling

        if (bsi->mRotationHandle != USHRT_MAX && bsi->mRotationHandle + nCtrl > sd->mShortControlPoints.size())
            std::cout << "rotation overflow" << std::endl; // FIXME: debugging

        //if (bsi->mRotationMultiplier != FLT_MAX && bsi->mRotationOffset != FLT_MAX) {
        if (bsplineinterpolate<Ogre::Quaternion>(q, degree, interval, nCtrl, sd->mShortControlPoints,
            bsi->mRotationHandle, bsi->mRotationMultiplier, bsi->mRotationOffset))
        {
        if (0)//mModel->getModelName().find("pider") != std::string::npos && mNode->getName() == "Bip01 R UpperArm")
            //std::cout << "R UpperArm, q " << q.x << " " << q.y << " " << q.z << " " << q.w
            std::cout << "R UpperArm, q " << q.getRoll() << " " << q.getPitch() << " " << q.getYaw() << " " << time
                << ", interval " << interval << std::endl;

            //if (bsi->mRotation.x < -5000) // some small value
            if (mBoneName != "Bip01 NonAccum") // HACK
            {
                if (!(mModel->getModelName().find("pider") != std::string::npos && (mNode->getName() == "Bip01 R UpperArm"
                    || mNode->getName() == "Bip01 R Forearm"
                    || mNode->getName() == "Bip01 R Hand"
                    || mNode->getName() == "Bip01 L UpperArm")))
                {
                    if (!q.isNaN() && q != Ogre::Quaternion::ZERO) // FIXME: why does this happen?
                            mNode->setOrientation(q);
                    else
                        std::cout << "q " << q.x << " " << q.y << " " << q.z << " " << q.w << std::endl;
                }
                else
                    mNode->setOrientation(mNode->getInitialOrientation());
            }
            if (q == Ogre::Quaternion::ZERO)
                std::cout << "zero q" << std::endl;
            if (q.isNaN())
                std::cout << "nan q" << std::endl;
            //else
                //mNode->setOrientation(bsi->mRotation.Inverse() * q);
        }
        //}

        // FIXME should verify that the size of of short vector is greater than offset + nCtrl
        if (bsi->mTranslationHandle != USHRT_MAX && bsi->mTranslationHandle + nCtrl > sd->mShortControlPoints.size())
            std::cout << "translation overflow" << std::endl; // FIXME: debugging

        Ogre::Vector3 old = mNode->getPosition(); // FIXME: debug only
        //if (bsi->mTranslationMultiplier != FLT_MAX && bsi->mTranslationOffset != FLT_MAX) {
        if (bsplineinterpolate<Ogre::Vector3>(v, degree, interval, nCtrl, sd->mShortControlPoints,
            bsi->mTranslationHandle, bsi->mTranslationMultiplier, bsi->mTranslationOffset))
        {
            float dist;
            //if ((dist = old.squaredDistance(mNode->getPosition())) < 10) // FIXME: debug only
            //if ((dist = old.squaredDistance(v)) < 10000) // FIXME: debug only, horrible hack
            if (0)//!(mModel->getModelName().find("pider") != std::string::npos && (mNode->getName() == "Bip01 L UpperArm"
                 //       || mNode->getName() == "Bip01 R Forearm"
                   //     || mNode->getName() == "Bip01 R UpperArm"
                     //   || mNode->getName() == "Bip01 L Forearm")))
                mNode->setPosition(/*-bsi->mTranslation +*/ v);
            else if (1)
                mNode->setPosition(mNode->getInitialPosition());
            else if (0)
            {
                bool res = bsplineinterpolate<Ogre::Vector3>(old, degree, interval, nCtrl, sd->mShortControlPoints,
                    bsi->mTranslationHandle, bsi->mTranslationMultiplier, bsi->mTranslationOffset);
                std::cout << mNode->getName() << " dist " << dist << ", time " << time << std::endl;
            }
        }
        //}

        if (bsi->mScaleHandle != USHRT_MAX && bsi->mScaleHandle+nCtrl > sd->mShortControlPoints.size())
            std::cout << "scale overflow" << std::endl; // FIXME: debugging

        //if (bsi->mScaleMultiplier != FLT_MAX && bsi->mScaleOffset != FLT_MAX)
        if (bsplineinterpolate<float>( scale, degree, interval, nCtrl, sd->mShortControlPoints,
                    bsi->mScaleHandle, bsi->mScaleMultiplier, bsi->mScaleOffset ))
        {
            mNode->setScale(Ogre::Vector3(scale, scale, scale)); // assume uniform scaling
        }

        return;
    }

    // FIXME float
#else
    // HACK: without this, animations move sideways
    if (mBoneName == "Bip01 NonAccum")
    {
        mNode->setOrientation(mNode->getOrientation()); // nochange
#if 1
        // FIXME: without multiplying by the inverse of the Bip01 NonAccum rotation, some
        //        creatures sink half way into the floor/ground.
        mNode->setPosition(getRotation(time).Inverse() * getTranslation(time));
#else
        // doesn't work for some creatures
        mNode->setPosition(mNode->getOrientation().Inverse() * getTranslation(time));
#endif
    }
    else
    {
        mNode->setOrientation(getRotation(time));
        mNode->setPosition(getTranslation(time));
    }

    mNode->setScale(getScale(time));
#endif
}

Ogre::Quaternion NiBtOgre::MultiTargetTransformController::Value::interpQuatKey (
        const NiBtOgre::KeyGroup<Ogre::Quaternion>& keyGroup, uint32_t cycleType, float time)
{
    if (time <= keyGroup.indexMap.begin()->first)
        return keyGroup.keys[keyGroup.indexMap.begin()->second].value;

    std::map<float, int>::const_iterator nextIt = keyGroup.indexMap.lower_bound(time); // >=
    if (nextIt != keyGroup.indexMap.end())
    {
        std::map<float, int>::const_iterator lastIt = nextIt;
        --lastIt; // point to prev key

        float lastTime = lastIt->first;
        float nextTime = nextIt->first;
        float x = (time - lastTime) / (nextTime - lastTime);

        if (keyGroup.interpolation == 5)      // CONST
        {
            if ( x < 0.5f ) // step up at half way (or should it be step up at 1.0f instead?)
                return keyGroup.keys[lastIt->second].value;
            else
                return keyGroup.keys[nextIt->second].value;
        }
        else if (keyGroup.interpolation == 1) // LINEAR
        {
            const Ogre::Quaternion& v1 = keyGroup.keys[lastIt->second].value;
            const Ogre::Quaternion& v2 = keyGroup.keys[nextIt->second].value;

            if (1)
            {
                Ogre::Quaternion v3 = v1;
                if (v1.Dot(v2) < 0)
                    v3 = v1.Inverse();
                //return Ogre::Quaternion::nlerp(x, v3, v2, true/*shortestPath*/);
                return Ogre::Quaternion::Slerp(x, v3, v2);
            }
            else
            {
                // FIXME: just experimenting
                Ogre::Quaternion r;
                r.x = v1.x + (v2.x - v1.x) * x;
                r.y = v1.y + (v2.y - v1.y) * x;
                r.z = v1.z + (v2.z - v1.z) * x;
                r.w = v1.w + (v2.w - v1.w) * x;
                return r;
            }
        }
        else if (keyGroup.interpolation == 3) // TBC
        {
            // Slaughter Fish
            // FIXME: use linear for now
            const Ogre::Quaternion& v1 = keyGroup.keys[lastIt->second].value;
            const Ogre::Quaternion& v2 = keyGroup.keys[nextIt->second].value;

            return Ogre::Quaternion::nlerp(x, v1, v2);
        }
        else // quadratic interpolation not supported for QuatKey
            throw std::runtime_error("Unsupported interpolation type for Quaternion keys");
    }
    else // no key has >= time; i.e. time is greater than any of the keys' (TODO: should this happen?)
    {
        // FIXME: do we ever get here?
        if (cycleType == 2) // CYCLE_CLAMP
            return keyGroup.keys[keyGroup.indexMap.rbegin()->second].value;
        else
            return keyGroup.keys[keyGroup.indexMap.begin()->second].value; // wrap around
    }
}

Ogre::Quaternion NiBtOgre::MultiTargetTransformController::Value::getXYZRotation (
        float time, Ogre::Bone *bone, const NiInterpolator *interpolator) const
{
    const NiTransformInterpolator* transInterp = dynamic_cast<const NiTransformInterpolator*>(interpolator);
    if (!transInterp)
        return bone->getOrientation();

    NiTransformData *mTransformData = mModel->getRef<NiTransformData>(transInterp->mDataRef);

    float xrot = interpolate<float>(mTransformData->mXRotations, 2/*CYCLE_CLAMP*/, time);
    float yrot = interpolate<float>(mTransformData->mYRotations, 2/*CYCLE_CLAMP*/, time);
    float zrot = interpolate<float>(mTransformData->mZRotations, 2/*CYCLE_CLAMP*/, time);
    Ogre::Quaternion xr(Ogre::Radian(xrot), Ogre::Vector3::UNIT_X);
    Ogre::Quaternion yr(Ogre::Radian(yrot), Ogre::Vector3::UNIT_Y);
    Ogre::Quaternion zr(Ogre::Radian(zrot), Ogre::Vector3::UNIT_Z);

    //if (mInterpolator->mRotation.w == -3.40282e+38 || mInterpolator->mRotation.x == -3.40282e+38 ||
    //    mInterpolator->mRotation.y == -3.40282e+38 || mInterpolator->mRotation.z == -3.40282e+38)
    //if (transInterp->mRotation.x < -5000) // some small value
        return (zr*yr*xr);
    //else
        //return transInterp->mRotation.Inverse() * (zr*yr*xr);
}

Ogre::Quaternion NiBtOgre::MultiTargetTransformController::Value::getRotation (Ogre::Real time, Ogre::Bone *bone, const NiInterpolator *interpolator) const
{
    const NiTransformInterpolator*
        transInterp = dynamic_cast<const NiTransformInterpolator*>(interpolator);
    if (transInterp)
    {
        NiTransformData *mTransformData = mModel->getRef<NiTransformData>(transInterp->mDataRef);

        //const NiTransformData* transData = transInterp->transformData.getPtr();
//      if (!mTransformData)
//          return bone->getOrientation(); // shouldn't happen, already checked in NiControllerSequence

        if (mTransformData->mQuaternionKeys.keys.size() > 0)
        {
            //FIXME: other mRotationType
            //if (mTransformData->mRotationType != 1) // LINEAR_KEY
                //throw std::runtime_error("MultiTargetTransformController: unsupported rotation type");

            return interpQuatKey(mTransformData->mQuaternionKeys, 2, time);
        }
        else if (!mTransformData->mXRotations.keys.empty() ||
                 !mTransformData->mYRotations.keys.empty() || !mTransformData->mZRotations.keys.empty())
        {
            // FIXME: check mRotationType
            //std::cout << "TransformData.mRotationType " << mTransformData->mRotationType << std::endl;
            return getXYZRotation(time, bone, interpolator);
        }

        return bone->getOrientation(); // no change // FIXME: throw?
    }

    const NiBSplineCompTransformInterpolator* bsi
        = dynamic_cast<const NiBSplineCompTransformInterpolator*>(interpolator);
    if (bsi)
    {
        const NiBSplineData* sd = mModel->getRef<NiBSplineData>(bsi->mSplineDataRef);
        const NiBSplineBasisData* bd = mModel->getRef<NiBSplineBasisData>(bsi->mBasisDataRef);
//      if (!sd || !bd)
//          return bone->getOrientation(); // shouldn't happen, already checked in NiControllerSequence

        unsigned int nCtrl = bd->mNumControlPoints;
        int degree = 3; // degree of the polynomial
        float interval
            = ( ( time - bsi->mStartTime ) / ( bsi->mStopTime - bsi->mStartTime ) ) * float(nCtrl - degree);

        //Ogre::Quaternion q = Ogre::Quaternion();
        Ogre::Quaternion q = bone->getOrientation(); // FIXME which?

        //if (bsi->mRotationHandle != USHRT_MAX && bsi->mRotationMultiplier != FLT_MAX && bsi->mRotationOffset != FLT_MAX)
        bsplineinterpolate<Ogre::Quaternion>( q, degree, interval, nCtrl, sd->mShortControlPoints,
                bsi->mRotationHandle, bsi->mRotationMultiplier, bsi->mRotationOffset );

        return q;
    }
    else // float?
        return bone->getOrientation(); // FIXME float
}

Ogre::Vector3 NiBtOgre::MultiTargetTransformController::Value::getTranslation (float time, Ogre::Bone *bone, const NiInterpolator *interpolator) const
{
    const NiTransformInterpolator*
        transInterp = dynamic_cast<const NiTransformInterpolator*>(interpolator);
    if (transInterp)
    {
        NiTransformData *mTransformData = mModel->getRef<NiTransformData>(transInterp->mDataRef);

//      const Nif::NiTransformData* transData = transInterp->transformData.getPtr();
//      if (!transData)
//          return bone->getPosition();

        if(mTransformData->mTranslations.keys.size() > 0)
            return interpolate<Ogre::Vector3>(mTransformData->mTranslations, 2/*CYCLE_CLAMP*/, time);

        return bone->getPosition();
    }

    const NiBSplineCompTransformInterpolator* bsi
        = dynamic_cast<const NiBSplineCompTransformInterpolator*>(interpolator);
    if (bsi)
    {
        const NiBSplineData* sd = mModel->getRef<NiBSplineData>(bsi->mSplineDataRef);
        const NiBSplineBasisData* bd = mModel->getRef<NiBSplineBasisData>(bsi->mBasisDataRef);
//      if (!sd || !bd)
//          return bone->getOrientation(); // shouldn't happen, already checked in NiControllerSequence

        unsigned int nCtrl = bd->mNumControlPoints;
        int degree = 3; // degree of the polynomial
        float interval
            = ( ( time - bsi->mStartTime ) / ( bsi->mStopTime - bsi->mStartTime ) ) * float(nCtrl - degree);

        //Ogre::Vector3 v = Ogre::Vector3();
        Ogre::Vector3 v = bone->getPosition(); // FIXME which?

        //if (bsi->mTranslationHandle != USHRT_MAX && bsi->mTranslationMultiplier != FLT_MAX && bsi->mTranslationOffset != FLT_MAX)
        bsplineinterpolate<Ogre::Vector3>( v, degree, interval, nCtrl, sd->mShortControlPoints,
                bsi->mTranslationHandle, bsi->mTranslationMultiplier, bsi->mTranslationOffset );

        return v;
    }
    else // float
        return bone->getPosition(); // FIXME float
}

Ogre::Vector3 NiBtOgre::MultiTargetTransformController::Value::getScale (float time, Ogre::Bone *bone, const NiInterpolator *interpolator) const
{
    const NiTransformInterpolator*
        transInterp = dynamic_cast<const NiTransformInterpolator*>(interpolator);
    if (transInterp)
    {
        NiTransformData *mTransformData = mModel->getRef<NiTransformData>(transInterp->mDataRef);

        if (!mTransformData || transInterp->mScale == FLT_MIN)
            return bone->getScale();

        if(mTransformData->mScales.keys.size() > 0)
            return Ogre::Vector3(interpolate<float>(mTransformData->mScales, 2/*CYCLE_CLAMP*/, time));

        return bone->getScale();
    }

    const NiBSplineCompTransformInterpolator* bsi
        = dynamic_cast<const NiBSplineCompTransformInterpolator*>(interpolator);
    if (bsi)
    {
        const NiBSplineData* sd = mModel->getRef<NiBSplineData>(bsi->mSplineDataRef);
        const NiBSplineBasisData* bd = mModel->getRef<NiBSplineBasisData>(bsi->mBasisDataRef);
        if (!sd || !bd)
            return bone->getScale(); // shouldn't happen, already checked in NiControllerSequence

        unsigned int nCtrl = bd->mNumControlPoints;
        int degree = 3; // degree of the polynomial
        float interval
            = ((time - bsi->mStartTime) / (bsi->mStopTime - bsi->mStartTime)) * float(nCtrl - degree);
        float scale;

        //Ogre::Vector3 s = Ogre::Vector3();
        Ogre::Vector3 s = bone->getScale(); // FIXME which?
        scale = s.x; // FIXME: assume uniform scaling

        //if (bsi->mScaleHandle != USHRT_MAX && bsi->mScaleMultiplier != FLT_MAX && bsi->mScaleOffset != FLT_MAX)
        bsplineinterpolate<float>( scale, degree, interval, nCtrl, sd->mShortControlPoints,
                bsi->mScaleHandle, bsi->mScaleMultiplier, bsi->mScaleOffset );

        return Ogre::Vector3(scale, scale, scale); // assume uniform scaling
    }
    else // float
        return bone->getScale(); // FIXME float
}

Ogre::Real NiBtOgre::MultiTargetTransformController::Value::getValue() const
{
    // Should not be called
    return 0.0f;
}

void NiBtOgre::MultiTargetTransformController::Value::setValue (Ogre::Real time)
{
    for (size_t i = 0; i < mTargetInterpolators.size(); ++i)
    {
        Ogre::Bone *bone = mTargetInterpolators[i].first;
        const NiInterpolator *interpolator = mTargetInterpolators[i].second;

        bone->setOrientation(getRotation(time, bone, interpolator));
        bone->setPosition(getTranslation(time, bone, interpolator));
        bone->setScale(getScale(time, bone, interpolator));
    }
}
