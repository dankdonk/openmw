//#include <OgreTechnique.h>
#include <OgreNode.h>
//#include <OgreTagPoint.h>
//#include <OgreParticleSystem.h>
//#include <OgreEntity.h>
//#include <OgreAnimationState.h>
//
//#include <components/nif/base.hpp>
#include <components/nif/controller.hpp>
//#include <components/nif/controlled.hpp>
//#include <components/nif/property.hpp>
//
//#include <components/misc/resourcehelpers.hpp>

//#include "objectscene.hpp" // MaterialControllerManager
//#include "ogrenifloader.hpp"

namespace
{
} // namespace

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
