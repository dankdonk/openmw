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

  Much of the information on the NIF file structures are based on the NifSkope
  documenation.  See http://niftools.sourceforge.net/wiki/NifSkope for details.

*/
#include "niinterpolator.hpp"

#include <cassert>
#include <stdexcept>

#include "nistream.hpp"
#include "ninode.hpp" // static_cast NiNode
#include "nimodel.hpp"
#include "nidata.hpp"

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

NiBtOgre::NiInterpolator::NiInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiObject(index, stream, model, data)
{
}

const NiBtOgre::KeyGroup<float> *NiBtOgre::NiInterpolator::getMorphKeyGroup()
{
    return nullptr;
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiBSplineInterpolator::NiBSplineInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream->read(mStartTime);
    stream->read(mStopTime);
    stream->read(mSplineDataRef);
    stream->read(mBasisDataRef);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiBSplinePoint3Interpolator::NiBSplinePoint3Interpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiBSplineInterpolator(index, stream, model, data)
{
    stream->read(mUnknown1);
    stream->read(mUnknown2);
    stream->read(mUnknown3);
    stream->read(mUnknown4);
    stream->read(mUnknown5);
    stream->read(mUnknown6);
}

NiBtOgre::NiBSplineFloatInterpolator::NiBSplineFloatInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiBSplineInterpolator(index, stream, model, data)
{
    stream->read(mValue);
    stream->read(mHandle);
}

NiBtOgre::NiBSplineCompFloatInterpolator::NiBSplineCompFloatInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiBSplineFloatInterpolator(index, stream, model, data)
{
    stream->read(mFloatOffset);
    stream->read(mFloatHalfRange);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBSplineTransformInterpolator::NiBSplineTransformInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiBSplineInterpolator(index, stream, model, data)
{
    stream->read(mTranslation);
    stream->read(mRotation);
    stream->read(mScale);
    stream->read(mTranslationHandle);
    stream->read(mRotationHandle);
    stream->read(mScaleHandle);
}

// Seen in NIF version 20.0.0.4, 20.0.0.5
NiBtOgre::NiBSplineCompTransformInterpolator::NiBSplineCompTransformInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiBSplineTransformInterpolator(index, stream, model, data)
{
    stream->read(mTranslationOffset);
    stream->read(mTranslationMultiplier);
    stream->read(mRotationOffset);
    stream->read(mRotationMultiplier);
    stream->read(mScaleOffset);
    stream->read(mScaleMultiplier);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5 (????)
NiBtOgre::NiBlendInterpolator::NiBlendInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream->read(mUnknownShort);
    stream->read(mUnknownInt);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBlendBoolInterpolator::NiBlendBoolInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiBlendInterpolator(index, stream, model, data)
{
    stream->read(mValue);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBlendFloatInterpolator::NiBlendFloatInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiBlendInterpolator(index, stream, model, data)
{
    stream->read(mValue);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBlendPoint3Interpolator::NiBlendPoint3Interpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiBlendInterpolator(index, stream, model, data)
{
    stream->read(mValue);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiBoolInterpolator::NiBoolInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream->read(mBoolalue);
    stream->read(mDataRef);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiFloatInterpolator::NiFloatInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream->read(mFloatValue);
    stream->read(mDataRef);

    if (stream->nifVer() == 0x0a01006a) // 10.1.0.106
        stream->skip(sizeof(int32_t)); // e.g. creatures/horse/bridle.nif version 10.1.0.106
}

const NiBtOgre::KeyGroup<float> *NiBtOgre::NiFloatInterpolator::getMorphKeyGroup()
{
    if (mDataRef == -1)
        return nullptr;

    return &mModel.getRef<NiFloatData>(mDataRef)->mData;
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPathInterpolator::NiPathInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream->skip(sizeof(std::uint16_t));
    stream->skip(sizeof(std::uint32_t));
    stream->skip(sizeof(float));
    stream->skip(sizeof(float));
    stream->skip(sizeof(std::uint16_t));

    stream->read(mPosDataRef);
    stream->read(mFloatDataRef);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiPoint3Interpolator::NiPoint3Interpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream->read(mPoint3Value);
    stream->read(mDataRef);
}

// Seen in NIF ver 20.0.0.4, 20.0.0.5
NiBtOgre::NiTransformInterpolator::NiTransformInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream->read(mTranslation);
    stream->read(mRotation);
    stream->read(mScale);
    stream->read(mDataRef);
}

// Seen in NIF version 20.2.0.7
NiBtOgre::NiLookAtInterpolator::NiLookAtInterpolator(uint32_t index, NiStream *stream, const NiModel& model, BuildData& data)
    : NiInterpolator(index, stream, model, data)
{
    stream->read(mUnknown);
    stream->read(mLookAt);

    stream->readLongString(mTarget);
    if (stream->nifVer() <= 0x14050000) // up to 20.5.0.0
    {
        stream->read(mTranslation);
        stream->read(mRotation);
        stream->read(mScale);
    }
    stream->read(mUnknownLink1);
    stream->read(mUnknownLink2);
    stream->read(mUnknownLink3);
}
