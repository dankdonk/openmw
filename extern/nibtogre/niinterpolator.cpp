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
#include "niinterpolator.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "nistream.hpp"
#include "niavobject.hpp" // static_cast NiNode
#include "nimodel.hpp"

// Seen in NIF version 20.2.0.7
NiBtOgre::NiBSplineInterpolator::NiBSplineInterpolator(NiStream& stream, const NiModel& model)
    : NiInterpolator(stream, model)
{
    stream.read(mStartTime);
    stream.read(mStopTime);
    stream.read(mSplineDataIndex);
    stream.read(mBasisDataIndex);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiBSplinePoint3Interpolator::NiBSplinePoint3Interpolator(NiStream& stream, const NiModel& model)
    : NiBSplineInterpolator(stream, model)
{
    stream.read(mUnknown1);
    stream.read(mUnknown2);
    stream.read(mUnknown3);
    stream.read(mUnknown4);
    stream.read(mUnknown5);
    stream.read(mUnknown6);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBSplineTransformInterpolator::NiBSplineTransformInterpolator(NiStream& stream, const NiModel& model)
    : NiBSplineInterpolator(stream, model)
{
    stream.read(mTranslation);
    stream.read(mRotation);
    stream.read(mScale);
    stream.read(mTranslationOffset);
    stream.read(mRotationOffset);
    stream.read(mScaleOffset);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBSplineCompTransformInterpolator::NiBSplineCompTransformInterpolator(NiStream& stream,
        const NiModel& model)
    : NiBSplineTransformInterpolator(stream, model)
{
    stream.read(mTranslationBias);
    stream.read(mTranslationMultiplier);
    stream.read(mRotationBias);
    stream.read(mRotationMultiplier);
    stream.read(mScaleBias);
    stream.read(mScaleMultiplier);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5 (????)
NiBtOgre::NiBlendInterpolator::NiBlendInterpolator(NiStream& stream, const NiModel& model)
    : NiInterpolator(stream, model)
{
    stream.read(mUnknownShort);
    stream.read(mUnknownInt);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBlendBoolInterpolator::NiBlendBoolInterpolator(NiStream& stream, const NiModel& model)
    : NiBlendInterpolator(stream, model)
{
    stream.read(mValue);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBlendFloatInterpolator::NiBlendFloatInterpolator(NiStream& stream, const NiModel& model)
    : NiBlendInterpolator(stream, model)
{
    stream.read(mValue);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBlendPoint3Interpolator::NiBlendPoint3Interpolator(NiStream& stream, const NiModel& model)
    : NiBlendInterpolator(stream, model)
{
    stream.read(mValue);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBoolInterpolator::NiBoolInterpolator(NiStream& stream, const NiModel& model)
    : NiInterpolator(stream, model)
{
    stream.read(mBoolalue);
    stream.read(mDataIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiFloatInterpolator::NiFloatInterpolator(NiStream& stream, const NiModel& model)
    : NiInterpolator(stream, model)
{
    stream.read(mFloatValue);
    stream.read(mDataIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPathInterpolator::NiPathInterpolator(NiStream& stream, const NiModel& model)
    : NiInterpolator(stream, model)
{
    stream.read(mPosDataIndex);
    stream.read(mFloatDataIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPoint3Interpolator::NiPoint3Interpolator(NiStream& stream, const NiModel& model)
    : NiInterpolator(stream, model)
{
    stream.read(mPoint3Value);
    stream.read(mDataIndex);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiTransformInterpolator::NiTransformInterpolator(NiStream& stream, const NiModel& model)
    : NiInterpolator(stream, model)
{
    stream.read(mTranslation);
    stream.read(mRotation);
    stream.read(mScale);
    stream.read(mDataIndex);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiLookAtInterpolator::NiLookAtInterpolator(NiStream& stream, const NiModel& model)
    : NiInterpolator(stream, model)
{
    stream.read(mUnknown);

    //stream.getPtr<NiNode>(mLookAt, model.objects());
    std::int32_t index = -1;
    stream.read(index);
    mLookAt = model.getRef<NiNode>(index);

    stream.readLongString(mTargetIndex);
    if (stream.nifVer() <= 0x14050000) // up to 20.5.0.0
    {
        stream.read(mTranslation);
        stream.read(mRotation);
        stream.read(mScale);
    }
    stream.read(mUnknownLink1);
    stream.read(mUnknownLink2);
    stream.read(mUnknownLink3);
}
