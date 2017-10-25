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
#ifndef NIBTOGRE_NIINTERPOLATOR_H
#define NIBTOGRE_NIINTERPOLATOR_H

#include <string>
#include <cstdint>

#include <OgreVector3.h>
#include <OgreQuaternion.h>

#include "niobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiInterpolator <--------------------------------- /* NiObject */
//     NiBSplineInterpolator
//         NiBSplineFloatInterpolator <------------- /*NiBSplineInterpolator */
//         NiBSplinePoint3Interpolator
//         NiBSplineTransformInterpolator
//             NiBSplineCompTransformInterpolator
//     NiBlendInterpolator
//         NiBlendBoolInterpolator
//         NiBlendFloatInterpolator
//         NiBlendPoint3Interpolator
//         NiBlendTransformInterpolator <----------- /* NiBlendInterpolator */
//     NiKeyBasedInterpolator <--------------------- /* not implemented */
//         NiBoolInterpolator
//             NiBoolTimelineInterpolator <--------- /* NiBoolInterpolator */
//         NiFloatInterpolator
//         NiPathInterpolator
//         NiPoint3Interpolator
//         NiTransformInterpolator
//     NiLookAtInterpolator
namespace NiBtOgre
{
    class NiStream;
    class Header;

    typedef NiObject NiInterpolator; // Seen in NIF version 20.0.0.4, 20.0.0.5

    // Seen in NIF version 20.2.0.7
    struct NiBSplineInterpolator : public NiInterpolator
    {
        float                 mStartTime;
        float                 mStopTime;
        NiBSplineDataRef      mSplineDataIndex;
        NiBSplineBasisDataRef mBasisDataIndex;

        NiBSplineInterpolator(NiStream& stream, const NiModel& model);
    };

    typedef NiBSplineInterpolator NiBSplineFloatInterpolator; // Seen in NIF version 20.2.0.7

    // Seen in NIF version 20.2.0.7
    struct NiBSplinePoint3Interpolator : public NiBSplineInterpolator
    {
        float mUnknown1;
        float mUnknown2;
        float mUnknown3;
        float mUnknown4;
        float mUnknown5;
        float mUnknown6;

        NiBSplinePoint3Interpolator(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiBSplineTransformInterpolator : public NiBSplineInterpolator
    {
        Ogre::Vector3 mTranslation;
        Ogre::Quaternion mRotation;
        float         mScale;
        std::uint32_t mTranslationOffset;
        std::uint32_t mRotationOffset;
        std::uint32_t mScaleOffset;

        NiBSplineTransformInterpolator(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiBSplineCompTransformInterpolator : public NiBSplineTransformInterpolator
    {
        float mTranslationBias;
        float mTranslationMultiplier;
        float mRotationBias;
        float mRotationMultiplier;
        float mScaleBias;
        float mScaleMultiplier;

        NiBSplineCompTransformInterpolator(NiStream& stream, const NiModel& model);
    };

    struct NiBlendInterpolator : public NiInterpolator
    {
        std::uint16_t mUnknownShort;
        std::uint32_t mUnknownInt;

        NiBlendInterpolator(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiBlendBoolInterpolator : public NiBlendInterpolator
    {
        unsigned char mValue;

        NiBlendBoolInterpolator(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiBlendFloatInterpolator : public NiBlendInterpolator
    {
        float mValue;

        NiBlendFloatInterpolator(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiBlendPoint3Interpolator : public NiBlendInterpolator
    {
        Ogre::Vector3 mValue;

        NiBlendPoint3Interpolator(NiStream& stream, const NiModel& model);
    };

    typedef NiBlendInterpolator NiBlendTransformInterpolator; // Seen in NIF version 20.0.0.4, 20.0.0.5

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiBoolInterpolator : public NiInterpolator
    {
        bool          mBoolalue;
        NiBoolDataRef mDataIndex;

        NiBoolInterpolator(NiStream& stream, const NiModel& model);
    };

    typedef NiBoolInterpolator NiBoolTimelineInterpolator; // Seen in NIF version 20.0.0.4, 20.0.0.5

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiFloatInterpolator : public NiInterpolator
    {
        float          mFloatValue;
        NiFloatDataRef mDataIndex;

        NiFloatInterpolator(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiPathInterpolator : public NiInterpolator
    {
        NiPosDataRef   mPosDataIndex;
        NiFloatDataRef mFloatDataIndex;

        NiPathInterpolator(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiPoint3Interpolator : public NiInterpolator
    {
        Ogre::Vector3 mPoint3Value;
        NiPosDataRef  mDataIndex;

        NiPoint3Interpolator(NiStream& stream, const NiModel& model);
    };

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiTransformInterpolator : public NiInterpolator
    {
        Ogre::Vector3      mTranslation;
        Ogre::Quaternion   mRotation;
        float              mScale;
        NiTransformDataRef mDataIndex;

        NiTransformInterpolator(NiStream& stream, const NiModel& model);
    };

    class NiNode;

    // Seen in NIF version 20.2.0.7
    struct NiLookAtInterpolator : public NiInterpolator
    {
        std::uint16_t    mUnknown;
        NiNode          *mLookAt; // Ptr
        StringIndex      mTarget;
        Ogre::Vector3    mTranslation;
        Ogre::Quaternion mRotation;
        float            mScale;
        NiPoint3InterpolatorRef mUnknownLink1;
        NiFloatInterpolatorRef  mUnknownLink2;
        NiFloatInterpolatorRef  mUnknownLink3;

        NiLookAtInterpolator(NiStream& stream, const NiModel& model);
    };
}

#endif // NIBTOGRE_NIINTERPOLATOR_H
